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

void SIFT::neighbors(int scaleIndex, int current) {
    int mid, largest, smallest; //if there exists larger value larger == 1
    int cstart, cstop, rstart, rstop;
    map<pair<int, int>, Extrema> ::iterator iter;

    for (int i = 0; i < dogs[scaleIndex][current].cols; i++) {
        for (int j = 0; j < dogs[scaleIndex][current].rows; j++) {
            largest = 0;
            smallest = 0;

            mid = dogs[scaleIndex][current].at<float>(j, i);

            cstart = i - 1;
            cstop = i + 1;
            rstart = j - 1;
            rstop = j + 1;
            boundsCheck(dogs[scaleIndex][current].rows, dogs[scaleIndex][current].cols, &cstart, &cstop, &rstart, &rstop);

            for (int col = cstart; col <= cstop; col++) {
                for (int row = rstart; row <= rstop; row++) {
                    //check 8 neighbors
                    //do not evaluate again if at row col of mid value
                    if ((col != i) || (row != j)) {
                        if (largest != 1 && dogs[scaleIndex][current].at<float>(row, col) >= mid) {
                            largest = 1;
                        }
                        if (smallest != 1 && dogs[scaleIndex][current].at<float>(row, col) <= mid) {
                            smallest = 1;
                        }
                    }
                }
            }

            if (largest != 1) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && largest == 0; tc++) {
                    for (int tr = rstart; tr <= rstop && largest == 0; tr++) {
                        if (dogs[scaleIndex][current+1].at<float>(tr, tc) > mid || dogs[scaleIndex][current-1].at<float>(tr, tc) > mid) {
                            largest = 1;
                        }
                    }
                }
            }

            if (smallest != 1) {
                //top array & bottom array
                for (int tc = cstart; tc <= cstop && smallest == 0; tc++) {
                    for (int tr = rstart; tr <= rstop && smallest == 0; tr++) {
                        if (dogs[scaleIndex][current+1].at<float>(tr, tc) < mid || dogs[scaleIndex][current-1].at<float>(tr, tc) < mid) {
                            smallest = 1;
                        }
                    }
                }
            }

            if (largest == 0 || smallest == 0) {
                // Section 4 & 4.1
                // if (checkExtrema(i, j, current) ||
                //     eliminateEdgeResponse(i, j, current)) {
                //     continue;
                // }

                iter = extremas.find(make_pair(i, j));
                if (iter == extremas.end()) {
                    Extrema extrema;
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