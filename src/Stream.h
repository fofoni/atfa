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
}

#include <vector>
#include <stdexcept>
#include <numeric>

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

    struct Scenario
    {

        enum OOV { // On, Off, VAD
            On,  // always enabled
            Off, // always disabled
            VAD  // enabled when VAD detects voice
        };

        OOV filter_learning;
        OOV filter_output;

        unsigned delay; // in miliseconds

        float volume; // 0 -- 1

        bool paused;

        container_t imp_resp;

        Scenario(
            const container_t& ir = container_t(1,1),
            OOV flearn = VAD, OOV fout = VAD,
            unsigned d = 100, float vol = .5, bool p = false
        )
          : filter_learning(flearn), filter_output(fout),
            delay(d), volume(vol), paused(p), imp_resp(ir)
        {
        }

    };

#ifndef ATFA_DEBUG
    /// The stream's rate in samples per second.
    static const unsigned samplerate = 11025;

    /// The number of data samples held internally be the stream structure.
    static const size_t buf_size = 8*samplerate; // (num. of seconds * srate)
#else
    /// The stream's rate in samples per second.
    static const unsigned samplerate = 1;

    /// The number of data samples held internally be the stream structure.
    static const size_t buf_size = 24;
#endif

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
        if (remaining < pa_frames) {
            std::copy(read_ptr, read_ptr + pa_frames, out_buf);
            std::copy(in_buf,   in_buf + pa_frames,   write_ptr);
            read_ptr  = read_ptr  + pa_frames;
            write_ptr = write_ptr + pa_frames;
        }
        else {
            std::copy(read_ptr, data_out.end(),     out_buf);
            std::copy(in_buf,   in_buf + remaining, write_ptr);
            read_ptr  = data_out.begin() + (remaining - pa_frames);
            write_ptr = data_in.begin()  + (remaining - pa_frames);
            std::copy(data_out.begin(),   read_ptr,
                      out_buf + remaining);
            std::copy(in_buf + remaining, in_buf + pa_frames,
                      data_in.begin());
        }
    }

    /// Initializes important values
    /**
      * Constructs a scenario in which there is no delay, and the impulse
      * response has one sample of value 1. Also acquires memory for the data
      * structure.
      *
      * \see data
      * \see read
      */
    Stream()
        : scene(), data_in(buf_size), data_out(buf_size),
          write_ptr(data_in.begin()), read_ptr(data_out.begin())
    {
        set_delay(scene.delay); // sets delay_samples and filter_ptr
#ifdef ATFA_DEBUG
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (samplerate == 0) throw std::runtime_error("Stream: Bad srate");
#endif
    }

    /// Contructs from a `Scenario` object
    /**
      * Constructs a Stream object from `Scenario` parameters.
      *
      * \see Scenario
      */
    Stream(const Scenario& s)
        : scene(s), data_in(buf_size), data_out(buf_size),
          write_ptr(data_in.begin()), read_ptr(data_out.begin())
    {
    }

    /// Runs the stream with predefined scenario parameters.
    PaStream *echo();

    /// Stops the stream that is running
    void stop(PaStream *s);

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
        scene.delay = msec;
    }

    /// Sets the room impulse response
    void set_filter(const container_t &h);

    /// Used for debugging, together with `simulate`
    void dump_state(const container_t speaker_buf) const;

    /// Simulates a PortAudio session, used for debugging
    void simulate();

    /// A structure holding the many parameters that define a scenario setup
    Scenario scene;

private:

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

};

#endif // STREAM_H
