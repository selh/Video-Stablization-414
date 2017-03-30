#ifndef _ORIENTATION_
#define _ORIENTATION_

#include "scale.h"
#include "descriptor.h" //uses magnitude and angle calculator
#include <cmath>
#include <map>

#define M_INV_2PI 0.15915 // 1/(2PI) used in gaussian weighted circle calculation

using namespace std;
using namespace cv;


/*Builds a weighted histogram based on gradient direction and gradient magnitude.*/
void extremaOrientation(Mat& image);

#include "orientation.cpp"
#endif