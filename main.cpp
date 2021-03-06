// C++ Includes
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <queue>
#include <ctime>

// C and Unix Includes
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

// Local Includes
#include "common.hpp"


using namespace cv;
using namespace std;

int main(int argc, char** argv2) {	
    struct timeval start, end;
    stringstream stream;
    
    /*char* str = "XPTO";
    cout << str << endl;
    cout << *str << endl;
    cout << setiosflags(ios::hex) << (int*)str << endl;*/
    
    argc = 7;
    string argv[7];
    argv[0] = "beadtracker";
    argv[1] = "/home/matheus/Videos/BeadTracker/cell2.avi";
    //argv[1] = "/media/Toaster05/6-02-2011 pair 1 pure protein.avi";
    argv[2] = "17.6";
    argv[3] = "325";
    argv[4] = "297";
    argv[5] = "724";
    argv[6] = "490";
    
    // Checking command line arguments
    if (argc != 3 and argc != 7) {
        sendMessage(string("You must call beadtracker with at ") + 
                string("least 3 command line arguments."));
        return -1;
    }
    // If we got region dimensions
    Rect region;
    if (argc == 7) {
        for (int i = 3; i < 7; i++) {
            stream << argv[i] << endl;
        }
        stream >> region.x; 
        stream >> region.y;
        stream >> region.width;
        stream >> region.height;
        stream.str(""); // Clean stream
    }
    
    // Getting file
    video_path = argv[1];
    
    // Getting scale
    stream << argv[2];
    stream >> scale;
    sendMessage(stream.str());
    if (argc == 3) {
        region = getregion(); 
    }
    show = true;
    // Goes into tracking routine only if getregion() succeeded in its task
    if (succeeded) {
        gettimeofday(&start, NULL);
        bead_detection(region);
        gettimeofday(&end, NULL);
        sendMessage("Single thread version execution time: " 
                + time2str(diff(end, start)));
    }
    return 0;
}


/*
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
            //if (waitKey(1000/30) != -1) {
                //mod3_down = true;
                //break;
            //}
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
*/