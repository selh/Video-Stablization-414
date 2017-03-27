#ifndef _ORIENTATION_
#define _ORIENTATION_

#include "descriptor.h" //uses magnitude and angle calculator
#include <math>
#include <map>

#define M_PI 3.14
#define M_E 2.71

using namespace std;
using namespace cv;

/* Takes angles in degrees */
void distrHistVals(vector<float>* histogram, float angle, float weighted);

/*Builds a weighted histogram based on gradient direction and gradient magnitude.*/
void buildWeightedHist(Mat& magnitude, 
                       Mat& direction, 
                       vector<int>* extrema, 
                       map< pair<int, int>, vector<float> >* key_orientation);


#endif