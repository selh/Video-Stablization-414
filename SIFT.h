#pragma once

#define DOG_SIGMA 1.6
#define K_FACTOR sqrt(2)

#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

#define SCALES 4
// Defined as the number of DoGs per scale
#define INTERVALS 3

struct Extrema {
    //x, y, σ, m, θ
    int scale;
    int scaleIndex;
    Point location;
    float sigma;
    float magnitude;
    float orientation;
    int intensity; // TODO: correct?
    Vec<float, 128> descriptor;
};

class SIFT {
private:
    Mat gray_img;
    Mat imageScales[SCALES];
    Mat gaussians[SCALES][INTERVALS + 1];
    Mat dogs[SCALES][INTERVALS];

    map<pair<int, int>, Extrema> extremas;

    // Generating DoG pyamid
    void boundsCheck(int arr_row, int arr_col, 
                 int* cstart, int* cstop, 
                 int* rstart, int* rstop );
    void differenceOfGaussian(int index, float sigma);
    void neighbors(int scaleIndex, int current);
public:
    ~SIFT();
    SIFT(Mat& template_image);

    void run();
    void extremaMapper(Mat& image);
};