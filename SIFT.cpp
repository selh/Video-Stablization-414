#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <math.h>

using namespace cv;
using namespace std;

/*Fix for loop bounds if out of bounds */
void boundsCheck(int arr_row, int arr_col, 
                 int* cstart, int* cstop, 
                 int* rstart, int* rstop ){
  
  if( *cstart < 0 ){
    *cstart = 0;
  }
  if( *cstop > arr_col-1 ){
    *cstop = arr_col-1;
  }

  if( *rstart < 0 ){
    *rstart = 0;
  }
  if( *rstop > arr_row-1 ){
    *rstop = arr_row-1;
  }

}

/*Takes the middle array (input_arr) and compares its pixel to its own 
  8 neighbors and the 9 neighbors in top and bottom arrays.
  Precondition: all arrays must be the same size
*/
void neighbors(Mat& input_arr, const Mat& top_arr, const Mat& btm_arr, vector<int>* extrema){

  int mid, largest, smallest; //if there exists larger value larger == 1
  int cstart, cstop, rstart, rstop;

  for(int i=0; i< input_arr.cols; i++){
    for(int j=0; j < input_arr.rows; j++){
      largest  = 0;
      smallest = 0;
      mid = input_arr.at<uchar>(j,i);

      cstart = i - 1;
      cstop  = i + 1;
      rstart = j - 1;
      rstop  = j + 1;
      boundsCheck(input_arr.rows, input_arr.cols, &cstart, &cstop, &rstart, &rstop );
      //cout << cstart << " " << cstop << " " << rstart << " " << rstop << endl;
      //printf("middle val: %d ", mid);
      for(int col=cstart; col <= cstop; col++){
        for(int row=rstart; row <= rstop; row++){
          //check 8 neighbors
          //do not evaluate again if at row col of mid value
          if ( (col != i) || (row != j) ){
            if( largest != 1 && input_arr.at<uchar>(row,col) >= mid ){
              largest=1;
              //printf("mid: %d input: %d\n", mid, input_arr.at<uchar>(row,col) );
            }
            if( smallest != 1 && input_arr.at<uchar>(row,col) <= mid ){
              smallest=1;
            }
      	  }
        }
      }
      //cout << " smallest: " << smallest << " largest: " << largest ;

      if( largest != 1 ){
        //top array & bottom array
        for(int tc=cstart; tc <= cstop && largest == 0; tc++){
          for(int tr=rstart; tr <= rstop && largest == 0; tr++){
            if( top_arr.at<uchar>(tr,tc) > mid || btm_arr.at<uchar>(tr,tc) > mid ){
              largest=1;
            }
          }
        }
      }

      if( smallest != 1 ){
        //top array & bottom array
        for(int tc=cstart; tc <= cstop && smallest == 0; tc++){
          for(int tr=rstart; tr <= rstop && smallest == 0; tr++){
            if( top_arr.at<uchar>(tr,tc) < mid || btm_arr.at<uchar>(tr,tc) < mid ){
              smallest=1;
            }
          }
        }
      }

  
      if( largest == 0 || smallest == 0 ){
        extrema->push_back(i);
        extrema->push_back(j);
      }


    }
  }
}


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

  Mat gray_img, img_scale2, img_scale3; //holds the image resized to each scale at octave
  //Convert to grayscale image
  cvtColor(template_img, gray_img, CV_BGR2GRAY);

  Mat sigma_img1 = Mat::zeros(gray_img.size(), gray_img.type()); 
  Mat sigma_img2 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat sigma_img3 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat sigma_img4 = Mat::zeros(gray_img.size(), gray_img.type()); //holds the images with gaussian filter
  Mat sigma_img5 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type()); 
  Mat sigma_img6 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat sigma_img7 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat sigma_img8 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  //Mat sigma_img5, sigma_img6, sigma_img7, sigma_img8; //holds the images with gaussian filter

  Mat diff_img1 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat diff_img2 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat diff_img3 = Mat::zeros(gray_img.size(), gray_img.type()); //holds the DoG approximation (octave 1)
  Mat diff_img4 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat diff_img5 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type());
  Mat diff_img6 = Mat::zeros(Size( gray_img.cols/2, gray_img.rows/2), gray_img.type()); //holds the DoG approximation (octave 2)


  double sigma=1.6; //starting sigma value for gaussian
  double k=sqrt(2), sig_factor=0; 

  vector<int> extrema = vector<int>();

  //Seperate into octaves
  //Scale (octave 1)
  sig_factor = sigma;
  GaussianBlur(gray_img, sigma_img1, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img1, sigma_img2, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img2, sigma_img3, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img3, sigma_img4, Size(3,3), sig_factor);
  subtract(sigma_img2, sigma_img1, diff_img1);
  subtract(sigma_img2, sigma_img1, diff_img2);
  subtract(sigma_img4, sigma_img3, diff_img3);


  //Scale (octave 2)
  sig_factor = 2*sigma;
  resize(sigma_img2, img_scale2, Size( gray_img.cols/2, gray_img.rows/2));

  GaussianBlur(img_scale2, sigma_img5, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img5, sigma_img6, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img6, sigma_img7, Size(3,3), sig_factor);
  sig_factor = k*sig_factor;
  GaussianBlur(sigma_img7, sigma_img8, Size(3,3), sig_factor);
  subtract(sigma_img6, sigma_img5, diff_img4);
  subtract(sigma_img7, sigma_img6, diff_img5);
  subtract(sigma_img8, sigma_img7, diff_img6);

  // diff_img1 = diff_img1*20;
  // diff_img2 = diff_img2*20;
  // diff_img3 = diff_img3*20;
  // //Display images
  // imwrite("difference.jpg", diff_img1);
  // imwrite("diff2.jpg", diff_img2);
  // imwrite("diff3.jpg", diff_img3);

  //cout << img_scale2.cols << " " << img_scale2.rows << " end " << endl;
  // for( int i=0; i< diff_img2.rows; i++){
  // 	printf("\n");
  // 	for(int j=0; j < diff_img2.cols; j++){
  // 		printf( "%d ", diff_img2.at<uchar>(i,j));
  // 	}
  // }
  // for( int i=0; i< 20; i++){
  //   printf("\n");
  //   for(int j=0; j < 20; j++){
  //     printf( "%d ", diff_img2.at<uchar>(i,j));
  //   }
  // }

  neighbors(diff_img2, diff_img3, diff_img1, &extrema);
  //neighbors(diff_img5, diff_img6, diff_img4, &extrema);


  int x_cor, y_cor;
  for(int x=0; x < extrema.size(); x+=2){
    //cout << "(" << extrema[x] << "," << extrema[x+1] << ") "; 
    x_cor = extrema[x];
    y_cor = extrema[x+1];
    circle(gray_img, Point(x_cor, y_cor), 1, Scalar(0,0,255));

  }


  // diff_img4 = diff_img4*40;
  // diff_img5 = diff_img5*40;
  // diff_img6 = diff_img6*40;
  // namedWindow("Display", WINDOW_AUTOSIZE );
  // imshow("Display", diff_img1);
  // namedWindow("Display2", WINDOW_AUTOSIZE );
  // imshow("Display2", diff_img2);
  // namedWindow("Display3", WINDOW_AUTOSIZE );
  // imshow("Display3", diff_img3);
  // namedWindow("Display4", WINDOW_AUTOSIZE );
  // imshow("Display4", sigma_img4);
  // namedWindow("D1", WINDOW_AUTOSIZE );
  // imshow("D1", diff_img4);
  // namedWindow("D2", WINDOW_AUTOSIZE );
  // imshow("D2", diff_img5);
  // namedWindow("D3", WINDOW_AUTOSIZE );
  // imshow("D3", diff_img6);

  namedWindow("Display Image", WINDOW_AUTOSIZE );
  imshow("Display Image", gray_img);
  waitKey(0);

  return 0;
}