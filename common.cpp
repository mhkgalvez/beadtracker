#include "common.hpp"

using namespace std;
using namespace cv;

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
const int PAGE_UP = 1113941;
const int PAGE_DOWN = 1113942;

bool show = false;
double scale;
bool succeeded = true;
int fps;
string video_path;
int minRadius;
int maxRadius;
double param1;
double param2;
double minDistance;
double dp;

Rect getregion() {
    VideoStream& video = VideoStream::load();
    Point p1, p2;
    namedWindow("Select a region");   
    try {
        video.open(video_path);
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
                    5, 1.5, Scalar(0, 255, 255));
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
                case ESC_KEY:
                    exit = true;
                    succeeded = false;
                    break;
                case ENTER_KEY_RIGHT: // Enter key right and Enter key key have the same behavior.                    
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

void bead_detection(Rect rect) {
    int r1, r2, c1, c2; // Deviations from the center of beads region
    r1 = r2 = c1 = c2 = 0;
    Mat region;
    bool paused = false;
    try {
        VideoStream& video = VideoStream::load();
        Mat frame;
        video.open(video_path);
        
        if (show) namedWindow("Visual tracking"); // Create OpenCV window
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
                // Start reading new frame
                if (video >> frame == false) 
                    throw runtime_error("Error while trying to read frame.");

                // Getting picture region around the main beads
                region = frame.operator ()(rect);//frame.rowRange(190 + r1, 540 + r2);
                //region = region.colRange(550 + c1, 1000 + c2);
                Mat gray;               // Gray-scale image
                vector<Vec3f> circles;  // Circles (Center coordinates and radius)
                cvtColor(region, gray, CV_BGR2GRAY); // Convert to gray-scale
                //GaussianBlur(gray, gray, Size(9, 9), 2, 2); // Reduce noise
                HoughCircles(gray, circles, CV_HOUGH_GRADIENT, dp, 
                        minDistance, param1, param2, minRadius, maxRadius); // Detect circles
                if (show) {
                    // Write information
                    stream << "Circles detected: " << circles.size() 
                            << " | FPS: " << fps;                        
                    putText(region, stream.str(), Point(5, 13), 
                            5, 0.7, Scalar(0, 255, 255));
                    stream.str(string());       // Clear stream
                    stream << "Frames processed: " << i + 1 << "/" 
                            << video.frame_count();
                    putText(region, stream.str(), Point(5, 27), 
                            5, 0.7, Scalar(0, 255, 255));
                    stream.str(string());       // Clear stream
                }
                // Save detected centers into a file
                vector<Point> centers;
                for (uint i = 0; i < circles.size(); i++) 
                    centers.push_back(Point(cvRound(circles[i][0]), 
                            cvRound(circles[i][1])));
                vector<int> positions = bead_tracking(centers); // Get only two positions
                if (positions.size() == 2 or positions.size() == 1) {
                    // Draw the circles detected                    
                    for(size_t i = 0; i < positions.size(); i++)
                    {
                        Point center(cvRound(circles[positions[i]][0]), 
                                cvRound(circles[positions[i]][1]));
                        centers.push_back(center);
                        int radius = cvRound(circles[positions[i]][2]);
                        // Circle center
                        circle(region, center, 3, Scalar(0,255,0), -1, 8, 0);
                        // Circle outline
                        circle(region, center, radius, Scalar(0, 0, 255), 3, 8, 0);
                    }
                }
            } 
            else { 
                // Go back to the previous index
                i--;
            }
            if (show) { 
                imshow("Visual tracking", region); // Show image in window
                int key;
                if ((key = waitKey(1000/fps)) != -1) {
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
                        case PAGE_UP:
                            if (fps < 60) {
                                fps++;
                            }
                            break;
                        case PAGE_DOWN:
                            if (fps > 1) {
                                fps--;
                            }
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

double distance(Point& bead1, Point& bead2) {
    return sqrt(double((bead1.x - bead2.x) * (bead1.x - bead2.x) 
            + (bead1.y - bead2.y) * (bead1.y - bead2.y)));
}

/* Keep track of the two beads, save data to file and return their positions 
 * in the vector taken as entry arguments. This can be used outside to draw the
 * correct beads in the scene.
 */ 
Tracking bead_tracking(std::vector<cv::Point> circles) {
    static double old_distance = 0;
    double new_distance = 0;
    double err = 10.0;
    static ofstream bead1_x, bead1_y, bead2_x, bead2_y, dist;
    // Create directories
    static bool first_time = true;
    stringstream dirpath;
    if (first_time) {
        dirpath << get_current_dir_name() << "/out";
        // Attempt to create directory with read, write and search permissions
        int ret = mkdir(dirpath.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret == -1 and errno != EEXIST) {
            throw GeneralException(strerror(errno));
        }
        stringstream stream(video_path);
        string segment;
        vector<string> list;
        while (getline(stream, segment, '/')) {
            list.push_back(segment);
        }
        // Suppose list.size() is never equals to zero
        stringstream stream2;
        stream2 << time(NULL); // Use time(NULL) in order to create a 
                               // different directory every single time
        dirpath << "/" << list[list.size()-1] + "_" + stream2.str();
        ret = mkdir(dirpath.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (ret == -1) {
            throw GeneralException(strerror(errno));
        }
        // Open files
        bead1_x.open((dirpath.str() + string("/bead1_x.txt")).c_str());
        bead1_y.open((dirpath.str() + string("/bead1_y.txt")).c_str());
        bead2_x.open((dirpath.str() + string("/bead2_x.txt")).c_str());
        bead2_y.open((dirpath.str() + string("/bead2_y.txt")).c_str());
        dist.open((dirpath.str() + string("/dist.txt")).c_str());
        if (!bead1_x.is_open() or !bead1_y.is_open() or !bead2_x.is_open()
                or !bead2_y.is_open() or !dist.is_open()) {
            throw GeneralException("Any of the files could not be open. Function: bead_tracking()");
        }        
        first_time = false;
    }
    // Arbitrarily suppose that the two first points detected are the correct ones
    
    /* BIG PROBLEM. THIS ASSUMPTION REQUIRES THAT I FIRST DETECT 2 BEADS!
     */
    Point* top;
    Point* bottom;
    if (circles[1].y < circles[0].y) {
        top = &circles[1];
        bottom = &circles[0];
    }
    else {
        top = &circles[0];
        bottom = &circles[1];
    }
    static long int sum_x = 0;
    static long int sum_y = 0;
    static int index = 0;
    Point temp = *top;
    sum_x += temp.x;
    sum_y += temp.y;
    static int formerX = 0, formerY = 0;
    if (index == 0) {
        formerX = top->x;
        formerY = top->y;
    }
    if (index > 0) {
        int newX, newY;
        newX = sum_x/(index+1);
        newY = sum_y/(index+1);
        if (abs(newX - top->x) > 5) {
            top->x = newX;
        }
        else {
            top->x = formerX;
        }
        if (abs(newY - top->y) > 5) {
            top->y = newY;
        }
        else {
            top->y = formerY;
        }
    }
    cout << index+1 << "_x -- top->x: " << top->x << " -- newX: " << sum_x/(index+1) << endl;
    cout << index+1 << "_y -- top->y: " << top->y << " -- newY: " << sum_y/(index+1) << endl;
    index++;
    if (circles.size() == 1) {
        // Save to file (both beads are going to be this single one)
        
        // Keep old_distance 0 and send the single position as a return
        old_distance = 0;
        vector<int> positions;
        positions.push_back(0);
        // Print information to files
        bead1_x << bottom->x << endl;
        bead1_y << bottom->y << endl;
        bead2_x << top->x << endl;
        bead2_y << top->y << endl;
        dist << 0 << endl;
        return positions;  
    }
    else if (circles.size() == 2) {
        // First time
        if (old_distance == 0) {
            old_distance = distance(bottom, top);
            // Print information to files
            bead1_x << bottom->x << endl;
            bead1_y << bottom->y << endl;
            bead2_x << top->x << endl;
            bead2_y << top->y << endl;
            dist << old_distance << endl;            
            vector<int> positions;
            positions.push_back(0);
            positions.push_back(1); 
            return positions; 
        }
        else {
            new_distance = distance(bottom, top); 
            double diff = abs(new_distance - old_distance);
            // Only accept as valid if it refers to the two correct beads.
            // Otherwise return nothing
            if (diff < err) {
                // Print information to files
                bead1_x << bottom->x << endl;
                bead1_y << bottom->y << endl;
                bead2_x << top->x << endl;
                bead2_y << top->y << endl;
                dist << new_distance << endl;                  
                old_distance = new_distance;
                vector<int> positions;
                positions.push_back(0);
                positions.push_back(1); 
                return positions;                
            }           
        }
    }
    /* BIG PROBLEM. IF THERE ARE MORE THAN 3 BEADS THE PREVIOUS 
     * ASSUMPTION OF TOP AND BOTTOM IS TERRIBLY WRONG!!!
     */
    else if (circles.size() > 2) {
        // First time
        if (old_distance == 0) {
            // Find the lest of the distances and assume it refers 
            // to the correct beads
            double least = distance(circles[0], circles[1]);
            int least_indices[2];
            for (uint i = 0; i < circles.size(); ++i) {
                for (uint j = i+1; j < circles.size(); ++j) {
                    double new_distance = distance(circles[i], circles[j]);
                    if (new_distance <= least) {
                        least = new_distance;
                        least_indices[0] = i; 
                        least_indices[1] = j;
                    }
                }
            }      
            old_distance = least;
            // Print information to files
            bead1_x << circles[least_indices[0]].x << endl;
            bead1_y << circles[least_indices[0]].y << endl;
            bead2_x << circles[least_indices[1]].x << endl;
            bead2_y << circles[least_indices[1]].y << endl;
            dist << old_distance << endl;
            vector<int> positions;
            positions.push_back(least_indices[0]);
            positions.push_back(least_indices[1]);
            return positions;
        }
        else {
            for (uint i = 0; i < circles.size(); ++i) {
                for (uint j = i+1; j < circles.size(); ++j) {
                    double new_distance = distance(circles[i], circles[j]);
                    double diff = abs(new_distance - old_distance);
                    if (diff < err) {
                        // Print information to files
                        bead1_x << circles[i].x << endl;
                        bead1_y << circles[i].y << endl;
                        bead2_x << circles[j].x << endl;
                        bead2_y << circles[j].y << endl;
                        dist << old_distance << endl;                        
                        // This is probably the pair we want
                        vector<int> positions;
                        positions.push_back(i);
                        positions.push_back(j);
                        old_distance = new_distance;
                        return positions;
                    }
                }
            }            
        }
    }
    // Print information to files
    bead1_x << "-" << endl;
    bead1_y << "-" << endl;
    bead2_x << "-" << endl;
    bead2_y << "-" << endl;
    dist << "-" << endl;     
    return vector<int>();
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

const char* CLOSE_MESSAGE = "<<<close>>>";

// Interprocess communication via FIFO
void sendMessage(string message) {
    /*ofstream fifo;
    
    // Initialize FIFO
    struct passwd *pw = getpwuid(getuid());
    stringstream sstr;
    sstr << pw->pw_dir << "/bin/BeadTracker/out";
    fifo.open(sstr.str().c_str(), ios_base::app);
       
    if (fifo.is_open() == false) {
        throw runtime_error("FIFO not open!");
    }    
    
    message += "\n";
    fifo << message;
    fifo.close();*/
    cout << message << endl;
}

void sendMessage(const char* message) {
    string str_message = message;
    sendMessage(str_message);
}
