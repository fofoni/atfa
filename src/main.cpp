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

#include <iostream>
#include <stdexcept>

extern "C" {
#   include <portaudio.h>
}

#include "Signal.h"

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
 * 2. Creates one \ref Signal "Signal", `sound_me`, from an input file.
 * 4. Creates an impulse response.
 * 5. Filters `sound_me` according to the impulse response.
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
    //       - fazer os callbacks e a classe nova da fila circular

    cout << "Finishing..." << endl;

    return 0;

}
