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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>

#include <portaudio.h>

#include "Signal.h"

using namespace std;

/// Initialize PortAudio.
/**
  * Initializes a PortAudio session. Also prints out a list of available
  * devices that PortAudio sees.
  *
  * \throws `std::runtime_error` if PortAudio initialization fails.
  *
  * \see `portaudio_end()`
  *
  * \todo make the device listing optional
  */
void portaudio_init() {

    PaError err;

    // init portaudio
    err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "PortAudio initialization error " << err << ":" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        throw runtime_error("PortAudio error.");
    }

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
  * \throws `std::runtime_error` if PortAudio closing fails.
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
 * No command-line parameters yet.
 *
 * This function:
 * 1. Prints version info
 * 2. Creates two `[Signal](\ref Signal)`s, `sound_me` and `sound_other` from
 *    the two input files.
 * 3. Delays the second.
 * 4. Creates an impulse response.
 * 5. Creates a new Signal `signal_result` which is the first filtered, added to
 *    the second, delayed.
 * 6. Initializes a PortAudio session, plays the resulting sound, and closes
 *    PortAudio.
 *
 * \todo get input files from command-line.
 *
 * \param[in] argc      argument count (unused)
 * \param[in] argv      argument values (unused)
 * \returns 0 if no errors
 */
int main(int argc, char *argv[]) {

    cout << "ATFA version " << ATFA_VERSION << "." << endl;
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    Signal sound_me("../wav_samples/bigbrain.wav");
    Signal sound_other("../wav_samples/didntwork.wav");
    sound_other.delay(Signal::MS, 1000);

    Signal imp_resp;
    imp_resp.set_size(8);
    (imp_resp)[0] = (imp_resp)[1] = (imp_resp)[6] = (imp_resp)[7] = .1;
    (imp_resp)[2] = (imp_resp)[3] = (imp_resp)[4] = (imp_resp)[5] = .15;

    Signal sound_result;
    sound_me.filter(imp_resp, sound_result);
    sound_result.add(sound_other);
    sound_result.gain(.5);
    /* ... */ // adaptative filter ainda nao implementado
    sound_result.delay(Signal::MS, 1000);

    // init portaudio
    portaudio_init();

    // play audio
    sound_result.play();

    // close portaudio
    portaudio_end();

    cout << "Finishing..." << endl;

    return 0;

}
