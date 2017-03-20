#include <opencv2/opencv.hpp>
#include <math.h>

using namespace cv;

#define EXTREMA_THRESHOLD 0.03
#define CONSTANT_R 10 // SIFT paper recommends using r=10
#define THRESHOLD_R (((CONSTANT_R + 1) * (CONSTANT_R + 1)) / CONSTANT_R)

float retrieveFloat(Mat** matrix, int x, int y, int s) {
    // TODO: check bounds here?
    return matrix[s]->at<float>(Point(x, y));
}

Mat calculateHessianMatrix33(Mat** scaleSpace, int x, int y, int s) {
    //TODO: handle points on edges of image (if x=0 we will try to access x-1 or -1)
    float dxx =
        ( retrieveFloat(scaleSpace, x+1, y, s) +
          retrieveFloat(scaleSpace, x-1, y, s) -
          2*retrieveFloat(scaleSpace, x, y, s) ) / 2;
    float dyy =
        ( retrieveFloat(scaleSpace, x, y+1, s) +
          retrieveFloat(scaleSpace, x, y-1, s) -
          2*retrieveFloat(scaleSpace, x, y, s) ) / 2;
    float dss =
        ( retrieveFloat(scaleSpace, x, y, s+1) +
          retrieveFloat(scaleSpace, x, y, s-1) -
          2*retrieveFloat(scaleSpace, x, y, s) ) / 2;
    float dxy = 
        ( (retrieveFloat(scaleSpace, x+1, y+1, s) / 2 - retrieveFloat(scaleSpace, x-1, y+1, s) / 2) -
          (retrieveFloat(scaleSpace, x+1, y-1, s) / 2 - retrieveFloat(scaleSpace, x-1, y-1, s) / 2) ) / 2;
    float dxs =
        ( (retrieveFloat(scaleSpace, x+1, y, s+1) / 2 - retrieveFloat(scaleSpace, x-1, y, s+1) / 2) -
          (retrieveFloat(scaleSpace, x+1, y, s-1) / 2 - retrieveFloat(scaleSpace, x-1, y, s-1) / 2) ) / 2;
    float dys =
        ( (retrieveFloat(scaleSpace, x, y+1, s+1) / 2 - retrieveFloat(scaleSpace, x, y-1, s+1) / 2) -
          (retrieveFloat(scaleSpace, x, y+1, s-1) / 2 - retrieveFloat(scaleSpace, x, y-1, s-1) / 2) ) / 2;

    Mat hessian(3, 3, CV_32F);
    hessian.at<float>(Point(0, 0)) = dxx;
    hessian.at<float>(Point(0, 1)) = dxy;
    hessian.at<float>(Point(1, 0)) = dxy;
    hessian.at<float>(Point(1, 1)) = dyy;
    hessian.at<float>(Point(0, 2)) = dxs;
    hessian.at<float>(Point(2, 0)) = dxs;
    hessian.at<float>(Point(2, 2)) = dss;
    hessian.at<float>(Point(1, 2)) = dys;
    hessian.at<float>(Point(2, 1)) = dys;

    return hessian;
}

bool checkExtrema(Mat** scaleSpace, int x, int y, int s) {
    // Section 4 of SIFT paper
    // http://dsp.stackexchange.com/questions/10403/sift-taylor-expansion
    // https://www.cs.ubc.ca/~lowe/papers/ijcv04.pdf
    Mat hessian = calculateHessianMatrix33(scaleSpace, x, y, s);
    Mat colDerivative(3, 1, CV_32F);
    colDerivative.at<float>(Point(0, 0)) = (retrieveFloat(scaleSpace, x+1, y, s) - retrieveFloat(scaleSpace, x-1, y, s)) / 2;
    colDerivative.at<float>(Point(0, 1)) = (retrieveFloat(scaleSpace, x, y+1, s) - retrieveFloat(scaleSpace, x, y-1, s)) / 2;
    colDerivative.at<float>(Point(0, 2)) = (retrieveFloat(scaleSpace, x, y, s+1) - retrieveFloat(scaleSpace, x, y, s-1)) / 2;

    Mat extrema = -(hessian.inv()) * colDerivative;
    // TODO: does this work?
    // http://stackoverflow.com/questions/22826456/convert-cvmatexpr-to-type
    float value = retrieveFloat(scaleSpace, x, y, s) + (1/2) * ((Mat)(colDerivative.t() * extrema)).at<float>(0);

    return abs(value) >= EXTREMA_THRESHOLD;
}

bool eliminateEdgeResponse(Mat** scaleSpace, int x, int y, int s) {
    Mat hessian = calculateHessianMatrix33(scaleSpace, x, y, s);

    float tr = hessian.at<float>(Point(0, 0)) + hessian.at<float>(Point(1, 1));
    float det = hessian.at<float>(Point(0, 0)) * hessian.at<float>(Point(1, 1)) - pow(hessian.at<float>(Point(1,0)), 2);

    // If tr^2/det >= (r+1)^2 / r then the princpal curvature is too large, eliminate point
    return (pow(tr, 2) / det) >= THRESHOLD_R;
}