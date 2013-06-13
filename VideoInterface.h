#ifndef VIDEOINTERFACE_HPP
#define	VIDEOINTERFACE_HPP

// C++ Includes
#include <iostream>
#include <string>
#include <sstream>
#include <typeinfo>

// Third-part Includes
#include <opencv2/opencv.hpp>
#include "ffmpeg.hpp"

// Local Includes
#include "ReadException.hpp"
#include "OpenVideoException.hpp"
#include "GeneralException.hpp"

/*class Mat {
public:
    Mat(int, int, int, void*, size_t) {};
};*/

typedef struct _video_info {
    double fps;
    double duration;
    int frameCount;
} VideoInformation;

class VideoInterface {
private:
    static VideoInterface *singleton; // Store the single intance of the class
    
    bool openSuccess;       // Says to reading functions if open() was completely executed
    int currFrame;          // Store the current frame for the purpose of streaming reading
    VideoInformation* vinfo;// Store information on the open video
    bool save;
    bool finishReading;
    
    // FFMPEG variables
    AVFormatContext     *formatCtx;
    int                 vStreamIndex;
    AVCodec             *codec;
    AVCodecContext      *codecCtx;
    AVFrame             *frame;
    AVFrame             *frameRGB;
    AVPacket            packet;
    uint8_t             *rawData;
    struct SwsContext   *sws_ctx;
    
    // Constructors and Destructors
    VideoInterface();
    VideoInterface(VideoInterface const&); // Avoid creating instances by = calls
    void operator=(VideoInterface const&); // Avoid creating instances by = calls
    virtual ~VideoInterface(void);

    void thirdPartInit();                  //Init third-part libraries variables and structures
    std::string saveToPPM(std::string path, AVFrame *frame, int width, int height, int iframe);
    void writeOnPipe(std::string path, AVFrame *frame, int width, int height);
    void ffmpegToOpencv(cv::Mat& opencv_data);

public:
    std::string pathPrefix;
    
    // Creation ad deletion
    static VideoInterface& load(); // Load the singleton and return a reference to the object
    void release();                      // Free memory space. Causes a new instance to be created by a subsequent call of load()

    // File dealing functions
    void open(std::string filePath);          // Open the video file. Return false in case of failure and true otherwise
    void close();		// Close the current open file. Return false in case of failure and true otherwise 

    // Frame dealing functions
    bool operator>>(cv::Mat& opencvFrame);      // Read frame in a stream
    VideoInterface& operator>>(bool saveToPPM);
    
    // Getters
    int getFrameCount();
    double getFPS();
    double getDuration();
    int getCurrentFrame();    
};

#endif