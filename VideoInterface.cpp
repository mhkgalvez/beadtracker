#include <fstream>

#include "VideoInterface.h"
#include "GeneralException.hpp"

using namespace cv;
using namespace std;

VideoInterface *VideoInterface::singleton = NULL;

VideoInterface::VideoInterface() {	
    thirdPartInit();
    
    // Initialize prefix for relative paths
    pathPrefix = "/home/matheus/Videos/BeadTracker";
    
    // Initialize vinfo once and for all
    vinfo = new VideoInformation;   
    
    // Say that reading is not finished yet
    finishReading = false;
    
    // Register all CODECs for later use
    av_register_all();     
}

VideoInterface::~VideoInterface(void) {
    if (openSuccess) close();
    
    
    delete vinfo;
}

void VideoInterface::thirdPartInit() {
    currFrame = 0;
    openSuccess = false;
    //vinfo = NULL;
    save = false;
    
    // FFMPEG
    formatCtx = NULL;
    vStreamIndex = -1;
    codec = NULL;
    codecCtx = NULL;
    frame = NULL;
    frameRGB = NULL;
    rawData = NULL;     
}

// Singleton implementation can be thread safe. Must test.
// This function initializes the single instance of the class and return a pointer to it. 
VideoInterface& VideoInterface::load() {
	if (singleton == NULL) {
		singleton = new VideoInterface();
	}
    
	return *singleton;
}

void VideoInterface::release() {
	delete singleton;
}

void VideoInterface::open(string filePath) {
    // Check if it is already open
    if (openSuccess) {
        throw GeneralException("There is an already open video in this interface.");
    }
    
    // Open video file
    if (avformat_open_input(&formatCtx, filePath.c_str(), NULL, NULL) != 0)  {
        throw OpenVideoException("Video could not be open. Path: " + filePath);     
    }
    
    //Retrieve stream information
    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        throw OpenVideoException("Could not ind stream information for the given video.");
    }
    
    // Find first video stream
    vStreamIndex = -1;
    for (uint i = 0; i < formatCtx->nb_streams; ++i) {
        if (formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vStreamIndex = i;
            break;
        }
    }
    if (vStreamIndex == -1) {
        throw OpenVideoException("Could not find a video stream.");
    }
    
    // Get a pointer to the codec context for the video stream
    codecCtx = formatCtx->streams[vStreamIndex]->codec;
    
    /*------------------------------*/
    // Grab useful information
    double duration = ((double) formatCtx->duration)/AV_TIME_BASE;
    double fps = 1/av_q2d(codecCtx->time_base);
    cout << "Num: " << codecCtx->time_base.num << " / Den: " << codecCtx->time_base.den << endl;
    int frameCount = duration * fps;
    /*cout << "Numerator:\t" << codecCtx->time_base.num << endl;
    cout << "Denominator:\t" << codecCtx->time_base.den << endl;
    cout << "FPS:\t\t" << fps << endl;
    cout << "Duration:\t" << duration << " secs." << endl;
    cout << "Frame count:\t" << frameCount  << endl;*/
    
    // Fill VideoInformation structure
    vinfo->duration = duration;
    vinfo->fps = fps;
    vinfo->frameCount = frameCount;
    /*------------------------------*/
    
    // Find the decoder for the video stream
    codec = avcodec_find_decoder(codecCtx->codec_id);
    if (codec == NULL) {
        throw OpenVideoException("Unsuported codec!");
    }
    
    // Open codec
    AVDictionary *options = NULL;
    if (avcodec_open2(codecCtx, codec, &options) < 0) {
        throw OpenVideoException("Could not open codec!");
    }   
    
    /*------- Prepare interface for first reading -------*/
    
    // Allocate an AVFrame structure
    frame = avcodec_alloc_frame();
    frameRGB = avcodec_alloc_frame();
    if (frameRGB == NULL or frame == NULL) {
        throw OpenVideoException("Not able to allocate frame space.");
    }
    
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, codecCtx->width, codecCtx->height);
    rawData = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    sws_ctx = sws_getContext(codecCtx->width, codecCtx->height, 
            codecCtx->pix_fmt, codecCtx->width, codecCtx->height, PIX_FMT_RGB24, 
            SWS_BILINEAR, NULL, NULL, NULL);
    
    av_dump_format(formatCtx, 0, filePath.c_str(), 0);
  
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture*) frameRGB, rawData, PIX_FMT_RGB24, 
            codecCtx->width, codecCtx->height);    
    
    openSuccess = true;
}

void VideoInterface::close() {
    // Free buffer
    av_free(rawData);
            
    // Free the RGB image
    av_free(frameRGB);

    // Free the YUV frame
    av_free(frame);
    
    // Free packet
    av_free_packet(&packet);
    
    // Close Format context
    avformat_close_input(&formatCtx);    
    
    // Free AVFormatContext and all its streams
    //avformat_free_context(formatCtx);
    
    // Close Codec context
    avcodec_close(codecCtx);
    
    // Say to the reading function to stop restart reading counting
    finishReading = true;
    
    // Reinit ffmpeg variables
    thirdPartInit();
}

string VideoInterface::saveToPPM(string path, AVFrame *frame, int width, int height, int iframe) {
    ofstream file;
    string fileName;
    
    try {
        // Open file
        fileName += path;
        fileName += "/frame";
        fileName += dynamic_cast< std::ostringstream & >( ( std::ostringstream() << std::dec << iframe ) ).str();
        fileName += ".ppm";
    }
    catch (bad_cast& ex) {
        throw GeneralException("Error while trying to dynamic cast. Message: " + string(ex.what()));
    }
    
    // Open file
    file.open(fileName.c_str());
    
    // Write on file
    if (file.is_open()) {
        // Write header
        file << "P6\n" << width << " " << height << "\n255\n";
        
        // Write pixel data
        for (int i = 0; i < height; i++) {
            file.write((const char*) (frame->data[0] + i*frame->linesize[0]), 
                    streamsize(width * 3));
        } 
    }
    else {
        throw GeneralException("PPM file could not be open to be written. File: " + fileName);
    }
    
    file.close();
    
    return fileName;
}

bool VideoInterface::operator>>(Mat& opencvFrame) {
    static int i = 0;
    // Check to see if file is open
    if (!openSuccess) {
        throw ReadException("File not open yet.");
    }
    
    if (i == vinfo->frameCount) {
        i = 0;
        return true;  // Leave in case of finish reading video frames
    }
    
    if (finishReading) {
        finishReading = false;
        i = 0;
        return false;
    }
    
    // Read frame until it is a video frame
    do {
        av_read_frame(formatCtx, &packet);
    } while (packet.stream_index != vStreamIndex);
    
    int frameFinished = 0;
    
    // Is this a packet from the video stream?
    if (packet.stream_index == vStreamIndex) {
        while (!frameFinished) {
            // Decode video frame
            avcodec_decode_video2(codecCtx, frame, &frameFinished, &packet);
        }
        string name;
        // Convert the image from its native format to RGB
        sws_scale(sws_ctx, (uint8_t const * const *) frame->data, frame->linesize, 
                0, codecCtx->height, frameRGB->data, frameRGB->linesize);
        if (save) name = saveToPPM("/home/matheus/Videos/BeadTracker/frames", 
                frameRGB, codecCtx->width, codecCtx->height, i);
        i++;

        Size size;
        size.height = codecCtx->height;
        size.width = codecCtx->width; 
         
        opencvFrame = Mat(size, CV_8UC3, rgbTobgr(rawData, size.area() * 3), 0);
                
        //GaussianBlur(opencvFrame2, out, Size(5, 5), 0, 0);
        //imwrite(file, out);
        
        // Free the RGB image
        av_freep(&frameRGB);
	  
        // Free the YUV frame
        av_freep(&frame);
        av_free_packet(&packet);

        // Reallocate frames and attach rawData again to frameRGB
    	frame = avcodec_alloc_frame();
    	frameRGB = avcodec_alloc_frame();
        if (frameRGB == NULL or frame == NULL) {
            throw OpenVideoException("Not able to allocate frame space.");
        }
        
    	avpicture_fill((AVPicture *) frameRGB, rawData, PIX_FMT_RGB24, codecCtx->width, 
                codecCtx->height);   
    }
    return false;
}

VideoInterface& VideoInterface::operator>>(bool saveToPPM) {
    this->save = saveToPPM;
    return (*this);
}

int VideoInterface::getFrameCount() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->frameCount;
}

double VideoInterface::getFPS() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->fps;
}

double VideoInterface::getDuration() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->duration;
}

inline int VideoInterface::getCurrentFrame() {
    return currFrame;
}

/* Convert an image data array in RGB format to the BGR OpenCV format. You should pass the pointer
 * to the vector as argument as well as the total amount of pixels.
 * The function returns the pointer passed as argument. 
 */
uint8_t* rgbTobgr(uint8_t* data, int area) {
    for (int i = 0; i < area; i+=3) {
        uint8_t red, blue;
        
        red = data[i];
        blue = data[i + 2];
        
        data[i] = blue;
        data[i + 2] = red;
    }
    return data;
}