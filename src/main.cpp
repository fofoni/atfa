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

#include <portaudio.h>

#include "Signal.h"

std::ostringstream FileError::msg;

double Signal::DFTDriver::costbl[Signal::DFTDriver::tblsize];
double Signal::DFTDriver::sintbl[Signal::DFTDriver::tblsize];
double Signal::DFTDriver::temp_re[Signal::DFTDriver::tblsize];
double Signal::DFTDriver::temp_im[Signal::DFTDriver::tblsize];
Signal::DFTDriver Signal::dft;

using namespace std;

/// Initialize PortAudio.
/**
  * Initializes a PortAudio session. Also prints out a list of available
  * devices that PortAudio sees.
  *
  * \throws `std::runtime_error` if PortAudio initialization fails.
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

void playsig(Signal s) {
    portaudio_init(); // init portaudio
    s.play(); // play audio
    portaudio_end(); // close portaudio
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
 * \todo we should warn here whether this build is debug or release.
 *       search for CMAKE_CXX_FLAGS_RELEASE and CMAKE_CXX_FLAGS_DEBUG
 *       http://stackoverflow.com/questions/7724569/debug-vs-release-in-cmake
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
    imp_resp.set_size(8192);
    //(imp_resp)[0] = (imp_resp)[1] = (imp_resp)[6] = (imp_resp)[7] = .1;
    //(imp_resp)[2] = (imp_resp)[3] = (imp_resp)[4] = (imp_resp)[5] = .15;
    //imp_resp[0] = imp_resp[16383] = .5;
    // lowpass (~3.2kHz)
    imp_resp[0] = .2; imp_resp[1] = .3; imp_resp[2] = .3; imp_resp[3] = .2;
    // highpass(~7.8kHz)
    imp_resp[8188] = .2; imp_resp[8189] = -.3;
    imp_resp[8190] = .3; imp_resp[8191] = -.2;

//    sound_me.filter(imp_resp);
    sound_me.add(sound_other);
    sound_me.gain(.5);
    /* ... */ // adaptative filter ainda nao implementado
    sound_me.delay(Signal::MS, 1000);

//    playsig(sound_me);

    cout << "Finishing..." << endl;

    Signal::container_t re(32, 1);
    Signal::container_t im(32, 0);
    /* // test case:
    re[0] =  0.386642643323190;
    re[1] =  1.063856991587017;
    re[2] = -0.511566142547817;
    re[3] =  1.435219791353060;
    re[4] =  1.329230594198427;
    re[5] =  0.464761643813005;
    re[6] = -0.297817288426939;
    re[7] =  0.185029914137056;
    im[0] = -3.1485760842645796;
    im[1] = -0.7357721605413995;
    im[2] =  0.0724111077279661;
    im[3] =  1.0495693082726549;
    im[4] =  1.0189200775301872;
    im[5] =  0.1243409207477646;
    im[6] = -0.3193952996871738;
    im[7] = -0.8869369816328675;*/
    re[1] = 2;
    im[24] = -.001;
    Signal::dft(re, im);
    for (unsigned i = 0; i != re.size(); ++i)
        cout << re[i] << " + j" << im[i] << endl;
    cout << endl;
    Signal::dft(re, im, Signal::DFTDriver::INVERSE);
    for (unsigned i = 0; i != re.size(); ++i)
        cout << re[i] << " + j" << im[i] << endl;

    return 0;

}
