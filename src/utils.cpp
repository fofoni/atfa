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
 * \file utils.cpp
 *
 * blablabla
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <iostream>
#include <sstream>

extern "C" {
#   include <portaudio.h>
}

#include "utils.h"

std::ostringstream FileError::msg;

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
        std::cerr << "PortAudio initialization error " << err << ":"
                  << std::endl;
        std::cerr << "  " << Pa_GetErrorText(err) << std::endl;
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
