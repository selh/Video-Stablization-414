#include "SIFT.h"

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

  SIFT sift(template_img);
  sift.run();
  sift.extremaMapper(template_img);

  namedWindow("Display Image", WINDOW_AUTOSIZE );
  imshow("Display Image", template_img);
  waitKey(0);

  return 0;
}