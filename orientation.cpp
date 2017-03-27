#ifdef _ORIENTATION_


// /*Fix for loop bounds if out of bounds */
// void boundsCheck(int arr_row, int arr_col, 
//                  int* cstart, int* cstop, 
//                  int* rstart, int* rstop ){
  
//   if( *cstart < 0 ){
//     *cstart = 0;
//   }
//   if( *cstop > arr_col-1 ){
//     *cstop = arr_col-1;
//   }

//   if( *rstart < 0 ){
//     *rstart = 0;
//   }
//   if( *rstop > arr_row-1 ){
//     *rstop = arr_row-1;
//   }

// }

/*TODO: Weighted circle dependent on the scale size of the point*/
float gaussianWeightedCircle(Mat& image, int x, int y){

  vector<int> region;
  int value=0, num_vals=0;
  float mean =0, variance = 0, weight = 0, exponent = 0;
  int cstart, cstop, rstart, rstop;

  int center = image.at<uchar>(y,x);

  cstart = x - 2;
  cstop  = x + 2;
  rstart = y - 2;
  rstop  = y + 2;
  boundsCheck(image.rows, image.cols, &cstart, &cstop, &rstart, &rstop);
  //calculate variance
  for(int col = cstart; col <= cstop; col++){
    for(int row = rstart; row <= rstop; row++){
      value = image.at<uchar>(row, col);
      region.push_back(value);
      mean += value;
      num_vals++;
    }
  }

  mean = mean/num_vals;

  for(int i=0; i< region.size(); i++){
    variance = pow( (region[i] - mean), 2 );
  }

  variance = variance/(num_vals - 1);

  exponent = pow(M_E,(-0.5)*pow( (center - mean)/(variance), 2 ));

  weight = (1/(3*M_PI))*exponent;
  //sigma 1.5*scale size
  return weight;
}

/* Takes angles in degrees */
void distrHistVals(vector<float>* histogram, float angle, float weighted){
  int index = 0;

  if( angle >= 0 && angle <= 10 ){
    (*histogram)[0] = weighted;
  }
  else if ( angle > 10 && angle <= 360 ){
    index = floor(angle/10);
    (*histogram)[index] += weighted;
  }

}

/*Builds a weighted histogram based on gradient direction and gradient magnitude.*/
void buildWeightedHist(Mat& image, vector<int>* extrema, 
                       map< pair<int, int>, vector<float> >* key_orientation){

  vector<float> histogram(36);
  vector<float> orientations;
  int x_cor, y_cor, weighted;
  int max;

  for(int i=0; i< extrema->size(); i+=2){
    x_cor = (*extrema)[i];
    y_cor = (*extrema)[i+1];

    for(int h=0; h< histogram.size(); h++){
      histogram[h] = 0;
    }

    weighted = calculateMagnitude(image, x_cor, y_cor)*gaussianWeightedCircle(image, x_cor, y_cor);
    distrHistVals(&histogram, calculateOrientation(image, x_cor, y_cor), weighted);

    max = 0;
    for(int j=0; j< histogram.size(); j++){
      if ( histogram[j] > max ){
        max = histogram[j];
      }
    }

    for(int k=0; k< histogram.size(); k++){
      if( histogram[k] >= max*0.8 ){
        orientations.push_back(k); //orientation
        orientations.push_back(histogram[k]); //magnitude
      }
    }

    //insert all orientations to corresponding point
    key_orientation->insert(make_pair(make_pair(x_cor, y_cor), orientations));
    orientations.clear();
  }

}


#endif