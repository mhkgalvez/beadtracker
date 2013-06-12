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
    //Grabbing useful information
    double duration = ((double) formatCtx->duration)/AV_TIME_BASE;
    double fps = 1/av_q2d(codecCtx->time_base);
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

void VideoInterface::saveToPPM(string path, AVFrame *frame, int width, int height, int iframe) {
    FILE *file;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "%s/frame%d.ppm", path.c_str(), iframe);
    file=fopen(szFilename, "wb");
    if (file == NULL)
      return;

    // Write header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y = 0; y < height; y++)
      fwrite(frame->data[0]+y*frame->linesize[0], 1, width*3, file);

    // Close file
    fclose(file);
}

void VideoInterface::ffmpegToOpencv(Mat& opencv_data) {  
    cout << "Amount of pixels: " << codecCtx->height * codecCtx->width << endl;
    
    for (int i = 0; i < codecCtx->height; i++) {
        for (int j = 0; j < codecCtx->width * 3; j+=3) {
            int red, blue;
            red = i * codecCtx->height + j;
            blue = red + 2;
            
            int temp = rawData[red];
            rawData[red] = rawData[blue];
            rawData[blue] = temp;
        }
    }
    
    /*for (int i = 0; i < 12; i+=3) {
        cout << "(" << (int) rawData[i] << ", " << (int) rawData[i+1] << ", " << (int) rawData[i+2] << ")" << endl;
    }*/
    
    opencv_data = Mat(codecCtx->height, codecCtx->width, 0, rawData, sizeof(uint8_t));
    
    for (int i = 0; i < codecCtx->height; i++) {
        for (int j = 0; j < codecCtx->width * 3; j+=3) {
            int red, green, blue;
            string color;

            int index = i * codecCtx->width * 3 + j;
            
            red = opencv_data.data[index];
            green = opencv_data.data[index+1];
            blue = opencv_data.data[index+2];

            
            if (red and !green and !blue) color = " R";
            else if (!red and green and !blue) color = " G";
            else if (!red and !green and blue) color = " B";
            else color = " X";
           
            cout << color;
            //cout << "(" << (int) opencv_data.data[i] << ", " << (int) opencv_data.data[i+1] << ", " << (int) opencv_data.data[i+2] << ")" << endl;
        }
        cout << endl;
    }
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
    
    int frameFinished;
    
    // Is this a packet from the video stream?
    if (packet.stream_index == vStreamIndex) {
        // Decode video frame
        avcodec_decode_video2(codecCtx, frame, &frameFinished, &packet);
        
        // Did we get a video frame?
        if (frameFinished) {
            // Convert the image from its native format to RGB
            sws_scale(sws_ctx, (uint8_t const * const *) frame->data, frame->linesize, 
                    0, codecCtx->height, frameRGB->data, frameRGB->linesize);
            if (save) saveToPPM("/media/ubuntu/8e8531d1-6bc6-4a57-8b6c-83e757bf778c/home/matheus/Videos/frames", 
                    frameRGB, codecCtx->width, codecCtx->height, i);
            i++;
            
            // Saving to Matrix
            ffmpegToOpencv(opencvFrame);
            
            string file = pathPrefix + "/frames/file";
            stringstream sStream;
            sStream << i;
            file += sStream.str();
            file += ".jpg";
   
            Mat opencvFrame2;
            cout << "Saving image into an OpenCV format. File: " << file << endl;
            cvtColor(opencvFrame, opencvFrame2, CV_GRAY2BGR);
            imwrite(file, opencvFrame2);
        }
        
        // Free the RGB image
        av_freep(&frameRGB);
	  
        // Free the YUV frame
        av_freep(&frame);
        av_free_packet(&packet);

        // Reallocate frames and reattach rawData to frameRGB
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