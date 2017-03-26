// #include <stdio.h>
// #include <opencv2/core/core.hpp>
#include <vector>
#include <math.h>
#include "scale.h"

// using namespace cv;
// using namespace std;


int main(int argc, char** argv) {

  if ( argc != 2 ){
    printf("usage: DisplayImage.out <Image_Path>\n");
    return -1;
  }

  Mat template_img;
  template_img = imread( argv[1], 1 );

  if ( !template_img.data ){
    printf("No Template data \n");
    return -1;
  }

  //=========Initalize arrays to hold image scaled for each octave==========//
  Mat gray_img, img_scale2, img_scale3, img_scale4; //holds the image resized to each scale at octave

  //=========Convert to grayscale image===========//
  cvtColor(template_img, gray_img, CV_BGR2GRAY);

  // //holds the DoG approximation (octave 1)
  // Mat diff_img1 = Mat::zeros(gray_img.size(), gray_img.type());
  // Mat diff_img2 = Mat::zeros(gray_img.size(), gray_img.type());
  // Mat diff_img3 = Mat::zeros(gray_img.size(), gray_img.type()); 
  //holds the DoG approximation (octave 2)
  Mat diff_img4 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat diff_img5 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat diff_img6 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type()); 
  //holds the DoG approximation (octave 3)
  Mat diff_img7 = Mat::zeros(Size( gray_img.cols/4, gray_img.rows/4), gray_img.type());
  Mat diff_img8 = Mat::zeros(Size( gray_img.cols/4, gray_img.rows/4), gray_img.type());
  Mat diff_img9 = Mat::zeros(Size( gray_img.cols/4, gray_img.rows/4), gray_img.type()); 

  //holds the DoG approximation (octave 4)
  Mat diff_img10 = Mat::zeros(Size( gray_img.cols/8, gray_img.rows/8), gray_img.type());
  Mat diff_img11 = Mat::zeros(Size( gray_img.cols/8, gray_img.rows/8), gray_img.type());
  Mat diff_img12 = Mat::zeros(Size( gray_img.cols/8, gray_img.rows/8), gray_img.type()); 

  //========== Arrays to hold extrema for now ==========//
    map< pair<int,int>, pair<int,int> > extrema_table;

  // map2.find(make_pair(10,10))->second.first
  // map2[make_pair(10,10)] = make_pair(5,5);

  // vector<int> extrema = vector<int>();
  // vector<int> extrema2 = vector<int>();
  // vector<int> extrema3 = vector<int>();

  //========== Approximate Laplacian of Gaussian ===========//
  //differenceOfGaussian(gray_img, diff_img1, diff_img2, diff_img3, DOG_SIGMA);
  resize(gray_img, img_scale2, Size( gray_img.cols/2, gray_img.rows/2));
  differenceOfGaussian(img_scale2, diff_img4, diff_img5, diff_img6, pow(K_FACTOR,2)*DOG_SIGMA);
  resize(gray_img, img_scale3, Size( gray_img.cols/4, gray_img.rows/4));
  differenceOfGaussian(img_scale3, diff_img7, diff_img8, diff_img9, pow(K_FACTOR,4)*DOG_SIGMA);
  resize(gray_img, img_scale3, Size( gray_img.cols/8, gray_img.rows/8)); 
  differenceOfGaussian(img_scale4, diff_img10, diff_img11, diff_img12, pow(K_FACTOR,8)*DOG_SIGMA);



  //========== Find extrema in first scale space and map on to image  =========//
  //neighbors(diff_img2, diff_img3, diff_img1, &extrema_table, 1);
  neighbors(diff_img5, diff_img6, diff_img4, &extrema_table, 2);
  neighbors(diff_img8, diff_img9, diff_img7, &extrema_table, 4);
  neighbors(diff_img11, diff_img12, diff_img10, &extrema_table, 8);

  //extremaCleaner(&extrema_table);
  extremaMapper(&extrema_table, gray_img);
  // extremaMapper(&extrema_table, gray_img, 2);
  // extremaMapper(&extrema_table, gray_img, 4);


  namedWindow("Display Image", WINDOW_AUTOSIZE );
  imshow("Display Image", gray_img);
  waitKey(0);

  return 0;
}