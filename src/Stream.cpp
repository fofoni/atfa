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
    Stream::index_t& delay_counter = data->delay_counter;
    const unsigned& delay_samples = data->delay_samples;

    Stream::sample_t *out = static_cast<Stream::sample_t *>(out_buf);
    const Stream::sample_t *in = static_cast<const Stream::sample_t *>(in_buf);
    (void) time_info; // prevent unused variable warning
    (void) status_flags;

    if (delay_counter > delay_samples) {
        if (in_buf != NULL && out_buf != NULL) {
            for (unsigned long i = 0; i != frames_per_buf; ++i) {
                data->write(*in++);
                *out++ = 5*data->read();
            }
            delay_counter += frames_per_buf;
        }
    }
    else {
        if (in_buf != NULL) {
            for (unsigned long i = 0; i != frames_per_buf; ++i) {
                data->write(*in++);
            }
            delay_counter += frames_per_buf;
        }
    }

    return paContinue;

}

void Stream::echo(int delay, unsigned sleep) {

    PaStream *stream;
    PaError err;

    delay_samples = samplerate * delay/1000;

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
