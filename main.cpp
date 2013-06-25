// C++ Includes
#include <iostream>
#include <thread>
#include <stack>

// C and Unix Includes
#include <semaphore.h>

// Third-part libraries Includes

#include <opencv2/opencv.hpp>

// Local Includes
#include "VideoStream.hpp"
#include "OpenVideoException.hpp"
#include "Frame.hpp"

using namespace cv;
using namespace std;

stack< Frame > stack1;
volatile stack< Frame > stack2;

sem_t sem1, sem2;

void t1_routine();
void t2_routine();
void t3_routine();

int main(int argc, char** argv) {	
    thread t1(t1_routine);
    thread t2(t2_routine);
    thread t3(t3_routine);
    
    // Init semaphores
    sem_init(&sem1, 0, 1);
    
    t1.join();
    t2.join();
    t3.join();
    
    return 0;
}

void t1_routine() {    
    try {
        // Load stream
        VideoStream& vstream = VideoStream::load();
        
        // Open video
        vstream.open("/home/matheus/Videos/BeadTracker/cell.avi");
        
        // Read frames
        for (int i = 0; i < vstream.frameCount(); i++) {
            Mat frame;
            vstream >> frame;
            
            Frame wrapper(vstream.currentFrame()-1, vector<Vec3f>(), frame);
            
            // Critical section
            sem_wait(&sem1);
            stack1.push(wrapper);
            sem_post(&sem1);
            // End of critical section
        }
        
        // Close video
        vstream.close();
    }
    catch (std::runtime_error& exception) {
        cout << exception.what() << endl;
    }
}

void t2_routine() {
    try {
        VideoStream& vstream = VideoStream::load();
        
    }
    catch (std::runtime_error& exception) {
        cout << exception.what() << endl;
    }   
}
void t3_routine() {
}
