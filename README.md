Video Stabilization
===

Video stabilization project for CMPT 414. 
Uses our own implementation of SIFT for feature matching between frames.

REQUIREMENTS:
- G++ 4.9 or higher
- CMake
- OpenCV with ffmpeg support


HOW TO USE:
---
```
./DisplayImage <OPTIONS>

-i, --input <input video/image file>
  Provide the video file or image file you wish to run on.
  
-j, --matchImage <input second image file>
  Provide the second image file that you wish to compare the first image to. (For image matching only)
  
-o, --output <output video file name>
  Provide the name of which you want the output video to be named. Extension must be .avi
  (For video-related modes only)
  
-m, --mode <VS/VM/IM>
  Use mode VS for video stabilization, VM for video matching or IM for image matching.
  
-d, --distane <distance>
  Image space distance threshold that can be used to discard outliers or false matches.
  Will default to 50 if no threshold is provided.
  
-r, --ratio <ratio>
  Feature Space ratio threshold. Will default to 0.2 if no threshold is provided.

-t, --motionthreshold <threshold>
  Threshold for derivative of motion between frames in pixels. If this threshold is exceeded,
  stabilization is skipped during that frame. Will default to 1.5 if no threshold is provided.
  
-h, --help
  Print out help instructions.
  
```

Example for running video stabilization:
```
$ cmake .
$ make
$ ./DisplayImage -m VS -i data/video/shaky_1.mp4 -o stabilized.avi
```

Functions are seperated into sections under SIFT.cpp as labeled:
----
 
1. LOG APPROXIMATION - DIFFERENCE OF GAUSSIAN (line 53)</br>
2. FIND EXTREMA BETWEEN DOG APPROXIMATIONS (line 96)</br>
3. REMOVE NOISY EXTREMA (line 191)</br>
4. ORIENTATION AND DESCRIPTORS (line 275)</br>
5. FEATURE MATCHING (line 487)</br>

Sources
---
- D. Lowe. (2005). “Distinctive Image Features from Scale-Invariant Keypoints” [Online] Available: https://www.cs.ubc.ca/~lowe/papers/ijcv04.pdf
- Blue Spotted Butterfly. [Online] Available: butterflyhousemarinecove.org
- Monarch Butterfly. [Online] Available: pngimg.com
- Deer in forest. [Online] Available: https://www.lhup.edu/~dsimanek/3d/stereo/3dgallery18.htm
- Walking in a field. [Online] Available: https://www.youtube.com/watch?v=0Sn2rXlPJZw
- Walking on a mountain. [Online] Available: https://www.youtube.com/watch?v=L2QvvP2njIs
- jarro2783 (2014). cxxopts library [Online] Available: https://github.com/jarro2783/cxxopts