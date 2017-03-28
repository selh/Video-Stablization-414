#pragma once

#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// DoG
#define SCALES 4
#define INTERVALS 3 // Defined here as the number of DoGs per scale
#define DOG_SIGMA 1.6
#define K_FACTOR sqrt(2)

// Extrema thresholding
#define EXTREMA_THRESHOLD 0.03
#define CONSTANT_R 10 // SIFT paper recommends using r=10
#define THRESHOLD_R (((CONSTANT_R + 1) * (CONSTANT_R + 1)) / CONSTANT_R)

struct Extrema {
    //x, y, σ, m, θ
    int scale;
    int scaleIndex;
    int intervalIndex;
    Point location; // Normal image-space coordinates
    Point scaleLocation; // Scale-space coordinates
    float sigma;
    float magnitude;
    float orientation;
    int intensity;
    Vec<float, 128> descriptor;
};

class SIFT {
private:
    Mat gray_img;
    Mat imageScales[SCALES];
    Mat gaussians[SCALES][INTERVALS + 1];
    Mat dogs[SCALES][INTERVALS];
    Mat magnitudes[SCALES][INTERVALS];
    Mat orientations[SCALES][INTERVALS];

    map<pair<int, int>, Extrema> extremas;

    // Generating DoG pyamid
    void boundsCheck(int arr_row, int arr_col, 
                 int* cstart, int* cstop, 
                 int* rstart, int* rstop );
    void differenceOfGaussian(int index, int scale);
    void neighbors(int scaleIndex, int current);

    // Extrema checking
    float retrieveFloat(int x, int y, int scaleIndex, int intervalIndex);
    Mat calculateHessianMatrix33(int x, int y, int scaleIndex, int intervalIndex);
    bool checkExtrema(int x, int y, int scaleIndex, int intervalIndex);
    bool eliminateEdgeResponse(int x, int y, int scaleIndex, int intervalIndex);

    // Generating magnitudes and orientations for later steps
    float calculateMagnitude(Mat& image, int x, int y);
    float calculateOrientation(Mat& image, int x, int y);
    void generateMagnitudes(int scaleIndex, int intervalIndex);
    void generateOrientations(int scaleIndex, int intervalIndex);

    // Orientation

    // Descriptor generation
    float gaussianWeightingFunction(Extrema extrema, int x, int y);
    vector<float> generateDescriptorHistogram(Extrema extrema, Point topLeft);
    Vec<float, 128> generateDescriptor(Extrema extrema);
public:
    ~SIFT();
    SIFT(Mat& template_image);

    void run();
    void extremaMapper(Mat& image);
    map<pair<int, int>, Extrema>* getExtremas();
};