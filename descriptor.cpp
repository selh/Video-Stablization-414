#include "descriptor.h"
#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

// THIS FILE ASSUMES CV_32F!

// TODO: might be slow if we have to repeat many times?
// could generate the 16x16 gaussian matrix beforehand and just index into it.
float gaussianWeightingFunction(feature keypoint, int x, int y) {
    float distance = sqrt(pow(keypoint.location.x - x, 2) + pow(keypoint.location.y - y, 2));
    // 128 = 2 * (0.5 * windowsize=16)^2
    // e ^ (-(x - mu)^2 / (2*sigma^2))
    return exp(-pow(distance, 2) / 128);
}

// todo: cache results??? use results from SIFT (5)?
float calculateMagnitude(Mat& image, int x, int y) {
    float leftTerm  = pow(image.at<float>(Point(x+1, y)) - image.at<float>(Point(x-1,y)), 2);
    float rightTerm = pow(image.at<float>(Point(x, y+1)) - image.at<float>(Point(x,y-1)), 2);

    return sqrt(leftTerm + rightTerm);
}

float calculateOrientation(Mat& image, int x, int y) {
    float numerator   = image.at<float>(Point(x, y+1)) - image.at<float>(Point(x, y-1));
    float denominator = image.at<float>(Point(x+1, y)) - image.at<float>(Point(x+1, y));

    //return atan(numerator / denominator);
    // or this? want full 360
    return atan2(numerator, denominator);
}

int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

// 4x4 grid starting with topLeft
// remember to delete returned result
vector<float> generateHistogram(Mat& image, feature keypoint, Point topLeft) {
    //TODO: gaussian with sigma 1/2 * keypoint->magnitude, offset from keypoint location
    vector<float> histogram(8);
    for (int i = 0; i < 8; i++) {
        histogram[i] = 0;
    }
    
    for (int x = topLeft.x; x < topLeft.x + 4; x++) {
        for (int y = topLeft.y; y < topLeft.y + 4; y++) {
            if (x < 0 || y < 0 || x >= image.size().width || y >= image.size().height) {
                // Avoid OOB
                continue;
            }
            float m = calculateMagnitude(image, x, y);
            float o = calculateOrientation(image, x, y);
            o += keypoint.orientation; // rotate in respect with keypoint orientation
            // 45 degree bins or PI/4
            // does this actually work...?
            int bin = positive_modulo((o / (M_PI / 4)), 8);

            // magnitude is weighted by the distance from the keypoint using Gaussian blur
            // TODO: trilinear interpolation???
            histogram[bin] += m * gaussianWeightingFunction(keypoint, x, y);
        }
    }

    return histogram;
}

void normalizeDescriptor(vector<float>& descriptor, bool threshold) {
    float length = 0;
    for (int i = 0; i < descriptor.size(); i++) {
        if (threshold && descriptor[i] > 0.2) {
            descriptor[i] = 0.2;
        }
        length += pow(descriptor[i], 2);
    }
    length = sqrt(length);
    for (int i = 0; i < descriptor.size(); i++) {
        descriptor[i] /= length;
    }
}

// remember to delete result
// image should have gaussian filter applied already (and grayscale)
vector<float> generateDescriptor(feature keypoint, Mat& image) {
    // Assert matrix is in correct form
    int matrixType = image.type();
    assert(matrixType == CV_32F);
    
    vector<float> descriptor;

    // 16x16 window around keypoint (start at location.x/y - 8 to +8)
    for (int x = keypoint.location.x - 8; x < keypoint.location.x + 8; x += 4) {
        for (int y = keypoint.location.y - 8; y < keypoint.location.y + 8; y += 4) {
            if (x < 0 || y < 0 || x >= image.size().width || y >= image.size().height) {
                // Avoid OOB
                continue;
            }
            vector<float> histogram = generateHistogram(image, keypoint, Point(x, y));
            // Copy into our main descriptor
            descriptor.insert(descriptor.end(), histogram.begin(), histogram.end());
        }
    }

    // Normalize resulting descriptor vector, which is 128 dimensions
    normalizeDescriptor(descriptor, false);
    //threshold values to 0.2 and re-normalize...
    normalizeDescriptor(descriptor, true);

    return descriptor;
}