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

static int stream_in_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    Stream *data = static_cast<Stream *>(user_data);

    (void) out_buf; // prevent unused variable warning
    const Stream::sample_t *in = static_cast<const Stream::sample_t *>(in_buf);
    (void) time_info;
    (void) status_flags;

    if (in_buf == NULL)
        for (unsigned long i = 0; i != frames_per_buf; ++i)
            data->write(0);
    else
        for (unsigned long i = 0; i != frames_per_buf; ++i)
            data->write(*in++);

    return paContinue;

}

void Stream::echo(unsigned sleep) {

    PaStream *stream;
    PaError err;

    // open stream
    err = Pa_OpenDefaultStream(
                &stream,
                1,
                0,
                paFloat32,
                samplerate,
                paFramesPerBufferUnspecified,
                stream_in_callback,
                this
    );
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error opening stream for audio input:") +
                    " " + Pa_GetErrorText(err)
        );

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error starting stream for audio input:") +
                    " " + Pa_GetErrorText(err)
        );

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
