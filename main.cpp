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
        int pixelDistanceThreshold = options["distance"].as<int>();
        float ratioThreshold = options["ratio"].as<float>();

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

            Mat combined = SIFT::drawMatches(firstImage, secondImage, firstResults, secondResults, pixelDistanceThreshold, ratioThreshold);
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

            // http://docs.opencv.org/2.4/doc/tutorials/highgui/video-write/video-write.html
            string outputFileName = options["output"].as<string>();
            int codec = static_cast<int>(video.get(CV_CAP_PROP_FOURCC));
            Size outputSize = Size((int) video.get(CV_CAP_PROP_FRAME_WIDTH), (int) video.get(CV_CAP_PROP_FRAME_HEIGHT));
            VideoWriter outputVideo;
            outputVideo.open(outputFileName, codec, video.get(CV_CAP_PROP_FPS), outputSize, true);

            Mat transformationMat;
            // For calculating motion and its derivative
            Point2f smoothedMotionVector;
            const long startingFrame = 2;
            const long smoothingFrames = 5;

            // For timing of calls to SIFT, etc.
            long frame = startingFrame;
            chrono::high_resolution_clock::time_point t1, t2;
            while (true) {
                cout << "Frame #" << frame << endl;
                cout << "Feature size: " << currentFeature.size() << " vs " << previousFeature.size() << endl;

                if (mode == Mode::VideoStabilization) { // Mode: Video stabilization
                    t1 = chrono::high_resolution_clock::now();
                    pair<vector<Point2f>, vector<Point2f>> matchingPairs = SIFT::getBestMatchingPairs(currentFeature, previousFeature, pixelDistanceThreshold, ratioThreshold);

                    // Calculation of motion vector
                    Point2f motionVector;
                    vector<Point2f>::iterator i1, i2;
                    for (i1 = matchingPairs.first.begin(), i2 = matchingPairs.second.begin();
                        i1 < matchingPairs.first.end() && i2 < matchingPairs.second.end();
                        i1++, i2++) {
                        motionVector += ((*i2) - (*i1));
                    }
                    motionVector /= ((int)matchingPairs.first.size()); //normalize to size of matchingPairs. cast to int from unsigned long
                    cout << "Motion vector: " << motionVector << endl;
                    float derivativeOfMotion = norm(motionVector - smoothedMotionVector);
                    
                    // Calculate new smoothed motion vector
                    if (frame < startingFrame + smoothingFrames) {
                        smoothedMotionVector += motionVector * (1.f / smoothingFrames);
                    } else {
                        smoothedMotionVector = (1.f / (smoothingFrames - 1)) * smoothedMotionVector + (1.f / smoothingFrames) * motionVector;
                    }

                    if (derivativeOfMotion < 1.5f && matchingPairs.first.size() > 0 && matchingPairs.second.size() > 0) {
                        // Instead of findHomography, calculate a rigid transform so we do not skew the output image
                        Mat rigidTransform = estimateRigidTransform(matchingPairs.first,matchingPairs.second,false);

                        if(!rigidTransform.empty()) {
                            // Convert rigid transformation to a homography
                            Mat homography = Mat::zeros(3, 3, rigidTransform.type());
                            Rect matrixSelection(0, 0, 3, 2);
                            rigidTransform(matrixSelection).copyTo(homography(matrixSelection));
                            homography.at<double>(2,2) = 1.0;

                            if (transformationMat.empty()) {
                                // First frame data
                                homography.copyTo(transformationMat);
                            } else if (!homography.empty()) {
                                transformationMat *= homography;

                                cout << "Homography matrix:" << endl;
                                cout << homography << endl;
                            }
                        }
                    }
                    t2 = chrono::high_resolution_clock::now();
                    cout << "Matching time: " << chrono::duration_cast<chrono::milliseconds>(t2 - t1).count() << "ms" << endl;

                    // Copy current frame to previous
                    Mat transform = Mat::zeros(currentFrame.size(), currentFrame.type());
                    if (!transformationMat.empty()) {
                        warpPerspective(currentFrame, transform, transformationMat, transform.size());
                    } else {
                        // If there's no transformation simply copy it over
                        currentFrame.copyTo(transform);
                    }
                    
                    imshow("Stabilized", transform);
                    waitKey(10);

                    // Output to video file
                    outputVideo << transform;
                } else { // Mode: Show feature matching
                    Mat combined = SIFT::drawMatches(currentFrame, previousFrame, currentFeature, previousFeature, pixelDistanceThreshold, ratioThreshold);
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