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

  SIFT sift;
  vector<Feature> firstResults = sift.run(template_img_1);
  vector<Feature> secondResults = sift.run(template_img_2);

  // //printing out orientations
  // cout << "key point orientations" << endl;
  // for( auto iter= firstResults.begin(); iter != firstResults.end(); iter++ ){
  //   for( int i= 0; i < iter->second.orientation.size(); i++){
  //     cout << iter->second.orientation[i] << " " ;
  //   }
  //   cout << endl;
  // }

  ////
  cout << "Count 1:" << firstResults.size() << endl;
  cout << "Count 2:" << secondResults.size() << endl;


  // Exhaustive search
  vector<Feature>::iterator firstIt;
  vector<Feature>::iterator secondIt;
  int count = 0;
  for (firstIt = firstResults.begin(); firstIt != firstResults.end(); firstIt++) {
    Feature first = (*firstIt);
    Vec<float, 128> firstDescriptor = first.descriptor;
    
    // Initialize
    Point firstClose;
    double firstDistance = -1;
    Point secondClose;
    double secondDistance = -1;
    for (secondIt = secondResults.begin(); secondIt != secondResults.end(); secondIt++) {
      Feature second = (*secondIt);
      Vec<float, 128> secondDescriptor = second.descriptor;
      double distance = norm(firstDescriptor - secondDescriptor);
      if (distance < firstDistance || firstDistance == -1) {
        secondDistance = firstDistance;
        secondClose = firstClose;
        firstDistance = distance;
        firstClose = second.location;
      }
    }

    cout << "1: (" << first.location.x << "," << first.location.y << ")" << endl;
    cout << "2: (" << firstClose.x << "," << firstClose.y << ") d-distance: " << firstDistance << ", p-distance: " << norm(first.location - firstClose) << endl;
    cout << "3: (" << secondClose.x << "," << secondClose.y << ") d-distance: " << secondDistance << ", p-distance: " << norm(first.location - secondClose) << endl;
    cout << "ratio: " << (firstDistance / secondDistance) << endl;
    cout << endl;

    if (norm(first.location - firstClose) < 50) {
      count++;
    }
  }  
  cout << "Image space distances < 50: " << count << endl;

  sift.featureMapper(template_img_1, firstResults);
  namedWindow("Display Image1", WINDOW_AUTOSIZE );
  imshow("Display Image1", template_img_1);
  sift.featureMapper(template_img_2, secondResults);
  namedWindow("Display Image2", WINDOW_AUTOSIZE );
  imshow("Display Image2", template_img_2);
  waitKey(0);

  return 0;
}