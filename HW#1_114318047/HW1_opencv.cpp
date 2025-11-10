#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <opencv2/opencv.hpp>
#include "bmp.hpp"  // declares bmp::BMPImage, bmp::readBMP, bmp::writeBMP
/*
    bottom-up
    the first row of the image pixel data is the bottom row of the image
*/
//write a program that to implement "bmp" format image reading and writing.
static void task1(const char* input,const char* output)
{
    // Using OpenCV
    cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);
    cv::imwrite(output, cvimg);
}

//Do a 270-degree clockwise rotation over the input image to generate the output imagestatic void task2()
static void task2(const char* input,const char* output)
{
    // Using OpenCV
    cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);
    cv::rotate(cvimg, cvimg, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::imwrite(output, cvimg);
}


// Interchange the channels of the rotated image,i.e.,R=>G,G=>B,B=>R
static void task3(const char* input,const char* output)
{
    // Using OpenCV
    cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);

    cv::Mat remapped(cvimg.size(), cvimg.type());

    int from_to[] = { 2,0, 0,1, 1,2 };
    cv::mixChannels(&cvimg, 1, &remapped, 1, from_to, 3);

    cv::imwrite(output, remapped);

}

int main() {
    int choice;
    while (true) {
        std::cout << "\n================ Results Menu ================\n"
                  << " 1) Task 1: Read and Write BMP image\n"
                  << " 2) Task 2: Rotate BMP image 270-degree clockwise\n"
                  << " 3) Task 3: Interchange the channels of the rotated image\n"
                  << " 0) Exit\n"
                  << "Enter the task number: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1 << 20, '\n');
            std::cout << "Invalid input. Please enter a number between 0 and 3.\n";
            continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1: task1("test_image.bmp","task1_opencv.bmp"); break;
            case 2: task2("test_image.bmp","task2_opencv.bmp"); break;
            case 3: task3("task2_opencv.bmp","task3_opencv.bmp"); break;
            case 0: return 0;
            default: std::cout << "Unknown selection. Try 0-3.\n"; break;
        }
    }
}









