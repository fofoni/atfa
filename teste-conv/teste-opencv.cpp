#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>

#include <portaudio.h>

#include "Signal.h"

enum return_code {
    RETURN_OK,
    PA_INIT_ERROR,
    PA_OPEN_ERROR,
    PA_START_ERROR,
    PA_STOP_ERROR,
    PA_TERM_ERROR
};

// PortAudio callback function
static int callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    float *data = ((Signal *)user_data)->data;
    unsigned long total_samples = ((Signal *)user_data)->samples;
    unsigned long counter = ((Signal *)user_data)->counter;

    float *out = (float *) out_buf;
    (void) in_buf; // prevent unused variable warning
    (void) time_info;
    (void) status_flags;

    frames_per_buf += counter;
    for (; counter < frames_per_buf; counter++) {
        if (counter == total_samples) {
            ((Signal *)user_data)->counter = total_samples;
            return paComplete;
        }
        *out++ = data[counter];
    }

    ((Signal *)user_data)->counter = counter;
    return 0;

}

using namespace std;

int main(int argc, char *argv[]) {

    cout << "BlaBle version " << BlaBle_VERSION << "." << endl;
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    Signal sound("../wav_samples/bigbrain.wav");

    PaError err;

    // init portaudio
    err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "PortAudio initialization error " << err << ":" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_INIT_ERROR;
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

    // open stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(
                &stream,
                0,
                1,
                paFloat32,
                sound.sample_rate,
                paFramesPerBufferUnspecified,
                callback,
                &sound
    );
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " at opening stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_OPEN_ERROR;
    }

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " at starting stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_START_ERROR;
    }

    // sleep
    Pa_Sleep(sound.samples * sound.sample_rate * 1000);

    // close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " when stopping stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_STOP_ERROR;
    }

    // close portaudio
    err = Pa_Terminate();
    if (err != paNoError) {
        cerr << "PortAudio closing error " << err << ":" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_TERM_ERROR;
    }

    return 0;

}
