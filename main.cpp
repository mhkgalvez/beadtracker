// C++ Includes
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <chrono>

// C and Unix Includes
#include <unistd.h>
#include <time.h>
#include <pwd.h>

// Third-part libraries Includes

#include <opencv2/opencv.hpp>
#include "ffmpeg.hpp"

// Local Includes
#include "VideoStream.hpp"
#include "OpenVideoException.hpp"

using namespace cv;
using namespace std;

const string resPath = "/home/matheus/Videos/BeadTracker/";

void filtering(Mat& frame, Mat& output) {
    
}

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
        clock_t startTime = clock();
        
        if (startTime == -1) {
            cout << "Execution time could not be measured. Processor was unable to return clock time." << endl;            
        }
        
        vi >> false; // Set save mode
        vi.open(videoPath);
        
        cout << "Duration: " << vi.duration() << endl;
        cout << "Total frames: " << vi.frameCount() << endl;

        vector<Vec3f> circles;
        
        // Reading frames
        double percent = 0;
        long filteringTime = 0;
        for (int j = 0; j < vi.frameCount(); j++) {
            vi >> frame;
            Mat gray;

            /*if (vi.currentFrame() % 2 == 0) {                
                cout << percent << "% completed... " << endl;
                percent += 200.0/vi.frameCount();
            }*/

            // Calculate time to process all the filters
            clock_t startTimeFilters = clock();
            
            if (startTimeFilters == -1) {
                cout << "Filters' execution time could not be measured. Processor was unable to return clock time." << endl;    
            }
            
            cvtColor(frame, gray, CV_BGR2GRAY); // To gray-scale
            GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Blur    
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                    gray.rows/8, 40, 20, 5, 30); // Circular Hough
            
            if (startTimeFilters != -1) {
                filteringTime += clock() - startTimeFilters;
            }

            // Draw circles in frame
            for( size_t i = 0; i < circles.size(); i++ ) {
                Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
                int radius = cvRound(circles[i][2]);

                // Circle center
                circle(frame, center, 3, Scalar(0,255,0), -1, 8, 0 );

                // Circle outline
                circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
            }

            /*imshow("Movie", frame);
            if (waitKey(1000/vi.fps()) != -1) break;*/
        }
        vi.close();
        
        if (percent != 100) {
            cout << "100% completed." << endl;
        }
        
        if (startTime != -1) {
            cout << "Time elapsed: " << double(clock() - startTime)/CLOCKS_PER_SEC * 1000 << " milliseconds." << endl;
            cout << "Filters processing: " << double(filteringTime)/CLOCKS_PER_SEC * 1000 << " milliseconds." << endl;
        }
    }
    catch (std::runtime_error ex) {
        cout << "Exception details: " << endl;
        cout << ex.what() << endl;
        return -1;
    }
    
    return 0;
}
