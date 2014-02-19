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
        *out++ = (.5*p[0].sample + p[1].sample + p[2].sample + .5*p[3].sample);
    }

    return paContinue;

}

void Stream::echo(unsigned delay, unsigned sleep) {

    PaStream *stream;
    PaError err;

    set_delay(delay);

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
    Pa_Sleep(10000);
    std::cout << "delay ";
    set_delay(1000);
    std::cout << "changed!" << std::endl;
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

void Stream::dump_state(const container_t speaker_buf) {
    std::cout << "internal: [ ";
    container_t::const_iterator it = data.begin();
    std::cout << it->sample;
    for (++it; it != data.end(); ++it)
        std::cout << ", " << it->sample;
    std::cout << " ];" << std::endl;
    std::cout << "speaker: [ ";
    it = speaker_buf.begin();
    std::cout << it->sample;
    for (++it; it != speaker_buf.end(); ++it)
        std::cout << ", " << it->sample;
    std::cout << " ];" << std::endl << std::endl;
}

void Stream::simulate() {

    PaStreamCallbackFlags flg = 0;
    static const sample_t mic_buf_arr[24+8] = {
        -987, 610, -377, 233, -144, 89, -55, 34,
        -21, 13, -8, 5, -3, 2, -1, 1,
        0, 1, 1, 2, 3, 5, 8, 13,
        21, 34, 55, 89, 144, 233, 377, 610
    };
    container_t mic_buf(mic_buf_arr, mic_buf_arr + 24+8);
    container_t speaker_buf(24+8);

    set_delay(10*1000);

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
