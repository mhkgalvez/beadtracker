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

queue< Frame > queue1;
queue< Frame > queue2;
mutex mtx_queue1, mtx_queue2;
volatile bool mod3_down = false;

void t1_routine(string video_path);
void t2_routine();
void t3_routine();
void seq_version(string video_path);

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

int main(int argc, char** argv) {	
    struct timeval start, end;
    const string video_path = "/home/matheus/Videos/BeadTracker/cell.avi";
    
    gettimeofday(&start, NULL);
    thread t1(t1_routine, video_path);
    //thread t2(t2_routine);
    thread t3(t3_routine);
    
    t1.join();
    //t2.join();
    t3.join();
    
    gettimeofday(&end, NULL);
    cout << "Threaded version execution time: " 
            << diff(end, start) << "ms\n";
    
    gettimeofday(&start, NULL);
    seq_version(video_path);
    gettimeofday(&end, NULL);
    cout << "Single thread version execution time: " 
            << diff(end, start) << "ms\n";
    
    return 0;
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
            
            //printf("Mods 1 and 2 index: %d\n", i);
            
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

void seq_version(string video_path) {
    try {
        VideoStream& video = VideoStream::load();
        Mat frame;
        video.open(video_path);
        
        /*VideoCapture video(0);
        
        video.open(video_path);
        
        if (video.isOpened() == false) {
            cout << "Smaug has come!\n";
            return;
        }
        
        namedWindow("Video");
        for (int i = 0; i < video.get(CV_CAP_PROP_FRAME_COUNT); i++) {
            video >> frame;
            Mat gray;
            vector<Vec3f> circles;
            cvtColor(frame, gray, CV_BGR2GRAY); // Convert to gray-scale
            GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                    gray.rows/8, 80, 40, 5, 30); // Detect circles
            // Draw the circles detected
            for(size_t i = 0; i < circles.size(); i++)
            {
                Point center(cvRound(circles[i][0]), 
                        cvRound(circles[i][1]));
                int radius = cvRound(circles[i][2]);
                // Circle center
                circle(frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
                // Circle outline
                circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
            }
            imshow("Video", frame); // Show image in window
            if (waitKey(1000/30) != -1) break;
        }*/
        
        namedWindow("Video");
        for (int i = 0; i < video.frame_count(); i++) {
            if (video >> frame == false) throw runtime_error("ERROR!");
            Mat gray;
            vector<Vec3f> circles;
            cvtColor(frame, gray, CV_BGR2GRAY); // Convert to gray-scale
            GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                    gray.rows/8, 80, 40, 5, 30); // Detect circles
            // Draw the circles detected
            for(size_t i = 0; i < circles.size(); i++)
            {
                Point center(cvRound(circles[i][0]), 
                        cvRound(circles[i][1]));
                int radius = cvRound(circles[i][2]);
                // Circle center
                circle(frame, center, 3, Scalar(0,255,0), -1, 8, 0 );
                // Circle outline
                circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0 );
            }
            /*imshow("Video", frame); // Show image in window
            if (waitKey(1000/48) != -1) break;*/
            
            delete[] frame.data;
            frame.data = NULL;
        }
        video.close();
    }
    catch (exception& ex) {
        cout << "Exception: "
                <<ex.what() + string("\n");     
    }
}
