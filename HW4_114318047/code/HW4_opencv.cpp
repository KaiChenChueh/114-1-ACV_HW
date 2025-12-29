#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>

static void task1(const std::string& puzzle, const std::string& templ, const std::string& output)
{
    cv::Mat src = cv::imread(puzzle);
    cv::Mat tpl = cv::imread(templ);

    if (src.empty() || tpl.empty()) {
        std::cout << "Image load failed\n";
        return;
    }

    // turns to grayscale
    cv::Mat graySrc, grayTpl;
    cv::cvtColor(src, graySrc, cv::COLOR_BGR2GRAY);
    cv::cvtColor(tpl, grayTpl, cv::COLOR_BGR2GRAY);

    cv::Mat result;
    // template matching(CCOEFF_NORMED=normalized cross correlation)
    cv::matchTemplate(graySrc, grayTpl, result, cv::TM_CCOEFF_NORMED);

    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    // minMaxLoc to find the best match location
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    // draw rectangle on the detected area
    cv::rectangle(src,maxLoc,cv::Point(maxLoc.x + tpl.cols, maxLoc.y + tpl.rows),cv::Scalar(0, 0, 255),2);

    cv::imwrite(output, src);
    cv::imshow("Task1 Result", src);
    cv::waitKey(0);
}

static void task2(const std::string& puzzle, const std::string& templ, const std::string& output)
{
    cv::Mat src = cv::imread(puzzle);
    cv::Mat tpl = cv::imread(templ);

    if (src.empty() || tpl.empty()) {
        std::cout << "Image load failed\n";
        return;
    }

    // turns to grayscale
    cv::Mat graySrc, grayTpl;
    cv::cvtColor(src, graySrc, cv::COLOR_BGR2GRAY);
    cv::cvtColor(tpl, grayTpl, cv::COLOR_BGR2GRAY);

    double threshold = 0.4;  // Threshold for detection 0.42

    // Multi-scale template matching
    for (double scale = 0.5; scale <= 1.5; scale += 0.1) {
        cv::Mat resizedTpl;
        cv::resize(grayTpl, resizedTpl, cv::Size(), scale, scale);

        // Skip if the resized template is larger than the source image
        if (resizedTpl.cols >= graySrc.cols ||
            resizedTpl.rows >= graySrc.rows)
            continue;
            
        // Template matching
        cv::Mat result;
        cv::matchTemplate(graySrc, resizedTpl, result, cv::TM_CCOEFF_NORMED);

        double minVal, maxVal;
        cv::Point minLoc, maxLoc;
        cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

        int matchCount = 0;

        // Draw rectangles for matches above the threshold
        for (int y = 0; y < result.rows; y++) {
            for (int x = 0; x < result.cols; x++) {
                if (result.at<float>(y, x) >= threshold) {
                    matchCount++; 
                    cv::rectangle(src,cv::Point(x, y), cv::Point(x + resizedTpl.cols, y + resizedTpl.rows), cv::Scalar(0, 255, 0),2);
                }
            }
        }

        std::cout << "scale = " << scale
            << "  maxVal = " << maxVal
            << "  matches = " << matchCount << std::endl;
    }

    cv::imwrite(output, src);

    // Display (resized)
    cv::Mat display;
    cv::resize(src, display, cv::Size(), 0.5, 0.5);
    cv::imshow("Task2 Result", display);
    cv::waitKey(0);
}

int main() {
    std::cout << "----- Homework 4 (OpenCV Version) -----\n";
    while (true) {
        std::cout << "\n[1] Task 1 Solve jigsaw puzzles\n"
                  << "[2] Task 2 Find the pieces of jigsaw puzzle\n"
                  << "[0] Exit\n"
                  << "Select: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear(); std::cin.ignore(1000, '\n'); continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1:
                task1("task1_puzzle.bmp","task1_template.bmp","task1_opencv.bmp"); break;
            case 2:
                task2("task2_puzzle.bmp","task2_template.bmp","task2_opencv.bmp"); break;
            default: 
                std::cout << "Invalid choice.\n"; 
                break;
        }
    }
    return 0;
}