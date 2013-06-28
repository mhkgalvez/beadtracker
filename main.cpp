// C++ Includes
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

// C and Unix Includes

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
const Frame END_FRAME = Frame::end_frame();

mutex mtx_queue1, mtx_queue2;

void t1_routine();
void t2_routine();
void t3_routine();

int main(int argc, char** argv) {	
    /*thread t1(t1_routine);
    thread t2(t2_routine);
    thread t3(t3_routine);
    
    t1.join();
    t2.join();
    t3.join();*/
    t1_routine();
    t2_routine();
    
    return 0;
}

void t1_routine() {   
    bool locked = false;
    Mat *frame;
    try {
        // Load stream
        VideoStream& vstream = VideoStream::load();
        
        // Open video
        vstream.open("/home/matheus/Videos/BeadTracker/drop.avi");

        // Read frames
        for (int i = 0; i <= vstream.frame_count(); i++) {
            frame = new Mat();
            vstream >> (*frame);
            //return frame;
            
            Frame wrapper(vstream.next_frame_id(), vector<Vec3f>(), frame);
            
            // Critical section
            //mtx_queue1.lock();
            locked = true;
            
            if (i < vstream.frame_count()) queue1.push(wrapper);
            else queue1.push(END_FRAME);
            
            locked = false;
            //mtx_queue1.unlock();
            // End of critical section
        }
        
        // Close video
        vstream.close();
    }
    catch (std::exception& ex) {
        cout << ex.what() + string("\n");
        
        if (locked) {
            mtx_queue1.unlock();
        }
    }
    cout << "Sai, CARAMBAAA!\n";
}

void t2_routine() {
    bool locked = false;
    try {
        Frame frame; 
        
        while (true) {
            //cout << i++ << endl;
            
            // Begin of mutual exclusion region
            //mtx_queue1.lock();
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
                cout << "end\n";
                locked = false;
                mtx_queue1.unlock();
                break;
            }
            
            locked = false;
            //mtx_queue1.unlock();
            // End of mutual exclusion region
            
            Mat gray, *src = frame.data();

            cvtColor(*src, gray, CV_BGR2GRAY);
            GaussianBlur(gray, gray, Size(9, 9), 2, 2);

            vector<Vec3f> circles;
            HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, gray.rows/8, 80, 40, 5, 30);

            //Frame out(frame.id(), circles, gray);
        }
    }
    catch (exception& ex) {
        if (typeid(ex) != typeid(runtime_error())) {
            cout << "System threw a general exception.\n";
        }
        else {
            cout << ex.what() + string("\n");
        }
        
        // Unlock if locked
        if (locked) {
            mtx_queue1.unlock();
        }
    }   
}

void t3_routine() {
    bool locked = false;
    try {
        
    }
    catch (std::exception& ex) {
        cout << ex.what() + string("\n");
        
        if (locked) {
            //mtx_queue1.unlock();
        }
    }     
}
