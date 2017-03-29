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
  map<pair<int, int>, Extrema>* firstResults = sift1.getExtremas();
  SIFT sift2(template_img_2);
  sift2.run();
  map<pair<int, int>, Extrema>* secondResults = sift2.getExtremas();

  //printing out orientations
  cout << "key point orientations" << endl;
  for( auto iter= firstResults->begin(); iter != firstResults->end(); iter++ ){
    for( int i= 0; i < iter->second.orientation.size(); i++){
      cout << iter->second.orientation[i] << " " ;
    }
    cout << endl;
  }

  ////
  // cout << "Count 1:" << firstResults->size() << endl;
  // cout << "Count 2:" << secondResults->size() << endl;


  // // Exhaustive search
  // map<pair<int, int>, Extrema>::iterator firstIt;
  // map<pair<int, int>, Extrema>::iterator secondIt;
  // int count = 0;
  // for (firstIt = firstResults->begin(); firstIt != firstResults->end(); firstIt++) {
  //   Extrema first = firstIt->second;
  //   Vec<float, 128> firstDescriptor = first.descriptor;
    
  //   // Initialize
  //   Point firstClose;
  //   double firstDistance = -1;
  //   Point secondClose;
  //   double secondDistance = -1;
  //   for (secondIt = secondResults->begin(); secondIt != secondResults->end(); secondIt++) {
  //     Extrema second = secondIt->second;
  //     Vec<float, 128> secondDescriptor = second.descriptor;
  //     double distance = norm(firstDescriptor - secondDescriptor);
  //     if (distance < firstDistance || firstDistance == -1) {
  //       secondDistance = firstDistance;
  //       secondClose = firstClose;
  //       firstDistance = distance;
  //       firstClose = second.location;
  //     }
  //   }

  //   cout << "1: (" << first.location.x << "," << first.location.y << ")" << endl;
  //   cout << "2: (" << firstClose.x << "," << firstClose.y << ") d-distance: " << firstDistance << ", p-distance: " << norm(first.location - firstClose) << endl;
  //   cout << "3: (" << secondClose.x << "," << secondClose.y << ") d-distance: " << secondDistance << ", p-distance: " << norm(first.location - secondClose) << endl;
  //   cout << "ratio: " << (firstDistance / secondDistance) << endl;
  //   cout << endl;

  //   if (norm(first.location - firstClose) < 50) {
  //     count++;
  //   }
  // }  
  // cout << "Image space distances < 50: " << count << endl;

  sift1.extremaMapper(template_img_1);
  namedWindow("Display Image1", WINDOW_AUTOSIZE );
  imshow("Display Image1", template_img_1);
  sift2.extremaMapper(template_img_2);
  namedWindow("Display Image2", WINDOW_AUTOSIZE );
  imshow("Display Image2", template_img_2);
  waitKey(0);

  return 0;
}