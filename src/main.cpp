#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

Mat blur(const Mat& img) {
  Mat res;
  res.create(img.size(), img.type());
//GaussianBlur(img, res, Size_<int>{23, 23}, 5);
  GaussianBlur(img, res, Size_<int>{5, 5}, 5);
  return res;
}

Mat edge(const Mat& img) {
  Mat res;
  res.create(img.size(), img.type());
  Mat kern = (Mat_ <char>(3,3) <<  0, -1,  0,
                                  -1,  0,  1,
                                   0,  1,  0);
  filter2D(img, res, img.depth(), kern);
  return res;
}

Mat mark(Mat img) {
  for (int i=0; i<img.rows; ++i) {
    uchar *p = img.ptr<uchar>(i);
    p[i] = 255;
  }
  return img;
}

int q(Mat img, uchar th) {
  int res = 0;
  for (int i=0; i<img.rows; ++i) {
    uchar *p = img.ptr<uchar>(i);
    for (int j=0; j<img.cols; ++j) {
      if (p[j] >= th) ++res;
    }
  }
  cout << "res="<< res << endl;
  cout << "N="<< (img.rows*img.cols) << endl;
  return (res*100)/(img.rows*img.cols);
}

int main(int argc, char **argv) {
  if (argc == 2) {
    cout << " Usage: display_image ImageToLoadAndDisplay" << endl;

    Mat image { imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE) };   // Read the file

    if (!image.data) {                              // Check for invalid input
      cout <<  "Could not open or find the image" << std::endl;
      return -1;
    }

    namedWindow("image", WINDOW_NORMAL); // Create a window for display.
    imshow("image", image);                   // Show our image inside it.

    namedWindow("blur(image)", WINDOW_NORMAL); // Create a window for display.
    imshow("blur(image)", blur(image));                   // Show our image inside it.

    namedWindow("edge(blur(image))", WINDOW_NORMAL); // Create a window for display.
    imshow("edge(blur(image))", edge(blur(image)));                   // Show our image inside it.

    cout << "q(e)=" << q(blur(image), 255/4) << endl;
    waitKey(0);                                          // Wait for a keystroke in the window
  } else {
    VideoCapture cap(0); // open the video camera no. 0

    if (!cap.isOpened())  // if not success, exit program
    {
      cout  << "Cannot open the video file" << endl;
      return -1;
    }

    double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH); //get the width of frames of the video
    double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT); //get the height of frames of the  video
    cout << "Frame size : " << dWidth << " x " << dHeight << endl;
    namedWindow("MyVideo",CV_WINDOW_AUTOSIZE); //create a window called "MyVideo"

    while (true) {
      Mat frame;
      if (! cap.read(frame)) //if not success, break loop
      {
        cout << "Cannot read a frame from video file" << endl;
        return -1;
      }

      imshow("MyVideo", frame); //show the frame in "MyVideo" window
      if (waitKey(30) > 0) 
      {
        cout << "esc key is pressed by user" << endl;
        break; 
      }
    }
  }

  return 0;
}

