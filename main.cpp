#include "SIFT.h"

// using namespace cv;
// using namespace std;


int main(int argc, char** argv) {
  if ( argc != 3 ){
    printf("usage: DisplayImage.out <Image_Path> <Image_Path>\n");
    return -1;
  }

  Mat template_img_1;
  template_img_1 = imread( argv[1], 1 );

  if ( !template_img_1.data ){
    printf("No Template data \n");
    return -1;
  }

  Mat template_img_2;
  template_img_2 = imread( argv[2], 1 );

  if ( !template_img_2.data ){
    printf("No Template data \n");
    return -1;
  }

  SIFT sift1(template_img_1);
  sift1.run();
  vector<Feature>* firstResults = sift1.getFeatures();
  SIFT sift2(template_img_2);
  sift2.run();
  vector<Feature>* secondResults = sift2.getFeatures();

  cout << "number of features " << sift1.numFeatures() << endl;
  cout << "number of features " << sift2.numFeatures() << endl;


  //sift1.extremaMapper(template_img_1);
  // namedWindow("Display Image1", WINDOW_AUTOSIZE );
  // imshow("Display Image1", template_img_1);
  //sift2.extremaMapper(template_img_2);
  // namedWindow("Display Image2", WINDOW_AUTOSIZE );
  // imshow("Display Image2", template_img_2);

  Mat combined = sift1.drawMatches(template_img_1, template_img_2, secondResults);
  namedWindow("Combined", WINDOW_AUTOSIZE );
  imshow("Combined", combined);
  imwrite("butter3.jpg", combined);
  waitKey(0);

  return 0;
}