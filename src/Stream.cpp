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
#include <chrono>

#include "Stream.h"
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
  *
  * \todo Instead of calling Stream::get_filtered_sample() for each sample, we
  *       should use the Signal::filter mechanism, which uses the more efficient
  *       overlap-and-add algorithm. Perhaps we will need to force the
  *       _frames per buffer_ PortAudio parameter to be of a specific value.
  */
static int stream_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    Stream * const data = static_cast<Stream *>(user_data);

    (void) time_info; // prevent unused variable warning
    (void) status_flags;

//    float *ob=(float*)out_buf;
//    const float *in=(const float *) in_buf;
//    for (int i=0; i<frames_per_buf; ++i)
//        *ob++ = *in++;

    data->read_write(static_cast<const Stream::sample_t *>(in_buf),
                     static_cast<Stream::sample_t *>(out_buf),
                     frames_per_buf);

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
                1,  // num. input channels
                1,  // num. output channels
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

    rir_thread = new std::thread(&Stream::rir_fft, this);

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error starting stream for audio I/O:") +
                    " " + Pa_GetErrorText(err)
        );

    {
        std::lock_guard<std::mutex> lk(running_mutex);
        is_running = true;
    }

//    constexpr size_t blk_len = 128;

//    std::vector<sample_t> ib(blk_len);
//    std::vector<sample_t> ob(blk_len);

//    for (unsigned long coe = 0; coe < 400; coe++)
//        read_write(&(ib[0]), &(ob[0]), blk_len);

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

    {
        std::lock_guard<std::mutex> lk(running_mutex);
        is_running = false;
    }

    rir_thread->join();
    delete rir_thread;
    rir_thread = nullptr;

}

void Stream::rir_fft() {
    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (!running())
            break;
    }
}

/**
  * This function just sets the internal copy of the room impulse response (RIR)
  * samples to be equal to the one specified.
  *
  * \param[in]  h   A vector containing the RIR samples
  */
void Stream::set_filter(const container_t& h) {
    scene.imp_resp = h;
}
