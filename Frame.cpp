/* 
 * File:   Frame.cpp
 * Author: matheus
 * 
 * Created on June 25, 2013, 3:36 PM
 */

#include "Frame.hpp"

using namespace std;
using namespace cv;

const int END_FRAME_ID = -8;

Frame::Frame() {
    this->_id = -1; // Not an initialized (valid) frame
}

Frame::Frame(bool is_end) {
    this->_id = -1; // Not an initialized (valid) frame
    if (is_end) this->_id = END_FRAME_ID;
}

Frame::Frame(int id, std::vector<cv::Vec3f> circles, cv::Mat data) {
    if (id < 0) {
        throw GeneralException("Frame id must be a non-negative number.");
    }
    
    this->_id = id;
    this->_circles = circles;
    this->_data = data;
}

Frame::Frame(const Frame& orig) {
    this->_id = orig._id;
    this->_circles = orig._circles;
    this->_data = orig._data;
}

Frame::~Frame() {
}

int Frame::id() {
    return _id;
}

vector<Vec3f> Frame::circles() {
    return _circles;
}

Mat Frame::data() {
    return _data;
}

Frame Frame::end_frame() {
    vector<Vec3f> circles;
    Mat data;
    
    Frame end(true);
    return end;
}

bool Frame::is_end_frame(Frame& frame) {
    if (frame.id() == END_FRAME_ID) return true;
    return false;
}

