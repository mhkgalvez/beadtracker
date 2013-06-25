/* 
 * File:   Frame.cpp
 * Author: matheus
 * 
 * Created on June 25, 2013, 3:36 PM
 */

#include "Frame.hpp"

using namespace std;
using namespace cv;

Frame::Frame() {
    this->_id = -1; // Not an initialized (valid) frame
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

int Frame::id() const {
    return _id;
}

vector<Vec3f> Frame::circles() const {
    return _circles;
}

Mat Frame::data() const {
    return _data;
}

