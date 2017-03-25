#ifndef _SCALE_H
#define _SCALE_H

#define DOG_SIGMA 1.6
#define K_FACTOR sqrt(2)

/*Fix for loop bounds if out of bounds */
void boundsCheck(int arr_row, int arr_col,
                 int* cstart, int* cstop, 
                 int* rstart, int* rstop );

/*Calculates the difference of Gaussian for 3 sigmas*/
void differenceOfGaussian(Mat& gray_img,  Mat& diff_img1, 
                          Mat& diff_img2, Mat& diff_img3, 
                          const int sigma);

/*Draws circles on to the original gray scale image, scale_size should be size of
which the image was subsampled at */
void extremaMapper(vector<int>* extrema, Mat& image, int scale_size=1);

/*Takes the middle array (input_arr) and compares its pixel to its own 
  8 neighbors and the 9 neighbors in top and bottom arrays.
  Precondition: all arrays must be the same size*/
void neighbors(Mat& input_arr, const Mat& top_arr, const Mat& btm_arr, vector<int>* extrema);

#include "scale.cpp"
#endif