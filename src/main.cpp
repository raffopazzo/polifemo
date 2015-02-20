#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

Mat blur(const Mat& img, int factor) {
  if (factor) {
    Mat res;
    res.create(img.size(), img.type());
    GaussianBlur(img, res, Size{factor, factor}, factor);
    return res;
  } else {
    return img;
  }
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

template<typename T>
Mat mark(Mat img, Rect_<T> rect, const uchar color = 0) {
  for (int i = 0; i < rect.height; ++i) {
    img.ptr<uchar>(rect.y + i, rect.x)[0] = color;
    img.ptr<uchar>(rect.y + i, rect.x + rect.width)[0] = color;
  }
  uchar *top_row = img.ptr<uchar>(rect.y);
  uchar *bot_row = img.ptr<uchar>(rect.y + rect.height);
  for (int i = 0; i < rect.width; ++i) {
    top_row[rect.x + i] = bot_row[rect.x + i] = color;
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

struct config {
  explicit config(int argc, char **argv) {
    for (int i=1; i<argc; ++i) {
      int t1, t2, t3, t4;
      if (4 == sscanf(argv[i], "--roi=%d,%dx%d,%d", &t1, &t2, &t3, &t4)) {
        roi.x = t1;
        roi.y = t2;
        roi.width = t3;
        roi.height = t4;
      } else if (1 == sscanf(argv[i], "--blur=%d", &t1)) {
        blur = t1;
      } else if (1 == sscanf(argv[i], "--q=%d", &t1)) {
        q = t1;
      } else if (1 == sscanf(argv[i], "--calibrate=%d", &t1)) {
        calibrate = (t1 != 0);
      } else {
        cerr << "Invalid argument " << argv[i] << endl;
        throw std::runtime_error("There is some invalid argument");
      }
    }
  }
  Rect roi{0, 0, 640, 480};
  int blur {23};
  int q {10};
  bool calibrate{false};
};

int main(int argc, char **argv) {
  config cfg{argc, argv};

  VideoCapture cap(0);

  if ( ! cap.isOpened()) {
    cout  << "Cannot open the video file" << endl;
    return -1;
  }

  namedWindow("video", CV_WINDOW_AUTOSIZE);

  Mat bw;
  do {
    Mat frame;
    if ( ! cap.read(frame)) {
      cout << "Cannot read a frame from video file" << endl;
      return -1;
    }
    cvtColor(frame, bw, CV_BGR2GRAY);

    Mat img{blur(bw, cfg.blur)};
    if (cfg.calibrate) {
        imshow("video", mark(img, cfg.roi, 255));
    } else {
        Mat roi{edge(img), cfg.roi};
        cout << "q(roi)=" << q(roi, cfg.q) << endl;
        imshow("video", roi);
    }
  } while (waitKey(30) < 0);

  return 0;
}

