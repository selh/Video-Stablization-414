#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#include <opencv2/opencv.hpp>

using namespace cv;

struct feature {
    //x, y, σ, m, θ
    Point location;
    float sigma;
    float magnitude;
    float orientation;
};

float* generateDescriptor(feature keypoint, Mat* image);

#endif