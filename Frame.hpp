/* 
 * File:   Frame.hpp
 * Author: matheus
 *
 * Created on June 25, 2013, 3:36 PM
 */

#ifndef FRAME_HPP
#define	FRAME_HPP

// Third-part includes
#include <opencv2/opencv.hpp>

// C++ includes
#include <vector>

// Local includes
#include "GeneralException.hpp"

extern const int END_FRAME_ID;

class Frame {
private:
    int _id;
    std::vector<cv::Vec3f> _circles;
    cv::Mat* _data;
public:
    Frame();
    Frame(bool is_end);
    Frame(int id, std::vector<cv::Vec3f> circles, cv::Mat* data);
    Frame(const Frame& orig);
    virtual ~Frame();
    
    int id();
    std::vector<cv::Vec3f> circles();
    cv::Mat* data();
    
    static Frame end_frame();
    static bool is_end_frame(Frame&);

};

#endif	/* FRAME_HPP */

