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
 * \file main.cpp
 *
 * Holds the `main()` function and other routines.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include "Signal.h"

#ifndef ATFA_DIR
#   include <cstring>
    extern "C" {
#       include <libgen.h>
    }
    char static_filename[] = __FILE__;
    char static_dirname[] = __FILE__;
    char static_projdirname[] = __FILE__;
    // whatch out, because dirname() may modify its argument,
    // and ATFA_DIR might get evaluated more than once

    /// Macro for getting the path to the project directory from cmake
    /** Should be passed from `CMakeLists.txt`, but if it's not, we try to
        deduce it from the `__FILE__` macro */
#   define ATFA_DIR (static_cast<const char *>( \
        std::strcpy(static_dirname, dirname(static_filename)), \
        std::strcpy(static_filename, __FILE__), \
        std::strcpy(static_projdirname, dirname(static_dirname)), \
        static_projdirname \
    ))

#endif

std::ostringstream FileError::msg;

const std::vector<double> Signal::DFTDriver::costbl =
        Signal::DFTDriver::initialize_costbl();
const std::vector<double> Signal::DFTDriver::sintbl =
        Signal::DFTDriver::initialize_sintbl();

Signal::DFTDriver Signal::dft;

using namespace std;

/// Initialize PortAudio.
/**
  * Initializes a PortAudio session. Also prints out a list of available
  * devices that PortAudio sees., if requested.
  *
  * \param[in] list_devices Whether or not to print the device list.
  *
  * \throws std::runtime_error  if PortAudio initialization fails.
  *
  * \see `portaudio_end()`
  */
void portaudio_init(bool list_devices=false) {

    PaError err;

    // init portaudio
    err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "PortAudio initialization error " << err << ":" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        throw runtime_error("PortAudio error.");
    }

    if (!list_devices) return;

    // view devices
    int devs = Pa_GetDeviceCount();
    if (devs < 0) {
        cerr << "PortAudio devices error 0x";
        cerr << hex << err << dec << ":" << endl;
        cerr << "  " << Pa_GetErrorText(devs) << endl;
    }
    for (int i = 0; i < devs; i++) {
        const PaDeviceInfo *dev_info = Pa_GetDeviceInfo(i);
        cout << "Device " << i << ": " << dev_info->name << endl;
        cout << "  struct version:" << dev_info->structVersion << endl;
        cout << "  host api index:" << dev_info->hostApi << endl;
        cout << "  max input ch:  " << dev_info->maxInputChannels << endl;
        cout << "  max output ch: " << dev_info->maxOutputChannels << endl;
        cout << "  default srate: " << dev_info->defaultSampleRate << endl;
        cout << endl;
    }

}

/// Close PortAudio.
/**
  * Ends a PortAudio session.
  *
  * \throws std::runtime_error  if PortAudio closing fails.
  *
  * \see `portaudio_init()`
  */
void portaudio_end() {
    PaError err = Pa_Terminate();
    if (err != paNoError) {
        cerr << "PortAudio closing error " << err << ":" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        throw runtime_error("PortAudio error.");
    }
}

/// `main()` function.
/**
 * No command-line parameters.
 *
 * This function:
 * 1. Prints version info
 * 2. Creates two \ref Signal "Signal"s, `sound_me` and `sound_other` from two
 *    input files.
 * 3. Delays the second.
 * 4. Creates an impulse response.
 * 5. Filters `sound_me` according to the impulse response and adds it to the
 *    second, delayed.
 * 6. Initializes a PortAudio session, plays the resulting sound, and closes
 *    PortAudio.
 *
 * \param[in] argc      argument count (unused)
 * \param[in] argv      argument values (unused)
 * \returns 0 if no errors
 */
int main(int argc, char *argv[]) {

    cout << "ATFA version " << ATFA_VERSION << "." << endl;
#ifdef ATFA_DEBUG
    cout << ">>> THIS IS A DEBUG BUILD" << endl;
#endif
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    // ... ??
    // TODO: - coisar o TODO do vector lá do outro arquivo
    //       - fazer os callbacks e a classe nova da fila circular

        Signal sound_me(string(ATFA_DIR) + "/wav_samples/bigbrain.wav");



        Signal imp_resp;

        imp_resp.set_size(8192);

        // lowpass (~3.2kHz)

        imp_resp[0] = .2;    imp_resp[1] = .3;

        imp_resp[2] = .3;    imp_resp[3] = .2;

        // highpass(~7.8kHz)

        imp_resp[8188] = .2; imp_resp[8189] = -.3;

        imp_resp[8190] = .3; imp_resp[8191] = -.2;



        sound_me.filter(imp_resp);

        sound_me.normalize();

        /* ... */ // adaptative filter ainda nao implementado

        sound_me.delay(Signal::MS, 1000);



        portaudio_init(); // init portaudio

            sound_me.play(); // play audio

            portaudio_end(); // close portaudio

    cout << "Finishing..." << endl;

    return 0;

}
