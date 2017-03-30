#include "SIFT.h"

// using namespace cv;
// using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: DisplayImage.out <Video Path>\n");
        return -1;
    }

    VideoCapture video(argv[1]);
    if (!video.isOpened()) {
        printf("No video data\n");
        return -1;
    }

    SIFT sift;
    Mat currentFrame, previousFrame;
    bool success = video.read(previousFrame); //First frame as previous
    if (!success) {
        cout << "No frames in video" << endl;
    }
    success = video.read(currentFrame); //Second frame as current
    if (!success) {
        cout << "No frames in video" << endl;
    }
    vector<Feature> currentFeature = sift.run(currentFrame);
    vector<Feature> previousFeature = sift.run(previousFrame);
    namedWindow("Unstabilized", WINDOW_AUTOSIZE );
    namedWindow("Stabilized", WINDOW_AUTOSIZE );
    
    while (true) {
        cout << "Analyzing..." << endl;
        // Analyze frames

        // Copy current frame to previous
        currentFrame.copyTo(previousFrame);
        previousFeature = move(currentFeature);
        // Read in next frame
        bool success = video.read(currentFrame);
        if (!success) {
            break;
        }
        currentFeature = sift.run(currentFrame);

        cout << "Displaying result..." << endl;
        imshow("Unstabilized", currentFrame);
        waitKey(0);
    }

    return 0;
}