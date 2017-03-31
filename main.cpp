#include <iostream>
#include "SIFT.h"

using namespace std;


int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: DisplayImage <Video Path>" << endl;
        return -1;
    }

    VideoCapture video(argv[1]);
    if (!video.isOpened()) {
        cout << "No video data found in file \"" << argv[1] << "\"" << endl;
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

    SIFT::featureMapper(currentFrame, currentFeature);
    SIFT::featureMapper(previousFrame, previousFeature);
    
    while (true) {
        Mat combined = SIFT::drawMatches(currentFrame, previousFrame, &currentFeature, &previousFeature);
        namedWindow("Combined", WINDOW_AUTOSIZE );
        imshow("Combined", combined);
        //imwrite("FeatureMatchV1.jpg", combined);
        waitKey(1);

        // Copy current frame to previous
        currentFrame.copyTo(previousFrame);
        previousFeature = move(currentFeature);
        // Read in next frame
        bool success = video.read(currentFrame);
        if (!success) {
            break;
        }

        //cout << "Displaying result..." << endl;
        // imshow("Unstabilized", currentFrame);
        // waitKey(1);

        // Analyze frames
        cout << "Analyzing..." << endl;
        currentFeature = sift.run(currentFrame);
        SIFT::featureMapper(currentFrame, currentFeature);
    }

    return 0;
}