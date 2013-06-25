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

class Frame {
private:
    int _id;
    std::vector<cv::Vec3f> _circles;
    cv::Mat _data;
public:
    Frame();
    Frame(int id, std::vector<cv::Vec3f> circles, cv::Mat data);
    Frame(const Frame& orig);
    virtual ~Frame();
    
    int id() const;
    std::vector<cv::Vec3f> circles() const;
    cv::Mat data() const;

};

#endif	/* FRAME_HPP */

