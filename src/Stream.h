/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

/**
 *
 * \file Stream.h
 *
 * Holds the interface to the `Stream` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#ifndef STREAM_H
#define STREAM_H

extern "C" {
#   include <portaudio.h>
#   include <dlfcn.h>
}

#ifdef ATFA_LOG_MATLAB
# include <cstring>
#endif

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "VAD.h"
#include "AdaptiveFilter.h"
#include "widgets/LEDIndicatorWidget.h"

typedef unsigned long pa_fperbuf_t;

/// Represents an input/output stream of audio samples
/**
  * Holds data and provides routines for dealing with streams that represent
  * communication systems with echo. Currently, all Streams are implemented as a
  * circular array of single-precision floating-point samples. Streams are aware
  * of their sample rates.
  *
  * The stream uses the circular array as an internal memory. The array holds
  * `2 * buf_size` audio samples. When we tell the stream to start running, the
  * _write pointer_ (`write_ptr`) points to the first element of this array.
  * Everytime we receive a new audio sample from the microphone (through
  * PortAudio), we write it to the location pointed to by the write pointer, and
  * also to the location pointed to by `write_ptr + buf_size`. This way, we will
  * always have a doubled vector of samples. When the write pointer reaches the
  * middle of the circular vector, that is, the `buf_size+1`-th element, it
  * rewinds back to the first element. A useful diagram is presented in the
  * description for the `data` element.
  *
  * Whenever we need a new audio sample to playback (which happens whenever we
  * receive a new sample -- the audio input and output are coerent), we read it
  * from the the location pointed to by the _read pointer_ (`read_ptr`), and
  * increment the read pointer (rewinding it if necessary).
  *
  * By calling the `set_delay()` method, we place the read pointer at a
  * specified number of samples behind the write pointer, so that running the
  * stream makes it echo everything it "hears".
  *
  * - The `data` member of the class holds the circular array.
  * - `semantic_end` is a pointer to the "`buf_size+1`-th element". For looping
  *   through "all" the samples, we should do
  *   `for (iterator = data.begin(); iterator != semantic_end; ++iterator) {}`
  * - For writing samples in the manner specified above in second paragraph, we
  *   use the `write()` routine.
  * - Although there is an analogous `read()` routine, if we want the audio to
  *   pass through the room impulse response, we use the `get_filtered_sample()`
  *   method instead. It modifies the read pointer in the same way as `read()`
  *   does.
  */
class Stream
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_t> container_t;

    typedef bool (*vad_algorithm_t)(container_t::const_iterator,
                                    container_t::const_iterator);

    struct Scenario
    {

        enum OOV { // On, Off, VAD
            On,  ///< always enabled
            Off, ///< always disabled
            VAD  ///< enabled when VAD detects voice
        };

        enum RIR_source_t {NoRIR, Literal, File};

        OOV filter_learning;
        OOV filter_output;

        unsigned delay; // in miliseconds

        float volume; // 0 -- 1

        container_t imp_resp;

        std::string rir_file;
        std::string adapf_file;

        Scenario(
            OOV flearn = On, OOV fout = On,
            unsigned d = 30, float vol = .5,
            const container_t& ir = container_t(1,1),
            const AdaptiveFilter<sample_t>& adapf = AdaptiveFilter<sample_t>()
        )
          : filter_learning(flearn), filter_output(fout),
            delay(d), volume(vol), imp_resp(ir), rir_file(""),
            adapf_file(adapf.get_path())
        {}

    };

    /// The stream's rate in samples per second.
    static constexpr unsigned samplerate = 11025;

    static constexpr size_t blk_size = 128;

    static constexpr unsigned blks_in_buf = 1500;

    static constexpr unsigned blks_in_fft = 64;

    static constexpr size_t fft_size = blks_in_fft * blk_size;

    /// The number of data samples held internally by the stream structure.
    static constexpr size_t buf_size = blks_in_buf * blk_size;

    /// Returns the next audio sample
    /**
      * Reads the audio sample pointed to by `read_ptr` and makes the pointer
      * indicate the next sample. Handles the case in which the pointer must be
      * rewinded, because the data structure is circular.
      *
      * \returns the next audio sample in line
      *
      * \see data
      * \see write
      */
    template<class InputIt, class OutputIt>
    void read_write(InputIt in_buf, OutputIt out_buf, pa_fperbuf_t pa_frames) {
        pa_fperbuf_t remaining = data_out.end() - read_ptr;
        long overflow = (long)pa_frames - (long)remaining;
        auto read_end_ptr = (overflow < 0) ?
                    (read_ptr + pa_frames) :
                    (data_out.begin() + overflow);
        /* Nesse loop, a gente calcula a cada iteração o índice do VAD
         * correspondente àquela amostra. Tem dois jeitos de otimizar isso:
         *  "Sem perda": sabendo que o tamanho do bloco do VAD é maior ou igual
         *               a `pa_frames' (verificar), calcular de antemão onde
         *               tem VAD e onde não tem, e dividir o loop abaixo em
         *               dois.
         *  "Com perda": Calcular um valor único de índice do VAD para a
         *               primeira das `pa_frames' amostras, e usar esse valor
         *               pra todas as amostras subsequentes. Ao invés de usar
         *               a primeira amostra, pode usar a amostra média.
         */
        while (read_ptr != read_end_ptr) {
            auto vad_idx = (adapf_ptr-data_in.begin())/blk_size;
            *out_buf = scene.volume *
                       adapf->get_sample(
                           *adapf_ptr, *read_ptr,
                           scene.filter_learning==Scenario::On || (
                               scene.filter_learning==Scenario::VAD &&
                               vad[vad_idx]
                           )
                       );
#ifdef ATFA_LOG_MATLAB
            // TODO: esse bloco todo tem que ser rodado somente se
            //       a gente ainda não estourou o buffer do wvec.
            {
                sample_t *it; unsigned n;
                adapf->get_impresp(&it, &n);
                std::memcpy(&(wvec[w_ptr][0]), it, n*sizeof(sample_t));
            }
            ++w_ptr;
#endif
            ++read_ptr, ++adapf_ptr, ++out_buf;
            if (read_ptr == data_out.end())
                read_ptr = data_out.begin();
            if (adapf_ptr == data_in.end())
                adapf_ptr = data_in.begin();
        }
        if (overflow < 0) { // there was no overflow
            write_ptr = std::copy(in_buf, in_buf + pa_frames, write_ptr);
        }
        else {
            std::copy(in_buf, in_buf + remaining, write_ptr);
            write_ptr = std::copy(in_buf + remaining, in_buf + pa_frames,
                    data_in.begin());
        }
        size_t current_offset = blk_offset + pa_frames;
        unsigned blk_count_inc = current_offset / blk_size;
        // if (blk_count_inc)
        {
            std::lock_guard<std::mutex> lk(blk_mutex);
            blk_count += blk_count_inc;
        }
        blk_cv.notify_one();
        blk_offset = current_offset % blk_size;
    }


#ifdef ATFA_LOG_MATLAB
# define ATFA_STREAM_INIT_WPTR w_ptr(0),
#else
# define ATFA_STREAM_INIT_WPTR
#endif
    /// Contructs from a `Scenario` object
    /**
      * Constructs a Stream object from `Scenario` parameters.
      *
      * \see Scenario
      */
    Stream(LEDIndicatorWidget *ledw = nullptr, const Scenario& s = Scenario())
        : ATFA_STREAM_INIT_WPTR
          scene(s), adapf(new AdaptiveFilter<sample_t>()),
          is_running(false), data_in(buf_size), data_out(buf_size),
          vad(blks_in_buf),
          write_ptr(data_in.begin()), read_ptr(data_out.begin()),
          vad_ptr(vad.begin()),
          h_freq_re(fft_size), h_freq_im(fft_size),
          led_widget(ledw)
    {
        set_delay(scene.delay); // sets delay_samples and filter_ptr
        set_filter(scene.imp_resp, false); // sets h_freq_re and h_freq_im
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (samplerate == 0) throw std::runtime_error("Stream: Bad srate");
#ifdef ATFA_LOG_MATLAB
        {
            for (auto& w : wvec)
                for (auto& x : w)
                    x = 0;
        }
#endif
    }

#ifdef ATFA_LOG_MATLAB
    static sample_t wvec[blks_in_buf*blk_size][128];
    int w_ptr;
#endif

    /// Runs the stream with predefined scenario parameters.
    PaStream *echo();

#ifdef ATFA_LOG_MATLAB
# define ATFA_STREAM_STOP_ATTR __attribute__((optimize("-O0")))
#else
# define ATFA_STREAM_STOP_ATTR
#endif
    /// Stops the stream that is running
    void stop(PaStream *s) ATFA_STREAM_STOP_ATTR;

    /// Sets the delay parameter, given in miliseconds
    /**
      * Calculates how many samples are equivalent to the specified delay, and
      * moves the read pointer to that many samples behind the write pointer.
      *
      * This function doesn't check whether the given delay is valid, so that
      * the application must be sure that the delay `msec` is such that
      * \f$\texttt{samplerate}\cdot\texttt{msec}\le
      *     1000\cdot\texttt{buf\_size}\f$
      *
      * \param[in]  msec    The time delay, specified in miliseconds
      *
      * \see delay_samples
      * \see read_ptr
      */
    void set_delay(unsigned msec) {
        delay_samples = static_cast<int>(samplerate * double(msec)/1000);
        size_t remaining = data_out.end() - read_ptr;
        if (delay_samples < remaining)
            filter_ptr = read_ptr + delay_samples;
        else
            // We won't ckeck, for performance, that delay_samples <= buf_size
            // The application must enforce this.
            filter_ptr = data_out.begin() + (delay_samples - remaining);
        // ---
        size_t passed = write_ptr - data_in.begin();
        if (passed >= delay_samples)
            adapf_ptr = write_ptr - delay_samples;
        else
            adapf_ptr = data_in.end() - (delay_samples - passed);
        // ---
        scene.delay = msec;
    }

    /// Sets the room impulse response
    void set_filter(const container_t &h, bool substitute = true);

    template<class InputIt>
    void set_filter(InputIt first, InputIt last, bool substitute = true) {
        set_filter(container_t(first, last), substitute);
    }

    /// Used for debugging, together with `simulate`
    void dump_state(const container_t speaker_buf) const;

    /// Simulates a PortAudio session, used for debugging
    void simulate();

    /// A structure holding the many parameters that define a scenario setup
    Scenario scene;

    bool running() {
        std::lock_guard<std::mutex> lk(running_mutex);
        return is_running;
    }

    void setLED(LEDIndicatorWidget *ledw) {
        led_widget = ledw;
    }

    void setVADAlgorithm(int idx) {
        vad_algorithm_t algs[2] = { &vad_hard, &vad_soft };
        calcVAD = algs[idx];
    }

    void setAdapfAlgorithm(AdaptiveFilter<sample_t> *adapf_new) {
        if (adapf)
            delete adapf;
        adapf = adapf_new;
    }

    const char *get_adapf_title() {
        return adapf->get_title();
    }

    const char *get_adapf_listing() {
        return adapf->get_listing();
    }

    void reset_adapf_state() {
        adapf->reset_state();
    }

private:

    AdaptiveFilter<sample_t> *adapf;

    std::condition_variable blk_cv;
    int blk_count; // how many blocks need to be processed by rir_fft.
                        // should only be accessed by owner of lock on blk_mutex
    std::mutex blk_mutex;

    size_t blk_offset; // hoy many samples gave been written to current block

    bool is_running;
    std::mutex running_mutex;

    std::mutex io_mutex;

    /// The delay of the communication channel, measured in samples
    index_t delay_samples;

    /// The container for holding the internal memory of the data structure
    /**
      * A C++ vector that holds the internal memory representation of the
      * stream. The vector contains _two_ copies of each audio sample. It can
      * be though of as a concatenation of two identical vectors, each equal to
      * a "circular buffer" of size `buf_size` that is used to keep track of the
      * stream's state.
      *
      * \image html diag_stream_data.png
      * \image latex diag_stream_data.pdf "The `data` vector structure" width=\columnwidth
      *
      * \see read
      * \see get_last_n
      * \see write
      * \see Stream
      */
    container_t data_in;
    container_t data_out;

    std::vector<bool> vad;

    /// An iterator to the next location to be written on the stream
    /**
      * A sample should be written to the stream like:
      *
      *     *write_ptr = sample;
      *     *(write_ptr++ + buf_size) = sample;
      *     if (write_ptr == semantic_end) write_ptr = data.begin();
      *
      * It is important to write the sample to both locations (`write_ptr` and
      * `write_ptr + buf_size`) in case we read from the stream through
      * functions like `get_last_n()`.
      *
      * \see write
      * \see semantic_end
      * \see read_ptr
      */
    container_t::iterator write_ptr;

    /// An iterator to the next location to be read from the stream
    /**
      * A sample should be read from the stream like:
      *
      *     sample = *read_ptr++;
      *     if (read_ptr == semantic_end) read_ptr = data.begin();
      *
      * \see read
      * \see semantic_end
      * \see write_ptr
      */
    container_t::iterator read_ptr;
    container_t::iterator filter_ptr;

    std::vector<bool>::iterator vad_ptr;
    vad_algorithm_t calcVAD = &vad_hard;

    void rir_fft();

    std::thread *rir_thread;

    container_t::const_iterator rir_ptr;
    container_t::const_iterator adapf_ptr;

    container_t h_freq_re;
    container_t h_freq_im;

    LEDIndicatorWidget *led_widget;

};

using Scene = Stream::Scenario;

#endif // STREAM_H
