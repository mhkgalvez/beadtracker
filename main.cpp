// C++ Includes
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>

// C and Unix Includes
#include <unistd.h>
#include <time.h>
#include <pwd.h>

// Third-part libraries Includes

#include <opencv2/opencv.hpp>
#include "ffmpeg.hpp"

// Local Includes
#include "VideoStream.h"
#include "OpenVideoException.hpp"

using namespace cv;
using namespace std;

const string resPath = "/home/matheus/Videos/BeadTracker/";

int main(int argc, char** argv) {	
    string videoPath, frames_str;
    
    cout << "Type a video name: ";
    getline(cin, videoPath);
    //videoPath = "out2.avi";
    
    string newDir = "/home/matheus/Videos/BeadTracker/frames";
    if (chdir(newDir.c_str()) != 0) {
        throw GeneralException("Error on Unix API. Function: chdir(char*)");
    }  
    system("rm *");
    
    namedWindow("Movie");
    
    videoPath = resPath + videoPath;
    VideoStream& vi = VideoStream::load();
    Mat frame;
    try {         
        vi >> false; // Set save mode
        for (int i = 0; i < 1; i++) {
            vi.open(videoPath);
            
            // Reading frames
            for (int j = 0; j < vi.frameCount() /*&& j < 50*/; j++) {
                 vi >> frame;
                 imshow("Movie", frame);
                 if (waitKey(1000/vi.fps()) != -1) break;
                 cout << vi.currentFrame() << endl;
            }
            cout << endl;
            vi.close();
        }
    }
    catch (std::runtime_error ex) {
        cout << ex.what() << endl;
        return -1;
    }
    
    return 0;
}
