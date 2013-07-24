/* 
 * File:   common.hpp
 * Author: matheus
 *
 * Created on July 22, 2013, 10:51 AM
 */

#ifndef COMMON_HPP
#define	COMMON_HPP

// C++ and C++ Third Part libraries
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cmath>

// C and Unix
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

// Local Includes
#include "VideoStream.hpp"
#include "OpenVideoException.hpp"
#include "Frame.hpp"

extern const int ARROW_LEFT;
extern const int ARROW_RIGHT;
extern const int ARROW_UP;
extern const int ARROW_DOWN;
extern const int ESC_KEY;
extern const int PAUSE_BREAK;
extern const int ENTER_KEY;
extern const int ENTER_KEY_RIGHT;
extern const int RIGHT_PLUS;
extern const int RIGHT_MINUS;
extern const int RIGHT_TIMES;
extern const int RIGHT_BAR;
extern const int DOT_KEY;
extern const int SLASH_KEY;
extern const int PAGE_UP;
extern const int PAGE_DOWN;

extern bool show;
extern double scale;
extern bool succeeded;
extern int fps;

cv::Rect getregion(std::string path);
void bead_detection(std::string video_path, cv::Rect rect);
double distance(cv::Point& a, cv::Point& b);
std::vector<int> bead_tracking(std::vector<cv::Point> circles);
double diff(struct timeval t1, struct timeval t2);
std::string time2str(long long milliseconds);

extern const char* CLOSE_MESSAGE;
void sendMessage(cv::string message);
void sendMessage(const char* message);


#endif	/* COMMON_HPP */

