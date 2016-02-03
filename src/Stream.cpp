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
 * \file Stream.cpp
 *
 * Holds the implementation of the `Stream` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

extern "C" {
#   include <portaudio.h>
}

#include <iostream>

#include "Stream.h"
#include "Signal.h"
#include "utils.h"

/// Callback function for dealing with PortAudio
static int stream_callback(
        const void *in_buf, void *out_buf, unsigned long frames_per_buf,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags, void *user_data
);


/**
  * See the description for the `signal_callback()` function, in the Signal.cpp
  * file for information on how this function accomplishes audio I/O, together
  * with PortAudio.
  *
  * \param[in]  in_buf      Pointer to a buffer of samples retrieved from an
  *                         input audio device. We read these samples into the
  *                         stream, at the location pointed to by the Stream's
  *                         Stream::write_ptr.
  * \param[out] out_buf     Pointer to a buffer where the callback function will
  *                         store samples to be given to an output audio device.
  *                         We write to this location the samples we get from
  *                         the stream through the Stream::read_ptr pointer.
  * \param[in]  frames_per_buf  Number of samples we will write/read to/from the
  *                             PortAudio buffers. This is actually the number
  *                             of _frames_, which in turn is equal to number of
  *                             samples because we're working with mono-channel
  *                             signals.
  * \param[in]  time_info       PortAudio time information. (unused)
  * \param[in]  status_flags    PortAudio status flags. (unused)
  * \param[in,out]  user_data   Pointer to an arbitrary data-holder passed to
  *                             the stream open function. In this case, this is
  *                             a pointer to the Stream object.
  *
  * \see Stream::echo
  */
static int stream_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    Stream * const data =
        static_cast<Stream *>(user_data);
    Stream::sample_t *out = static_cast<Stream::sample_t *>(out_buf);
    const Stream::sample_t *in = static_cast<const Stream::sample_t *>(in_buf);
    (void) time_info; // prevent unused variable warning
    (void) status_flags;

    // TODO: fazer duas FFTs pequenas, ao invés de uma gigante, e usar o truque
    // da parte imaginária.
    std::copy(in, in + frames_per_buf, data->temp_container1_re.begin());
    std::fill(data->tempcont1_mid, data->temp_container1_re.end(), 0);
    std::fill(data->temp_container1_im.begin(),
              data->temp_container1_im.end(), 0);
    Signal::dft(data->temp_container1_re, data->temp_container1_im);
    for (unsigned long i = 0; i != 2*frames_per_buf; ++i) {
        data->temp_container2_re[i] =
            data->temp_container1_re[i] * data->h_freq_re[i] -
            data->temp_container1_im[i] * data->h_freq_im[i];
        data->temp_container2_im[i] =
            data->temp_container1_re[i] * data->h_freq_im[i] +
            data->temp_container1_im[i] * data->h_freq_re[i];
    }
    Signal::dft(data->temp_container2_re, data->temp_container2_im,
                Signal::DFTDriver::INVERSE);

    Stream::container_t::const_iterator it = data->temp_container2_re.begin();
    bool prev_is_geq0 = true;
    int zccount = 0;
    for (; it != data->tempcont2_mid; ++it) {
        // microphone --(filter)--> memory       (part 1)
        if (data->write_add(*it) >= 0) {
            if (!prev_is_geq0)
                ++zccount;
            prev_is_geq0 = true;
        }
        else
            prev_is_geq0 = false;
    }
    data->write_zcr(zccount);
    // octave:14> zc2 = conv(zc, [1 1 1 1 1 1 1 1 1 1 1 1 1]/13)(7:end-6);
    // octave:15> vad = (kron(zc2,ones(1,128))-14.933).^2 > 8.6044;
    for (; it != data->temp_container2_re.end(); ++it) {
        /* by now, it is already possible to start sending audio samples to the
           speaker buffer, and we want to do this as soon as possible. */
        // memory ----> speaker
        *out++ = 4 * data->scene.volume * data->read();
        // microphone --(filter)--> memory       (part 2)
        data->write(*it);
    }
    data->ooa_rewind_wptr();

    return paContinue;

}

/**
  * This is one of the main methods in the Stream class. It runs the stream,
  * simulating a communications environment in which the user listens to echoes
  * of his own voice.
  *
  * Creates a PortAudio session for audio I/O.
  *
  * To use this method, you should first set the scenario parameters using the
  * methods `set_filter` and `set_delay`.
  *
  * \throws std::runtime_error if any of the PortAudio steps fail (check the
  *         source code)
  *
  * \see Stream
  * \see stream_callback
  * \see stop
  */
PaStream *Stream::echo() {

    PaStream *stream;
    PaError err;

    // initialize portaudio
    portaudio_init();

    // open i/o stream
    err = Pa_OpenDefaultStream(
                &stream,
                1,
                1,
                paFloat32,
                samplerate,
                pa_framespb,
                stream_callback,
                this
    );
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error opening stream for audio I/O:") +
                    " " + Pa_GetErrorText(err)
        );

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error starting stream for audio I/O:") +
                    " " + Pa_GetErrorText(err)
        );

    return stream;

}

/**
  * This method closes a PortAudio session created by `echo()`.
  *
  * \throws std::runtime_error if any of the PortAudio steps fail (check the
  *         source code)
  *
  * \see Stream
  * \see stream_callback
  * \see echo
  */
void Stream::stop(PaStream *s) {

    PaError err;

    // close stream
    err = Pa_StopStream(s);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error closing stream for audio input:") +
                    " " + Pa_GetErrorText(err)
        );

    // close portaudio
    portaudio_end();

    X9();

}

/**
  * This function just sets the internal copy of the room impulse response (RIR)
  * samples to be equal to the one specified.
  *
  * The caller must make sure that _h_ has at most _pa_framespb_ plus 1
  * elements.
  *
  * \param[in]  h   A vector containing the RIR samples
  */
void Stream::set_filter(const container_t &h) {
    scene.imp_resp = h;
    update_freq_resp();
}

#include "autogen/filter200Hz.h"

void Stream::update_freq_resp() {
    std::copy(scene.imp_resp.begin(), scene.imp_resp.end(), h_freq_re.begin());
    std::fill(h_freq_re.begin() + scene.imp_resp.size(), h_freq_re.end(), 0);
    std::fill(h_freq_im.begin(), h_freq_im.end(), 0);
    Signal::dft(h_freq_re, h_freq_im);
    // now, filter out the 0~200Hz range, to help the VAD
    for (unsigned long k = 0; k != h_freq_re.size(); ++k) {
        sample_t temp_re = h_freq_re[k] * filter200Hz_re[k] -
                           h_freq_im[k] * filter200Hz_im[k];
        sample_t temp_im = h_freq_re[k] * filter200Hz_im[k] +
                           h_freq_im[k] * filter200Hz_re[k];
        h_freq_re[k] = temp_re;
        h_freq_im[k] = temp_im;
    }
}


/**
  * This function is called by `simulate()` to print to the screen the current
  * state of the simulated environment. It prints the internal state of the
  * Stream object and the samples that have been written to the output.
  *
  * \param[in]  speaker_buf     A vector with the samples that have been sent
  *                             to the output (speaker).
  *
  * \see simulate
  */
void Stream::dump_state(const container_t speaker_buf) const {
    std::cout << "internal: [ ";
    container_t::const_iterator it = data.begin();
    std::cout << *it;
    for (++it; it != data.end(); ++it)
        std::cout << ", " << *it;
    std::cout << " ];" << std::endl;
    std::cout << "speaker: [ ";
    it = speaker_buf.begin();
    std::cout << *it;
    for (++it; it != speaker_buf.end(); ++it)
        std::cout << ", " << *it;
    std::cout << " ];" << std::endl << std::endl;
}

/**
  * This function simulates the PortAudio functioning by calling the callback
  * function some four times, each time providing it with different in/out
  * buffers.
  *
  * \see dump_state
  */
void Stream::simulate() {

    PaStreamCallbackFlags flg = 0;
    static const sample_t mic_buf_arr[64] = {
        -987, 610, -377, 233, -144, 89, -55, 34,
        -21, 13, -8, 5, -3, 2, -1, 1,
        0, 1, 1, 2, 3, 5, 8, 13,
        21, 34, 55, 89, 144, 233, 377, 610,
        1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0
    };
    container_t mic_buf(mic_buf_arr, mic_buf_arr + 64);
    container_t speaker_buf(64);

    set_delay(16*1000);

    dump_state(speaker_buf);
    stream_callback(&mic_buf[0], &speaker_buf[0], 8, NULL, flg, this);
    dump_state(speaker_buf);
    stream_callback(&mic_buf[8], &speaker_buf[8], 8, NULL, flg, this);
    dump_state(speaker_buf);
    stream_callback(&mic_buf[16], &speaker_buf[16], 8, NULL, flg, this);
    dump_state(speaker_buf);
    stream_callback(&mic_buf[24], &speaker_buf[24], 8, NULL, flg, this);
    dump_state(speaker_buf);

}
