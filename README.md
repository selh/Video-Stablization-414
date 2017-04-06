Video Stabilization
===

Video stabilization project for CMPT 414. 
Uses our own implementation of SIFT for feature matching between frames.

REQUIREMENTS:
g++ 4.9 or higher


HOW TO USE:
---
./DisplayImage <OPTIONS>

-i, --input <input video/image file>
  Provide the video file or image file you wish to run on
-j, --matchImage <input second image file>
  Provide the second image file that you wish to compare the first image to. (For image matching only)
-o, --output <output file name>
  Provide the name of which you want the output video/image to be named
-m, --mode <VS/VM/IM>
  Use mode VS for video stabilization, VM for video matching or IM for image matching
-d, --distane <distance>
  Image space distance threshold that can be used to discard outliers or false matches.
  Will default to 50 if no threshold is provided
-r, --ratio <ratio>
  Feature Space ratio threshold. Will default to 0.2 if no threshold is provided.
-h, --help
  Print out help instructions


Functions are seperated into sections under SIFT.cpp as labeled:
----
 
1.LOG APPROXIMATION - DIFFERENCE OF GAUSSIAN (line 53)
2.FIND EXTREMA BETWEEN DOG APPROXIMATIONS (line 96)
3.REMOVE NOISY EXTREMA (line 191)
4.ORIENTATION AND DESCRIPTORS (line 275)
5.FEATURE MATCHING (line 487)


