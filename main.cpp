#include <chrono>
#include <iostream>
#include "SIFT.h"
#include "cxxopts.hpp"

using namespace std;

int main(int argc, char** argv) {
    // Parse out command line arguments
    cxxopts::Options options("DisplayImage", "Performs video stabilization using SIFT");
    options.add_options()
        ("d,debug", "Enable debugging")
        ("f,file", "Video file name", cxxopts::value<std::string>())
        ;
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

    namedWindow("Combined", WINDOW_AUTOSIZE );
    // namedWindow("Unstabilized", WINDOW_AUTOSIZE );
    // namedWindow("Stabilized", WINDOW_AUTOSIZE );

    // SIFT::featureMapper(currentFrame, currentFeature);
    // SIFT::featureMapper(previousFrame, previousFeature);

    //OPENCV DOCUMENTATION
        const string NAME = "output.avi";   // Form the new name with container
        int ex = static_cast<int>(video.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

        // Transform from int to char via Bitwise operators
        char EXT[] = {(char)(ex & 0XFF) , (char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24), 0};

        Size S = Size((int) video.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
                    (int) video.get(CV_CAP_PROP_FRAME_HEIGHT));

        VideoWriter outputVideo;
        outputVideo.open(NAME, ex, video.get(CV_CAP_PROP_FPS), S, true);

    Mat transformationMat;
    long frame = 2;
    while (frame < 100) {
        cout << "Frame #" << frame << endl;
        cout << "Feature size: " << currentFeature.size() << " vs " << previousFeature.size() << endl;
        auto t1 = chrono::high_resolution_clock::now();
        Mat H = SIFT::drawMatches(currentFrame, previousFrame, currentFeature, previousFeature);
        if (frame == 2) {
            H.copyTo(transformationMat);
        } else {
            transformationMat *= H;
        }
        auto t2 = chrono::high_resolution_clock::now();
        cout << "Matching time: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << "ms" << endl;
        // imshow("Combined", combined);
        // waitKey(1);

        // Copy current frame to previous
        Mat transform = Mat::zeros(currentFrame.size(), currentFrame.type());
        warpPerspective(currentFrame, transform, transformationMat, transform.size());
        
        imshow("Stabilized", transform);
        if (waitKey(10) == 27) {
            cout << "Pressed ESC, exiting..." << endl;
            break;
        }
        waitKey(1);
        outputVideo << transform;


        currentFrame.copyTo(previousFrame);
        previousFeature = move(currentFeature);
        // Read in next frame
        bool success = video.read(currentFrame);
        if (!success) {
            break;
        }

        // Analyze frames
        t1 = chrono::high_resolution_clock::now();
        currentFeature = sift.run(currentFrame);
        t2 = chrono::high_resolution_clock::now();
        cout << "SIFT time: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << "ms" << endl << endl;;
        // SIFT::featureMapper(currentFrame, currentFeature);

        frame++;
    }

    return 0;
}