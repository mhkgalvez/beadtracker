#ifndef VIDEOISTREAM_HPP
#define	VIDEOISTREAM_HPP

// C++ Includes
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <typeinfo>
#include <mutex>

// C and Unix
#include <pthread.h>

// Third-part Includes
#include <opencv2/opencv.hpp>
#include "ffmpeg.hpp"

// Local Includes
#include "ReadException.hpp"
#include "OpenVideoException.hpp"
#include "GeneralException.hpp"

extern std::mutex mtx_singleton; // Mutex for controlling singleton thread-safety

typedef struct _video_info {
    double fps;
    double duration;
    int frameCount;
} VideoInformation;

class VideoStream {
private:
    static VideoStream *singleton; // Store the single intance of the class
    
    bool openSuccess;       // Says to reading functions if open() was completely executed
    int currFrame;          // Store the current frame for the purpose of streaming reading
    VideoInformation* vinfo;// Store information on the open video
    bool save;
    bool has_next;
    
    // FFMPEG variables
    AVFormatContext     *format_ctx;
    int                 v_stream_index;
    AVCodec             *codec;
    AVCodecContext      *codec_ctx;
    AVFrame             *frame;
    AVFrame             *frame_rgb;
    AVPacket            packet;
    uint8_t             *raw_data;
    struct SwsContext   *sws_ctx;
    
    // Constructors and Destructors
    VideoStream();
    VideoStream(VideoStream const&); // Avoid creating instances by = calls
    void operator=(VideoStream const&); // Avoid creating instances by = calls
    virtual ~VideoStream(void);

    void init();                  //Init third-part libraries variables and structures
    std::string save2PPM(std::string path, AVFrame *frame, int width, int height, int iframe);

public:
    std::string pathPrefix;
    
    // Creation ad deletion
    static VideoStream& load(); // Load the singleton and return a reference to the object
    void release();                      // Free memory space. Causes a new instance to be created by a subsequent call of load()

    // File dealing functions
    void open(std::string filePath);          // Open the video file. Return false in case of failure and true otherwise
    void close();		// Close the current open file. Return false in case of failure and true otherwise 

    // Frame dealing functions
    bool operator>>(cv::Mat& opencvFrame);      // Read frame in a stream
    VideoStream& operator>>(bool saveToPPM);
    
    // Getters
    int frame_count();
    double fps();
    double duration();
    int next_frame_id();
    bool has_next_frame();
};

uint8_t* rgb2bgr(uint8_t* data, int area);

#endif