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
    
    dump_format(formatCtx, 0, filePath.c_str(), 0);
  
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
    /*file;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "%s/temp.ppm", path.c_str());
    file=fopen(szFilename, "wb");
    if (file == NULL)
      return;

    // Write header
    fprintf(file, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y = 0; y < height; y++)
      fwrite(frame->data[0]+y*frame->linesize[0], 1, width*3, file);

    // Close file
    fclose(file);*/
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

void VideoInterface::writeOnPipe(std::string path, AVFrame* frame, int width, int height) {
    ofstream pipe;
    string pipeName;
    
    cout << "Pipe to write: " << pipeName << endl;
    pipeName += path += "/mypipe.ppm";
    
    // Open FIFO
    pipe.open(pipeName.c_str());
    
    // Write on FIFO
    if (pipe.is_open()) {
        // Write header
        pipe << "P6\n" << width << " " << height << "\n255\n";
        
        // Write pixel data
        for (int i = 0; i < height; i++) {
            pipe.write((const char*) (frame->data[0] + i*frame->linesize[0]), 
                    streamsize(width * 3));
        }
    }
    else {
        throw GeneralException("Named pipe could not be open to be written. File: " + pipeName);
    }    
    
    pipe.close();
    
}

void VideoInterface::ffmpegToOpencv(Mat& opencvData) {  
    static bool execute = true;
    
    if (execute) {
        cout << "Amount of pixels: " << codecCtx->height * codecCtx->width << endl;
        cout << "Frame height: " << frame->height << endl;
        cout << "Frame width: " << frame->width << endl;

        /*for (int i = 0; i < codecCtx->height; i++) {
            for (int j = 0; j < codecCtx->width * 3; j+=3) {
                int red, blue;
                red = i * codecCtx->height + j;
                blue = red + 2;

                int temp = rawData[red];
                rawData[red] = rawData[blue];
                rawData[blue] = temp;
            }
        }*/

        /*for (int i = 0; i < 12; i+=3) {
            cout << "(" << (int) rawData[i] << ", " << (int) rawData[i+1] << ", " << (int) rawData[i+2] << ")" << endl;
        }*/

        opencvData = Mat(codecCtx->height, codecCtx->width, 0, rawData, sizeof(uint8_t));


        uint8_t *p0 = frameRGB->data[0];
        for (int y = 0; y < frame->height; y++) {
            uint8_t *p = p0;
            for (int x = 0; x < frame->width * 3; x+=3) {
                int red, green, blue;
                string color;

                red = p[x];
                green = p[x+1];
                blue = p[x+2];


                if (red and !green and !blue) color = " R";
                else if (!red and green and !blue) color = " G";
                else if (!red and !green and blue) color = " B";
                else color = " X";

                //cout << "[" << green << "] ";             
                cout << color;
            }
            cout << endl;
            p0 += frame->linesize[0];
        }       
        
        /*for (int i = 0; i < codecCtx->height; i++) {
            cout << "Line " << i << " size: " << frameRGB->linesize[i] << " bytes." << endl;
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
        }*/
        execute = false;
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
    
    int frameFinished = 0;
    
    // Is this a packet from the video stream?
    if (packet.stream_index == vStreamIndex) {
        
        while (!frameFinished) {
            // Decode video frame
            avcodec_decode_video2(codecCtx, frame, &frameFinished, &packet);
        }
        
        // Convert the image from its native format to RGB
        sws_scale(sws_ctx, (uint8_t const * const *) frame->data, frame->linesize, 
                0, codecCtx->height, frameRGB->data, frameRGB->linesize);
        if (save) saveToPPM("/home/matheus/Videos/BeadTracker/frames", 
                frameRGB, codecCtx->width, codecCtx->height, i);
        i++;

        // Saving to Matrix
        /*ffmpegToOpencv(opencvFrame);
        
        string name = saveToPPM("/home/matheus/Videos/BeadTracker/frames", frameRGB,
                codecCtx->width, codecCtx->height, 0);*/
        writeOnPipe(pathPrefix + "/frames", frameRGB, codecCtx->width, codecCtx->height);

        string file = pathPrefix + "/mypipe.ppm";
        stringstream sStream;
        sStream << i-1;
        file += sStream.str();

        Mat opencvFrame2 = imread(file, -1);
        //cvtColor(opencvFrame, opencvFrame2, CV_GRAY2BGR);
        imwrite(file, opencvFrame2);
        
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