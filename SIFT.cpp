#include "SIFT.h"

SIFT::SIFT() {

}

SIFT::~SIFT() {

}

vector<Feature> SIFT::run(Mat& template_image) {
    cvtColor(template_image, gray_img, CV_BGR2GRAY);
    gray_img.convertTo(gray_img, CV_32F);

    extremas.clear();
    features.clear();

    // Generate DoGs
    int scale = 1;
    for (int i = 0; i < SCALES; i++, scale *= 2) {
        resize(gray_img, imageScales[i], gray_img.size() / scale);
        this->differenceOfGaussian(i, scale);
    }

    // Generate extrema, magnitudes and orientations
    for (int i = 0; i < SCALES; i++) {
        for (int j = 1; j < INTERVALS - 1; j++) {
            this->neighbors(i, j);
            this->generateMagnitudes(i, j);
            this->generateOrientations(i, j);
        }
    }

    // Create orientations
    extremaOrientation();

    // Generate descriptors
    map<pair<int,int>,Extrema>::iterator it;
    for (it = extremas.begin(); it != extremas.end(); it++) {
        Extrema extrema = it->second;
        for (int orientation : extrema.orientation) {
            Feature feature;
            feature.location = extrema.location;
            // Need to convert orientation to radian
            feature.descriptor = this->generateDescriptor(extrema, ((float)orientation) * M_PI / 180);
            features.push_back(feature);
        }
    }

    return features;
}

//LOG APPROXIMATION - DIFFERENCE OF GAUSSIAN 

void SIFT::boundsCheck(int arr_row, int arr_col,
                       int* cstart, int* cstop,
                       int* rstart, int* rstop ) {
    if (*cstart < 0) {
        *cstart = 0;
    }
    if (*cstop >= arr_col ) {
        *cstop = arr_col - 1;
    }

    if (*rstart < 0) {
        *rstart = 0;
    }
    if (*rstop >= arr_row ) {
        *rstop = arr_row - 1;
    }
}

void SIFT::featureMapper(Mat& image, vector<Feature>& features) {
    int x_cor, y_cor;
    int scale_size = 0;
    vector<Feature>::iterator iter;

    for(iter = features.begin(); iter != features.end(); iter++){
        Feature feature = (*iter);
        circle(image, feature.location, 3, Scalar(0,0,255));
    }
}

void SIFT::differenceOfGaussian(int index, int scale) {
    // e.g. for INTERVALS = 3, we need to calculate 4 gaussian blurs to generate three DoGs
    float sigma = scale * DOG_SIGMA; // Each Gaussian pyramid starts at scale * sigma
    for (int i = 0; i < INTERVALS + 1; i++) {
        GaussianBlur(imageScales[index], gaussians[index][i], Size(3, 3), sigma);
        sigma = K_FACTOR * sigma;
        if (i != 0) {
            subtract(gaussians[index][i], gaussians[index][i - 1], dogs[index][i - 1]);
        }
    }
}

//FIND EXTREMA BETWEEN DOG APPROXIMATIONS

void SIFT::neighbors(int scaleIndex, int intervalIndex) {
    int mid, scale, scaled_x, scaled_y;
    bool largest, smallest; //if there exists larger value larger == 1
    int cstart, cstop, rstart, rstop;
    map<pair<int, int>, Extrema> ::iterator iter;

    for (int i = 0; i < dogs[scaleIndex][intervalIndex].cols; i++) {
        for (int j = 0; j < dogs[scaleIndex][intervalIndex].rows; j++) {
            largest = false;
            smallest = false;

            // Note that it is cast to an integer here. We use equality checks later on so we can't use floats
            mid = dogs[scaleIndex][intervalIndex].at<float>(j, i);

            cstart = i - 1;
            cstop  = i + 1;
            rstart = j - 1;
            rstop  = j + 1;
            boundsCheck(dogs[scaleIndex][intervalIndex].rows, dogs[scaleIndex][intervalIndex].cols, &cstart, &cstop, &rstart, &rstop);

            for (int col = cstart; col <= cstop; col++) {
                for (int row = rstart; row <= rstop; row++) {
                    //check 8 neighbors
                    //do not evaluate again if at row col of mid value
                    if ((col != i) || (row != j)) {
                        if (!largest && dogs[scaleIndex][intervalIndex].at<float>(row, col) >= mid) {
                            largest = true;
                        }
                        if (!smallest && dogs[scaleIndex][intervalIndex].at<float>(row, col) <= mid) {
                            smallest = true;
                        }
                    }
                }
            }

            if (!largest) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && !largest; tc++) {
                    for (int tr = rstart; tr <= rstop && !largest; tr++) {
                        if (dogs[scaleIndex][intervalIndex+1].at<float>(tr, tc) > mid || dogs[scaleIndex][intervalIndex-1].at<float>(tr, tc) > mid) {
                            largest = true;
                        }
                    }
                }
            }

            if (!smallest) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && !smallest; tc++) {
                    for (int tr = rstart; tr <= rstop && !smallest; tr++) {
                        if (dogs[scaleIndex][intervalIndex+1].at<float>(tr, tc) < mid || dogs[scaleIndex][intervalIndex-1].at<float>(tr, tc) < mid) {
                            smallest = true;
                        }
                    }
                }
            }

            if (!largest || !smallest) {
                // Section 4 & 4.1
                if (checkExtrema(i, j, scaleIndex, intervalIndex) ||
                    eliminateEdgeResponse(i, j, scaleIndex, intervalIndex)) {
                    continue;
                }
                scale = pow(2, scaleIndex);
                scaled_x = i*scale;
                scaled_y = j*scale;
                iter = extremas.find(make_pair(scaled_x, scaled_y));

                if (iter == extremas.end()) {
                    Extrema extrema;
                    extrema.intervalIndex = intervalIndex;
                    extrema.scaleIndex = scaleIndex;
                    extrema.scale = scale;
                    extrema.location = Point(scaled_x, scaled_y);
                    extrema.scaleLocation = Point(i, j);
                    extrema.intensity = mid;
                    // default values.
                    extrema.magnitude = 1;
                    extrema.orientation = vector<int>();
                    extremas.insert(make_pair(make_pair(scaled_x, scaled_y), extrema));
                } else {
                    iter->second.intervalIndex = intervalIndex;
                    iter->second.scaleIndex = scaleIndex;
                    iter->second.intensity = mid;
                    iter->second.scale = pow(2, scaleIndex);
                    iter->second.location = Point(scaled_x, scaled_y);
                    iter->second.scaleLocation = Point(i, j);
                }
            }
        }
    }
}

//REMOVE NOISY EXTREMA

// Checks bounds before grabbing float.
float SIFT::retrieveFloat(int x, int y, int scaleIndex, int intervalIndex) {
    if (x < 0 || y < 0
        || x >= dogs[scaleIndex][intervalIndex].size().width
        || y >= dogs[scaleIndex][intervalIndex].size().height) {
        return 0;
    }
    return dogs[scaleIndex][intervalIndex].at<float>(Point(x, y));
}

Mat SIFT::calculateHessianMatrix33(int x, int y, int scaleIndex, int intervalIndex) {
    //TODO: handle points on edges of image (if x=0 we will try to access x-1 or -1)
    float dxx =
        ( retrieveFloat(x+1, y, scaleIndex, intervalIndex) +
          retrieveFloat(x-1, y, scaleIndex, intervalIndex) -
          2*retrieveFloat(x, y, scaleIndex, intervalIndex) ) / 2;
    float dyy =
        ( retrieveFloat(x, y+1, scaleIndex, intervalIndex) +
          retrieveFloat(x, y-1, scaleIndex, intervalIndex) -
          2*retrieveFloat(x, y, scaleIndex, intervalIndex) ) / 2;
    float dss =
        ( retrieveFloat(x, y, scaleIndex, intervalIndex+1) +
          retrieveFloat(x, y, scaleIndex, intervalIndex-1) -
          2*retrieveFloat(x, y, scaleIndex, intervalIndex) ) / 2;
    float dxy = 
        ( (retrieveFloat(x+1, y+1, scaleIndex, intervalIndex) / 2 - retrieveFloat(x-1, y+1, scaleIndex, intervalIndex) / 2) -
          (retrieveFloat(x+1, y-1, scaleIndex, intervalIndex) / 2 - retrieveFloat(x-1, y-1, scaleIndex, intervalIndex) / 2) ) / 2;
    float dxs =
        ( (retrieveFloat(x+1, y, scaleIndex, intervalIndex+1) / 2 - retrieveFloat(x-1, y, scaleIndex, intervalIndex+1) / 2) -
          (retrieveFloat(x+1, y, scaleIndex, intervalIndex-1) / 2 - retrieveFloat(x-1, y, scaleIndex, intervalIndex-1) / 2) ) / 2;
    float dys =
        ( (retrieveFloat(x, y+1, scaleIndex, intervalIndex+1) / 2 - retrieveFloat(x, y-1, scaleIndex, intervalIndex+1) / 2) -
          (retrieveFloat(x, y+1, scaleIndex, intervalIndex-1) / 2 - retrieveFloat(x, y-1, scaleIndex, intervalIndex-1) / 2) ) / 2;

    Mat hessian(3, 3, CV_32F);
    hessian.at<float>(Point(0, 0)) = dxx;
    hessian.at<float>(Point(0, 1)) = dxy;
    hessian.at<float>(Point(1, 0)) = dxy;
    hessian.at<float>(Point(1, 1)) = dyy;
    hessian.at<float>(Point(0, 2)) = dxs;
    hessian.at<float>(Point(2, 0)) = dxs;
    hessian.at<float>(Point(2, 2)) = dss;
    hessian.at<float>(Point(1, 2)) = dys;
    hessian.at<float>(Point(2, 1)) = dys;

    return hessian;
}

bool SIFT::checkExtrema(int x, int y, int scaleIndex, int intervalIndex) {
    // Section 4 of SIFT paper
    // http://dsp.stackexchange.com/questions/10403/sift-taylor-expansion
    // https://www.cs.ubc.ca/~lowe/papers/ijcv04.pdf
    Mat hessian = calculateHessianMatrix33(x, y, scaleIndex, intervalIndex);
    Mat colDerivative(3, 1, CV_32F);
    colDerivative.at<float>(Point(0, 0)) = (retrieveFloat(x+1, y, scaleIndex, intervalIndex) - retrieveFloat(x-1, y, scaleIndex, intervalIndex)) / 2;
    colDerivative.at<float>(Point(0, 1)) = (retrieveFloat(x, y+1, scaleIndex, intervalIndex) - retrieveFloat(x, y-1, scaleIndex, intervalIndex)) / 2;
    colDerivative.at<float>(Point(0, 2)) = (retrieveFloat(x, y, scaleIndex, intervalIndex+1) - retrieveFloat(x, y, scaleIndex, intervalIndex-1)) / 2;

    Mat extrema = -(hessian.inv()) * colDerivative;

    // TODO: resample (?) if extrema (or offset) is more than 0.5 in any dimension?

    // http://stackoverflow.com/questions/22826456/convert-cvmatexpr-to-type
    float value = retrieveFloat(x, y, scaleIndex, intervalIndex) + (1/2) * ((Mat)(colDerivative.t() * extrema)).at<float>(0);

    return abs(value) < EXTREMA_THRESHOLD;
}

bool SIFT::eliminateEdgeResponse(int x, int y, int scaleIndex, int intervalIndex) {
    Mat hessian = calculateHessianMatrix33(x, y, scaleIndex, intervalIndex);

    float tr = hessian.at<float>(Point(0, 0)) + hessian.at<float>(Point(1, 1));
    float det = hessian.at<float>(Point(0, 0)) * hessian.at<float>(Point(1, 1)) - pow(hessian.at<float>(Point(1,0)), 2);

    if (det < 0) {
        return true;
    }

    // If tr^2/det >= (r+1)^2 / r then the princpal curvature is too large, eliminate point
    return (pow(tr, 2) / det) >= THRESHOLD_R;
}

//ORIENTATION AND DESCRIPTORS

float SIFT::calculateMagnitude(Mat& image, int x, int y) {
    float leftTerm  = pow(image.at<float>(Point(x+1, y)) - image.at<float>(Point(x-1,y)), 2);
    float rightTerm = pow(image.at<float>(Point(x, y+1)) - image.at<float>(Point(x,y-1)), 2);

    return sqrt(leftTerm + rightTerm);
}

float SIFT::calculateOrientation(Mat& image, int x, int y) {
    float numerator   = image.at<float>(Point(x, y+1)) - image.at<float>(Point(x, y-1));
    float denominator = image.at<float>(Point(x+1, y)) - image.at<float>(Point(x-1, y));

    return atan2(numerator, denominator);
}

void SIFT::generateMagnitudes(int scaleIndex, int intervalIndex) {
    Mat gaussian = gaussians[scaleIndex][intervalIndex];
    magnitudes[scaleIndex][intervalIndex] = Mat::zeros(gaussian.size(), gaussian.type());

    // Start at 1 and end at (length - 1) to prevent out of bounds access
    for (int x = 1; x < gaussian.size().width - 1; x++) {
        for (int y = 1; y < gaussian.size().height - 1; y++) {
            magnitudes[scaleIndex][intervalIndex].at<float>(Point(x, y)) = calculateMagnitude(gaussian, x, y);
        }
    }
}



/* Takes angles as radians, converts to degrees */
void SIFT::distrHistVals(vector<float>* histogram, float angle, float weighted){
//cout << weighted << " ";
  int index = 0;
  int degrees = 360 + angle* ( 180 / M_PI );
  degrees = degrees % 360;

  if( degrees == 0 || degrees < 10 || degrees == 360 ){
    (*histogram)[0] += weighted;
  }
  else if ( degrees >= 10 && degrees < 360 ){
    index = floor(degrees/10);
    (*histogram)[index] += weighted;
  }

}

/*Builds a weighted histogram based on gradient direction and gradient magnitude.*/
void SIFT::extremaOrientation(){

  vector<float> histogram(37);
  int x_cor, y_cor;
  int cstart, cstop, rstart, rstop;
  float  weighted, max;
  int scale, scaleIndex, interval;

  for(auto iter= extremas.begin(); iter != extremas.end(); iter++){
    x_cor = iter->second.scaleLocation.x;
    y_cor = iter->second.scaleLocation.y;

    for(int h=0; h< histogram.size(); h++){
      histogram[h] = 0;
    }

    //get weighted values for each neigbour and add to histogram
    scale = iter->second.scale;
    scaleIndex = iter->second.scaleIndex;
    interval = iter->second.intervalIndex;

    cstart = x_cor - scaleIndex;
    cstop  = x_cor + scaleIndex;
    rstart = y_cor - scaleIndex;
    rstop  = y_cor + scaleIndex;
    boundsCheck(imageScales[scaleIndex].rows, imageScales[scaleIndex].cols, &cstart, &cstop, &rstart, &rstop);
    //calculate variance
    for(int col = cstart; col <= cstop; col++){
      for(int row = rstart; row <= rstop; row++){
        weighted = magnitudes[scaleIndex][interval].at<float>(row,col)*gaussianWeightingFunction(iter->second, col, row, scale);
        distrHistVals(&histogram, orientations[scaleIndex][interval].at<float>(row,col), weighted);
      }
    }

    max = 0;
    for(int j=0; j< histogram.size(); j++){
      if ( histogram[j] > max ){
        max = histogram[j];
      }
    }

    if( max != 0 ){
      for(int k=0; k< histogram.size(); k++){
        if( histogram[k] >= max*0.8 ){
          iter->second.orientation.push_back(k*10); //orientation
        }
      }
    }
    else{ //give it orientation 0 if maximum of histogram found to be 0
      //cout << "zero " ;
      iter->second.orientation.push_back(0); //orientation
    }


  }
}


void SIFT::generateOrientations(int scaleIndex, int intervalIndex) {
    Mat gaussian = gaussians[scaleIndex][intervalIndex];
    orientations[scaleIndex][intervalIndex] = Mat::zeros(gaussian.size(), gaussian.type());

    // Start at 1 and end at (length - 1) to prevent out of bounds access
    for (int x = 1; x < gaussian.size().width - 1; x++) {
        for (int y = 1; y < gaussian.size().height - 1; y++) {
            orientations[scaleIndex][intervalIndex].at<float>(Point(x, y)) = calculateOrientation(gaussian, x, y);
        }
    }
}

//TODO: check if gaussian equations should be same
/* If scale provided calculates gaussian with sigma = 1.5*scale*/
float SIFT::gaussianWeightingFunction(Extrema extrema, int x, int y, int scale) {

    float distance = pow(extrema.location.x - x, 2) + pow(extrema.location.y - y, 2);

    if( scale < 0 ){
        // 128 = 2 * (0.5 * windowsize=16)^2
        // e ^ (-(x - mu)^2 / (2*sigma^2))
        return exp(-(distance) / (2 * pow(1.6, 2)));
    }
    else{
      //cout << distance / pow((scale * 1.5), 2) << " ";

        distance = exp(-0.5 * (distance / pow(scale * 1.5, 2)));
        return M_INV_2PI*distance;
    }
}


vector<float> SIFT::generateDescriptorHistogram(Extrema extrema, Point topLeft, float orientation) {
    vector<float> histogram(8);
    for (int i = 0; i < 8; i++) {
        histogram[i] = 0;
    }
    
    for (int x = topLeft.x; x < topLeft.x + 4; x++) {
        for (int y = topLeft.y; y < topLeft.y + 4; y++) {
            if (x < 1 || y < 1 || x / extrema.scale >= (gray_img.size().width / extrema.scale) - 1 || y / extrema.scale >= (gray_img.size().height / extrema.scale) - 1) {
                // Avoid OOB
                continue;
            }
            float m = magnitudes[extrema.scaleIndex][extrema.intervalIndex].at<float>(Point(x, y) / extrema.scale);
            float o = orientations[extrema.scaleIndex][extrema.intervalIndex].at<float>(Point(x, y) / extrema.scale);
            o += orientation; // rotate in respect with extrema orientation

            // Put into bins 0 - 7
            if (o < 0) {
                o += 2 * M_PI;
            }
            int bin = (int)(o / (M_PI / 4)) % 8;
            
            // magnitude is weighted by the distance from the extrema using Gaussian blur
            // TODO: trilinear interpolation???
            histogram[bin] += m * gaussianWeightingFunction(extrema, x, y);
        }
    }
    
    return histogram;
}

Vec<float, 128> SIFT::generateDescriptor(Extrema extrema, float orientation) {
    Vec<float, 128> featureVector;
    for (int i = 0; i < 128; i++) {
        featureVector[i] = 0;
    }

    // 16x16 window around extrema (start at location.x/y - 8 to +8)
    int count = 0;
    for (int x = extrema.location.x - 8; x < extrema.location.x + 8; x += 4) {
        for (int y = extrema.location.y - 8; y < extrema.location.y + 8; y += 4) {
            if (x < 1 || y < 1 || x >= gray_img.size().width - 1 || y >= gray_img.size().height - 1) {
                // Avoid OOB
                count += 8;
                continue;
            }
            vector<float> histogram = generateDescriptorHistogram(extrema, Point(x, y), orientation);
            // Copy into our feature vector
            for (int j = 0; j < 8; j++, count++) {
                featureVector[count] = histogram[j];
            }
        }
    }

    // Normalize resulting descriptor vector, which is 128 dimensions
    float length = norm(featureVector);
    if (length == 0) {
        return featureVector;
    }
    featureVector /= length;
    // Threshold values to 0.2 and re-normalize...
    bool renormalize = false;
    for (int i = 0; i < 128; i++) {
        if (featureVector[i] > 0.2) {
            featureVector[i] = 0.2;
            renormalize = true;
        }
    }
    if (renormalize) {
        featureVector /= norm(featureVector);
    }

    return featureVector;
}

//FEATURE MATCHING

/*Images provided to this function should have extrema pre-drawn*/
Mat SIFT::drawMatches(Mat& image1, Mat& image2, vector<Feature>& features1, vector<Feature>& features2){
  
  Mat combined_img;
  int new_width, new_height;
  int img1_row, img1_col, img2_row, img2_col;

  new_width  = image1.cols + image2.cols;
  new_height = image1.rows;
  if ( image2.rows > new_height ){
    new_height = image2.rows;
  }

  //set up image boundaries for first and second
  img1_row = image1.rows;
  img1_col = image1.cols;

  img2_row = image2.rows;
  img2_col = image1.cols + image2.cols;

  //need to do this b/c don't know image type and depth
  resize(image1, combined_img, Size(new_width, new_height));

  if( image2.rows != image1.rows || image2.cols != image1.cols ){
    combined_img.setTo(Scalar(0,0,0));
  }

  //Transfer images over
  image1.copyTo(combined_img.rowRange(0, img1_row).colRange(0, img1_col));
  image2.copyTo(combined_img.rowRange(0, img2_row).colRange(img1_col, img2_col));

  drawNearestNeighborsRatio(features1, features2, combined_img, img1_col);

  return combined_img;
}

pair<vector<Point2f>, vector<Point2f>> SIFT::getBestMatchingPairs(vector<Feature>& features1, vector<Feature>& features2) {
  Point second_point;
  vector<Feature>::iterator firstIt;
  vector<Feature>::iterator secondIt;

  vector<Point2f> goodFeatures1, goodFeatures2;
  for (firstIt = features1.begin(); firstIt != features1.end(); firstIt++) {
    Feature first = (*firstIt);
    Vec<float, 128> firstDescriptor = first.descriptor;
    
    // Initialize
    Point firstClose;
    double firstDistance = -1;
    Point secondClose;
    double secondDistance = -1;
    for (secondIt = features2.begin(); secondIt != features2.end(); secondIt++) {
      Feature second = (*secondIt);
      if (norm(first.location - second.location) > 50) {
          continue;
      }
      Vec<float, 128> secondDescriptor = second.descriptor;
      double distance = norm(firstDescriptor - secondDescriptor);
      if (distance < firstDistance || firstDistance == -1) {
        secondDistance = firstDistance;
        secondClose = firstClose;
        firstDistance = distance;
        firstClose = second.location;
      }
    }

    // TODO: make these command line arguments
    if (norm(first.location - firstClose) < 50 && (firstDistance / secondDistance) < 0.2) {
      // The current pair of features is a good match to pass to findHomography
      goodFeatures1.push_back(Point2f(first.location.x, first.location.y));
      goodFeatures2.push_back(Point2f(firstClose.x, firstClose.y));
    }
  }

  return make_pair(goodFeatures1, goodFeatures2);
}

void SIFT::drawNearestNeighborsRatio(vector<Feature>& features1, vector<Feature>& features2, Mat& combined_img, int img_offset) {
    pair<vector<Point2f>, vector<Point2f>> matchingPair = SIFT::getBestMatchingPairs(features1, features2);
    Point second_point;
    vector<Point2f>::iterator firstIt;
    vector<Point2f>::iterator secondIt;
    for (firstIt = matchingPair.first.begin(), secondIt = matchingPair.second.begin();
        firstIt != matchingPair.first.end() && secondIt != matchingPair.second.end();
        firstIt++, secondIt++) {
        Point2f firstPoint = (*firstIt);
        Point2f secondPoint (*secondIt);
        secondPoint.x += img_offset;

        line(combined_img, firstPoint, secondPoint, Scalar(0,255,0));
    }
}