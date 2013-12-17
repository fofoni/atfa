#ifndef SIGNAL_H
#define SIGNAL_H

/**
 *
 * \file Signal.h
 *
 * Holds everything to do with the `Signal` class.
 *
 * \todo Separate implementation and declarations in different files.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include <cstdlib>
#include <cmath>

#include <portaudio.h>
#include <sndfile.hh>

#ifndef NULL
/// null pointer
# define NULL ((void *) 0)
#endif

/// A time- or frequency-domain signal
/**
  * Holds data and provides routines for dealing with time-domain and
  * frequency-domain signals. Currently, all Signals are an array of
  * single-precision floating-point samples. Signals know their sample rate.
  *
  * \todo `float`s should be a typedef sample_t (since we don't want a template)
  *
  * \todo Implement "stream" signals, to provide real-time processing.
  */
class Signal
{

public:

    /// This is a type for specifying whether a time interval is given in
    /// milliseconds or in samples.
    enum delay_type {
        MS,     ///< Time interval given in milliseconds.
        SAMPLE  ///< Time interval given in samples.
    };

    Signal(); ///< Constructs an empty signal
    Signal(std::string filename); ///< Constructs a signal from an audio file.
    ~Signal(); ///< Frees memory used
    void copyfrom(Signal &other); ///< Constructs a signal as a copy of another.

    /// Pointer to the array of samples.
    /**
      * This member is public only because we need to pass it to the libsndfile
      * read audio file function.
      *
      * \todo All uses of `data` are already encapsulated inside the Signal
      *       class implementation. This should be private.
      *
      * \todo Consider making this a std::vector, or std::valarray.
      */
    float *data;

    /// Number of samples.
    /**
      * This member is public only because we need to pass it to libsndfile
      * functions.
      *
      * \todo Encapsulate (if they're not already) all uses of `samples` inside
      *       the Signal class implementation. Then make this private.
      */
    unsigned long samples;

    /// %Signal sample rate in hertz
    /**
      * \todo Can we make this a private member?
      */
    int sample_rate;

    unsigned long counter; ///< general-purpose variable for external use.

    inline float& operator [](unsigned long index); ///< Returns a sample.

    void set_size(unsigned long n); ///< Changes the number of samples.

    void set_samplerate(int sr); ///< Changes the signal sample rate.

    void delay(delay_type t, unsigned long d); ///< Delays the signal in time.
    void add(Signal &other); ///< Adds the `other` signal to the caller.
    void gain(float g); ///< Applies gain `g` to the signal.
    void filter(Signal &imp_resp, Signal& conv); ///< Convolves the sinal.

    void play(bool sleep=true); ///< Makes PortAudio playback the audio signal.

};

/// PortAudio callback function
static int callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
);

/**
  * Initializes the signal with no meta-data and no samples. The user needs to
  * specify the sample rate and create samples before using the signal.
  */
Signal::Signal()
    : data(NULL), sample_rate(0), counter(0)
{
}

/**
  * If the signal is not empty, free the pointer to the array of samples.
  */
Signal::~Signal()
{
    if (data != NULL)
        free(data);
}

/**
  * Gets a sample of the signal. For performance reasons, this method does not
  * check that the given index is valid.
  *
  * \param[in] index    The index of the desired sample. Signal indexes are
  *                     zero-based.
  *
  * \returns a reference to the sample.
  */
inline float& Signal::operator [](unsigned long index) {
    // no checking that index is valid for performance
    /*if (index >= samples) {
        std::cerr << "oops" << std::endl;
        return data[samples-1];
    }*/
    return data[index];
}

/**
  * Changes the signal length. Allocates more space if we are growing the
  * signal, and deletes the last samples if we are shrinking it.
  *
  * \param[in] n        The desired signal length.
  *
  * \throws `std::runtime_error` if the memory reallocation fails.
  */
void Signal::set_size(unsigned long n) {
    float *ptr = (float *)realloc(data, n*sizeof(float));
    if (ptr == NULL)
        throw std::runtime_error("Error trying to (re)allocate space.");
    data = ptr;
    for (unsigned long i = samples; i < n; i++)
        data[i] = 0;
    samples = n;
}

/**
  * Constructs a signal as a copy of another one. If this signal is not empty,
  * we destroy it.
  *
  * \param[in] other    The signal to be copied from.
  *
  * \throws `std::runtime_error` if memory allocation fails. Memory allocation
  *         happens because the signal sizes might differ.
  *
  * \todo This method should be a C++ copy-constructor. Also, make sure `other`
  *       is `_const_ Signal&`.
  */
void Signal::copyfrom(Signal &other) {
    sample_rate = other.sample_rate;
    float *ptr = (float *)malloc(other.samples*sizeof(float));
    if (ptr == NULL)
        throw std::runtime_error("Error trying to (re)allocate space.");
    if (data != NULL) free(data);
    data = ptr;
    for (unsigned long i = 0; i < other.samples; i++)
        data[i] = other[i];
    samples = other.samples;
}

/**
  * Constructs a signal getting the signal data from an audio file. This is done
  * using the [libsndfile][libsndfile] library. The filetypes supported are
  * listed [here][libsndfile_features]. WAV is supported, but MP3 is not.
  *
  * If the given file is stereo, of multi-channel, just the first channel
  * will be read. (On stereo audio files, this is the left channel.)
  *
  * The sample rate is extracted from the file's meta-data info.
  *
  * \param[in]  filename    Audio file name.
  *
  * \throws `std::runtime_error` if file openening fails.
  *
  * \throws `std::runtime_error` if file reading fails.
  *
  * \todo Should test `buf` for `malloc` error.
  *
  * \todo Should throw a more catchable exception at file open failure.
  */
Signal::Signal(std::string filename)
    : data(NULL), counter(0)
{

    // open file
    SNDFILE *file;
    SF_INFO info;
    if (!( file = sf_open(filename.c_str(), SFM_READ, &info) ))
        throw std::runtime_error(std::string("Cannot open file `") + filename +
                                 "' for reading.");

    // check number of channels
    unsigned chans = info.channels;
    if (chans > 1)
        std::cerr << "Warning: file " << filename << " has more than one" <<
                     " channel." << std::endl << "  We will use just the" <<
                     " first (which is the left channel on stereo WAV files).";

    // set properties
    set_size(info.frames);
    sample_rate = info.samplerate;

    // read file
    float *buf = (float *)malloc(samples*chans*sizeof(float));
    // TEST BUF!!!!!
    unsigned long items_read = sf_read_float(file, buf, samples*chans);
    if (items_read != samples*chans)
        throw std::runtime_error(std::string("Error reading file `") +
                                 filename + "'.");
    for (unsigned i = 0; i < samples; ++i)
        data[i] = buf[i*chans];
    free(buf);

}

/**
  * Adds zeroed samples at the beginning of the signal.
  *
  * \param[in]  t       A `delay_type` element.
  * \param[in]  d       The time interval to be delayed, given in the units
  *                     specified by `t`.
  *
  * \throws `std::runtime_error` if memory realloc fails.
  */
void Signal::delay(delay_type t, unsigned long d) {
    if (sample_rate==0 && t==MS)
        std::cerr << "Warning: trying to delay a signal by milliseconds" <<
                     " without specifying the sample rate." << std::endl <<
                     "  This has no effect." << std::endl;
    if (t == MS)
        d *= float(sample_rate)/1000;
    // now, `d' is number of samples
    if (d == 0) return;
    set_size(samples + d);
    for (unsigned long i = samples-1; i >= d; i--) {
        data[i] = data[i-d];
    }
    for (unsigned long i = 0; i < d; i++)
        data[i] = 0;
}

/**
  * Generates a new signal, which is the convolution of the caller signal
  * and a given filter impulse response (FIR).
  *
  * \param[in]  imp_resp    The filter impulse response to be convolved with.
  * \param[out] conv        The resulting signal.
  *
  * \throws `std::runtime_error` if memory alloc fails.
  *
  * \todo Resolve possible sample_rate conflicts before filtering, using
  *       the same approach as in `Signal::add()`
  *
  * \todo `imp_resp` should be `_const_ Signal&`.
  *
  * \todo Implement a DFT method, and rewrite this using overlap-and-save or
  *       overlap-and-add.
  *
  * \todo Find a way of returning `conv` without it getting destroyed at
  *       stack unwinding.
  */
void Signal::filter(Signal& imp_resp, Signal &conv) {
    conv.sample_rate = sample_rate;
    conv.set_size(samples + imp_resp.samples - 1);
    for (unsigned long i = 0; i < imp_resp.samples; i++) {
        conv.data[i] = 0;
        for (unsigned long j = 0; j <= i; j++)
            conv[i] += data[i-j] * imp_resp[j];
    }
    for (unsigned long i = imp_resp.samples; i < samples; i++) {
        conv.data[i] = 0;
        for (unsigned long j = 0; j < imp_resp.samples; j++)
            conv[i] += data[i-j] * imp_resp[j];
    }
    for (unsigned long i = 0; i < conv.samples-samples; i++) {
        conv.data[samples+i] = 0;
        for (unsigned long j = i+1; j < imp_resp.samples; j++)
            conv[samples+i] += data[samples+i-j] * imp_resp[j];
    }
}

/**
  * The PortAudio library implements the stream playback using _callback_
  * functions. These functions get called at interrupt time whenever PortAudio
  * needs a new buffer of samples to pass to the hardware. Callback functions
  * should not take long to return; in particular, they should __not__ throw
  * or catch exceptions, or doing I/O.
  *
  * This callback function just reads a given `Signal` and passes its samples
  * to PortAudio.
  *
  * \param[in]  in_buf      Pointer to a buffer of samples retrieved from an
  *                         input audio device. This parameter is unused because
  *                         we're not reading from any device.
  * \param[out] out_buf     Pointer to a buffer where the callback function will
  *                         store samples to be given to an output audio device.
  * \param[in]  frames_per_buf  Number of samples we will store in the buffer.
  *                             This is actually the number of _frames_, which
  *                             in turn is equal to number of samples because
  *                             we're working with mono-channel signals.
  * \param[in]  time_info       PortAudio time information. (unused)
  * \param[in]  status_flags    PortAudio status flags. (unused)
  * \param[in/out]  user_data   Pointer to an arbitrary data-holder passed to
  *                             the stream open function. We use this to get
  *                             the signal samples, and to keep track of where
  *                             in the signal we are (using the
  *                             `Signal::counter` auxiliary member).
  *
  * \see Signal::play
  */
static int callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    float *data = ((Signal *)user_data)->data;
    unsigned long total_samples = ((Signal *)user_data)->samples;
    unsigned long counter = ((Signal *)user_data)->counter;

    float *out = (float *) out_buf;
    (void) in_buf; // prevent unused variable warning
    (void) time_info;
    (void) status_flags;

    frames_per_buf += counter;
    for (; counter < frames_per_buf; counter++) {
        if (counter == total_samples) {
            ((Signal *)user_data)->counter = total_samples;
            return paComplete;
        }
        *out++ = data[counter];
    }

    ((Signal *)user_data)->counter = counter;
    return 0;

}

/**
  * Creates a PortAudio stream for audio playback of the signal content. If
  * `sleep` is `true`, we wait for the playback to end before returning. (If
  * it's false, the function returns, while playback goes on in the background.)
  *
  * \param[in]  sleep   Whether or not to sleep before returning.
  *
  * \throws std::runtime_error if any of the PortAudio steps fail (check the
  *         source code)
  *
  * \see callback
  */
void Signal::play(bool sleep) {

    PaStream *stream;
    PaError err;

    // open stream
    err = Pa_OpenDefaultStream(
                &stream,
                0,
                1,
                paFloat32,
                sample_rate,
                paFramesPerBufferUnspecified,
                callback,
                this
    );
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error opening stream for audio output:") +
                    "n  " + Pa_GetErrorText(err)
        );

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        throw std::runtime_error(
                    std::string("Error starting stream for audio output:") +
                    "n  " + Pa_GetErrorText(err)
        );
    }

    // sleep
    if (sleep)
        Pa_Sleep(1000 * float(samples) / sample_rate);

    // close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        throw std::runtime_error(
                    std::string("Error closing stream for audio output:") +
                    "n  " + Pa_GetErrorText(err)
        );
    }

}

/**
  * Changes the sample rate of the signal. First, we reconstruct the time-domain
  * signal by linear interpolation. Then, we re-sample the continuous-time
  * reconstructed signal at the new sample rate.
  *
  * \param[in]  sr      The new sample rate in Hertz.
  *
  * \throws std::runtime_error if memory alloc fails
  *
  * \see Signal::sample_rate
  */
void Signal::set_samplerate(int sr) {
    unsigned long N = floor((long(sr) * float(samples)) / sample_rate);
    float *ptr = (float *)malloc(N * sizeof(float));
    if (ptr == NULL)
        throw std::runtime_error("Error trying to allocate space.");
    for (unsigned long i = 0; i < N; i++) {
        float x = i*float(samples-1)/(N-1);
        ptr[i] = (floor(x+1)-x) * data[(unsigned long)floor(x)] +
                 (x-floor(x))   * data[(unsigned long)ceil(x)];
    }
    free(data);
    data = ptr;
    samples = N;
    sample_rate = sr;
}

/**
  * Adds the `other` signal to the caller signal. First, we re-sample `other`
  * into a new temporary signal. Then we increase the caller's size if needed,
  * and finally add the signals sample-by-sample.
  *
  * \param[in]  other      The signal to be added to the caller.
  */
void Signal::add(Signal &other) {
    Signal toadd;
    toadd.copyfrom(other);
    toadd.set_samplerate(sample_rate);
    if (toadd.samples > samples)
        set_size(toadd.samples);
    for (unsigned long i = 0; i < toadd.samples; i++)
        data[i] += toadd[i];
}

/**
  * Apply a gain `g` to the signal. This can be useful, for example, to make
  * sure that the signal is in the [-1, 1] range.
  *
  * \param[in]  g       The signal gain to be applied.
  */
void Signal::gain(float g) {
    for (unsigned long i = 0; i < samples; i++)
        data[i] *= g;
}

#endif // SIGNAL_H
