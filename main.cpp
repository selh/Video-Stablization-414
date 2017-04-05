#include <chrono>
#include <iostream>
#include "SIFT.h"
#include "cxxopts.hpp"

using namespace std;

enum class Mode {
    VideoStabilization,
    VideoMatching,
    ImageMatching
};

int main(int argc, char** argv) {
    try {
        // Parse out command line arguments
        cxxopts::Options options("DisplayImage", "Performs video stabilization using SIFT");
        options.add_options()
            ("i,input", "Input video/image file name", cxxopts::value<std::string>())
            ("j,matchimage", "Input image file name (for image matching only)", cxxopts::value<std::string>())
            ("o,output", "Output video file name", cxxopts::value<std::string>()->default_value("output.avi"))
            ("m,mode", "VS (Video stabilization), VM (Video matching), IM (Image matching)", cxxopts::value<std::string>()->default_value("VS"))
            ("d,distance", "Image space distance threshold", cxxopts::value<int>()->default_value("50"))
            ("r,ratio", "Feature space ratio threshold", cxxopts::value<float>()->default_value("0.2"))
            ("h,help", "Print help")
            ;
        options.parse(argc, argv);

        if (options.count("help"))
        {
            cout << options.help({""}) << endl;
            return 0;
        }
        
        const string providedMode = options["mode"].as<string>();
        Mode mode;
        if (providedMode == "VS") {
            mode = Mode::VideoStabilization;
        } else if (providedMode == "VM") {
            mode = Mode::VideoMatching;
        } else if (providedMode == "IM") {
            mode = Mode::ImageMatching;
        } else {
            cout << "Invalid mode \"" << providedMode << "\"" << endl;
            return -1;
        }

        if (mode == Mode::ImageMatching) {
            // Handling images
            string firstFile = options["input"].as<string>();
            string secondFile = options["matchimage"].as<string>();
            Mat firstImage = imread(firstFile);
            Mat secondImage = imread(secondFile);

            if (!firstImage.data) {
                cout << "No data found in image \"" << firstFile << "\"" << endl;
                return -1;
            } else if (!secondImage.data) {
                cout << "No data found in image \"" << secondFile << "\"" << endl;
                return -1;
            }

            SIFT sift;
            vector<Feature> firstResults = sift.run(firstImage);
            vector<Feature> secondResults = sift.run(secondImage);

            SIFT::featureMapper(firstImage, firstResults);
            SIFT::featureMapper(secondImage, secondResults);

            Mat combined = SIFT::drawMatches(firstImage, secondImage, firstResults, secondResults);
            namedWindow("Combined", WINDOW_AUTOSIZE );
            imshow("Combined", combined);

            waitKey(0);
        } else {
            // Handling video
            VideoCapture video(options["input"].as<string>());
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
            namedWindow("Unstabilized", WINDOW_AUTOSIZE );
            namedWindow("Stabilized", WINDOW_AUTOSIZE );

            if (mode == Mode::VideoMatching) {
                SIFT::featureMapper(currentFrame, currentFeature);
                SIFT::featureMapper(previousFrame, previousFeature);
            }

            //OPENCV DOCUMENTATION
                const string NAME = options["output"].as<string>();   // Form the new name with container
                int ex = static_cast<int>(video.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form

                // Transform from int to char via Bitwise operators
                char EXT[] = {(char)(ex & 0XFF) , (char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24), 0};

                Size S = Size((int) video.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
                            (int) video.get(CV_CAP_PROP_FRAME_HEIGHT));

                VideoWriter outputVideo;
                outputVideo.open(NAME, ex, video.get(CV_CAP_PROP_FPS), S, true);

            Mat transformationMat;
            long frame = 2;
            chrono::high_resolution_clock::time_point t1, t2;
            while (frame < 100) {
                cout << "Frame #" << frame << endl;
                cout << "Feature size: " << currentFeature.size() << " vs " << previousFeature.size() << endl;

                if (mode == Mode::VideoStabilization) { // Mode: Video stabilization
                    t1 = chrono::high_resolution_clock::now();
                    pair<vector<Point2f>, vector<Point2f>> matchingPairs = SIFT::getBestMatchingPairs(currentFeature, previousFeature);
                    Mat H = findHomography(matchingPairs.first, matchingPairs.second, CV_RANSAC);
                    if (frame == 2) {
                        H.copyTo(transformationMat);
                    } else {
                        transformationMat *= H;
                    }
                    t2 = chrono::high_resolution_clock::now();
                    cout << "Matching time: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << "ms" << endl;

                    // Copy current frame to previous
                    Mat transform = Mat::zeros(currentFrame.size(), currentFrame.type());
                    warpPerspective(currentFrame, transform, transformationMat, transform.size());
                    
                    imshow("Stabilized", transform);
                    waitKey(10);

                    // Output to video file
                    outputVideo << transform;
                } else { // Mode: Show feature matching
                    Mat combined = SIFT::drawMatches(currentFrame, previousFrame, currentFeature, previousFeature);
                    imshow("Combined", combined);

                    waitKey(100); // Need to wait some amount of time, otherwise no window appears
                }

                // Move on to next frame
                currentFrame.copyTo(previousFrame);
                previousFeature = move(currentFeature);
                // Read in next frame
                bool success = video.read(currentFrame);
                if (!success) {
                    break; // Out of frames!
                }

                // Analyze new frame
                t1 = chrono::high_resolution_clock::now();
                currentFeature = sift.run(currentFrame);
                t2 = chrono::high_resolution_clock::now();
                cout << "SIFT time: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << "ms" << endl << endl;

                if (mode == Mode::VideoMatching) {
                    SIFT::featureMapper(currentFrame, currentFeature);
                }

                frame++;
            }
        }
    }
    catch (const cxxopts::OptionException& e)
    {
        cout << "Error parsing options: " << e.what() << endl;
        return 1;
    }

    return 0;
}