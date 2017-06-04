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

/*
 * TODO: olhar todos os 'throw's, ao longo do projeto inteiro, e se
 * certificar de que eles são pegos por um 'catch', que gera uma error
 * dialog, ao invés de crashar o programa.
 */

/*
 * TODO: trocar todos os do{...}while(0) por if(1){...}else(void)0 .
 * Rationale:
 * https://isocpp.org/wiki/faq/misc-technical-issues#macros-with-multi-stmts
 */

// TODO: rever todos os comentários de documentação

extern "C" {
#   include <portaudio.h>
}

#include <exception>

#ifdef ATFA_LOG_MATLAB
# include <iostream>
# include <algorithm>
# include <fstream>
# include <mat.h>
#endif

#include <cmath>
#include <random>

#include "Stream.h"
#include "Signal.h"
#include "VAD.h"
#include "AdaptiveFilter.h"
#include "utils.h"

#ifdef ATFA_LOG_MATLAB
Stream::sample_t Stream::wvec[Stream::blks_in_buf*Stream::blk_size][128];
#endif

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
  */
static int stream_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    if (frames_per_buf/4 == 0) return paContinue;

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

    ob[0] = teste2[0];
    ob[1] = teste2[0];
    ob[2] = teste2[0];
    ob[3] = teste2[0];

    for (unsigned i=1; i<frames_per_buf/4; ++i) {
        ob[4*i]   = teste2[i-1]*0.75f + teste2[i]*0.25f;
        ob[4*i+1] = teste2[i-1]*0.50f + teste2[i]*0.50f;
        ob[4*i+2] = teste2[i-1]*0.25f + teste2[i]*0.75f;
        ob[4*i+3] =                     teste2[i]*1.00f;
    }

    return paContinue;

}

void Stream::set_scene(const Scenario& new_scene) {
    scene = new_scene;
    set_delay(static_cast<unsigned>(scene.delay - scene.system_latency));
    set_filter(scene.imp_resp);
    setAdapfAlgorithm(new AdaptiveFilter<sample_t>(scene.adapf_file));
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

    adapf->initialize_data_structures();

    // no need for mutex, because the rir_thread has not started yet
    is_running = true;

    // the rir_thread will wait until it is notified, so no problem starting it
    // right away.
    rir_thread = new std::thread(&Stream::rir_fft, this);

    blk_count = 0;
    blk_offset = 0;
    std::fill(data_in.begin(),  data_in.end(),  0);
    std::fill(data_out.begin(), data_out.end(), 0);
    {
        std::mt19937 rng;
        std::normal_distribution<> gauss{0, std::pow(10,scene.noise_vol/20)};
        for (auto& x : awgn) x = gauss(rng);
    }
    write_ptr = data_in.begin();
    read_ptr  = data_out.begin();
    rir_ptr   = data_in.begin();
    awgn_ptr  = awgn.begin();
    auto stream_delay =
            static_cast<unsigned>(scene.delay - scene.system_latency);
    if (stream_delay < min_delay)
        throw std::out_of_range("[Stream::echo] Stream delay (delay minus"
                                " system latency) cannot be less than the"
                                " duration of one block.");
    set_delay(stream_delay);

#ifndef ATFA_DEBUG
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
                samplerate*4,
                paFramesPerBufferUnspecified,
                &stream_callback,
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
#define SDUMP(N) do {} while(0)
#else // ATFA_DEBUG
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
    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "[main] h_freq_re -> [";
        auto it=h_freq_re.begin();
        for (;
             it != h_freq_re.begin() + (h_freq_re.size() - 1);
             ++it)
        {
            std::cout << *it << ", ";
        }
        std::cout << *it << "]" << std::endl;
    }
    {
        std::lock_guard<std::mutex> lk(io_mutex);
        std::cout << "[main] h_freq_im -> [";
        auto it=h_freq_im.begin();
        for (;
             it != h_freq_im.begin() + (h_freq_im.size() - 1);
             ++it)
        {
            std::cout << *it << ", ";
        }
        std::cout << *it << "]" << std::endl;
    }

    PaStreamCallbackFlags status_flags;

#define CALLBACK_THEN_DUMP(N) do { \
    stream_callback(&(ib[0]), &(ob[0]), (N), nullptr, status_flags, this); \
    SDUMP((N)); \
} while (0)
    CALLBACK_THEN_DUMP(8);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);
    CALLBACK_THEN_DUMP(12);
    CALLBACK_THEN_DUMP(8);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);
    CALLBACK_THEN_DUMP(12);
    CALLBACK_THEN_DUMP(8);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);
    CALLBACK_THEN_DUMP(256);
    CALLBACK_THEN_DUMP(300);

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

#ifdef ATFA_LOG_MATLAB
# define ADBG_PASTE(x,y) x##y
# define MXVAR(NOME) ADBG_PASTE(mx_,NOME)
# define ADBG_STRINGIFY2(x) #x
# define ADBG_STRINGIFY(x) ADBG_STRINGIFY2(x)
# define STRMXVAR(NOME) ADBG_STRINGIFY(MXVAR(NOME))
# define MKMXVAR(NOME, NOME_MX, III, JJJ, VAL_IJ) do { \
    std::cout << "Criando " STRMXVAR(NOME) "..." << std::endl; \
    unsigned long iii=(III), jjj=(JJJ); \
    mxArray *MXVAR(NOME) = mxCreateDoubleMatrix(iii, jjj, mxREAL); \
    if (MXVAR(NOME) == NULL) \
        std::cout << "Erro ao criar " STRMXVAR(NOME) " ." << std::endl; \
    std::cout << "Populando buffer..." << std::endl; \
    for (unsigned int j=0; j<jjj; ++j) \
        for (unsigned int i=0; i<iii; ++i) \
            buffer[iii*j+i] = (VAL_IJ); \
    std::cout << "Transferindo buffer -> " STRMXVAR(NOME) " ..." << std::endl; \
    std::memcpy((void *)(mxGetPr(MXVAR(NOME))), \
                         (void *)buffer, \
                         iii*jjj*sizeof(double)); \
    std::cout << "Escrevendo " STRMXVAR(NOME) " no pmat ..." << std::endl; \
    if (matPutVariable(pmat, (NOME_MX), MXVAR(NOME)) != 0) \
        std::cout << "Erro ao gravar " STRMXVAR(NOME) " ." << std::endl; \
    std::cout << "Liberando memória..." << std::endl; \
    mxDestroyArray(MXVAR(NOME)); \
} while (0)
    std::cout << "Abrindo arquivo..." << std::endl;
    MATFile *pmat = matOpen("ATFA_LOG_MATLAB.mat", "w");
    if (pmat == NULL)
        std::cout << "Erro ao abrir .mat ." << std::endl;
    constexpr unsigned long SAMPLES_IN_PMAT = 192000;
    static double buffer[128*SAMPLES_IN_PMAT];
    MKMXVAR(in, "data_in",
            1, std::min(SAMPLES_IN_PMAT, data_in.size()),
            data_in[j]);
    MKMXVAR(out, "data_out",
            1, std::min(SAMPLES_IN_PMAT, data_out.size()),
            data_out[j]);
    MKMXVAR(vad, "cpp_vad",
            1, std::min(SAMPLES_IN_PMAT, vad.size()),
            vad[j]);
    MKMXVAR(w, "wvec",
            128, std::min(SAMPLES_IN_PMAT, blks_in_buf*blk_size),
            wvec[j][i]);
    MKMXVAR(Fs, "Fs",
            1, 1,
            samplerate);
    MKMXVAR(delay, "delay_samples",
            1, 1,
            delay_samples);
    MKMXVAR(rir, "cpp_rir",
            1, std::min(SAMPLES_IN_PMAT, scene.imp_resp.size()),
            scene.imp_resp[j]);
    std::cout << "Fechando o arquivo..." << std::endl;
    if (matClose(pmat) != 0)
        std::cout << "Erro ao fechar .mat ." << std::endl;
    std::cout << "Fim!" << std::endl;
#endif

    adapf->destroy_data_structures();

}

void Stream::rir_fft() {
    static container_t x_re(fft_size); // FFT helpers
    static container_t x_im(fft_size);
    static container_t y_re(fft_size);
    static container_t y_im(fft_size);
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
        blk_cv.wait(lk, [this]{
            // É !=0 mesmo, e não >0, pois blk_count<0 é um sinal para
            // abortar o thread
            return blk_count != 0;
        });
        count = blk_count;
        blk_count = 0;
        lk.unlock();
        RCOUT("count = " << count);
        if (count < 0) return;
        RCOUT("gonna process " << count << " block(s).");
        for (int i=0; i<count; ++i) {
            // process a single block.
            // TODO: com o truque da parte imaginária (ver Signal::filter),
            // talvez dê  pra aceitar RIRs com até o dobro do tamanho (a gente
            // dividiria a RIR em duas, e colocaria a primeira metade na parte
            // real e a segunda metade na parte imaginária, e convoluiria o
            // bloco com essa RIR complexa)
            RCOUT("-> Block #" << i);
            // For performance, we won't test that filter_ptr <= data_out.end();
            // We assume that the rest of the code enforces it. TODO: if DEBUG,
            //                            testar se filter_ptr <= data_out.end()
            pa_fperbuf_t remaining =
                    static_cast<pa_fperbuf_t>(data_out.end() - filter_ptr);
            RCOUT("remaining = " << remaining);
            long overflow = (long)fft_size - (long)remaining;
            RCOUT("overflow = " << overflow);
            // buf_size is an integer multiple of blk_size, so that
            // rir_ptr+blk_size is guaranteed to be <= data_in.end() .
            auto rir_end_ptr = rir_ptr + blk_size;
            RCOUT("rir_end_ptr    = data_in.begin()  + " <<
                  (rir_end_ptr    - data_in.begin()));
            {
                bool vad_in_this_block = (*calcVAD)(rir_ptr, rir_end_ptr);
                if (led_widget)
                    led_widget->setLEDStatus(vad_in_this_block);
                *vad_ptr++ = vad_in_this_block;
                if (vad_ptr == vad.end())
                    vad_ptr = vad.begin();
            }
            { auto blk_end = std::copy(rir_ptr, rir_end_ptr, x_re.begin());
              std::fill(blk_end, x_re.end(), 0); }
            std::fill(x_im.begin(), x_im.end(), 0);
            Signal::dft(x_re, x_im);
            for (index_t j = 0; j != fft_size; ++j) {
                y_re[j] = x_re[j]*h_freq_re[j] - x_im[j]*h_freq_im[j];
                y_im[j] = x_re[j]*h_freq_im[j] + x_im[j]*h_freq_re[j];
            }
            Signal::dft(y_re, y_im, DefaultDFT::INVERSE);
            // overlap and add: we add the first 'blks_in_fft-1' blocks,
            // which do overlap, and then we force the last block, which is
            // plain new and will override the old samples from one buffer
            // ago.
            auto y_last = y_re.begin() + (blks_in_fft-1)*blk_size;
            auto f_ptr = filter_ptr;
            auto y_first = y_re.begin();
            if (overflow < 0) {
                RCOUT("No overflow in this block.");
                for (; y_first != y_last;       ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                for (; y_first != y_re.end();   ++y_first, ++f_ptr, ++awgn_ptr)
                    *f_ptr = *y_first + *awgn_ptr;
                filter_ptr += blk_size;
            }
            else if (size_t(overflow) < blk_size) {
                RCOUT("Only the last block of the RIR overflows");
                for (; y_first != y_last;       ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                for (; f_ptr != data_out.end(); ++y_first, ++f_ptr, ++awgn_ptr)
                    *f_ptr = *y_first + *awgn_ptr;
                f_ptr = data_out.begin();
                for (; y_first != y_re.end();   ++y_first, ++f_ptr, ++awgn_ptr)
                    *f_ptr = *y_first + *awgn_ptr;
                filter_ptr += blk_size;
            }
            else if (size_t(overflow) < (blks_in_fft-1)*blk_size) {
                RCOUT("The first block fits the remaining of the buffer.");
                for (; f_ptr != data_out.end(); ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                f_ptr = data_out.begin();
                for (; y_first != y_last;       ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                for (; y_first != y_re.end();   ++y_first, ++f_ptr, ++awgn_ptr)
                    *f_ptr = *y_first + *awgn_ptr;
                filter_ptr += blk_size;
            }
            else {
                RCOUT("The convolution overflows already on the first block.");
                for (; f_ptr != data_out.end(); ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                f_ptr = data_out.begin();
                for (; y_first != y_last;       ++y_first, ++f_ptr)
                    *f_ptr += *y_first;
                for (; y_first != y_re.end();   ++y_first, ++f_ptr, ++awgn_ptr)
                    *f_ptr = *y_first + *awgn_ptr;
                filter_ptr = data_out.begin() +
                             static_cast<long>(blk_size - remaining);
            }
            if (rir_end_ptr == data_in.end()) {
                rir_ptr = data_in.begin();
                awgn_ptr = awgn.begin();
            }
            else
                rir_ptr = rir_end_ptr;
#ifdef ATFA_DEBUG
            RCOUT("rir_ptr        = data_in.begin()  + " <<
                  (rir_ptr        - data_in.begin()));
            RCOUT("filter_ptr     = data_out.begin() + " <<
                  (filter_ptr     - data_out.begin()));
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
void Stream::set_filter(const container_t& h, bool substitute) {
    if (h.size() >= fft_size - blk_size)
        // A convolução de 'h' com um bloco deve caber em uma fft
        throw std::length_error(std::string("RIR pode ter no máximo ") +
                                std::to_string(fft_size - blk_size) +
                                std::string(" amostras."));
    if (substitute)
        scene.imp_resp = h;
    std::fill(h_freq_re.begin(), h_freq_re.end(), 0);
    std::fill(h_freq_im.begin(), h_freq_im.end(), 0);
    std::copy(scene.imp_resp.begin(), scene.imp_resp.end(), h_freq_re.begin());
    Signal::dft(h_freq_re, h_freq_im);
}
