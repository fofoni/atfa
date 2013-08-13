#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main(int argc, char *argv[]) {

    cout << "BlaBle version " << BlaBle_VERSION << "." << endl << endl;

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <file>" << endl;
        return -1;
    }

    Mat img = imread(argv[1], CV_LOAD_IMAGE_COLOR);

    if (!img.data) {
        cout << "No image data." << endl;
        return -2;
    }

    namedWindow("Display Image", CV_WINDOW_NORMAL);
    imshow("Display Image", img);

    waitKey(0);

    return 0;

}
