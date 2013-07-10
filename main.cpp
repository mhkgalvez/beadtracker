// C++ Includes
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <ctime>

// C and Unix Includes
#include <sys/time.h>

// Third-part libraries Includes
#include <opencv2/opencv.hpp>

// Local Includes
#include "VideoStream.hpp"
#include "OpenVideoException.hpp"
#include "Frame.hpp"

using namespace cv;
using namespace std;

const int ARROW_LEFT = 1113937;
const int ARROW_RIGHT = 1113939;
const int ARROW_UP = 1113938;
const int ARROW_DOWN = 1113940;
const int ESC_KEY = 1048603;
const int PAUSE_BREAK = 1113875;
const int RIGHT_PLUS = 1114027;
const int RIGHT_MINUS = 1114029;
const int RIGHT_TIMES = 1114026;
const int RIGHT_BAR = 1114031;

queue< Frame > queue1;
queue< Frame > queue2;
mutex mtx_queue1, mtx_queue2;
volatile bool mod3_down = false;
bool show = false;

Rect getregion(string path);
    
void t1_routine(string video_path);
void t2_routine();
void t3_routine();
void seq_version(string video_path, Rect rect);


// Calculate difference between two times: (t1 - t2)
double diff(struct timeval t1, struct timeval t2) {
    double usec_diff, sec_diff = 0;
    
    usec_diff = t1.tv_usec - t2.tv_usec; // Calculate microseconds difference
    if (usec_diff < 0) { // Handle borrowing procedures
        usec_diff += 1000000;
        sec_diff--;
    }
    sec_diff = t1.tv_sec - t2.tv_sec + sec_diff; // Calculate seconds difference
    sec_diff *= 1000; // Make it milliseconds 
    usec_diff /= 1000;  
    
    return usec_diff + sec_diff; // Return the number of milliseconds
}

string time2str(long long miliseconds) {
    int msecs = miliseconds % 1000;
    int secs = miliseconds/1000;
    int mins = secs/60;
    secs = secs % 60;
    
    stringstream time;
    time << mins << ":" << secs << ":" << msecs;
    
    return time.str();
}

int main(int argc, char** argv) {	
    struct timeval start, end;
    const string video_path = "/home/matheus/Videos/BeadTracker/cell.avi";
   
    Rect region = getregion(video_path);
 
    show = true;
    gettimeofday(&start, NULL);
    seq_version(video_path, region);
    gettimeofday(&end, NULL);
    cout << "Single thread version execution time: " 
            << time2str(diff(end, start)) << endl;    
    return 0;
}

Rect getregion(string path) {
    VideoStream& video = VideoStream::load();
    Point p1, p2;
    namedWindow("Select a region...");    
    try {
        video.open(path);
        double x, y, width, height;
        double scale;
        bool exit = false;
        x = y = width = height = 400;
        scale = 5.1;
        Mat first_frame;
        if (video >> first_frame == false) throw runtime_error("ERROR!");        
        while (!exit) {
            p1 = Point(x, y); // Create points
            p2 = Point(x + width, y + height);
            // Draw a rectangle representing the region to be processed
            Mat temp;
            first_frame.copyTo(temp); // Copy frame to a temporary space 
            rectangle(temp, p1, p2, Scalar(0, 255, 0), 5, CV_AA, 0);
            // Resize image
            Mat img;
            int _gcd = gcd<int>(temp.cols, temp.rows);
            cout << "Image resized by the following factor: " << _gcd << endl;
            resize(temp, img, Size(temp.cols/_gcd, temp.rows/_gcd)); 
            imshow("Select a region...", img); // Show image resized
            int key = waitKey(1000/48);
            switch (key) {
                case ARROW_LEFT: 
                    x -= scale;
                    break;
                case ARROW_RIGHT:
                    x += scale;
                    break;
                case ARROW_DOWN:
                    y += scale;
                    break;
                case ARROW_UP:
                    y -= scale;
                    break;
                case ESC_KEY:
                    exit = true;
                    break;
                case RIGHT_PLUS:
                    width += scale;
                    break;
                case RIGHT_MINUS:
                    width -= scale;
                    break;
                case RIGHT_TIMES:
                    height += scale;
                    break;
                case RIGHT_BAR:
                    height -= scale;
                    break;
            }
        }
        destroyWindow("Select a region...");
        video.close();
    }
    catch (exception& ex) {
        cout << "Exception: " << ex.what() + string("\n");            
    }
    return Rect(p1, p2);
}

void seq_version(string video_path, Rect rect) {
    int r1, r2, c1, c2; // Deviations from the center of beads region
    r1 = r2 = c1 = c2 = 0;
    Mat region;
    bool paused = false;
    try {
        VideoStream& video = VideoStream::load();
        Mat frame;
        video.open(video_path);
        
        if (show) namedWindow("Video"); // Create OpenCV window
        for (int i = 0; i < video.frame_count(); i++) {
            if (!paused) {
                if (video >> frame == false) throw runtime_error("ERROR!");

                // Getting picture region around the main beads
                region = frame.operator ()(rect);//frame.rowRange(190 + r1, 540 + r2);
                //region = region.colRange(550 + c1, 1000 + c2);
                Mat gray;               // Gray-scale image
                vector<Vec3f> circles;  // Circles (Center coordinates and radius)
                cvtColor(region, gray, CV_BGR2GRAY); // Convert to gray-scale
                //GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
                HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                        gray.rows/8, 80, 40, 5, 30); // Detect circles
                // Draw the circles detected
                for(size_t i = 0; i < circles.size(); i++)
                {
                    Point center(cvRound(circles[i][0]), 
                            cvRound(circles[i][1]));
                    int radius = cvRound(circles[i][2]);
                    // Circle center
                    circle(region, center, 3, Scalar(0,255,0), -1, 8, 0 );
                    // Circle outline
                    circle(region, center, radius, Scalar(0,0,255), 3, 8, 0 );
                }
            } 
            else {
                i--;
            }
            if (show) { 
                imshow("Video", region); // Show image in window
                int key;
                if ((key = waitKey(1000/48)) != -1) {
                    switch (key) {
                        case ARROW_LEFT: 
                            c1--;
                            break;
                        case ARROW_UP:
                            r1--;
                            break;
                        case PAUSE_BREAK:
                            paused = !paused;
                            break;
                    }
                    if (key == ESC_KEY) break;
                }
            }
            if (!paused) {
                // Free user allocated memory in Mat class
                delete[] frame.data;
                frame.data = NULL;
            }
        }
        video.close();
    }
    catch (exception& ex) {
        cout << "Exception: " << ex.what() + string("\n");     
    }
}

void t1_routine(string video_path) {   
    bool locked = false;
    Mat frame;
    try {
        // Load stream
        VideoStream& video = VideoStream::load();
        
        // Open video
        video.open(video_path);

        // Read frames
        for (int i = 0; i < video.frame_count(); i++) {
            video >> frame;
            
            Mat gray;
            vector<Vec3f> circles;
            cvtColor(frame, gray, CV_BGR2GRAY); // Convert to gray-scale
            GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                    gray.rows/8, 80, 40, 5, 30); // Detect circles
            Frame out(i, circles, frame); // Create frame to 
                            // send to next module
            
            // Begin of mutual exclusion region
            mtx_queue2.lock();
            queue2.push(out); 
            mtx_queue2.unlock();
            // End of mutual exclusion region
          
            
            if (mod3_down) break;
        }

        // Begin of mutual exclusion region
        mtx_queue2.lock();
        queue2.push(Frame::end_frame()); 
        mtx_queue2.unlock();
        // End of mutual exclusion region        
        
        // Close video
        video.close();
    }
    catch (std::exception& ex) {
        if (typeid(ex) != typeid(runtime_error())) {
            cout << "System threw a general exception.\n";
        }
        else {
            cout << ex.what() + string("\n");
        }
        if (locked) {
            mtx_queue1.unlock();
        }
    }
    //cout << "Modulo 1 closed.\n";
}

void t2_routine() {
    bool locked = false;
    try {
        Frame frame;        
        while (true) {
            // Begin of mutual exclusion region
            mtx_queue1.lock();
            locked = true;
            
            if (queue1.empty()) {
                locked = false;
                mtx_queue1.unlock();
                continue;
            }
            // Pop frame from stack
            frame = queue1.front();
            queue1.pop();
            if (Frame::is_end_frame(frame)) {
                locked = false;
                mtx_queue1.unlock();
                break;
            }
            
            locked = false;
            mtx_queue1.unlock();
            // End of mutual exclusion region
            
            Mat gray;
            vector<Vec3f> circles;
            cvtColor(frame.data(), gray, CV_BGR2GRAY); // Convert to gray-scale
            GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                    gray.rows/8, 80, 40, 5, 30); // Detect circles
            Frame out(frame.id(), circles, frame.data()); // Create frame to 
                            // send to next module
            
            // Begin of mutual exclusion region
            mtx_queue2.lock();
            queue2.push(out); 
            mtx_queue2.unlock();
            // End of mutual exclusion region
            
            printf("Mod 2 index: %d\n", frame.id());
            
            if (mod3_down) break;
        }
        // Begin of mutual exclusion region
        mtx_queue2.lock();
        queue2.push(Frame::end_frame()); 
        mtx_queue2.unlock();
        // End of mutual exclusion region        
    }
    catch (exception& ex) {
        if (typeid(ex) != typeid(runtime_error())) {
            cout << "System threw a general exception.\n";
        }
        else {
            cout << ex.what() + string("\n");
        }
        if (locked) {
            mtx_queue1.unlock();
        }
    }
    cout << "Modulo 2 closed.\n";
}

void t3_routine() {
    bool locked = false;
    namedWindow("Video");
    try {
        Frame frame;
        while (true) {
            // Begin of mutual exclusion region
            mtx_queue2.lock();
            locked = true;

            if (queue2.empty()) {
                locked = false;
                mtx_queue2.unlock();
                continue;
            }

            frame = queue2.front();
            queue2.pop();            
            if (frame.is_end_frame(frame)) {
                locked = false;
                mtx_queue2.unlock();
                break;
            }
            
            locked = false;
            mtx_queue2.unlock();
            // End of mutual exclusion region
            
            // Draw the circles detected
            for(size_t i = 0; i < frame.circles().size(); i++ )
            {
                Point center(cvRound(frame.circles()[i][0]), 
                        cvRound(frame.circles()[i][1]));
                int radius = cvRound(frame.circles()[i][2]);
                // Circle center
                Mat src = frame.data();
                circle(src, center, 3, Scalar(0,255,0), -1, 8, 0 );
                // Circle outline
                circle(src, center, radius, Scalar(0,0,255), 3, 8, 0 );
            }           
            imshow("Video", frame.data()); // Show image in window
            /*if (waitKey(1000/30) != -1) {
                mod3_down = true;
                break;
            }*/
            //printf("Mod 3 index: %d\n", frame.id());
            Mat cv_frame = frame.data();
            
            // Free user-allocated data in Mat object and avoid memory leaks
            if (cv_frame.refcount == NULL and !Frame::is_end_frame(frame)) {
                delete[] cv_frame.data;
                cv_frame.data = NULL;
            }
        }
    }
    catch (std::exception& ex) {
        mod3_down = true;
        if (typeid(ex) != typeid(runtime_error())) {
            cout << "System threw a general exception.\n";
        }
        else {
            cout << ex.what() + string("\n");
        }
        if (locked) {
            mtx_queue2.unlock();
        }
    }     
}
