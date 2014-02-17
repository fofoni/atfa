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

#include "Stream.h"

static int stream_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    Stream *data = static_cast<Stream *>(user_data);

    Stream::sample_t *out = static_cast<Stream::sample_t *>(out_buf);
    const Stream::sample_t *in = static_cast<const Stream::sample_t *>(in_buf);
    (void) time_info; // prevent unused variable warning
    (void) status_flags;

    for (unsigned long i = 0; i != frames_per_buf; ++i) {
        data->write(*in++);
        Stream::container_t::const_iterator p = data->get_last_n(4);
        *out++ = (p[0].sample+p[1].sample+p[2].sample+p[3].sample);
    }

    return paContinue;

}

void Stream::echo(unsigned delay, unsigned sleep) {

    PaStream *stream;
    PaError err;

    set_delay(delay);

//    sample_t mic_buf[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
//                          16, 17, 18, 19, 20};
//    sample_t spk_buf[20];
//    PaStreamCallbackFlags flg;

//    dump_state(spk_buf);
//    stream_callback(mic_buf, spk_buf, 4, NULL, flg, this);
//    dump_state(spk_buf);

    // open i/o stream
    err = Pa_OpenDefaultStream(
                &stream,
                1,
                1,
                paFloat32,
                samplerate,
                paFramesPerBufferUnspecified,
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

    // sleep
    if (sleep == 0)
        while (Pa_IsStreamActive(stream))
            Pa_Sleep(1000);
    else
        Pa_Sleep(sleep);

    // close stream
    err = Pa_StopStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error closing stream for audio input:") +
                    " " + Pa_GetErrorText(err)
        );

}
