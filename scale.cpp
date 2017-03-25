#ifdef _SCALE_H

/*Draws circles on to the original gray scale image, scale_size should be size of
which the image was subsampled at */
void extremaMapper(map< pair<int,int>, pair<int,int> >* extrema_table, Mat& image, int scale_size){

  int x_cor, y_cor;

  if( scale_size <= 0 ){
    scale_size = 1;
  }

  for(map< pair<int,int>, pair<int,int> >::iterator iter=extrema->begin(); iter != extrema->end(); iter++){
    //cout << "(" << extrema[x] << "," << extrema[x+1] << ") "; 
    scale_size = iter->second.second;
    x_cor = scale_size*(iter->first.first);
    y_cor = scale_size*(iter->first.second);

    circle(image, Point(x_cor, y_cor), 2*scale_size, Scalar(0,0,255));
  }
}

/*Takes the middle array (input_arr) and compares its pixel to its own 
  8 neighbors and the 9 neighbors in top and bottom arrays.
  Precondition: all arrays must be the same size*/
void neighbors(Mat& input_arr, const Mat& top_arr, const Mat& btm_arr, 
               map< pair<int,int>, pair<int,int> >* extrema_table
               int scale_size ){

  int mid, largest, smallest; //if there exists larger value larger == 1
  int cstart, cstop, rstart, rstop;
  map< pair<int,int>, pair<int,int> >::iterator iter;

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

      for(int col=cstart; col <= cstop; col++){
        for(int row=rstart; row <= rstop; row++){
          //check 8 neighbors
          //do not evaluate again if at row col of mid value
          if ( (col != i) || (row != j) ){
            if( largest != 1 && input_arr.at<uchar>(row,col) >= mid ){
              largest=1;
            }
            if( smallest != 1 && input_arr.at<uchar>(row,col) <= mid ){
              smallest=1;
            }
          }
        }
      }

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

        iter = map2.find(make_pair(i,j));
        if( iter != extrema_table.end() ){
          map2[make_pair(i,j)] = make_pair(mid,scale_size);
        }
        else{
          iter->second.first = mid;
          iter->second.second = scale_size;
        }
      }

    }
  }
}

/*Calculates the difference of Gaussian for 3 sigmas*/
void differenceOfGaussian(Mat& gray_img, Mat& diff_img1, 
                          Mat& diff_img2, Mat& diff_img3, 
                          const int sigma){

  if( !gray_img.data ){
    cout << "Cannot compute difference of gaussian with no image" << endl;
    return;
  }

  int sig_factor = sigma;

  Mat sigma_img1 = Mat::zeros(gray_img.size(), gray_img.type()); 
  Mat sigma_img2 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat sigma_img3 = Mat::zeros(gray_img.size(), gray_img.type());
  Mat sigma_img4 = Mat::zeros(gray_img.size(), gray_img.type()); //holds the images with gaussian filter

  GaussianBlur(  gray_img, sigma_img1, Size(3,3), sig_factor);

  sig_factor = K_FACTOR*sig_factor;
  GaussianBlur(sigma_img1, sigma_img2, Size(3,3), sig_factor);

  sig_factor = K_FACTOR*sig_factor;
  GaussianBlur(sigma_img2, sigma_img3, Size(3,3), sig_factor);

  sig_factor = K_FACTOR*sig_factor;
  GaussianBlur(sigma_img3, sigma_img4, Size(3,3), sig_factor);

  subtract(sigma_img2, sigma_img1, diff_img1);
  subtract(sigma_img2, sigma_img1, diff_img2);
  subtract(sigma_img4, sigma_img3, diff_img3);

}



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


#endif