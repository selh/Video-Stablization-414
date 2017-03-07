#include <stdio.h>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char** argv){


	if ( argc != 2 )
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }

    Mat template_img;
    template_img = imread( argv[1], 1 );

    if ( !template_img.data )
    {
        printf("No Template data \n");
        return -1;
    }

 
    namedWindow("Display Template", WINDOW_AUTOSIZE );
    imshow("Display Template", template_img);

    waitKey(0);

	return 0;
}