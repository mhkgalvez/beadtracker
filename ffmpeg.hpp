/* 
 * File:   ffmpeg.hpp
 * Author: matheus
 *
 * Created on May 31, 2013, 11:13 AM
 */

#ifndef FFMPEG_HPP
#define	FFMPEG_HPP

#ifndef UINT64_C 
    #define UINT64_C long
#endif
extern "C" { 
    #include <libavformat/avformat.h> 
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
} 

#endif