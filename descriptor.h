#ifndef _DESCRIPTOR_H_
#define _DESCRIPTOR_H_

#include <vector>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

struct feature {
    //x, y, σ, m, θ
    Point location;
    float sigma;
    float magnitude;
    float orientation;
};

vector<float> generateDescriptor(feature keypoint, Mat& image);

#endif