#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <vector>

using namespace cv;
using namespace std;
using namespace std::chrono;

/********************************************************
* Filename    : HW2_opencv.cpp
* Author      : Stanley Chueh
* Note        : ACV HW2 OpenCV Version
*********************************************************/

static void drawBoundingBox(Mat& img, int minR, int minC, int maxR, int maxC, Scalar color = Scalar(0, 0, 255), int thickness = 2)
{
    rectangle(img, Point(minC, minR), Point(maxC, maxR), color, thickness);
} 

// Task1
static void task1(const string& input, const string& output)
{
    Mat img = imread(input);
    if (img.empty()) 
        throw runtime_error("Cannot open image!");

    // Convert to grayscale
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    // Threshold by intensity and color
    Mat mask = Mat::zeros(img.size(), CV_8UC1);
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            // Vec3b: [B, G, R]
            Vec3b px = img.at<Vec3b>(r, c);

            // Average intensity
            int intensity = (px[0] + px[1] + px[2]) / 3;
            if (intensity > 98 && px[0] > 80 && px[1] > 80 && px[2] > 80)
                // Make it white(road)
                mask.row(r).col(c).setTo(255);
        }
    }

    // Connected components to remove small regions
    Mat labels, stats, centroids;

    /*
        int connectedComponentsWithStats(
            InputArray  image,      
            OutputArray labels,     
            OutputArray stats,      // Region properties
            OutputArray centroids,  // Region centers
            int connectivity = 8,   // default 8-connected
            int ltype = CV_32S      // label image type (integer)
        );
    */
    int number_of_connected_components = connectedComponentsWithStats(mask, labels, stats, centroids, 4);

    // std::cout<< "Number of connected components: " << nLabels - 1 << std::endl;

    for (int i = 1; i < number_of_connected_components; ++i) {
        // CC_STAT_AREA = 4(4-neighbor)
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area < 900)
            // Make it black(small component)
            mask.setTo(0, labels == i);
    }

    imwrite(output, mask);
    std::cout << "Binarized image saved as task1_opencv.bmp\n";
}

// Task2
static void task2(const string& maskPath, const string& originalPath, const string& output)
{
    // Read task1_opencv.bmp as mask
    Mat mask = imread(maskPath, IMREAD_GRAYSCALE);

    Mat original = imread(originalPath);
    if (mask.empty() || original.empty())
        throw runtime_error("Cannot open images!");

    Mat invForestMask;

    // invert mask: forest regions are white for connected components
    threshold(mask, invForestMask, 128, 255, THRESH_BINARY_INV); 

    Mat labels, stats, centroids;
    int number_of_connected_components = connectedComponentsWithStats(invForestMask, labels, stats, centroids, 4);

    const int MIN_FOREST_AREA = 5000;

    int colorIndex = 0; // To ensure all colors are used
    for (int i = 1; i < number_of_connected_components; ++i) {
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area < MIN_FOREST_AREA) continue; //skip it

        int minC = stats.at<int>(i, CC_STAT_LEFT);
        int minR = stats.at<int>(i, CC_STAT_TOP);
        int width = stats.at<int>(i, CC_STAT_WIDTH);
        int height = stats.at<int>(i, CC_STAT_HEIGHT);

        Scalar bboxColor;
        // Assign different colors for bounding boxes
        if (colorIndex % 3 == 0)
            bboxColor = Scalar(0, 0, 255); // Red
        else if (colorIndex % 3 == 1)
            bboxColor = Scalar(0, 255, 0); // Green
        else
            bboxColor = Scalar(255, 0, 0); // Blue

        drawBoundingBox(original, minR, minC, minR + height, minC + width, bboxColor, 2);

        // Centroid
        int cx = centroids.at<double>(i, 0);
        int cy = centroids.at<double>(i, 1);
        circle(original, Point(cx, cy), 4, Scalar(255, 255, 0), -1);

        cout << "Forest " << i << ": Centroid=(" << cx << "," << cy
             << "), Area=" << area << endl;

        colorIndex++; // For the next color
    }

    imwrite(output, original);
    cout << "Task2 complete: saved " << output << endl;
}

// Task3
static void task3(const string& input, const string& output, bool analyzeTime)
{
    Mat img = imread(input);
    if (img.empty()) throw runtime_error("Cannot open image!");
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    // Stage 1 — Binarization
    auto start = high_resolution_clock::now();
    auto s1 = start;

    Mat binary;
    // Pixel with intensity > 110 will be set to 255
    // THRESH_BINARY: thresholding type
    threshold(gray, binary, 110, 255, THRESH_BINARY);

    // Stage 1 — Binarization - END
    auto s1_end = high_resolution_clock::now();

    // Stage 2 — Morphological operations (open + dilate)
    auto s2 = high_resolution_clock::now();

    // MORPH_RECT: 3x3 rectangular structuring element
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    Mat opened, dilated;

    morphologyEx(binary, opened, MORPH_OPEN, kernel);
    dilate(opened, dilated, getStructuringElement(MORPH_RECT, Size(7, 7)));

    // Stage 2 — Morphological operations (open + dilate) - END
    auto s2_end = high_resolution_clock::now();

    // Stage 3 — Connected components filter small roads
    auto s3 = high_resolution_clock::now();

    Mat labels, stats, centroids;
    int number_of_connected_components = connectedComponentsWithStats(dilated, labels, stats, centroids, 4);
    const int MIN_ROAD_AREA = 2000;

    for (int i = 1; i < number_of_connected_components; ++i) {
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area < MIN_ROAD_AREA)
            dilated.setTo(0, labels == i);
    }

    // Stage 3 — Connected components filter small roads - END
    auto s3_end = high_resolution_clock::now();

    // Stage 4 — Property analysis
    auto s4 = high_resolution_clock::now();

    Mat vis;
    cvtColor(dilated, vis, COLOR_GRAY2BGR);

    for (int i = 1; i < number_of_connected_components; ++i) {
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area < MIN_ROAD_AREA) continue;

        int minC = stats.at<int>(i, CC_STAT_LEFT);
        int minR = stats.at<int>(i, CC_STAT_TOP);
        int width = stats.at<int>(i, CC_STAT_WIDTH);
        int height = stats.at<int>(i, CC_STAT_HEIGHT);

        // Locate the region to label
        int cx = centroids.at<double>(i, 0);
        int cy = centroids.at<double>(i, 1);
    }

    // Stage 4 — Property analysis - END
    auto s4_end = high_resolution_clock::now();

    // Stage 5 — Bounding box drawing
    auto s5 = high_resolution_clock::now();
    for (int i = 1; i < number_of_connected_components; ++i) {
        int area = stats.at<int>(i, CC_STAT_AREA);
        if (area < MIN_ROAD_AREA) continue;

        int minC = stats.at<int>(i, CC_STAT_LEFT);
        int minR = stats.at<int>(i, CC_STAT_TOP);
        int width = stats.at<int>(i, CC_STAT_WIDTH);
        int height = stats.at<int>(i, CC_STAT_HEIGHT);

        drawBoundingBox(vis, minR, minC, minR + height, minC + width, Scalar(0, 0, 255), 2);
    }
    auto s5_end = high_resolution_clock::now();
    // Stage 5 — Bounding box drawing - END

    // Finding longest axis and orientation
    vector<Point> borderPts;
    int margin = 5;
    for (int r = 0; r < vis.rows; ++r)
        for (int c = 0; c < vis.cols; ++c)
            if ((r < margin || r >= vis.rows - margin || c < margin || c >= vis.cols - margin) && dilated.at<uchar>(r, c) == 255)
                borderPts.emplace_back(c, r);

    double maxDist = 0;
    Point p1, p2;
    for (size_t i = 0; i < borderPts.size(); ++i)
        for (size_t j = i + 1; j < borderPts.size(); ++j) {
            double dist = norm(borderPts[i] - borderPts[j]);
            if (dist > maxDist) {
                maxDist = dist;
                p1 = borderPts[i];
                p2 = borderPts[j];
            }
        }

    double angle = 180 - atan2(p2.y - p1.y, p2.x - p1.x) * 180.0 / CV_PI;
    cout << "Longest axis length: " << maxDist << endl;
    cout << "Orientation (deg): " << angle << endl;

    line(vis, p1, p2, Scalar(0, 255, 0), 2);
    circle(vis, p1, 3, Scalar(0, 0, 255), -1);
    circle(vis, p2, 3, Scalar(0, 0, 255), -1);

    imwrite(output, vis);

    if (analyzeTime) {
        cout << "\nTiming (ms):\n";
        cout << " Stage1 Binarization: " << duration_cast<microseconds>(s1_end - s1).count() << " µs\n";
        cout << " Stage2 Morphology:   " << duration_cast<microseconds>(s2_end - s2).count() << " µs\n";
        cout << " Stage3 Components:   " << duration_cast<microseconds>(s3_end - s3).count() << " µs\n";
        cout << " Stage4 Analysis:     " << duration_cast<microseconds>(s4_end - s4).count() << " µs\n";
        cout << " Stage5 BoundingBox:  " << duration_cast<microseconds>(s5_end - s5).count() << " µs\n";
        cout << " Total:               " << duration_cast<microseconds>(s5_end - start).count() << " µs\n";
    }

    cout << "Task3 complete: saved " << output << endl;
}

// Task4
static void task4() {
    task3("Ian_island_square.bmp", "task3_opencv.bmp", true);
}

int main()
{
    cout << "----- Homework 2 Menu (OpenCV version) -----\n";
    while (true) {
        cout << "\n================ Results Menu ================\n"
             << " 1) Task 1  - Generate binarized road\n"
             << " 2) Task 2  - Label forest + bounding boxes\n"
             << " 3) Task 3  - Road extraction + orientation\n"
             << " 4) Task 4  - Analyze timing\n"
             << " 0) Exit\n"
             << "Enter choice: ";

        int choice;
        cin >> choice;
        if (choice == 0) break;

        switch (choice) {
        case 1: task1("Ian_island_square.bmp", "task1_opencv.bmp"); break;
        case 2: task2("task1_opencv.bmp", "Ian_island_square.bmp", "task2_opencv.bmp"); break;
        case 3: task3("Ian_island_square.bmp", "task3_opencv.bmp", false); break;
        case 4: task4(); break;
        default: cout << "Invalid choice.\n"; break;
        }
    }
    return 0;
}
