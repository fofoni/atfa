#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <portaudio.h>

using namespace std;

typedef struct {
    float left_phase;
    float right_phase;
} pa_test_data_t;


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

    for (i = 0; i < frames_per_buf; i++) {

        *out++ = data->left_phase;
        *out++ = data->right_phase;

        // generate simple sawtooth wave that ranges between -1.0f and +1.0f

        data->left_phase += 0.01f;
        if (data->left_phase >= 1.0f) data->left_phase -= 2.0f;

        // higher pitch, so as to distinguish between left and right
        data->right_phase += 0.03f;
        if (data->right_phase >= 1.0f) data->right_phase -= 2.0f;

    }

    return 0;

}

int main(int argc, char *argv[]) {

    cout << "BlaBle version " << BlaBle_VERSION << "." << endl << endl;

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <file>" << endl;
        return -1;
    }

    cv::Mat img = cv::imread(argv[1], CV_LOAD_IMAGE_COLOR);

    if (!img.data) {
        cerr << "No image data." << endl;
        return -2;
    }

    cv::namedWindow("Display Image", CV_WINDOW_NORMAL);
    cv::imshow("Display Image", img);

    cv::waitKey(0);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        cerr << "PortAudio initialization error " << err << endl;
        return 1;
    }

    return 0;

}
