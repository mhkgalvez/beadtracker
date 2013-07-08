#include <fstream>

#include "VideoStream.hpp"
#include "GeneralException.hpp"

using namespace cv;
using namespace std;

mutex mtx_singleton; // Mutex for controlling singleton thread-safety

VideoStream *VideoStream::singleton = NULL; // Singleton instance

/*
 * Default constructor. Initialize necessary variabes as well as register codecs.
 */
VideoStream::VideoStream() {	
    init();   
    
    // Initialize prefix for relative paths
    pathPrefix = "/home/matheus/Videos/BeadTracker";
    
    // Initialize vinfo once and for all
    vinfo = new VideoInformation;   
    
    // Register all CODECs for later use
    av_register_all();     
}

/*
 * Destructor. If there is an open video, the function closes it. Free all 
 * dynamically allocated memory.
 */
VideoStream::~VideoStream(void) {
    if (openSuccess) close();
    
    
    delete vinfo;
}

/*
 * Some recurrent initializations that need to be repeated each time the user 
 * closes and open a video again. 
 */
void VideoStream::init() {
    currFrame = 0;
    openSuccess = false;
    has_next = true;
    
    //vinfo = NULL;
    save = false;
    
    // FFMPEG
    format_ctx = NULL;
    v_stream_index = -1;
    codec = NULL;
    codec_ctx = NULL;
    frame = NULL;
    frame_rgb = NULL;
    raw_data = NULL;     
}

/* Singleton implementation can be thread safe. Must test.
 * This function initializes the single instance of the class 
 * and returns a pointer to it.
 */  
VideoStream& VideoStream::load() {
    mtx_singleton.lock();
	if (singleton == NULL) {
		singleton = new VideoStream();
	}
    mtx_singleton.unlock();
	return *singleton;
}

/*
 * This static function frees the memory occupied by the singleton instance by
 * calling its destructor.
 */
void VideoStream::release() {
	delete singleton;
}

/*
 * This function receives a file name as argument and opens the video file, 
 * as well as loads all the variables related to it.
 */
void VideoStream::open(string filePath) {;
    // Check if it is already open
    if (openSuccess) {
        throw GeneralException("There is an already open video in this interface.");
    }
    
    // Open video file
    if (avformat_open_input(&format_ctx, filePath.c_str(), NULL, NULL) != 0)  {
        throw OpenVideoException("Video could not be open. Path: " + filePath);     
    }
    
    //Retrieve stream information
    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        throw OpenVideoException("Could not ind stream information for the given video.");
    }
    
    // Find first video stream
    v_stream_index = -1;
    for (uint i = 0; i < format_ctx->nb_streams; ++i) {
        if (format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            v_stream_index = i;
            break;
        }
    }
    if (v_stream_index == -1) {
        throw OpenVideoException("Could not find a video stream.");
    }
    
    // Get a pointer to the codec context for the video stream
    codec_ctx = format_ctx->streams[v_stream_index]->codec;
    
    /*------------------------------*/
    // Grab useful information
    double duration = ((double) format_ctx->duration)/AV_TIME_BASE;
    double fps = 1/av_q2d(codec_ctx->time_base);
    //cout << "Num: " << codecCtx->time_base.num << " / Den: " << codecCtx->time_base.den << endl;
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
    codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL) {
        throw OpenVideoException("Unsuported codec!");
    }
    
    // Open codec
    AVDictionary *options = NULL;
    if (avcodec_open2(codec_ctx, codec, &options) < 0) {
        throw OpenVideoException("Could not open codec!");
    }   
    
    /*------- Prepare interface for first reading -------*/
    
    // Allocate an AVFrame structure
    frame = avcodec_alloc_frame();
    frame_rgb = avcodec_alloc_frame();
    if (frame_rgb == NULL or frame == NULL) {
        throw OpenVideoException("Not able to allocate frame space.");
    }
    
    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(PIX_FMT_RGB24, codec_ctx->width, codec_ctx->height);
    raw_data = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, 
            codec_ctx->pix_fmt, codec_ctx->width, codec_ctx->height, PIX_FMT_RGB24, 
            SWS_BILINEAR, NULL, NULL, NULL);
    
    av_dump_format(format_ctx, 0, filePath.c_str(), 0);
  
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture*) frame_rgb, raw_data, PIX_FMT_RGB24, 
            codec_ctx->width, codec_ctx->height);    
    
    openSuccess = true;
}

/*
 * This function closes all ffmpeg structures and make the interface able 
 * to open another file.
 */
void VideoStream::close() {
    // Free buffer
    av_free(raw_data);
            
    // Free the RGB image
    av_free(frame_rgb);

    // Free the YUV frame
    av_free(frame);
    
    // Free packet
    av_free_packet(&packet);
    
    // Close Format context
    avformat_close_input(&format_ctx);    
    
    // Free AVFormatContext and all its streams
    //avformat_free_context(formatCtx);
    
    // Close Codec context
    avcodec_close(codec_ctx);
    
    // Reinit ffmpeg variables
    init();
}

/*
 * This function saves a frame to a ppm (raw image) file.
 */
string VideoStream::save2PPM(string path, AVFrame *frame, int width, int height, int iframe) {
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

/*
 * The extraction operator overloaded for reading purposes. You have to pass a
 * frame as argument and this frame will be loaded with the reading result.
 */
bool VideoStream::operator>>(Mat& opencvFrame) {
    // Check to see if file is open
    if (!openSuccess) {
        throw ReadException("File not open yet.");
    }
    
    // Check to see if frames have ended
    if (currFrame == vinfo->frameCount) {
        has_next = false;
        return false;  // Leave in case of finish reading video frames
    }
    if (av_read_frame(format_ctx, &packet) < 0) {
        throw ReadException("FFMPEG error. Function av_read_frame().");
    
    }
    if (packet.stream_index != v_stream_index) {
        return false;
    }
    int got_picture = 0; 
    avcodec_decode_video2(codec_ctx, frame, &got_picture, &packet);
    if (got_picture == 0) {
        return false;
    }
    // Convert the image from its native format to RGB
    sws_scale(sws_ctx, (uint8_t const * const *) frame->data, frame->linesize, 
        0, codec_ctx->height, frame_rgb->data, frame_rgb->linesize);
    // Calculate size and allocate new memory space
    Size size(codec_ctx->width, codec_ctx->height);
    uint8_t* bgr_data = rgb2bgr(raw_data, size.area() * 3);
    uint8_t* frame_data = new uint8_t[size.area() * 3];

    // Copy data to avoid segmentation fault after the ffmpeg structure 
    // is freed
    memcpy(frame_data, bgr_data, size.area() * 3);
    opencvFrame = Mat(size, CV_8UC3, frame_data, 0); 
    av_free_packet(&packet);
    currFrame++;
    /*// Free the RGB image
    av_freep(&frame_rgb);
    // Free the YUV frame
    av_freep(&frame);
    av_free_packet(&packet);
    // Reallocate frames and attach rawData again to frameRGB
    frame = avcodec_alloc_frame();
    frame_rgb = avcodec_alloc_frame();
    if (frame_rgb == NULL or frame == NULL) {
        throw OpenVideoException("Not able to allocate frame space.");
    }
    avpicture_fill((AVPicture *) frame_rgb, raw_data, PIX_FMT_RGB24, codec_ctx->width, 
            codec_ctx->height); 
    */
    return true;
}

/*
 * A polymorphic version of the extraction operator used, this time, to configure
 * the option to save the result to an image file (.ppm).
 */
VideoStream& VideoStream::operator>>(bool saveToPPM) {
    this->save = saveToPPM;
    return (*this);
}

/*
 * Return the number of frames in the video.
 */
int VideoStream::frame_count() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->frameCount;
}

/*
 * Return the frame rate (frame per seconds).
 */
double VideoStream::fps() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->fps;
}

/*
 * Return the total duration of the video.
 */
double VideoStream::duration() {
    if (!openSuccess) {
        throw GeneralException("Structure VideoInformation is not loaded. Try to load a file first.");
    }
    return vinfo->duration;
}

/*
 * Return the id of the next frame to be read by the stream.
 */
int VideoStream::next_frame_id() {
    return currFrame;
}

/* Return true if there is one more frame to read and false otherwise.
 */
bool VideoStream::has_next_frame() {
    return has_next;
}

/* Convert an image data array in RGB format to the BGR OpenCV format. You should pass the pointer
 * to the vector as argument as well as the total amount of pixels.
 * The function returns the pointer passed as argument. 
 */
uint8_t* rgb2bgr(uint8_t* data, int area) {
    for (int i = 0; i < area; i+=3) {
        uint8_t red, blue;
        
        red = data[i];
        blue = data[i + 2];
        
        data[i] = blue;
        data[i + 2] = red;
    }
    return data;
}