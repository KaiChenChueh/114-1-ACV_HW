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
* Filename    : HW2.cpp
* Author      : Stanley Chueh
* Note        : ACV HW2
*********************************************************/

// Helpers functions

// Task 1 Helpers
// Check if it's white(B=255,G=255,R=255)
static bool isWhite(const bmp::BMPImage& img, int rowSize, int r, int c) {
    const uint8_t* px = &img.data[r * rowSize + c * 3];
    return (px[0] == 255 && px[1] == 255 && px[2] == 255); 
}

// Set pixel to black
static void setBlack(bmp::BMPImage& img, int rowSize, int r, int c) {
    uint8_t* px = &img.data[r * rowSize + c * 3];
    px[0] = px[1] = px[2] = 0; // B,G,R
}

// BFS subfunction for connected component analysis
static void bfs(const bmp::BMPImage& img, int rowSize, int width, int height, std::vector<int>& visited, int sr, int sc, std::vector<std::pair<int, int>>& component_pixels, bool targetWhite)
{
    // 4-neighbors
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = { 0, 0, -1, 1};

    std::queue<std::pair<int, int>> q;
    q.push({sr, sc});

    // occupy starting pixel
    visited[sr * width + sc] = 1;

    while (!q.empty()) {
        auto front = q.front();
        int r = front.first;
        int c = front.second;

        q.pop();

        // Add to component pixels
        component_pixels.push_back({r, c});

        for (int k = 0; k < 4; ++k) {
            int nr = r + dr[k];
            int nc = c + dc[k];

            // Check bounds
            if (nr < 0 || nr >= height || nc < 0 || nc >= width)
                continue;

            int index = nr * width + nc;

            // If already visited, skip
            if (visited[index]) continue;

            // Check pixel color
            const uint8_t* np = &img.data[nr * rowSize + nc * 3];
            bool isWhite = (np[0] == 255 && np[1] == 255 && np[2] == 255);
            bool isBlack = (np[0] == 0 && np[1] == 0 && np[2] == 0);

            // Based on targetWhite flag
            if ((targetWhite && isWhite) || (!targetWhite && isBlack)) {
                // occupy and enqueue
                visited[index] = 1;
                q.push({nr, nc});
            }
        }
    }
}

// Task3 helpers
// Dilation
static void apply_dilatation(const bmp::BMPImage& img, bmp::BMPImage& dilated, int kernel_size) {
    const int width = img.width;
    const int height = img.height;
    const int rowSize = bmp::rowSizeBytes(width);

    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {

            // dilate_pixel indicates whether to set current pixel to white
            bool dilate_pixel = false;

            // initial kernel_r, kernel_c is -center of kernel
            for (int kernel_r = -kernel_size / 2; kernel_r <= kernel_size / 2; ++kernel_r) {
                for (int kernel_c = -kernel_size / 2; kernel_c <= kernel_size / 2; ++kernel_c) {

                    // neighbor row and column
                    int neighbor_r = r + kernel_r, neighbor_c = c + kernel_c;

                    // check bounds
                    if (neighbor_r >= 0 && neighbor_r < height && neighbor_c >= 0 && neighbor_c < width) {

                        // np points to neighbor pixel
                        const uint8_t* neighbor_pixel = &img.data[neighbor_r * rowSize + neighbor_c * 3];
                        if (neighbor_pixel[0] == 255 && neighbor_pixel[1] == 255 && neighbor_pixel[2] == 255) { // white pixel

                            // set current pixel to white
                            dilate_pixel = true;
                            break;
                        }
                    }
                }

                // Break, if already decided to dilate
                if (dilate_pixel) 
                    break;
            }

            // dilate_p points to current pixel in dilated image
            uint8_t* dilate_p = &dilated.data[r * rowSize + c * 3];
            if (dilate_pixel) {
                dilate_p[0] = 255; dilate_p[1] = 255; dilate_p[2] = 255; // Set to white
            } else {
                dilate_p[0] = 0; dilate_p[1] = 0; dilate_p[2] = 0; // Set to black
            }
        }
    }
}

// Erosion
static void apply_erosion(const bmp::BMPImage& img, bmp::BMPImage& eroded, int kernel_size) {
    const int width = img.width;
    const int height = img.height;
    const int rowSize = bmp::rowSizeBytes(width);

    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            bool erode_pixel = false;

            // initial kernel_r, kernel_c is -center of kernel
            for (int kernel_r = -kernel_size / 2; kernel_r <= kernel_size / 2; ++kernel_r) {
                for (int kernel_c = -kernel_size / 2; kernel_c <= kernel_size / 2; ++kernel_c) {
                    // neighbor row and column
                    int nr = r + kernel_c, nc = c + kernel_c;
                    if (nr >= 0 && nr < height && nc >= 0 && nc < width) {
                        const uint8_t* neighbor_pixel = &img.data[nr * rowSize + nc * 3];
                        if (neighbor_pixel[0] == 0 && neighbor_pixel[1] == 0 && neighbor_pixel[2] == 0) { // black pixel
                            erode_pixel = true;
                            break;
                        }
                    }
                }
                if (erode_pixel) break;
            }
            uint8_t* erode_p = &eroded.data[r * rowSize + c * 3];
            if (erode_pixel) {
                erode_p[0] = 0; erode_p[1] = 0; erode_p[2] = 0; // Set to black
            } else {
                erode_p[0] = 255; erode_p[1] = 255; erode_p[2] = 255; // Set to white
            }
        }
    }
}

// Draw bounding box
static void drawBoundingBox(bmp::BMPImage& img, int minR, int minC, int maxR, int maxC, uint8_t rColor, uint8_t gColor, uint8_t bColor, int thickness = 2)
{
    const int width = img.width;
    const int height = img.height;
    const int rowSize = bmp::rowSizeBytes(width);

    for (int t = 0; t < thickness; ++t) {

        // Horizontal edges (top and bottom)
        for (int x = minC - t; x <= maxC + t; ++x) {
            if (x >= 0 && x < width) {
                if (minR - t >= 0) {
                    // top edge
                    uint8_t* top = &img.data[(minR - t) * rowSize + x * 3];
                    top[0] = bColor; top[1] = gColor; top[2] = rColor;
                }
                if (maxR + t < height) {
                    uint8_t* bottom = &img.data[(maxR + t) * rowSize + x * 3];
                    bottom[0] = bColor; bottom[1] = gColor; bottom[2] = rColor;
                }
            }
        }

        // Vertical edges (left and right)
        for (int y = minR - t; y <= maxR + t; ++y) {
            if (y >= 0 && y < height) {
                if (minC - t >= 0) {
                    uint8_t* left = &img.data[y * rowSize + (minC - t) * 3];
                    left[0] = bColor; left[1] = gColor; left[2] = rColor;
                }
                if (maxC + t < width) {
                    uint8_t* right = &img.data[y * rowSize + (maxC + t) * 3];
                    right[0] = bColor; right[1] = gColor; right[2] = rColor;
                }
            }
        }
    }
}

// Task1
static void task1(const char* input, const char* output)
{
    // Generate a binarized image of road using intensity, color information and area filtering

    // Read image
    bmp::BMPImage img = bmp::readBMP(input);
    const int width = img.width;
    const int height = img.height;
    const int rowSize = bmp::rowSizeBytes(width);

    // Thresholds for road detection(color, intensity, area)
    const int road_intensity_threshold = 98; // Intensity threshold
    const int MIN_AREA = 900; // Minimum area for connected components 400

    // Process each pixel(Filter by color and intensity first)
    for (int r = 0; r < height; ++r) {
        uint8_t* rowPtr = &img.data[r * rowSize];
        for (int c = 0; c < width; ++c) {
            uint8_t* px = &rowPtr[c * 3]; // B, G, R

            // Calculate average intensity
            int average_intensity = ((int)px[0] + (int)px[1] + (int)px[2]) / 3;

            // Binarization condition for road using intensity and color thresholds
            if (average_intensity > road_intensity_threshold) {
                // white (road)
                px[0] = 255;   // B
                px[1] = 255;   // G
                px[2] = 255;   // R
            } else {
                // black (forest)
                px[0] = 0; // B
                px[1] = 0; // G
                px[2] = 0; // R
            }
        }
    }

    // Connected Component Analysis to remove small components (area filtering)
    std::vector<int> visited(width * height);

    for (int sr = 0; sr < height; ++sr) {
        for (int sc = 0; sc < width; ++sc) {
            // If not visited and is white pixel, start BFS
            if (!visited[sr * width + sc] && isWhite(img, rowSize, sr, sc)) {
                std::vector<std::pair<int, int>> component_pixels;
                bfs(img, rowSize, width, height, visited, sr, sc, component_pixels,true);

                // If component area is less than MIN_AREA, set all its pixels to black
                if ((int)component_pixels.size() < MIN_AREA) {
                    for (std::pair<int, int>& p : component_pixels) {
                        setBlack(img, rowSize, p.first, p.second);
                    }
                }
            }
        }
    }

    // Write image
    bmp::writeBMP(output, img);
    std::cout << "Binarized image saved as task1.bmp\n";
}

// Label the forest regions with 4-connected neighbors, assign unique colors, and draw bounding boxes
// Label components on mask, draw boxes on original
static void task2(const char* maskPath, const char* originalPath, const char* output)
{
    bmp::BMPImage mask = bmp::readBMP(maskPath);
    bmp::BMPImage original = bmp::readBMP(originalPath);

    const int width  = mask.width;
    const int height = mask.height;
    const int rowSize = bmp::rowSizeBytes(width);

    if (original.width != width || original.height != height)
        throw std::runtime_error("mask and original size mismatch");

    std::vector<int> visited(width * height);
    std::vector<std::tuple<int,int,int,int>> boxes;

    const int MIN_FOREST_AREA = 5000; // Minimum area to keep forest region

    // Scan for unvisited black pixels
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            // if not visited and is black pixel
            if (!visited[r * width + c]) {

                // px points to the pixel at (r, c) in mask
                uint8_t* px = &mask.data[r * rowSize + c * 3];

                // If it's a black pixel
                if (px[0] == 0 && px[1] == 0 && px[2] == 0) {
                    std::vector<std::pair<int, int>> component_pixels;

                    // Perform BFS to find all connected black pixels
                    bfs(mask, rowSize, width, height, visited, r, c, component_pixels, false);

                    // Find bounding box
                    /*
                        Find the min and max row and column indices among the component(component_pixels) pixels
                    */
                    int minR = height, maxR = -1, minC = width, maxC = -1;

                    // iterates through every pixel coordinate (row, column) in the component
                    for (const auto& p : component_pixels) {
                        if (p.first < minR) minR = p.first;
                        if (p.first > maxR) maxR = p.first;
                        if (p.second < minC) minC = p.second;
                        if (p.second > maxC) maxC = p.second;
                    }

                    // Only keep large enough black regions
                    if ((int)component_pixels.size() >= MIN_FOREST_AREA)
                    
                        // Add one new bounding box into vector.
                        boxes.emplace_back(minR, minC, maxR, maxC);
                }
            }
        }
    }

    // Draw bounding boxes and label centroids
    for (int i = 0; i < (int)boxes.size(); ++i) {
        int minR, minC, maxR, maxC;
        std::tie(minR, minC, maxR, maxC) = boxes[i];

        // label with different colors
        uint8_t rColor = (i % 3 == 0) ? 255 : 0;
        uint8_t gColor = (i % 3 == 1) ? 255 : 0;
        uint8_t bColor = (i % 3 == 2) ? 255 : 0;

        drawBoundingBox(original, minR, minC, maxR, maxC, rColor, gColor, bColor, 2);

        // Calculate centroid and area
        int centroidR = (minR + maxR) / 2;
        int centroidC = (minC + maxC) / 2;
        int area = (maxR - minR + 1) * (maxC - minC + 1);

        std::cout << "Box " << i + 1 << ": Centroid=(" << centroidC << "," << centroidR
                  << "), Area=" << area << "\n";

        // Mark centroid on the image(A cross)
        const int centroidSize = 5; // Size of the cross for the centroid
        for (int dr = -centroidSize; dr <= centroidSize; ++dr) {
            for (int dc = -centroidSize; dc <= centroidSize; ++dc) {
                if ((dr == 0 || dc == 0) && centroidR + dr >= 0 && centroidR + dr < height && centroidC + dc >= 0 && centroidC + dc < width) {
                    uint8_t* centroidPx = &original.data[(centroidR + dr) * rowSize + (centroidC + dc) * 3];
                    centroidPx[0] = 0; centroidPx[1] = 255; centroidPx[2] = 255; 
                }
            }
        }
    }

    bmp::writeBMP(output, original);
    std::cout << "Filtered and labeled forest regions saved as " << output << "\n";
}

static void task3(const char* input, const char* output, bool analyzeTime)
{
    using namespace std::chrono;

    // Use average intensity to filter first, then apply morphological operations
    // Read image
    bmp::BMPImage img = bmp::readBMP(input);
    const int width = img.width;
    const int height = img.height;
    const int rowSize = bmp::rowSizeBytes(width);

    // Timing variables
    auto start = high_resolution_clock::now();
    auto stage1_start = start;

    // Stage 1: Binarizing
    const int intensity_threshold = 110;
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            uint8_t* px = &img.data[r * rowSize + c * 3];
            int average_intensity = ((int)px[0] + (int)px[1] + (int)px[2]) / 3;

            if (average_intensity < intensity_threshold) {
                px[0] = 0; px[1] = 0; px[2] = 0; // Set to black
            } else {
                px[0] = 255; px[1] = 255; px[2] = 255; // Set to white
            }
        }
    }

    // Stage 1: Binarizing - END
    auto stage1_end = high_resolution_clock::now();

    // Stage 2: Morphological operations
    auto stage2_start = high_resolution_clock::now();
    const int kernel_size = 3;

    // Do Opening
    bmp::BMPImage eroded = img; // Copy for erosion

    // Erosion
    apply_erosion(img, eroded, kernel_size);

    bmp::BMPImage dilated = eroded; // Copy for first dilation

    // call dilate function
    apply_dilatation(eroded, dilated, kernel_size);

    // one more Dilation to restore road width
    bmp::BMPImage temp = dilated; // Copy for second dilation

    // Dilation
    apply_dilatation(dilated, temp, kernel_size+4);

    dilated = temp; // Update dilated image

    // Stage 2: Morphological operations
    auto stage2_end = high_resolution_clock::now();

    // Stage 3: Connected Component Analysis and Area Filtering
    auto stage3_start = high_resolution_clock::now();
    const int MIN_ROAD_AREA = 2000; // Minimum area for road components
    std::vector<int> visited(width * height);

    for (int sr = 0; sr < height; ++sr) {
        for (int sc = 0; sc < width; ++sc) {

            // If not visited
            if (!visited[sr * width + sc]) {
                uint8_t* px = &dilated.data[sr * rowSize + sc * 3];
                if (px[0] == 255 && px[1] == 255 && px[2] == 255) { // white pixel
                    std::vector<std::pair<int, int>> component_pixels;
                    
                    // Perform BFS to find all connected white pixels
                    bfs(dilated, rowSize, width, height, visited, sr, sc, component_pixels, true);

                    // If component area is less than MIN_ROAD_AREA, set all its pixels to black
                    if ((int)component_pixels.size() < MIN_ROAD_AREA) {
                        for (std::pair<int, int>& p : component_pixels) {
                            setBlack(dilated, rowSize, p.first, p.second);
                        }
                    }
                }
            }
        }
    }

    // Stage 3: Connected Component Analysis and Area Filtering - END
    auto stage3_end = high_resolution_clock::now();

    // Stage 4: Property Analysis
    auto stage4_start = high_resolution_clock::now();

    // Reset visited for bounding box drawing
    std::fill(visited.begin(), visited.end(), 0);

    std::vector<std::tuple<int, int, int, int>> boundingBoxes; // Store bounding boxes
    std::vector<int> componentAreas; // Store areas of components

    for (int sr = 0; sr < height; ++sr) {
        for (int sc = 0; sc < width; ++sc) {

            // If not visited
            if (!visited[sr * width + sc]) {
                uint8_t* px = &dilated.data[sr * rowSize + sc * 3];
                if (px[0] == 255 && px[1] == 255 && px[2] == 255) { // white pixel
                    std::vector<std::pair<int, int>> component_pixels;
                    bfs(dilated, rowSize, width, height, visited, sr, sc, component_pixels, true);

                    // Find bounding box
                    int minR = height, maxR = -1, minC = width, maxC = -1;
                    for (const auto& p : component_pixels) {
                        if (p.first < minR) minR = p.first;
                        if (p.first > maxR) maxR = p.first;
                        if (p.second < minC) minC = p.second;
                        if (p.second > maxC) maxC = p.second;
                    }

                    // Store bounding box and area
                    boundingBoxes.emplace_back(minR, minC, maxR, maxC);
                    componentAreas.push_back((int)component_pixels.size());
                }
            }
        }
    }

    // Stage 4: Property Analysis - END
    auto stage4_end = high_resolution_clock::now();

    // Stage 5: Draw Bounding Boxes
    auto stage5_start = high_resolution_clock::now();

    for (size_t i = 0; i < boundingBoxes.size(); ++i) {
        int minR, minC, maxR, maxC;
        std::tie(minR, minC, maxR, maxC) = boundingBoxes[i];

        // Draw bounding box in red
        drawBoundingBox(dilated, minR, minC, maxR, maxC, 255, 0, 0, 2);

        // Output component properties
        std::cout << "Component " << i + 1 << ": Area = " << componentAreas[i]
                  << ", Bounding Box = [(" << minR << ", " << minC << "), ("
                  << maxR << ", " << maxC << ")]\n";
    }

    // Stage 5: Draw Bounding Boxes - END
    auto stage5_end = high_resolution_clock::now();

    // Extra: Find the length and orientation of longest axis, and draw it
    std::vector<std::pair<int, int>> borderPoints;
    
    // Find white dots on the border with a margin of 5 pixels
    const int margin = 5;
    for (int r = 0; r < dilated.height; ++r) {
        for (int c = 0; c < dilated.width; ++c) {
            // r < margin: top border
            // c < margin: left border
            // c >= width - margin: right border
            // r >= height - margin: bottom border
            if ((r < margin || r >= dilated.height - margin || c < margin || c >= dilated.width - margin)) {
                // px points to the pixel at (r, c) in dilated image
                uint8_t* px = &dilated.data[r * rowSize + c * 3];
                if (px[0] == 255 && px[1] == 255 && px[2] == 255) { // white pixel
                    borderPoints.emplace_back(r, c); // Store as (row, column)
                }
            }
        }
    }
    double maxDist = 0.0;
    std::pair<int, int> p1, p2;

    // Find the two border points with the maximum distance
    for (size_t i = 0; i < borderPoints.size(); ++i) {
        for (size_t j = i + 1; j < borderPoints.size(); ++j) {

            // std::hypot(x, y) = sqrt(x^2 + y^2) => euclidean distance
            double dist = std::hypot(borderPoints[i].first - borderPoints[j].first, borderPoints[i].second - borderPoints[j].second);
            if (dist > maxDist) {
                maxDist = dist;
                p1 = borderPoints[i];
                p2 = borderPoints[j];
            }   
        }
    }

    // arctan2 to find angle in degrees
    double angle = std::atan2(p2.first - p1.first, p2.second - p1.second) * 180.0 / M_PI;
    std::cout << "Longest axis length: " << maxDist << "\n";
    std::cout << "Orientation (degrees): " << angle << "\n";

    // Draw the longest axis on the image
    uint8_t* p1Px = &dilated.data[p1.first * rowSize + p1.second * 3];
    uint8_t* p2Px = &dilated.data[p2.first * rowSize + p2.second * 3];

    // Use Bresenham's line algorithm to draw the line
    // https://zh.wikipedia.org/zh-tw/%E5%B8%83%E9%9B%B7%E6%A3%AE%E6%BC%A2%E5%A7%86%E7%9B%B4%E7%B7%9A%E6%BC%94%E7%AE%97%E6%B3%95
    // Find the line between p1 and p2
    int x1 = p1.second, y1 = p1.first;  // row & column
    int x2 = p2.second, y2 = p2.first; // row & column

    // Compute direction and absolute distance
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);

    // Determine step direction (left/right, up/down)
    int stepX = (x1 < x2) ? 1 : -1;
    int stepY = (y1 < y2) ? 1 : -1;

    // Initialize error 
    int err = dx - dy;

    while (true) {
        // Set to green
        uint8_t* pixel = &dilated.data[y1 * rowSize + x1 * 3];
        pixel[0] = 0; pixel[1] = 255; pixel[2] = 0;  

        // If reached the end point, stop
        if (x1 == x2 && y1 == y2) break;

        // Compute doubled error value
        int err2 = 2 * err;

        // Move horizontally if needed
        if (err2 > -dy) {
            err -= dy;
            x1 += stepX;
        }

        // Move vertically if needed
        if (err2 < dx) {
            err += dx;
            y1 += stepY;
        }
    }

    // Write final image
    bmp::writeBMP(output, dilated);

    // Output timing information and time complexity analysis
    if (!analyzeTime) 
        return;

    auto stage1_duration = duration_cast<microseconds>(stage1_end - stage1_start).count();
    auto stage2_duration = duration_cast<microseconds>(stage2_end - stage2_start).count();
    auto stage3_duration = duration_cast<microseconds>(stage3_end - stage3_start).count();
    auto stage4_duration = duration_cast<microseconds>(stage4_end - stage4_start).count();
    auto stage5_duration = duration_cast<microseconds>(stage5_end - stage5_start).count();
    auto total_duration  =  duration_cast<microseconds>(stage5_end - start).count();

    std::cout << "Task 3 Timing Information:\n";
    std::cout << " Stage 1 (Binarization): " << stage1_duration << " us\n";
    std::cout << " Stage 2 (Morphological Operations): " << stage2_duration << " us\n";
    std::cout << " Stage 3 (Connected Component Analysis): " << stage3_duration << " us\n";
    std::cout << " Stage 4 (Property Analysis): " << stage4_duration << " us\n";
    std::cout << " Stage 5 (Bounding Box Drawing): " << stage5_duration << " us\n";
    std::cout << " Total Time: " << total_duration << " us\n";

    // Time complexity analysis
    std::cout << "\nTime Complexity Analysis:\n";
    std::cout << " Stage 1 (Binarization): O(H * W)\n";
    std::cout << " Stage 2 (Morphological Operations): O(H * W * K^2), where K is the kernel size\n";
    std::cout << " Stage 3 (Connected Component Analysis): O(H * W)\n";
    std::cout << " Stage 4 (Property Analysis): O(H * W)\n";
    std::cout << " Stage 5 (Bounding Box Drawing): O(H * W)\n";
    std::cout << " Overall Time Complexity: O(H * W * K^2), as morphological operations dominate\n";
}

static void task4()
{
    task3("Ian_island_square.bmp","task3.bmp",true);
}   
int main() {
    std::cout << "----- Homework 2 Menu -----\n";
    while (true) {
        std::cout << "\n================ Results Menu ================\n"
                  << " 1) Task 1  - Generate binarized road\n"
                  << " 2) Task 2  - Label forest + bounding boxes\n"
                  << " 3) Task 3  - Road extraction + orientation\n"
                  << " 4) Task 4  - Analyze timing\n"
                  << " 0) Exit\n"
                  << "Enter the question number: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1 << 20, '\n');
            std::cout << "Invalid input. Please enter a number between 0 and 4.\n";
            continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1: task1("Ian_island_square.bmp","task1.bmp"); break;
            case 2: task2("task1.bmp","Ian_island_square.bmp","task2.bmp"); break;
            case 3: task3("Ian_island_square.bmp","task3.bmp",false); break;
            case 4: task4(); break;
            default: std::cout << "Unknown selection. Try 0-７.\n"; break; 
        }
    }
    return 0;
}
