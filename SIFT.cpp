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
        this->differenceOfGaussian(i, pow(K_FACTOR, scale) * DOG_SIGMA);
    }

    // Generate neighbors
    for (int i = 0; i < SCALES; i++) {
        for (int j = 1; j < INTERVALS - 1; j++) {
            this->neighbors(i, j);
        }
    }
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
        circle(image, extrema.location * extrema.scale, 2 * extrema.scale, Scalar(0,0,255));
    }
}

void SIFT::differenceOfGaussian(int index, float sigma) {
    // e.g. for INTERVALS = 3, we need to calculate 4 gaussian blurs to generate three DoGs
    for (int i = 0; i < INTERVALS + 1; i++) {
        GaussianBlur(imageScales[index], gaussians[index][i], Size(3, 3), sigma);
        sigma = K_FACTOR * sigma;
        if (i != 0) {
            subtract(gaussians[index][i], gaussians[index][i - 1], dogs[index][i - 1]);
        }
    }
}

void SIFT::neighbors(int scaleIndex, int intervalIndex) {
    int mid, largest, smallest; //if there exists larger value larger == 1
    int cstart, cstop, rstart, rstop;
    map<pair<int, int>, Extrema> ::iterator iter;

    for (int i = 0; i < dogs[scaleIndex][intervalIndex].cols; i++) {
        for (int j = 0; j < dogs[scaleIndex][intervalIndex].rows; j++) {
            largest = 0;
            smallest = 0;

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
                        if (largest != 1 && dogs[scaleIndex][intervalIndex].at<float>(row, col) >= mid) {
                            largest = 1;
                        }
                        if (smallest != 1 && dogs[scaleIndex][intervalIndex].at<float>(row, col) <= mid) {
                            smallest = 1;
                        }
                    }
                }
            }

            if (largest != 1) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && largest == 0; tc++) {
                    for (int tr = rstart; tr <= rstop && largest == 0; tr++) {
                        if (dogs[scaleIndex][intervalIndex+1].at<float>(tr, tc) > mid || dogs[scaleIndex][intervalIndex-1].at<float>(tr, tc) > mid) {
                            largest = 1;
                        }
                    }
                }
            }

            if (smallest != 1) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && smallest == 0; tc++) {
                    for (int tr = rstart; tr <= rstop && smallest == 0; tr++) {
                        if (dogs[scaleIndex][intervalIndex+1].at<float>(tr, tc) < mid || dogs[scaleIndex][intervalIndex-1].at<float>(tr, tc) < mid) {
                            smallest = 1;
                        }
                    }
                }
            }

            if (largest == 0 || smallest == 0) {
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
                    extrema.location = Point(i, j);
                    extrema.intensity = mid;
                    extremas.insert(make_pair(make_pair(i, j), extrema));
                } else {
                    iter->second.intensity = mid;
                    iter->second.scaleIndex = scaleIndex;
                    iter->second.scale = pow(2, scaleIndex);
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