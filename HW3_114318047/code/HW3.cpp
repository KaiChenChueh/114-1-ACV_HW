#define _USE_MATH_DEFINES
#include <cmath> // for M_PI

#include <iostream>
#include <stdexcept> // for runtime_error
#include <vector> // for std::vector
#include <cstdint> // for uint8_t
#include <opencv2/opencv.hpp>
#include "bmp.hpp"  // declares bmp::BMPImage, bmp::readBMP, bmp::writeBMP
#include <queue>
#include <utility> // for std::pair
#include <chrono> // for timing

/********************************************************
* Filename    : HW3.cpp
* Author      : Stanley Chueh
* Note        : ACV HW3
*********************************************************/

// Helpers functions to get/set BGR pixel values
static void setBGR(bmp::BMPImage &img, int x, int y, uint8_t b, uint8_t g, uint8_t r)
{
    const int rowSize = bmp::rowSizeBytes(img.width);
    uint8_t* ptr = &img.data[y * rowSize + x * 3];
    ptr[0] = b;
    ptr[1] = g;
    ptr[2] = r;
}

static void getBGR(const bmp::BMPImage &img, int x, int y, uint8_t &b, uint8_t &g, uint8_t &r)
{
    const int rowSize = bmp::rowSizeBytes(img.width);
    const uint8_t* ptr = &img.data[y * rowSize + x * 3];
    b = ptr[0];
    g = ptr[1];
    r = ptr[2];
}

// Coordinate Transformation: World (X,Y) to Image (u,v)
static void world_to_image(float X, float Y, float &u, float &v, float dx, float dy, float dz, float alpha, float gamma0, float theta0, float m1, float n1)
{
    float x0 = X - dx;
    float y0 = Y - dy;
    
    // R is the distance from camera center to point's projection on ground plane
    float R  = std::sqrt(x0 * x0 + y0 * y0);
    if (R < 1e-6f) { u = v = -1.0f; return; }

    // Calculate angles
    float phi = std::atan(dz / R);           
    float psi = std::atan2(x0, y0);          

    u = (m1 / (2.0f * alpha)) * (phi - theta0 + alpha);
    v = (n1 / (2.0f * alpha)) * (psi - gamma0 + alpha);
}

// Task1: Inverse Perspective Mapping 
static void task1(const char* input, const char* output)
{
    bmp::BMPImage img = bmp::readBMP(input);
    const int W = img.width;      
    const int H = img.height;     

    // Follow the parameters from the slides to convert (u,v) to (X,Y)
    const float alpha  = 15.0f * M_PI / 180.0f;
    const float dx     = 4.0f;
    const float dy     = -10.0f;
    const float dz     = 5.0f;
    const float gamma0 = 0.0f;
    const float theta0 = 0.025f;

    const float m1 = float(H - 1);
    const float n1 = float(W - 1);

    // Follow the steps in the slide(transfer the pixels (u,v) in [uHorizon:Height-1,0:width-1])
    // by foward warping(Image to World) to get the range [xmin:xmax,ymin:ymax] of IPM image (x,y)
    float uHorizon = float(H - 1) * (-theta0 + alpha) / (2.0f * alpha);
    int   uStart   = int(std::ceil(uHorizon)); // round up to next integer

    if (uStart < 0)   uStart = 0;
    if (uStart >= H)  uStart = H - 1;

    // For debugging: Visualize the horizon line on input image
    {
        bmp::BMPImage imgHorizon = img; 
        int lineRow = H - 1 - uStart;   
        
        // Draw a red line at uHorizon
        if(lineRow >= 0 && lineRow < H) {
            for(int col = 0; col < W; ++col) {

                // Draw 3 pixel thick line for visibility
                for(int k = 0; k < 3; ++k) {
                    if (lineRow - k >= 0)
                        setBGR(imgHorizon, col, lineRow - k, 0, 0, 255); // Red
                }
            }
        }
        bmp::writeBMP("1_input_horizon.bmp", imgHorizon);
    }

    // Determine World Coordinate Ranges by Forward Warping
    float xmin =  1e9f, xmax = -1e9f;
    float ymin =  1e9f, ymax = -1e9f;

    for (int u = uStart; u < H; ++u)
    {
        float u_norm = float(u) / float(H - 1);
        float phi    = theta0 - alpha + 2.0f * alpha * u_norm;
        float tanphi = std::tan(phi);

        if (std::fabs(tanphi) < 1e-6f) continue;                        

        for (int v = 0; v < W; ++v)
        {
            float v_norm = float(v) / n1;
            float psi    = gamma0 - alpha + 2.0f * alpha * v_norm;

            float X = dz / tanphi * std::sin(psi) + dx;
            float Y = dz / tanphi * std::cos(psi) + dy;

            if (Y <= 0.0f) continue;

            xmin = std::min(xmin, X);
            xmax = std::max(xmax, X);
            ymin = std::min(ymin, Y);
            ymax = std::max(ymax, Y);
        }
    }

    // Create Image to visualize World Coordinates
    bmp::BMPImage WorldImg;
    WorldImg.width = W; 
    WorldImg.height = H;
    WorldImg.data.assign(bmp::rowSizeBytes(W) * H, 0); 

    float xRange = xmax - xmin;
    float yRange = ymax - ymin;

    // 2. Forward Warp: Iterate Input (u,v) -> Plot on World (x,y)
    for (int u = uStart; u < H; ++u)
    {
        float u_norm = float(u) / float(H - 1);
        float phi    = theta0 - alpha + 2.0f * alpha * u_norm;

        for (int v = 0; v < W; ++v)
        {
            // Please refers to Transformation Equations in page1 of the title named
            // Test of Bertozzi and Broggi's Inverse Perspective Mapping Functions
            float v_norm = float(v) / n1;
            float psi    = gamma0 - alpha + 2.0f * alpha * v_norm;

            float X = dz / std::tan(phi) * std::sin(psi) + dx;
            float Y = dz / std::tan(phi) * std::cos(psi) + dy;
            
            // Ignore points behind the camera
            if (Y <= 0.0f) continue;

            // Normalize World X,Y to fit on the image canvas
            // map X [xmin, xmax] -> [0, W-1]
            int drawX = (int)((Y - ymin) / yRange * (W - 1));
            
            // map Y [ymin, ymax] -> [0, H-1] 
            // In BMP, y=0 is bottom. We want small Y (close) at bottom.
            int drawY = (int)((X - xmin) / xRange * (H - 1));

            if (drawX >= 0 && drawX < W && drawY >= 0 && drawY < H)
            {
                uint8_t b, g, r;

                // Get color from original input (u is top-down, need flip for getBGR)
                getBGR(img, v, H - 1 - u, b, g, r);
                
                // Set color on fan image (drawY is already bottom-up)
                setBGR(WorldImg, drawX, drawY, b, g, r);
            }
        }
    }
    bmp::writeBMP("2_world_coordinates.bmp", WorldImg);
    
    // 1. Define physical area you want to see
    float viewWidthMeters = 400.0f;  // Show 10 meters wide (Road width)
    float viewDepthMeters = 200.0f;  // Show 30 meters long (Distance)
    float pixelsPerMeter = 20.0f; 

    int outW = (int)(viewDepthMeters * pixelsPerMeter); // Width represents Depth
    int outH = (int)(viewWidthMeters * pixelsPerMeter); // Height represents Width

    std::cout << "Generating IPM: " << outW << "x" << outH << " pixels." << std::endl;

    bmp::BMPImage out;
    out.width  = outW;
    out.height = outH;
    const int outRowSize = bmp::rowSizeBytes(outW);
    out.data.assign(outRowSize * outH, 0); 

    // Calculate Boundaries
    float centerX = (xmax + xmin) / 2.0f; 
    float zoomX_min = centerX - (viewWidthMeters / 2.0f);
    float zoomX_max = centerX + (viewWidthMeters / 2.0f);
    float zoomY_min = ymin; 
    float zoomY_max = ymin + viewDepthMeters;

    // 3. Inverse Warping: Iterate Output (X,Y) -> Sample Input (u,v)
    for (int oy = 0; oy < outH; ++oy)      
    {
        for (int ox = 0; ox < outW; ++ox)  
        {
            // Calculate ratios
            float ratioX = float(ox) / float(outW - 1); 
            float ratioY = float(oy) / float(outH - 1);

            float Y = zoomY_min + ratioX * (zoomY_max - zoomY_min);
            float X = zoomX_max - ratioY * (zoomX_max - zoomX_min);

            if (Y <= 0.1f) continue;   

            float u_f, v_f;
            world_to_image(X, Y, u_f, v_f, dx, dy, dz, alpha, gamma0, theta0, m1, n1);

            int u_i = (int)std::round(u_f);
            int v_i = (int)std::round(v_f);

            if (u_i < 0 || u_i >= H || v_i < 0 || v_i >= W)
                continue;

            int ui_buf = H - 1 - u_i;
            int vi_buf = v_i;

            uint8_t b, g, r;
            getBGR(img, vi_buf, ui_buf, b, g, r);
            setBGR(out, ox, oy, b, g, r);
        }
    }
    
    bmp::writeBMP(output, out);

    // Save as PNG 
    cv::Mat bmpImage = cv::imread(output, cv::IMREAD_UNCHANGED);
    if (!bmpImage.empty()) {
        std::string pngOutput = std::string(output).substr(0, std::string(output).find_last_of('.')) + ".png";
        cv::imwrite(pngOutput, bmpImage);
        std::cout << "Saved as PNG: " << pngOutput << std::endl;
    } else {
        std::cerr << "Failed to load BMP for PNG conversion: " << output << std::endl;
    }
}

// Container for a point (Simple x,y)
struct Point { int x, y; };

static void task2(const char* input_ipm_file, const char* output_labeled_file)
{
    std::cout << "Task 2: Detecting and covering lanes..." << std::endl;

    bmp::BMPImage img = bmp::readBMP(input_ipm_file);
    const int W = img.width;
    const int H = img.height;

    // Define how thick the lane is (in pixels)
    // 12 pixel should cover lanes
    const int LANE_THICKNESS = 12; 

    // iterate over each column
    for (int x = 0; x < W; ++x) 
    {
        uint8_t mb, mg, mr;
        getBGR(img, x, H/2, mb, mg, mr);

        // detect the top edge of white lane
        for (int y = 0; y < H / 2; ++y) {
            uint8_t b, g, r;
            getBGR(img, x, y, b, g, r);

            // white lane detection
            bool isWhite = (r > 200 && g > 200 && b > 200);

            if (isWhite) {
                // Found Top Edge of White Lane
                // Draw downwards (y + k) to cover the whole lane width
                for (int k = 0; k < LANE_THICKNESS; ++k) {
                    if (y + k < H) { // Boundary check

                        // mark the lane in blue
                        setBGR(img, x, y + k, 255, 0, 0); // Blue
                    }
                }
                break; 
            }
        }

        // detect the bottom edge of white lane(bottom to top)
        for (int y = H - 1; y > H / 2; --y) {
            uint8_t b, g, r;
            getBGR(img, x, y, b, g, r);

            // white lane detection
            bool isWhite = (r > 200 && g > 200 && b > 200);

            if (isWhite) {
                // Found Bottom Edge of White Lane
                // Draw upwards (y - k) to cover the whole lane width
                for (int k = 0; k < LANE_THICKNESS; ++k) {
                    if (y - k >= 0) { // Boundary check

                        // mark the lane in green
                        setBGR(img, x, y - k, 0, 255, 0); // Green
                    }
                }
                break; 
            }
        }

        // Yellow lane detection in the middle third of the image
        for (int y = H/3; y < 2*H/3; ++y) {
            uint8_t b, g, r;
            getBGR(img, x, y, b, g, r);

            // yellow lane detection
            if (r > 180 && g > 180 && b < 150) {
                setBGR(img, x, y, 0, 0, 255); // Red

                // Make red line
                if (y+1 < H) setBGR(img, x, y+1, 0, 0, 255);
            }
        }
    }

    bmp::writeBMP(output_labeled_file, img);

    // Save as PNG
    cv::Mat bmpImage = cv::imread(output_labeled_file, cv::IMREAD_UNCHANGED);
    if (!bmpImage.empty()) {
        std::string pngOutput = std::string(output_labeled_file).substr(0, std::string(output_labeled_file).find_last_of('.')) + ".png";
        cv::imwrite(pngOutput, bmpImage);
        std::cout << "Saved as PNG: " << pngOutput << std::endl;
    } else {
        std::cerr << "Failed to load BMP for PNG conversion: " << output_labeled_file << std::endl;
    }
    std::cout << "Saved: " << output_labeled_file << std::endl;
}

int main() {
    std::cout << "----- Homework 3 Menu -----\n";
    while (true) {
        std::cout << "\n[1] Task 1 (Generate IPM)\n"
                  << "[2] Task 2 (Detect Lanes on IPM)\n"
                  << "[0] Exit\n"
                  << "Select: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1 << 20, '\n');
            std::cout << "Invalid input. Please enter a number between 0 and 4.\n";
            continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1: task1("WM691d4b56543ac.bmp","task1.bmp"); break;
            case 2: task2("task1.bmp", "task2.bmp"); break;
            default: std::cout << "Unknown selection. Try 0-７.\n"; break; 
        }
    }
    return 0;
}
