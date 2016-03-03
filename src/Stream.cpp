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

#ifdef ATFA_DEBUG
#include <iostream>
#include <algorithm>
#endif

#include "Stream.h"
#include "utils.h"

/// Callback function for dealing with PortAudio
static int stream_callback(
        const void *in_buf, void *out_buf, unsigned long frames_per_buf,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags, void *user_data
);

static float teste1[1024];
static float teste2[1024];

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

    float *ib = (float*)in_buf;
    float *ob = (float*)out_buf;

    for (unsigned i=0; i<frames_per_buf/4; ++i) {
        teste1[i] = (ib[4*i]+ib[4*i+1]+ib[4*i+2]+ib[4*i+3])/4;
    }

    data->read_write(static_cast<const Stream::sample_t *>(teste1),
                     static_cast<Stream::sample_t *>(teste2),
                     frames_per_buf/4);

    if (frames_per_buf/4 == 0) return paContinue;

    ob[0] = teste2[0];
    ob[1] = teste2[0];
    ob[2] = teste2[0];
    ob[3] = teste2[0];

    for (unsigned i=1; i<frames_per_buf/4; ++i) {
        ob[4*i]   = teste2[i-1]*0.75 + teste2[i]*0.25;
        ob[4*i+1] = teste2[i-1]*0.50 + teste2[i]*0.50;
        ob[4*i+2] = teste2[i-1]*0.25 + teste2[i]*0.75;
        ob[4*i+3] =                    teste2[i]*1.00;
    }

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

#ifndef ATFA_DEBUG
    PaStream *stream;
    PaError err;

    // no need for mutex, because the rir_thread has not started yet
    is_running = true;

    // the rir_thread will wait until it is notified, so no problem starting it
    // right away.
    rir_thread = new std::thread(&Stream::rir_fft, this);

    blk_count = 0;
    blk_offset = 0;
    std::fill(data_in.begin(),  data_in.end(),  0);
    std::fill(data_out.begin(), data_out.end(), 0);
    write_ptr = data_in.begin();
    read_ptr  = data_out.begin();
    rir_ptr   = data_in.begin();
    set_delay(scene.delay);

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

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError)
        throw std::runtime_error(
                    std::string("Error starting stream for audio I/O:") +
                    " " + Pa_GetErrorText(err)
        );

    return stream;

#define SCOUT(COE) do {} while(0)
#define SDUMP(N)
#else
#define SCOUT(COE) do { \
    std::lock_guard<std::mutex> lk(io_mutex); \
    std::cout << "[main] " << COE << std::endl; \
} while(0)

    SCOUT("======= ECHO =======");
    std::vector<sample_t> ib(500);
    std::vector<sample_t> ob(500);
    sample_t g_n=0;
    std::generate(ib.begin(), ib.end(), [&g_n]{g_n+=.002; return g_n-.002;});
#define SDUMP(N) do { \
    { \
        std::unique_lock<std::mutex> lk1(io_mutex, std::defer_lock); \
        std::unique_lock<std::mutex> lk2(blk_mutex, std::defer_lock); \
        std::lock(lk1, lk2); \
        std::cout << "[main] blk_count = " << blk_count << std::endl; \
    } \
    SCOUT("blk_offset = " << blk_offset); \
    SCOUT("write_ptr = data_in.begin()  + " << (write_ptr - data_in.begin())); \
    SCOUT("read_ptr  = data_out.begin() + " << (read_ptr - data_out.begin())); \
    { \
        std::lock_guard<std::mutex> lk(io_mutex); \
        std::cout << "[main] ob -> ["; \
        for (auto it=ob.begin(); it<ob.begin()+((N)-1); ++it) { \
            std::cout << *it << ", "; \
        } \
        std::cout << *(ob.begin()+((N)-1)) << "]" << std::endl; \
    } \
} while(0)
    SDUMP(500);
    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "[main] ib -> [";
        for (auto it=ib.begin(); it<ib.begin()+499; ++it) {
            std::cout << *it << ", ";
        }
        std::cout << *(ib.begin()+499) << "]" << std::endl;
    }

    PaStreamCallbackFlags status_flags;

    stream_callback(&(ib[0]), &(ob[0]), 10, nullptr, status_flags, this);
    SDUMP(10);
    stream_callback(&(ib[0]), &(ob[0]), 10, nullptr, status_flags, this);
    SDUMP(10);
    stream_callback(&(ib[0]), &(ob[0]), 120, nullptr, status_flags, this);
    SDUMP(120);
    stream_callback(&(ib[0]), &(ob[0]), 130, nullptr, status_flags, this);
    SDUMP(130);
    stream_callback(&(ib[0]), &(ob[0]), 130, nullptr, status_flags, this);
    SDUMP(130);
    stream_callback(&(ib[0]), &(ob[0]), 130, nullptr, status_flags, this);
    SDUMP(130);

    SCOUT("======= end ECHO =======");
    return nullptr;
#endif

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

#ifndef ATFA_DEBUG
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
#endif

#ifdef ATFA_DEBUG
    (void)s;
    SCOUT("======= STOP =======");
#endif

    {
        std::lock_guard<std::mutex> lk(running_mutex);
        is_running = false;
    }
    SCOUT("is_running = false!");
    {
        std::lock_guard<std::mutex> lk(blk_mutex);
        blk_count = -1;
    }
    SCOUT("blk_count set to -1");
    blk_cv.notify_one();
    SCOUT("blk_cv notified");

    rir_thread->join();
    SCOUT("rir_thread joined");
    delete rir_thread;
    SCOUT("rir_thread deleted");
    rir_thread = nullptr;

}

void Stream::rir_fft() {
#ifdef ATFA_DEBUG
#define RCOUT(COE) do { \
    std::lock_guard<std::mutex> lk(io_mutex); \
    std::cout << "[rir_thread] " << COE << std::endl; \
} while(0)
#else
#define RCOUT(COE) do {} while (0)
#endif
    RCOUT("spawned!");
    while (running()) {
        RCOUT("=== running ===");
        int count;
        std::unique_lock<std::mutex> lk(blk_mutex);
        blk_cv.wait(lk, [this]{return blk_count != 0;});
        count = blk_count;
        blk_count = 0;
        lk.unlock();
        RCOUT("count = " << count);
        if (count < 0) return;
        RCOUT("gonna process " << count << " block(s).");
        for (int i=0; i<count; ++i) {
            // process a single block.
            pa_fperbuf_t remaining = data_out.end() - filter_ptr;
            long overflow = (long)blk_size - (long)remaining;
            auto rir_end_ptr = rir_ptr + blk_size;
            if (overflow < 0) {
                filter_ptr = std::copy(rir_ptr, rir_end_ptr, filter_ptr);
            }
            else {
                auto rir_rem_ptr = rir_ptr + remaining;
                std::copy(rir_ptr, rir_rem_ptr, filter_ptr);
                filter_ptr = std::copy(rir_rem_ptr, rir_end_ptr,
                                       data_out.begin());
            }
            if (rir_end_ptr == data_in.end())
                rir_ptr = data_in.begin();
            else
                rir_ptr = rir_end_ptr;
#ifdef ATFA_DEBUG
            RCOUT("rir_ptr    = data_in.begin()  + " <<
                  (rir_ptr    - data_in.begin()));
            RCOUT("filter_ptr = data_out.begin() + " <<
                  (filter_ptr - data_out.begin()));
#endif
        }
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
