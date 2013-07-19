// C++ Includes
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <ctime>

// C and Unix Includes
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pwd.h>

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
const int ENTER_KEY = 1048586;
const int ENTER_KEY_RIGHT = 1113997;
const int RIGHT_PLUS = 1114027;
const int RIGHT_MINUS = 1114029;
const int RIGHT_TIMES = 1114026;
const int RIGHT_BAR = 1114031;
const int DOT_KEY = 1048622;
const int SLASH_KEY = 1048623;

bool show = false;
double scale;

bool succeeded = true;

Rect getregion(string path);
void beadtracking(string video_path, Rect rect);
double diff(struct timeval t1, struct timeval t2);
string time2str(long long milliseconds);

const char* CLOSE_MESSAGE = "<<<close>>>";
void sendMessage(string message);
void sendMessage(const char* message);

int main(int argc, char** argv) {	
    struct timeval start, end;
    string video_path;
        
    // Checking command line arguments
    if (argc != 3) {
        sendMessage("You must call beadtracker with at least 3 command line arguments.");
        return -1;
    }
    
    // Getting file
    video_path = argv[1];
    
    // Getting scale
    stringstream stream;
    stream << argv[2];
    double scale;
    stream >> scale;
    sendMessage(stream.str());
    Rect region = getregion(video_path); 
    show = true;
    // Goes into tracking routine only if getregion() succeeded in its task
    if (succeeded) {
        gettimeofday(&start, NULL);
        beadtracking(video_path, region);
        gettimeofday(&end, NULL);
        sendMessage("Single thread version execution time: " 
                + time2str(diff(end, start)));
    }
    return 0;
}

Rect getregion(string path) {
    VideoStream& video = VideoStream::load();
    Point p1, p2;
    namedWindow("Select a region");   
    try {
        video.open(path);
        double x, y, width, height;
        bool exit = false;
        Mat first_frame;
        if (video >> first_frame == false) 
            throw runtime_error("Error while trying to read frame.");        
        x = 2 * first_frame.cols/4;
        y = 2 * first_frame.rows/4;
        width = first_frame.cols/4;
        height = first_frame.rows/4;
        while (!exit) {
            p1 = Point(x, y); // Create points
            p2 = Point(x + width, y + height);
            // Draw a rectangle representing the region to be processed
            Mat temp;
            first_frame.copyTo(temp); // Copy frame to a temporary space 
            // Write
            stringstream stream;
            stream << "Current scale: " << scale;
            putText(temp, stream.str(), Point(5, 25), 
                    5, 1.5, Scalar(0, 0, 255));
            rectangle(temp, p1, p2, Scalar(0, 255, 0), 3, CV_AA, 0);
            // Resize image
            Mat img = temp;
            int _gcd;
            // Try to avoid resizing image to become too little
            if (temp.cols % 2 == 0 and temp.rows % 2 == 0) {
                _gcd = 2;
            }
            else {
                _gcd = gcd<int>(temp.cols, temp.rows);
            }
            if (temp.cols > 1024) {
                resize(temp, img, Size(temp.cols/_gcd, temp.rows/_gcd)); 
            }
            imshow("Select a region", img); // Show image resized
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
                case ENTER_KEY_RIGHT: // Enter keys and ESC key have the same behavior.
                case ESC_KEY:
                case ENTER_KEY:
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
                case DOT_KEY:
                    scale -= 0.2;
                    break;
                case SLASH_KEY:
                    scale += 0.2;
                    break;
            }
            // Check rectangle dimensions and make adjustments to avoid exceptions
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            if (x + width < 0) {
                if (x < first_frame.cols) x += scale;
                else width += scale;
            }
            if (y + height < 0) {
                if (y < first_frame.rows) y += scale;
                else height += scale;
            }
            if (x > first_frame.cols) x = first_frame.cols;
            if (y > first_frame.rows) y = first_frame.rows;
            if (x + width > first_frame.cols) {
                if (x > 0) x -= scale;
                else width -= scale;
            }
            if (y + height > first_frame.rows) {
                if (y > 0) y -= scale; 
                else height -= scale;
            }
        }
       
        destroyWindow("Select a region");
        sendMessage("Region selected.");
        video.close();
    }
    catch (exception& ex) {
        sendMessage(string("Exception: ") + ex.what() + string("\n"));   
        succeeded = false;
    }
    return Rect(p1, p2);
}

void beadtracking(string video_path, Rect rect) {
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
                double percent = ((i + 1) * 100)/video.frame_count();
                stringstream stream;
                static double previous = floor(percent);
                // Test whether percent is an integer value
                if (percent == floor(percent) and floor(percent) != previous) {
                    stream << percent;
                    sendMessage(stream.str() + string("% concluded."));
                    stream.str(string()); // Clear stream
                    previous = floor(percent);
                }
                if (video >> frame == false) 
                    throw runtime_error("Error while trying to read frame.");

                // Getting picture region around the main beads
                region = frame.operator ()(rect);//frame.rowRange(190 + r1, 540 + r2);
                //region = region.colRange(550 + c1, 1000 + c2);
                Mat gray;               // Gray-scale image
                vector<Vec3f> circles;  // Circles (Center coordinates and radius)
                cvtColor(region, gray, CV_BGR2GRAY); // Convert to gray-scale
                //GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
                HoughCircles(gray, circles, CV_HOUGH_GRADIENT, 1, 
                        gray.rows/8, 80, 40, 5, 30); // Detect circles
                // Write
                stream << "Circles detected: " << circles.size();
                putText(region, stream.str(), Point(5, 13), 
                        5, 0.7, Scalar(0, 0, 255));
                // Draw the circles detected
                for(size_t i = 0; i < circles.size(); i++)
                {
                    Point center(cvRound(circles[i][0]), 
                            cvRound(circles[i][1]));
                    int radius = cvRound(circles[i][2]);
                    // Circle center
                    circle(region, center, 3, Scalar(0,255,0), -1, 8, 0);
                    // Circle outline
                    circle(region, center, radius, Scalar(0, 0, 255), 3, 8, 0);
                }
            } 
            else { 
                // Go back to the previous index
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
                            if (!paused) sendMessage("BeadTracker paused.");
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
        sendMessage("BeadTracker has finished correctly.");
        sendMessage(CLOSE_MESSAGE);
        video.close();
    }
    catch (exception& ex) {
        sendMessage(string("Exception: ") + ex.what() + string("\n"));     
    }
}

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

// Convert milliseconds to a string in the format mm:ss:ms
string time2str(long long milliseconds) {
    int msecs = milliseconds % 1000;
    int secs = milliseconds/1000;
    int mins = secs/60;
    secs = secs % 60;
    
    stringstream time;
    time << mins << "m:" << secs << "s:" << msecs << "ms";
    
    return time.str();
}

// Interprocess communication via FIFO
void sendMessage(string message) {
    ofstream fifo;
    
    // Initialize FIFO
    struct passwd *pw = getpwuid(getuid());
    stringstream sstr;
    sstr << pw->pw_dir << "/bin/BeadTracker/fifo";
    fifo.open(sstr.str().c_str(), ios_base::out);
       
    if (fifo.is_open() == false) {
        throw runtime_error("FIFO not open!");
    }    
    
    message += "\n";
    fifo << message;
    fifo.close();
}

void sendMessage(const char* message) {
    string str_message = message;
    sendMessage(str_message);
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