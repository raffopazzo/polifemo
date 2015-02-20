#include <array>
#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "mongoose.h"

using namespace cv;
using namespace std;

array<atomic<bool>, 1> g_screens;

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
      } else if (1 == sscanf(argv[i], "--threshold=%d", &t1)) {
        threshold = t1;
      } else if (1 == sscanf(argv[i], "--calibrate=%d", &t1)) {
        calibrate = (t1 != 0);
      } else {
        cerr << "Invalid argument " << argv[i] << endl;
        throw std::runtime_error("There is some invalid argument");
      }
    }
  }
  Rect  roi{0, 0, 640, 480};
  int   blur{23};
  int   q{10};
  int   threshold{5};
  bool  calibrate{false};
};

class RestServer {

  class ServerDeleter {
  public:
    void operator()(mg_server *p) {
      mg_destroy_server(&p);
    }
  };

  static void destroy_server(mg_server *p);

  std::unique_ptr<mg_server, ServerDeleter> server;
  std::atomic<bool> run;
  std::thread       thread;

  static int event_handler(mg_connection *conn, mg_event ev) {
    if (ev == MG_AUTH) {
      return MG_TRUE;
    } else if (ev == MG_REQUEST) {
      int screen;
      if (1 == sscanf(conn->uri, "/screens/%d", &screen)) {
        if (screen < g_screens.size()) {
          mg_send_header(conn, "Content-Type", "application/json; charset=utf-8");
          mg_send_header(conn, "Access-Control-Allow-Origin", "*");
          mg_printf_data(conn, "{\"video\":%s}", g_screens[screen] ? "true" : "false");
        } else {
          mg_send_status(conn, 404);
          mg_send_header(conn, "Content-Type", "application/json; charset=utf-8");
          mg_send_header(conn, "Access-Control-Allow-Origin", "*");
          mg_printf_data(conn, "{\"error\": \"Unknown screen %d\"}", screen);
        }
      } else {
        mg_send_status(conn, 404);
        mg_send_header(conn, "Content-Type", "application/json; charset=utf-8");
        mg_send_header(conn, "Access-Control-Allow-Origin", "*");
        mg_printf_data(conn, "{\"error\": \"Invalid path\"}", screen);
      }
      return MG_TRUE;
    } else {
      return MG_FALSE;
    }
  }

  void thread_function() {
    while (run) {
      mg_poll_server(server.get(), 1000);
    }
  }

public:
  RestServer(const std::string& port) :
      run(false),
      server(mg_create_server(NULL, &RestServer::event_handler))
  {
    mg_set_option(server.get(), "listening_port", port.c_str());
  }

  ~RestServer() { if (run) stop(); }

  void start() {
    run = true;
    thread = std::thread{&RestServer::thread_function, this};
  }

  void stop() {
    run = false;
    thread.join();
  }

};

int main(int argc, char **argv) {
  config cfg{argc, argv};

  VideoCapture cap(0);

  if ( ! cap.isOpened()) {
    cerr  << "Cannot open the video file" << endl;
    return -1;
  }

  RestServer server{"8080"};
  server.start();

  namedWindow("video", CV_WINDOW_AUTOSIZE);

  Mat bw;
  do {
    Mat frame;
    if ( ! cap.read(frame)) {
      cerr << "Cannot read a frame from video file" << endl;
      return -1;
    }
    cvtColor(frame, bw, CV_BGR2GRAY);

    Mat img{blur(bw, cfg.blur)};
    if (cfg.calibrate) {
        imshow("video", mark(img, cfg.roi, 255));
    } else {
        Mat roi{edge(img), cfg.roi};
        auto q_value = q(roi, cfg.q);
        g_screens[0] = q_value >= cfg.threshold;
        imshow("video", roi);
    }
  } while (waitKey(30) < 0);

  return 0;
}

