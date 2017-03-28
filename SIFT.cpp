#include "SIFT.h"

SIFT::SIFT(Mat& template_image) {
    cvtColor(template_image, gray_img, CV_BGR2GRAY);
    gray_img.convertTo(gray_img, CV_32F);
}

SIFT::~SIFT() {

}

void SIFT::run() {
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
    // <...>

    // Generate descriptors
    map<pair<int,int>,Extrema>::iterator it;
    for (it = extremas.begin(); it != extremas.end(); it++) {
        it->second.descriptor = generateDescriptor(it->second);
    }
}

map<pair<int, int>, Extrema>* SIFT::getExtremas() {
    return &extremas;
}

void SIFT::boundsCheck(int arr_row, int arr_col,
    int* cstart, int* cstop,
    int* rstart, int* rstop) {
    if (*cstart < 0) {
        *cstart = 0;
    }
    if (*cstop > arr_col - 1) {
        *cstop = arr_col - 1;
    }

    if (*rstart < 0) {
        *rstart = 0;
    }
    if (*rstop > arr_row - 1) {
        *rstop = arr_row - 1;
    }
}

void SIFT::extremaMapper(Mat& image) {
    int x_cor, y_cor;
    int scale_size = 0;
    map<pair<int, int>, Extrema>::iterator iter;

    for(iter = extremas.begin(); iter != extremas.end(); iter++){
        Extrema extrema = iter->second;
        circle(image, extrema.location, 2 * extrema.scale, Scalar(0,0,255));
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

void SIFT::neighbors(int scaleIndex, int intervalIndex) {
    int mid;
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
            cstop = i + 1;
            rstart = j - 1;
            rstop = j + 1;
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

                iter = extremas.find(make_pair(i, j));
                if (iter == extremas.end()) {
                    Extrema extrema;
                    extrema.intervalIndex = intervalIndex;
                    extrema.scaleIndex = scaleIndex;
                    extrema.scale = pow(2, scaleIndex);
                    extrema.location = Point(i, j) * extrema.scale;
                    extrema.scaleLocation = Point(i, j);
                    extrema.intensity = mid;
                    // default values.
                    extrema.magnitude = 1;
                    extrema.orientation = 0;
                    extremas.insert(make_pair(make_pair(i, j), extrema));
                } else {
                    iter->second.intervalIndex = intervalIndex;
                    iter->second.scaleIndex = scaleIndex;
                    iter->second.intensity = mid;
                    iter->second.scale = pow(2, scaleIndex);
                    iter->second.location = Point(i, j) * iter->second.scale;
                    iter->second.scaleLocation = Point(i, j);
                }
            }
        }
    }
}

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
    // TODO: does this work?
    // http://stackoverflow.com/questions/22826456/convert-cvmatexpr-to-type
    float value = retrieveFloat(x, y, scaleIndex, intervalIndex) + (1/2) * ((Mat)(colDerivative.t() * extrema)).at<float>(0);

    return abs(value) >= EXTREMA_THRESHOLD;
}

bool SIFT::eliminateEdgeResponse(int x, int y, int scaleIndex, int intervalIndex) {
    Mat hessian = calculateHessianMatrix33(x, y, scaleIndex, intervalIndex);

    float tr = hessian.at<float>(Point(0, 0)) + hessian.at<float>(Point(1, 1));
    float det = hessian.at<float>(Point(0, 0)) * hessian.at<float>(Point(1, 1)) - pow(hessian.at<float>(Point(1,0)), 2);

    // If tr^2/det >= (r+1)^2 / r then the princpal curvature is too large, eliminate point
    return (pow(tr, 2) / det) >= THRESHOLD_R;
}


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

float SIFT::gaussianWeightingFunction(Extrema extrema, int x, int y) {
    float distance = sqrt(pow(extrema.location.x - x, 2) + pow(extrema.location.y - y, 2));
    // 128 = 2 * (0.5 * windowsize=16)^2
    // e ^ (-(x - mu)^2 / (2*sigma^2))
    return exp(-pow(distance, 2) / (2 * pow(1.6, 2)));
}

vector<float> SIFT::generateDescriptorHistogram(Extrema extrema, Point topLeft) {
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
            o += extrema.orientation; // rotate in respect with extrema orientation/

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

Vec<float, 128> SIFT::generateDescriptor(Extrema extrema) {
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
            vector<float> histogram = generateDescriptorHistogram(extrema, Point(x, y));
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
