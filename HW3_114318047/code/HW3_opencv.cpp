#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>


void task1(const std::string& inputVal, const std::string& outputVal) {
    std::cout << "Generating IPM from: " << inputVal << std::endl;

    cv::Mat img = cv::imread(inputVal);
    if (img.empty()) {
        std::cerr << "Error: Could not read image " << inputVal << std::endl;
        return;
    }

    const int W = img.cols;
    const int H = img.rows;

    // Camera Parameters
    const float alpha  = 15.0f * M_PI / 180.0f;
    const float dx     = 4.0f;
    const float dy     = -10.0f;
    const float dz     = 5.0f;
    const float gamma0 = 0.0f;
    const float theta0 = 0.025f;

    const float m1 = float(H - 1);
    const float n1 = float(W - 1);

    // 3. Define Output View (Zoom / Crop settings)
    float viewWidthMeters = 400.0f;   // Show 20 meters wide
    float viewDepthMeters = 200.0f;  // Show 100 meters deep
    float pixelsPerMeter  = 20.0f;   // Resolution

    // Calculate Output Dimensions
    int outW = (int)(viewDepthMeters * pixelsPerMeter); // Width = Depth
    int outH = (int)(viewWidthMeters * pixelsPerMeter); // Height = Lateral Width
    
    // Create Output Matrix (Black Background)
    cv::Mat outImg = cv::Mat::zeros(cv::Size(outW, outH), CV_8UC3);

    float centerX = 4.0f; 
    float zoomX_min = centerX - (viewWidthMeters / 2.0f);
    float zoomX_max = centerX + (viewWidthMeters / 2.0f);
    
    // Start from Camera (Y=0) to Max Depth
    float zoomY_min = 0.0f;
    float zoomY_max = viewDepthMeters;

    // 4. Inverse Warping Loop
    // Loop over every pixel in the OUTPUT image (ox, oy)
    for (int oy = 0; oy < outH; ++oy) {
        for (int ox = 0; ox < outW; ++ox) {
            
            // A. Map Pixel (ox, oy) -> World (X, Y)
            float ratioX = float(ox) / float(outW - 1);
            float ratioY = float(oy) / float(outH - 1); 

            float Y = zoomY_min + ratioX * (zoomY_max - zoomY_min);
            float X = zoomX_max - ratioY * (zoomX_max - zoomX_min);

            if (Y <= 0.1f) continue; // Behind camera

            // map (x,y) to (u,v)
            float x0 = X - dx;
            float y0 = Y - dy;
            float R = std::sqrt(x0 * x0 + y0 * y0);
            
            if (R < 1e-6f) continue;

            float phi = std::atan(dz / R);
            float psi = std::atan2(x0, y0);

            float u_float = (m1 / (2.0f * alpha)) * (phi - theta0 + alpha);
            float v_float = (n1 / (2.0f * alpha)) * (psi - gamma0 + alpha);

            int u = cvRound(u_float);
            int v = cvRound(v_float);

            // C. Bounds Check & Assign
            if (u >= 0 && u < H && v >= 0 && v < W) {
                // OpenCV uses (row, col) which is (u, v)
                outImg.at<cv::Vec3b>(oy, ox) = img.at<cv::Vec3b>(u, v);
            }
        }
    }

    // 5. Save Result
    // Flip vertically for correct orientation
    cv::flip(outImg, outImg, 0);
    cv::imwrite(outputVal, outImg);

    // save as png
    std::string pngOutput = outputVal.substr(0, outputVal.find_last_of('.')) + ".png";
    cv::imwrite(pngOutput, outImg);

    std::cout << "Saved: " << outputVal << std::endl;
}

void task2(const std::string& inputVal, const std::string& outputVal) {
    std::cout << "Detecting lanes on: " << inputVal << std::endl;

    cv::Mat img = cv::imread(inputVal);
    if (img.empty()) {
        std::cerr << "Error: Could not read image " << inputVal << std::endl;
        return;
    }

    const int W = img.cols;
    const int H = img.rows;
    const int LANE_THICKNESS = 14;
    int lastLaneY = -1;

    // iterate over each column
    for (int x = 0; x < W; ++x) {

        int max_bright_top = -1;
        int y_best_top = -1;

        // detect the top edge of colorless lane
        for (int y = 0; y < H / 2; ++y) {
            cv::Vec3b p = img.at<cv::Vec3b>(y, x);
            int b = p[0], g = p[1], r = p[2];
            int brightness = (r + g + b) / 3;

            bool isColorless = (std::abs(r - g) < 30 && std::abs(r - b) < 30 && std::abs(g - b) < 30);

            // check for the brightest colorless pixel
            if (isColorless && brightness > max_bright_top) {

                // update max brightness and position
                max_bright_top = brightness;

                // record the y position
                y_best_top = y;
            }
        }

        // Draw the detected top edge if found,130=> bright enough
        if (y_best_top != -1 && max_bright_top > 130) {
            for (int k = 0; k < LANE_THICKNESS; ++k) {
                int drawY = y_best_top - LANE_THICKNESS / 2 + k;
                if (drawY >= 0 && drawY < H) {

                    // set pixel to green
                    img.at<cv::Vec3b>(drawY, x) = cv::Vec3b(0, 255, 0); // Green
                }
            }
        }
        
        // detect the bottom edge of colorless lane
        int runStart = -1; 

        // Scan from middle to bottom
        for (int y = H / 2; y < H; ++y) { 
            
            cv::Vec3b p = img.at<cv::Vec3b>(y, x);
            int b = p[0], g = p[1], r = p[2];
            int brightness = (r + g + b) / 3;
            
            // White lane check
            bool isColorless = (std::abs(r - g) < 30 && std::abs(r - b) < 30 && std::abs(g - b) < 30);
            bool isWhite = (isColorless && brightness > 130);

            if (isWhite) {
                if (runStart == -1) runStart = y; // Start counting white pixels
            } 
            else {
                // End of white segment
                if (runStart != -1) {
                    int length = y - runStart;
                    int segmentCenter = runStart + length / 2;

                    if (length < 40) { // < 40 pixels is a lane
                        for (int k = runStart; k < y; ++k) {
                            img.at<cv::Vec3b>(k, x) = cv::Vec3b(255, 0, 0); // Paint Blue
                        }
                        // Update memory
                        lastLaneY = segmentCenter;
                    }
                    else {
                        // Longer white segment, use memory to prevent jumps
                        if (lastLaneY != -1) {
                            // Draw a standard 14px thick line at the remembered position
                            for (int k = 0; k < 14; ++k) {
                                int drawY = lastLaneY - 7 + k;

                                // Only paint if we are actually inside the white block area
                                if (drawY >= runStart && drawY < y) {
                                    // set pixel to blue
                                    img.at<cv::Vec3b>(drawY, x) = cv::Vec3b(255, 0, 0);
                                }
                            }
                        }
                    }
                    runStart = -1; // Reset
                }
            }
        }
        
        // Handle case where white touches the very bottom of the screen
        if (runStart != -1) {
            int length = H - runStart;
            if (length < 40) {
                 for (int k = runStart; k < H; ++k) img.at<cv::Vec3b>(k, x) = cv::Vec3b(255, 0, 0);
                 lastLaneY = runStart + length / 2;
            } else if (lastLaneY != -1) {
                 for (int k = 0; k < 14; ++k) {
                    int drawY = lastLaneY - 7 + k;
                    if (drawY >= runStart && drawY < H) img.at<cv::Vec3b>(drawY, x) = cv::Vec3b(255, 0, 0);
                 }
            }
        }

        for (int y = H / 3; y < 2 * H / 3; ++y) {
            cv::Vec3b p = img.at<cv::Vec3b>(y, x);
            int b = p[0], g = p[1], r = p[2];

            if (r > 160 && g > 160 && b < 140) {
                img.at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255); // Red
                if (y + 1 < H) img.at<cv::Vec3b>(y + 1, x) = cv::Vec3b(0, 0, 255);
            }
        }
    }
    
    cv::imwrite(outputVal, img);

    // save as png
    std::string pngOutput = outputVal.substr(0, outputVal.find_last_of('.')) + ".png";
    cv::imwrite(pngOutput, img);

    std::cout << "[Task 2] Saved: " << outputVal << std::endl;
}

int main() {
    std::cout << "----- Homework 3 (OpenCV Version) -----\n";
    while (true) {
        std::cout << "\n[1] Task 1 (Generate IPM)\n"
                  << "[2] Task 2 (Detect Lanes on IPM)\n"
                  << "[0] Exit\n"
                  << "Select: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear(); std::cin.ignore(1000, '\n'); continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1: task1("WM691d4b56543ac.bmp", "task1_opencv.bmp"); break;
            case 2: task2("task1_opencv.bmp", "task2_opencv.bmp"); break;
            default: 
                std::cout << "Invalid choice.\n"; 
                break;
        }
    }
    return 0;
}