#ifndef _LOCALIZATION_H_
#define _LOCALIZATION_H_

#include <opencv2/opencv.hpp>
using namespace cv;

bool checkExtrema(Mat** scaleSpace, int x, int y, int s);
bool eliminateEdgeResponse(Mat** scaleSpace, int x, int y, int s);

#endif