#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <portaudio.h>

enum return_code {
    RETURN_OK,
    PA_INIT_ERROR,
    PA_TERM_ERROR
};

static const int SAMPLE_RATE = 44100;

typedef struct {
    float left_phase[2];
    float right_phase;
} pa_test_data_t;

static pa_test_data_t data;

// Test callback function
static int pa_test_callback(
    const void *in_buf, void *out_buf, unsigned long frames_per_buf,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags, void *user_data
) {

    pa_test_data_t *data = (pa_test_data_t *) user_data;
    float *out = (float *) out_buf;
    unsigned int i;
    (void) in_buf; // prevent unused variable warning
    (void) time_info;
    (void) status_flags;

    for (i = 0; i < frames_per_buf; i++) {

        *out++ = .9*data->left_phase[0];
        *out++ = data->right_phase;

        // generate 440Hz sine
        // y[n] = 2*pi*(440Hz/SAMPLE_RATE)*y[n-1] - y[n-2]
        float new_phase = 1.99607132886337*data->left_phase[0] - data->left_phase[1];
        //if (data->left_phase >= 1.0f) data->left_phase -= 2.0f;
        data->left_phase[1] = data->left_phase[0];
        data->left_phase[0] = new_phase;

        // generate simple sawtooth wave that ranges between -1.0f and +1.0f
        data->right_phase += 0.01f;
        if (data->right_phase >= 1.0f) data->right_phase -= 2.0f;

    }

    return 0;

}

using namespace std;

int main(int argc, char *argv[]) {

    cout << "BlaBle version " << BlaBle_VERSION << "." << endl;
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    // testing cv
    if (argc > 1) {
        cv::Mat img = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);
        if (!img.data) {
            cerr << "No image data." << endl;
            return -2;
        }
        cv::namedWindow("Display Image", CV_WINDOW_NORMAL);
        cv::imshow("Display Image", img);
        cv::waitKey(0);
    }

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
        cerr << hex << err << hex << ":" << endl;
        cerr << "  " << Pa_GetErrorText(devs) << endl;
    }
    for (int i = 0; i < devs; i++) {
        const PaDeviceInfo *dev_info = Pa_GetDeviceInfo(i);
        cout << "DEV " << i << ": " << dev_info->name << endl;
        cout << "  struct version:" << dev_info->structVersion << endl;
        cout << "  host api index:" << dev_info->hostApi << endl;
        cout << "  max input ch:  " << dev_info->maxInputChannels << endl;
        cout << "  max output ch: " << dev_info->maxOutputChannels << endl;
        cout << "  default srate: " << dev_info->defaultSampleRate << endl;
        cout << endl;
    }

    data.left_phase[1] = -0.06264832417874368; // -sin(2*pi*(440Hz/SAMPLE_RATE))
    data.left_phase[0] = 0;
    data.right_phase = 0;

    // open stream
    PaStream *stream;
    err = Pa_OpenDefaultStream(
                &stream,
                0,
                2,
                paFloat32,
                SAMPLE_RATE,
                256,
                pa_test_callback,
                &data
    );
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " at opening stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_INIT_ERROR;
    }

    // start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " at starting stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_INIT_ERROR;
    }

    // sleep
    Pa_Sleep(10000);

    // close stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        cerr << "PortAudio error " << err << " when stopping stream:" << endl;
        cerr << "  " << Pa_GetErrorText(err) << endl;
        return PA_INIT_ERROR;
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
