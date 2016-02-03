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

#include <iostream>

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

#ifdef ATFA_DEBUG
    /// The number of audio samples per PortAudio buffer
    static const unsigned long pa_framespb = 16;
#else
    /// The number of audio samples per PortAudio buffer
    static const unsigned long pa_framespb = 128;
#endif

    struct Scenario
    {

        bool is_VAD_active; // Voice action detector

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
            const container_t& ir = container_t(1,1), bool vact = true,
            OOV flearn = VAD, OOV fout = VAD,
            unsigned d = 100, float vol = .5, bool p = false
        )
          : is_VAD_active(vact), filter_learning(flearn), filter_output(fout),
            delay(d), volume(vol), paused(p), imp_resp(ir)
        {
        }

    };

#ifndef ATFA_DEBUG
    /// The stream's rate in samples per second
    static const unsigned samplerate = 11025;

    /// The number of data samples held internally be the stream structure.
    /**
      * The actual size of the vector used to hold the samples is
      * `2*buf_size`, in order to make the data structure "look" circular.
      */
    static const size_t buf_size = 88320; // = 690*128 samples, 8.0109 seconds
#else
    /// The stream's rate in samples per second
    static const size_t buf_size = 32;

    /// The number of data samples held internally be the stream structure.
    /**
      * The actual size of the vector used to hold is `2*samplerate`, in order
      * to make the data structure "look" circular.
      */
    static const unsigned samplerate = 1;
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
    sample_t read() {
        sample_t s = *read_ptr;
        ++read_ptr;
        if (read_ptr == semantic_end)
            read_ptr = data.begin();
        return s;
    }

    /// Returns an "array" with the last \a n samples
    /**
      * Makes a pointer (actually, an iterator) to the n-th pointer behind
      * `read_ptr`. Handles the case in which the range
      * \f$\left[\texttt{read\_ptr} -\texttt{n},\texttt{read\_ptr}\right[\f$
      * crosses the end of the circular structure. That is, for any valid value
      * of `read_ptr`, this function can be called with any
      * \f$\texttt{n}\in\left[0,\texttt{buf\_size}\right]\f$. A valid `read_ptr`
      * is one such that `read_ptr - data.begin()` is in the range
      * \f$\left[0,\texttt{buf\_size}\right[\f$.
      *
      * \param[in]  n   The size, in samples, of the "array" that is returned
      *
      * \returns an iterator pointing to the last \a n samples.
      *
      * \see data
      */
    container_t::const_iterator get_last_n(index_t n) {
        ++read_ptr;
        // think of the following summation as being "modulo buf_size",
        // because the data structure is to be circular.
        container_t::const_iterator p = read_ptr + buf_size - n;
        if (read_ptr == semantic_end)
            read_ptr = data.begin();
        return p;
    }

    /// Returns a sample from the stream echoed by the impulse response
    /**
      * Just like `read()`, but returns a sample from the signal that was
      * convolved with the room impulse response.
      *
      * \returns a sample of the echoed signal
      *
      * \see read
      * \see read_ptr
      */
    sample_t get_filtered_sample() {
        container_t::const_iterator p = get_last_n(scene.imp_resp.size());
        sample_t accum = 0;
        for (index_t k = scene.imp_resp.size(); k != 0; --k, ++p)
            accum += scene.imp_resp[k-1] * (*p);
        return accum;
    }

    /// Writes an audio sample to the stream
    /**
      * Writes a sample to the data structure so that it can be later read by
      * `read()` or `get_last_n()`. Writes it twice (one in each copy of the
      * data structure), so that the list is always "doubled" to make it look
      * "circular".
      *
      * Handles the case in which the pointer must be rewinded.
      *
      * \param[in]  s   The value of sample to be written to the stream's
      *                 internal memory
      *
      * \see data
      * \see read
      */
    void write(sample_t s) {
        *write_ptr = s;
        *(write_ptr + buf_size) = s;
        ++write_ptr;
        if (write_ptr == semantic_end)
            write_ptr = data.begin();
    }

    /// Adds overlaping pieces of signals
    sample_t write_add(sample_t s) {
        *write_ptr += s;
        sample_t retval = *write_ptr;
        *(write_ptr + buf_size) = retval; // no need to add once again
        ++write_ptr;
        if (write_ptr == semantic_end)
            write_ptr = data.begin();
        return retval;
    }

    /// Rewinds write_ptr to beginning of next overlapping area (OAA algorithm)
    void ooa_rewind_wptr(unsigned long samples = pa_framespb) {
        if (write_ptr - data.begin() < long(samples))
            write_ptr += buf_size - samples;
        else
            write_ptr -= samples;
    }

    /// Contructs from a `Scenario` object
    /**
      * Constructs a Stream object from `Scenario` parameters.
      *
      * \see Scenario
      */
    Stream(const Scenario& s = Scenario())
        : scene(s), h_freq_re(2*pa_framespb), h_freq_im(2*pa_framespb),
          temp_container1_re(2*pa_framespb), temp_container1_im(2*pa_framespb),
          temp_container2_re(2*pa_framespb), temp_container2_im(2*pa_framespb),
          tempcont1_mid(temp_container1_re.begin() + pa_framespb),
          tempcont2_mid(temp_container2_re.begin() + pa_framespb),
          ZCR(2*buf_size/pa_framespb), zcr_ptr(ZCR.begin()),
          zcrptr_sem_end(ZCR.begin() + buf_size/pa_framespb),
          delay_samples(s.delay*samplerate), data(2*buf_size),
          write_ptr(data.begin()), read_ptr(data.begin()),
          semantic_end(data.begin() + buf_size)
    {
        update_freq_resp();
    }

    /// Updates the stream filter's frequency response
    void update_freq_resp();

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
        if (delay_samples <= static_cast<size_t>(write_ptr - data.begin()))
            read_ptr = write_ptr - delay_samples;
        else
            // We won't ckeck, for performance, that delay_samples <= buf_size
            // The application must enforce this.
            read_ptr = write_ptr + (buf_size - delay_samples);
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

    /// Holds the real part of the stream filter's frequency-response
    container_t h_freq_re;

    /// Holds the imaginary part of the stream filter's frequency-response
    container_t h_freq_im;

    /// A permanent, general-purpose, pre-reserved container for performance
    container_t temp_container1_re;

    /// The imaginary part of the pre-reserved container
    container_t temp_container1_im;

    /// Another one just like temp_container1_re
    container_t temp_container2_re;

    /// Another one just like temp_container1_im
    container_t temp_container2_im;

    /// A permanent iterator to the middle of temp_container1_re
    const container_t::iterator tempcont1_mid;

    /// A permanent iterator to the middle of temp_container2_re
    const container_t::iterator tempcont2_mid;

    /// TODO add desc
    std::vector<int>ZCR;

    /// TODO BURY THIS IN PRIVATE
    std::vector<int>::iterator zcr_ptr;
    const std::vector<int>::const_iterator zcrptr_sem_end;
    void write_zcr(int count) {
        *zcr_ptr = count;
        *(zcr_ptr + (buf_size/pa_framespb)) = count;
        ++zcr_ptr;
        if (zcr_ptr == zcrptr_sem_end)
            zcr_ptr = ZCR.begin();
    }

    /// DEBUG
    void X9() {
        std::cout << "zc = [" << std::endl;
        for (std::vector<int>::const_iterator it = ZCR.begin();
             it != ZCR.end(); ++it) {
            std::cout << "    " << *it << std::endl;
        }
        std::cout << "];" << std::endl << "voice = [" << std::endl;
        for (unsigned long k = 0; k != buf_size; ++k) {
            std::cout << "    " << data[k] << std::endl;
        }
        std::cout << "];" << std::endl;
    }

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
    container_t data;

    /// The high-pass frequency response for the helping the VAD (real part)
    static const container_t filter200Hz_re;

    /// The high-pass frequency response for the helping the VAD (imag part)
    static const container_t filter200Hz_im;

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
    container_t::const_iterator read_ptr;

    /// An iterator to the middle of the vector `data`
    /**
      * Points to the first repeating element in `data`. See the diagram
      * provided in the `data` element description and the explanation in the
      * `Stream` class description.
      */
    const container_t::const_iterator semantic_end;

    /// Returns a sample.
    /**
      * Gets a sample of the signal. Used only in debugging, when we want a
      * snapshot of the stream internal data.
      *
      * \param[in] index    The index of the desired sample.
      *
      * \returns a reference to the sample.
      */
    sample_t& operator [](index_t index) {
#ifdef ATFA_DEBUG
        if (index >= data.size()) throw std::runtime_error("Range error!!");
#endif
        return data[index];
    }

    /// Returns a "read-only" sample.
    /**
      * Just like the "read-write" version, but returns a const reference to a
      * sample.
      *
      * \param[in] index    The index of the desired sample.
      *
      * \returns a const reference to the sample.
      */
    const sample_t& operator [](index_t index) const {
#ifdef ATFA_DEBUG
        if (index >= data.size()) throw std::runtime_error("Range error!!");
#endif
        return data[index];
    }

};

#endif // STREAM_H
