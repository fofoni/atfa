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
 * \file Signal.cpp
 *
 * Holds the implementation of the `Signal` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include "Signal.h"

/// PortAudio callback function
static int callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
);

/**
  * Constructs a signal getting the signal data from an audio file. This is done
  * using the [libsndfile][libsndfile] library. The filetypes supported are
  * listed [here][libsndfile_features]. WAV is supported, but MP3 is not.
  *
  * If the given file is stereo, or otherwise multi-channel, just the first
  * channel will be read. (On stereo audio files, this is the left channel.)
  *
  * The sample rate is extracted from the file's meta-data info.
  *
  * \param[in]  filename    Audio file name.
  *
  * \throws `FileError` if file openening/reading fails.
  *
  * \todo get the www hyperlinks above working
  */
Signal::Signal(const std::string &filename)
    : counter(0), srate(0)
{

    // open file
    SNDFILE *file;
    SF_INFO info;
    if (!( file = sf_open(filename.c_str(), SFM_READ, &info) ))
        throw FileError(filename);

    // check number of channels
    unsigned chans = info.channels;
    if (chans > 1)
        std::cerr << "Warning: file " << filename << " has more than one" <<
                     " channel." << std::endl << "  We will use just the" <<
                     " first (which is the left channel on stereo WAV files).";

    // set properties
    set_size(info.frames);
    srate = info.samplerate;

    // read file
    index_t wav_samples = samples() * chans;
    container_t buf(wav_samples);
    index_t items_read = sf_read_float(file, &buf[0], wav_samples);
    if (items_read != wav_samples)
        throw FileError(filename);
    for (index_t i = 0; i < samples(); ++i)
        data[i] = buf[i*chans];

}

/**
  * Adds zeroed samples at the beginning of the signal.
  *
  * \param[in]  t       A `delay_type` element.
  * \param[in]  d       The time interval to be delayed, given in the units
  *                     specified by `t`.
  */
void Signal::delay(delay_t t, unsigned long d) {
    if (srate==0 && t==MS)
        std::cerr << "Warning: trying to delay a signal by milliseconds" <<
                     " without specifying the sample rate." << std::endl <<
                     "  This has no effect." << std::endl;
    if (t == MS)
        d *= double(srate)/1000;
    // now, `d' is number of samples
    if (d == 0) return;
    set_size(samples() + d);
    // shift the samples
    std::copy_backward(data.begin(), data.end()-d, data.end());
    for (index_t i = 0; i < d; i++)
        data[i] = 0;
}

/**
  * Generates a new signal, which is the convolution of the caller signal
  * and a given filter impulse response (FIR).
  *
  * \param[in]  imp_resp    The filter impulse response to be convolved with.
  * \param[out] conv        The resulting signal.
  *
  * \todo Implement a DFT method, and rewrite this using overlap-and-save or
  *       overlap-and-add.
  *
  */
void Signal::filter(Signal imp_resp) {
    imp_resp.set_samplerate(srate);
    container_t *big, *small;
    if (samples() > imp_resp.samples())
        big = &data, small = &imp_resp.data;
    else
        big = &imp_resp.data, small = &data;
    index_t final_size = samples() + imp_resp.samples() - 1;
    container_t conv(final_size);
    if (final_size > Signal::DFTDriver::tblsize) {
        std::cout << "final bigger: " << final_size << " > "
                  << Signal::DFTDriver::tblsize << "." << std::endl;
        long N = DFTDriver::tblsize - small->size() + 1;
        if (N <= 0) {
            std::cout << "fallback to time-domain filtering" << std::endl;
            for (index_t i = 0; i < imp_resp.samples(); i++)
                for (index_t j = 0; j <= i; j++)
                    conv[i] += data[i-j] * imp_resp[j];
            for (index_t i = imp_resp.samples(); i < samples(); i++)
                for (index_t j = 0; j < imp_resp.samples(); j++)
                    conv[i] += data[i-j] * imp_resp[j];
            for (index_t i = 0; i < conv.size()-samples(); i++)
                for (index_t j = i+1; j < imp_resp.samples(); j++)
                    conv[samples()+i] += data[samples()+i-j] * imp_resp[j];
            data = conv; // destroy old data, and copy new from `conv'
            return;
        }
        // divide em pedaços de tamanho tal que N+K-1==tblsize
        container_t h_re(DFTDriver::tblsize);
        container_t h_im(DFTDriver::tblsize);
        container_t x1(DFTDriver::tblsize);
        container_t x2(DFTDriver::tblsize);
        container_t y1(DFTDriver::tblsize);
        container_t y2(DFTDriver::tblsize);
        std::copy(small->begin(), small->end(), h_re.begin());
        dft(hre, h_im);
        index_t i;
        for (i = 0; i != big->size()/(2*N); ++i) {
            index_t i1 = i*2*N;
            index_t i2 = i1 + N;
            std::copy(big->begin() + i1, big->begin() + i1 + N, x1.begin());
            std::fill(x1.begin() + N, x1.end(), 0);
            std::copy(big->begin() + i2, big->begin() + i2 + N, x2.begin());
            std::fill(x2.begin() + N, x2.end(), 0);
            dft(x1, x2);
            for (index_t j = 0; j != DFTDriver::tblsize; ++j) {
                y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
            }
            dft(y1, y2, DFTDriver::INVERSE);
            for (index_t j = 0; j != DFTDriver::tblsize; ++j) {

            }
        }
    }
    else // tenta encontrar o menor tamanho de fft que funcione com uma fft só
        std::cout << "final fits: " << final_size << " <= "
             << Signal::DFTDriver::tblsize << "." << std::endl;
}

/**
  * The PortAudio library implements the stream playback using _callback_
  * functions. These functions get called at interrupt time whenever PortAudio
  * needs a new buffer of samples to pass to the hardware. Callback functions
  * should not take long to return; in particular, they should __not__ throw
  * or catch exceptions, or do I/O.
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
  *
  * \todo this is not showing up in doxygen
  */
static int callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    const Signal::sample_t *data = ((Signal *)user_data)->array();
    Signal::index_t total_samples = ((Signal *)user_data)->samples();
    Signal::index_t& counter = ((Signal *)user_data)->counter;

    Signal::sample_t *out = (Signal::sample_t *)out_buf;
    (void) in_buf; // prevent unused variable warning
    (void) time_info;
    (void) status_flags;

    frames_per_buf += counter;
    for (; counter < frames_per_buf; counter++) {
        if (counter == total_samples) return paComplete;
        *out++ = data[counter]; // provide a sample for playback
    }

    return 0;

}

/**
  * Creates a PortAudio stream for audio playback of the signal content. If
  * `sleep` is `true`, we wait for the playback to end before returning. (If
  * it's false, the function returns, while playback goes on in the background.)
  *
  * \param[in]  sleep   If set to true, the method will only return when the
  *                     playback ends (that is, when the end of the signal is
  *                     reached). Otherwise, it returns imediatly, and the
  *                     playback goes on in the background.
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
                srate,
                paFramesPerBufferUnspecified,
                callback,
                this
    );
    if (err != paNoError){
        std::cerr << double(srate) << std::endl;
        throw std::runtime_error(
                    std::string("Error opening stream for audio output:") +
                    " " + Pa_GetErrorText(err)
        );}

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error starting stream for audio output:") +
                    " " + Pa_GetErrorText(err)
        );

    // sleep
    if (sleep)
        Pa_Sleep(1000 * double(samples()) / srate);

    // "rewind" signal
    counter = 0;

    // close stream
    err = Pa_StopStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error closing stream for audio output:") +
                    " " + Pa_GetErrorText(err)
        );

}

/**
  * Changes the sample rate of the signal. First, we reconstruct the time-domain
  * signal by linear interpolation. Then, we re-sample the continuous-time
  * reconstructed signal at the new sample rate.
  *
  * \param[in]  sr      The new sample rate in Hertz.
  *
  * \see Signal::sample_rate
  */
void Signal::set_samplerate(int sr) {
    if (srate == 0) {
        srate = sr;
        return;
    }
    index_t N = floor((sr * sample_t(samples())) / srate);
    container_t new_data(N);
    for (index_t i = 0; i < N; i++) {
        double x = i*double(samples()-1)/(N-1);
        new_data[i] = (floor(x+1)-x) * data[(index_t)floor(x)] +
                      (x-floor(x))   * data[(index_t)ceil(x)];
    }
    data = new_data;
    srate = sr;
}

/**
  * Adds the `other` signal to the caller signal. First, we re-sample `other`
  * into a new temporary signal. Then we increase the caller's size if needed,
  * and finally add the signals sample-by-sample.
  *
  * \param[in]  other      The signal to be added to the caller.
  *
  * \todo this should be `operator +=`. Make also an
  *            `operator +(a,b) { return a+=b }`
  */
void Signal::add(const Signal &other) {
    Signal toadd = other;
    toadd.set_samplerate(srate);
    if (toadd.samples() > samples())
        set_size(toadd.samples());
    for (index_t i = 0; i < toadd.samples(); i++)
        data[i] += toadd[i];
}

/**
  * Apply a gain `g` to the signal. This can be useful, for example, to make
  * sure that the signal is in the [-1, 1] range.
  *
  * \param[in]  g       The signal gain to be applied.
  *
  * \todo make a `max` routine for getting the norm-infinity.
  */
void Signal::gain(double g) {
    for (container_t::iterator it = data.begin();
         it != data.end(); ++it)
        *it *= g;
}

void Signal::DFTDriver::operator ()(container_t& re, container_t& im,
                                    dir_t direction) {

    if (re.size() != im.size()) {
        std::ostringstream msg;
        msg << "Error: Signal::DFTDriver `re' and `im' vectors must be of the "
            << "same size." << std::endl << "  Got: " << re.size()
            << " and " << im.size() << ".";
        throw std::runtime_error(msg.str());
    }

    index_t L = re.size();

    if (L > tblsize) {
        std::ostringstream msg;
        msg << "Error: DFT of size " << L << ": too big." << std::endl
            << "  Maximum is " << tblsize << ".";
        throw std::runtime_error(msg.str());
    }

    // checks whether n=L is power of two, and also calculates bits=log2(n)
    {   unsigned long n = L;
        if (L == 0) // nothing to do
            return;
        for (bits = 0; n != 1; n /= 2, ++bits)
            if (n%2 != 0) { // then L is not power of two
                std::ostringstream msg;
                msg << "Error: tried to take DFT of vector of size " << L
                    << " (not power of two).";
                throw std::runtime_error(msg.str());
            }
        // here, 2^bits == L
    }

    // reverse bits
    for (unsigned i = 1; i != L-1; ++i) {
        unsigned j = br(i, bits);
        if (i >= j) continue;
        double temp = re[i];
        re[i] = re[j];
        re[j] = temp;
        temp = im[i];
        im[i] = im[j];
        im[j] = temp;
    }

    for (unsigned size = 1; size != L; size *= 2) {
        for (unsigned k = 0; k != L/2/size; ++k) {
            for (unsigned l = 0; l != size; ++l) {

                // apply single butterfly at indexes i1 and i2,
                // with coefficient exp(j*tau*frac) at i2
                // `frac' is an angle expressed in one-`L'ths of tau,
                // which is 2*pi.

                unsigned i1 = 2*size*k + l;
                unsigned i2 = i1 + size;

                unsigned frac = L - L/2/size*l; // integer division in 2nd term
                double cossine = Wre(frac);
                double sine = direction==DIRECT ? Wim(frac) : -Wim(frac);

                double tmp_re1 = re[i1];
                double tmp_im1 = im[i1];
                double tmp_re2 = re[i2] * cossine - im[i2] * sine;
                double tmp_im2 = re[i2] * sine    + im[i2] * cossine;

                re[i1] = tmp_re1 + tmp_re2;
                im[i1] = tmp_im1 + tmp_im2;
                re[i2] = tmp_re1 - tmp_re2;
                im[i2] = tmp_im1 - tmp_im2;

            }
        }
    }

    if (direction==INVERSE)
        for (unsigned i = 0; i != L; ++i)
            re[i] /= L, im[i] /= L;

}
