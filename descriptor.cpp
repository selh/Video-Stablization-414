#include "descriptor.h"
#include <vector>
#include <cmath>

using namespace std;

// THIS FILE ASSUMES CV_32F!

// todo: cache results???
float calculateMagnitude(Mat* image, int x, int y) {
    float leftTerm  = pow(image->at<float>(Point(x+1, y)) - image->at<float>(Point(x-1,y)), 2);
    float rightTerm = pow(image->at<float>(Point(x, y+1)) - image->at<float>(Point(x,y-1)), 2);

    return sqrt(leftTerm + rightTerm);
}

float calculateOrientation(Mat* image, int x, int y) {
    float numerator   = image->at<float>(Point(x, y+1)) - image->at<float>(Point(x, y-1));
    float denominator = image->at<float>(Point(x+1, y)) - image->at<float>(Point(x+1, y));

    //return atan(numerator / denominator);
    // or this? want full 360
    return atan2(numerator, denominator);
}

int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

// 4x4 grid starting with topLeft
// remember to delete returned result
float* generateHistogram(Mat* image, feature keypoint, Point topLeft) {
    //TODO: gaussian with sigma 1/2 * keypoint->magnitude, offset from keypoint location
    float* histogram = new float[8];
    for (int i = 0; i < 8; i++) {
        histogram[i] = 0;
    }
    
    for (int x = topLeft.x; x < topLeft.x + 4; x++) {
        for (int y = topLeft.y; y < topLeft.y + 4; y++) {
            if (x < 0 || y < 0 || x >= image->size().width || y >= image->size().height) {
                // Avoid OOB
                continue;
            }
            float m = calculateMagnitude(image, x, y);
            float o = calculateOrientation(image, x, y);
            o += keypoint.orientation; // rotate in respect with keypoint orientation
            // 45 degree bins or PI/4
            // does this actually work...?
            int bin = positive_modulo((o / (M_PI / 4)), 8);

            histogram[bin] += m; // multiple here by the gaussian
        }
    }

    return histogram;
}

// remember to delete result
// image should have gaussian filter applied already (and grayscale)
float* generateDescriptor(feature keypoint, Mat* image) {
    // Assert matrix is in correct form
    int matrixType = image->type();
    assert(matrixType == CV_32F);

    float* descriptor = new float[128];

    // 16x16 window around keypoint
    int i = 0;
    for (int x = keypoint.location.x - 8; x < keypoint.location.x + 8; x += 4) {
        for (int y = keypoint.location.y - 8; y < keypoint.location.y + 8; y += 4) {
            if (x < 0 || y < 0 || x >= image->size().width || y >= image->size().height) {
                // Avoid OOB
                continue;
            }
            float* histogram = generateHistogram(image, keypoint, Point(x, y));
            // Copy into our main descriptor
            memcpy(descriptor + i, histogram, 8 * sizeof *histogram);
            delete histogram;
            i += 8; //every time we copy 8 elements
        }
    }

    // Normalize resulting descriptor vector (128 dimensions)
    float length = 0;
    for (int i = 0; i < 128; i++) {
        length += pow(descriptor[i], 2);
    }
    length = sqrt(length);
    for (int i = 0; i < 128; i++) {
        descriptor[i] /= length;
    }


    return descriptor;
}