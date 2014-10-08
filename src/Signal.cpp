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

#include <iostream>
#include <stdexcept>

extern "C" {
#   include <portaudio.h>
#   include <sndfile.hh>
}

#include "Signal.h"

const std::vector<double> Signal::DFTDriver::costbl =
        Signal::DFTDriver::initialize_costbl();
const std::vector<double> Signal::DFTDriver::sintbl =
        Signal::DFTDriver::initialize_sintbl();

Signal::DFTDriver Signal::dft;

/// PortAudio callback function
static int signal_callback(
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
  * [libsndfile]: http://www.mega-nerd.com/libsndfile/
  * [libsndfile_features]: http://www.mega-nerd.com/libsndfile/#Features
  *
  * \param[in]  filename    Audio file name.
  *
  * \todo add "extern C" directive (speed?)
  *
  * \throws FileError if file openening/reading fails.
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
  * If we try do delay a signal by milliseconds, but the signal has no
  * associated sample rate, a warning is emitted, and nothing is done. No
  * exception is thrown.
  *
  * \param[in]  t       A delay type element.
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
  * Convolves the signal with the given finite impulse response (FIR).
  *
  * The algorthm used is the "overlap-and-add", and we use the FFT implemented
  * in the DFTDriver class to compute each step. We try to do it using the least
  * possible number of DFTs.
  *
  * \param[in]  imp_resp    The filter impulse response to be convolved with.
  *
  * \see DFTDriver::operator()
  */
void Signal::filter(Signal imp_resp) {
    imp_resp.set_samplerate(srate);
    index_t final_size = samples() + imp_resp.samples() - 1;
    if (final_size > Signal::DFTDriver::tblsize) {
        // divide the signal in chunks of size N such that N+K-1==tblsize
        container_t *big, *small;
        if (samples() > imp_resp.samples())
            big = &data, small = &imp_resp.data;
        else
            big = &imp_resp.data, small = &data;
        long N = DFTDriver::tblsize - small->size() + 1;
        container_t conv(final_size);
        if (N <= 0) {
            // fall back to time-domain filtering
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
        container_t h_re(DFTDriver::tblsize);
        container_t h_im(DFTDriver::tblsize);
        container_t x1(DFTDriver::tblsize);
        container_t x2(DFTDriver::tblsize);
        container_t y1(DFTDriver::tblsize);
        container_t y2(DFTDriver::tblsize);
        std::copy(small->begin(), small->end(), h_re.begin());
        dft(h_re, h_im);
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
                conv[i1 + j] += y1[j];
                conv[i2 + j] += y2[j];
            }
        }
        {
            index_t i1 = i*2*N;
            index_t i2 = i1 + N;
            long L = big->size() - i1;
            if (L > N) {
                std::copy(big->begin() + i1, big->begin() + i1 + N, x1.begin());
                std::fill(x1.begin() + N, x1.end(), 0);
                std::copy(big->begin() + i2, big->end(), x2.begin());
                std::fill(x2.begin() + (L-N), x2.end(), 0);
                dft(x1, x2);
                for (index_t j = 0; j != DFTDriver::tblsize; ++j) {
                    y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                    y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
                }
                dft(y1, y2, DFTDriver::INVERSE);
                for (index_t j = 0; j != DFTDriver::tblsize; ++j)
                    conv[i1 + j] += y1[j];
                for (index_t j = 0; j != L-N+small->size()-1; ++j)
                    conv[i2 + j] += y2[j];
            }
            else {
                std::copy(big->begin() + i1, big->end(), x1.begin());
                std::fill(x1.begin() + L, x1.end(), 0);
                std::fill(x2.begin(), x2.end(), 0);
                dft(x1, x2);
                for (index_t j = 0; j != DFTDriver::tblsize; ++j) {
                    y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                    y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
                }
                dft(y1, y2, DFTDriver::INVERSE);
                for (index_t j = 0; j != L+small->size()-1; ++j)
                    conv[i1 + j] += y1[j];
            }
        }
        data = conv; // destroy old data, and copy new from `conv'
    }
    else {
        // fit the whole signal in only one fft
        index_t L;
        for (L = DFTDriver::tblsize; L/2 >= final_size; L /= 2) ;
        // here, final_size fits in L
        container_t h_re(L);
        container_t h_im(L);
        container_t x1(L);
        container_t x2(L);
        container_t y1(L);
        container_t y2(L);
        std::copy(imp_resp.data.begin(), imp_resp.data.end(), h_re.begin());
        dft(h_re, h_im);
        std::copy(data.begin(), data.end(), x1.begin());
        dft(x1, x2);
        for (index_t j = 0; j != DFTDriver::tblsize; ++j) {
            y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
            y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
        }
        dft(y1, y2, DFTDriver::INVERSE);
        set_size(final_size);
        std::copy(y1.begin(), y1.begin() + final_size, data.begin());
    }
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
  * \param[in,out]  user_data   Pointer to an arbitrary data-holder passed to
  *                             the stream open function. We use this to get
  *                             the signal samples, and to keep track of where
  *                             in the signal we are (using the
  *                             `Signal::counter` auxiliary member).
  *
  * \see Signal::play
  */
static int signal_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    const Signal::sample_t *data = ((Signal *)user_data)->array();
    Signal::index_t total_samples = ((Signal *)user_data)->samples();
    Signal::index_t& counter = ((Signal *)user_data)->counter;

    Signal::sample_t *out = static_cast<Signal::sample_t *>(out_buf);
    (void) in_buf; // prevent unused variable warning
    (void) time_info;
    (void) status_flags;

    frames_per_buf += counter;
    for (; counter < frames_per_buf; counter++) {
        if (counter == total_samples) return paComplete;
        *out++ = data[counter]; // provide a sample for playback
    }

    return paContinue;

}

/**
  * Creates a PortAudio session for audio playback of the signal content. If
  * \a sleep is `true`, we wait for the playback to end before returning. (If
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
  * \see signal_callback
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
                signal_callback,
                this
    );
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error opening stream for audio output:") +
                    " " + Pa_GetErrorText(err)
        );

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
  * Changes the sample rate of the signal. The way it is done, this is
  * equivalent to reconstructing the time-domain signal by linear interpolation,
  * and then re-sampling the continuous-time reconstructed signal at the new
  * sample rate.
  *
  * \param[in]  sr      The new sample rate in Hertz.
  *
  * \see srate
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
  * First, we re-sample \a other into a new temporary signal. Then we increase
  * the caller's size if needed, and finally add the signals sample-by-sample.
  *
  * \param[in]  other      The signal to be added to the caller.
  * \returns    a reference to this signal, already added to the `other`.
  */
Signal& Signal::operator +=(Signal other) {
    other.set_samplerate(srate);
    if (other.samples() > samples())
        set_size(other.samples());
    for (index_t i = 0; i < other.samples(); i++)
        data[i] += other[i];
    return *this;
}

/**
  * This can be useful, for example, to make sure that the signal is within the
  * \f$\left[-1,1\right]\f$ range.
  *
  * \param[in]  g       The signal gain to be applied.
  */
void Signal::gain(double g) {
    for (container_t::iterator it = data.begin();
         it != data.end(); ++it)
        *it *= g;
}

/**
  * Take the signal's infinity-norm, which is the maximum absolute value of all
  * the samples of the signal.
  *
  * \returns    the \f$\ell^\infty\f$-norm of the signal.
  */
Signal::sample_t Signal::l_inf_norm() {
    sample_t max = 0;
    for (container_t::const_iterator it = data.begin(); it != data.end(); ++it)
        if (std::abs(*it) > max) max = std::abs(*it);
    return max;
}

/**
  * Implements the radix-2 time-decimation FFT algorithm. The computation
  * happens in-place, which means that the \a re and \a im parameters are
  * substituted by their new versions.
  *
  * Of course, the \a re and \a im vectors must be of the same size. This size
  * must be a power of two not greater than \ref tblsize.
  *
  * Refer to the DFTDriver class documentation for usage details.
  *
  * \throws std::runtime_error if any of the above conditions aren't met.
  *
  * \param[in,out]  re  Real part of the compelx signal on which the FFT will
  *                     act.
  * \param[in,out]  im  Imaginary part.
  * \param[in]      direction   Whether this is a direct or inverse DFT.
  */
void Signal::DFTDriver::operator ()(container_t& re, container_t& im,
                                    const dir_t direction) {

/*    if (re.size() != im.size()) {
        std::ostringstream msg;
        msg << "Error: Signal::DFTDriver `re' and `im' vectors must be of the "
            << "same size." << std::endl << "  Got: " << re.size()
            << " and " << im.size() << ".";
        throw std::runtime_error(msg.str());
    }*/

    index_t L = re.size();

/*    if (L > tblsize) {
        std::ostringstream msg;
        msg << "Error: DFT of size " << L << ": too big." << std::endl
            << "  Maximum is " << tblsize << ".";
        throw std::runtime_error(msg.str());
    }*/

    // checks whether n=L is power of two, and also calculates bits=log2(n)
    {   unsigned long n = L;
        if (L == 0) // nothing to do
            return;
        for (bits = 0; n != 1; n /= 2, ++bits)
;/*            if (n%2 != 0) { // then L is not power of two
                std::ostringstream msg;
                msg << "Error: tried to take DFT of vector of size " << L
                    << " (not power of two).";
                throw std::runtime_error(msg.str());
            }*/
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
                // with coefficient exp(j*tau*frac) at i2.
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
