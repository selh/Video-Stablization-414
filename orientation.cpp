#ifdef _ORIENTATION_

/*Weighted circle dependent on the scale size of the point
  parameter i should be x distance between center and the point (center - x)
  parameter j should be y distance between center and the point (center - y)*/
float gaussianWeightedCircle(int i, int j, int scale){

  float exponent;

  if( scale < 0 ){ 
    scale = abs(scale);
  }

  exponent = pow(i,2) + pow(j, 2);
  exponent = pow(exponent/(scale*1.5), 2);

  //exponent = ((i^2 + j^2)/1.5*scale)^2
  return M_INV_2PI*exp((-0.5)*exponent);
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
void extremaOrientation(Mat& image, 
                       map< pair<int,int>, pair<int,int> >* extrema_table, 
                       map< pair<int, int>, vector<float> >* key_orientation){

  vector<float> histogram(36);
  vector<float> orientations;
  int x_cor, y_cor;
  int cstart, cstop, rstart, rstop;
  int  weighted, scale, max;

  for(auto iter= extrema_table->begin(); iter != extrema_table->end(); iter++){
    x_cor = iter->first.first;
    y_cor = iter->first.second;

    for(int h=0; h< histogram.size(); h++){
      histogram[h] = 0;
    }

    //get weighted values for each neigbour and add to histogram
    scale = iter->second.second;
    cstart = x_cor - scale;
    cstop  = x_cor + scale;
    rstart = y_cor - scale;
    rstop  = y_cor + scale;
    boundsCheck(image.rows, image.cols, &cstart, &cstop, &rstart, &rstop);
    //calculate variance
    for(int col = cstart; col <= cstop; col++){
      for(int row = rstart; row <= rstop; row++){
        weighted = calculateMagnitude(image, y_cor, x_cor)*gaussianWeightedCircle(x_cor-col, y_cor-row, scale);
        distrHistVals(&histogram, calculateOrientation(image, y_cor, x_cor), weighted);
      }
    }

    max = 0;
    for(int j=0; j< histogram.size(); j++){
      if ( histogram[j] > max ){
        max = histogram[j];
      }
    }

    //if( max != 0 ){
      for(int k=0; k< histogram.size(); k++){
        //cout << histogram[k] << " ";
        if( histogram[k] >= max*0.8 ){
          orientations.push_back(k*10); //orientation
        }
      }

      //insert all orientations to corresponding point
      key_orientation->insert(make_pair(make_pair(x_cor, y_cor), orientations));
      orientations.clear();
    //}
  }

}


#endif