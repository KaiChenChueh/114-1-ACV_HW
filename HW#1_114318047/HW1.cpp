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
    // Read readBMP(const char* filename)
    bmp::BMPImage img = bmp::readBMP(input);

    // write
    bmp::writeBMP(output, img);

    // Using OpenCV
    const char* output_opencv = "task1_opencv.bmp";
    cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);
    cv::imwrite(output_opencv, cvimg);
}

//Do a 270-degree clockwise rotation over the input image to generate the output imagestatic void task2()
static void task2(const char* input,const char* output)
{
    bmp::BMPImage img = bmp::readBMP(input);

    const int N = img.width;               // width == height
    const int rowSize = bmp::rowSizeBytes(N);

    // Buffer
    std::vector<uint8_t> out(rowSize * N);

    // (r, c) -> (c, N-1-r)
    for (int r = 0; r < N; ++r) {
        const uint8_t* srcRow = &img.data[r * rowSize];
        for (int c = 0; c < N; ++c) {
            const uint8_t* srcPx = &srcRow[c * 3];

            uint8_t* dstRow = &out[c * rowSize];
            uint8_t* dstPx  = &dstRow[(N - 1 - r) * 3];

            dstPx[0] = srcPx[0]; // B
            dstPx[1] = srcPx[1]; // G
            dstPx[2] = srcPx[2]; // R
        }
    }
    img.data.swap(out);
    bmp::writeBMP(output, img);

    // Using OpenCV
    // const char* output_opencv = "task2_opencv.bmp";
    // cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);
    // cv::rotate(cvimg, cvimg, cv::ROTATE_90_COUNTERCLOCKWISE);
    // cv::imwrite(output_opencv, cvimg);
}


// Interchange the channels of the rotated image,i.e.,R=>G,G=>B,B=>R
static void task3(const char* input,const char* output)
{
    // Read image
    bmp::BMPImage img = bmp::readBMP(input);

    // Copy image
    bmp::BMPImage img_copy = img;

    // distination image has the same size as source
    const int outW = img_copy.width;
    const int outH = img_copy.height;
    const int outRow = bmp::rowSizeBytes(outW);

    for (int r = 0; r < outH; ++r) { //height
        // point to first byte of row r
        uint8_t* RowPtr = &img_copy.data[r * outRow]; 
        for (int c = 0; c < outW; ++c) { //width
            // point at pixel (r,c)
            /*
                Ex: 
                img.data:
                twp pixels in a row(0,0) (0,1)
                pixel:  (0,0)    |  (0,1)
                index:  0 1 2       3 4 5 
                bytes: B0 G0 R0  | B1 G1 R1

                r=c=0=> img.data[0]
            */
            // point to first byte of pixel of row r, column c
            uint8_t* Px = &RowPtr[c * 3]; // B,G,R

            // Original channels
            uint8_t B = Px[0];
            uint8_t G = Px[1];
            uint8_t R = Px[2];

            // Interchange channels: R=>G, G=>B, B=>R
            Px[0] = R;
            Px[1] = B;
            Px[2] = G;
        }
    }

    // Write image
    bmp::writeBMP(output, img_copy);

    // Using OpenCV
    // const char* output_opencv = "task3_opencv.bmp";
    // cv::Mat cvimg = cv::imread(input, cv::IMREAD_UNCHANGED);

    // cv::Mat remapped(cvimg.size(), cvimg.type());

    // int from_to[] = { 2,0, 0,1, 1,2 };
    // cv::mixChannels(&cvimg, 1, &remapped, 1, from_to, 3);

    // cv::imwrite(output_opencv, remapped);

}

// Bonus
// Resize the image as double size and one-half size
static void task1_bounus()
{
    // Read
    bmp::BMPImage img = bmp::readBMP("test_image.bmp");

    const int srcW = img.width;
    const int srcH = img.height;
    const int srcRow = bmp::rowSizeBytes(srcW);   // includes 4-byte padding

    const int dstW_2x = srcW * 2;
    const int dstH_2x = srcH * 2;
    const int dstRow_2x = bmp::rowSizeBytes(dstW_2x);

    const int dstW_05x = srcW / 2;
    const int dstH_05x = srcH / 2;
    const int dstRow_05x = bmp::rowSizeBytes(dstW_05x);

    // Allocate destination pixel buffer with correct padded row size
    std::vector<uint8_t> out_2x(dstRow_2x * dstH_2x);
    std::vector<uint8_t> out_05x(dstRow_05x * dstH_05x);

    // 2x (Nearest Neighbor)
    for (int r = 0; r < srcH; ++r) {
        //each row has srcRow bytes
        const uint8_t* srcRowPtr = &img.data[r * srcRow]; 
        // img.data
        /*
            Ex: 3x3 image
            [B20][G20][R20] [B21][G21][R21] [B22][G22][R22] [padding]
            [B10][G10][R10] [B11][G11][R11] [B12][G12][R12] [padding]
            [B00][G00][R00] [B01][G01][R01] [B02][G02][R02] [padding] 
            img.data=> one row vector=>[B00][G00][R00][B01][G01][R01][B02][G02][R02][B10][G10][R10]...

            r=0=>srcRowPtr=&img.data[0*rowSize]=>srcRowPtr=&img.data[0]=[B00]
            => width=3,rowSizeByte=3*3=9+padding=12 
            r=0=>img.data[0*12]=img.data[0]=[B00]
            r=1=>img.data[1*srcRow]=img.data[1*12]=img.data[12]=[B10]
        */
        for (int c = 0; c < srcW; ++c) {
            const uint8_t* srcPx = &srcRowPtr[c * 3]; // B,G,R
            /*
                srcRowPtr[0]=B00    srcRowPtr[0]=B01 
                srcRowPtr[1]=G00    srcRowPtr[1]=G01
                srcRowPtr[2]=R00    srcRowPtr[2]=R01

                c=0=>B00=srcRowPtr[0] c=1=>B01=srcRowPtr[3] c=2=>B02=srcRowPtr[6]
                c=0=>srcPx=&srcRowPtr[0*3]=&srcRowPtr[0]=&img.data[0]=[B00]
                c=1=>srcPx=&srcRowPtr[1*3]=&srcRowPtr[3]=&img.data[3]=[B01]
                c=2=>srcPx=&srcRowPtr[2*3]=&srcRowPtr[6]=&img.data[6]=[B02]
            */
            const int dst_r = r * 2;
            const int dst_c = c * 2;

            uint8_t* dstRowPtr = &out_2x[dst_r * dstRow_2x];
            uint8_t* dstPx     = &dstRowPtr[dst_c * 3];
            /*
                resize to 2x=>6x6
                1 pixel in src image => 2x2 block in dst image
                [B12][G12][R12] [B13][G13][R13] [B14][G14][R14]   [B15][G15][R15] [B16][G16][R16] [B17][G17][R17] [padding]
                [B06][G06][R06] [B07][G07][R07] [B08][G08][R08]   [B09][G09][R09] [B10][G10][R10] [B11][G11][R11] [padding]
                [B00][G00][R00] [B01][G01][R01] [B02][G02][R02]   [B03][G03][R03] [B04][G04][R04] [B05][G05][R05] [padding] 
                row=6=>rowSizeByte=6*3=18+padding=20
                r=0,c=0=>B[00] r=1,c=0=>B[06] r=2,c=0=>B[12]
                r=0,c=1=>dst_c=2,dstRowPtr[2*3]=dstRowPtr[6]=B[02]
            */

            // Copy B, G, R to 4 pixels
            // Copy the same color into 2x2 block
            for (int dr = 0; dr < 2; ++dr) {
                for (int dc = 0; dc < 2; ++dc) {
                    uint8_t* p = &out_2x[(dst_r + dr) * dstRow_2x + (dst_c + dc) * 3];
                    // srcPx[0] srcPx[1] srcPx[2] are B,G,R in one pixel
                    /*
                        dr=dc=0=> p[0]=>&out_2x[0]=[B00]
                        dr=0,dc=1=> p[0]=>&out_2x[3]
                    */
                    p[0] = srcPx[0]; // B   
                    p[1] = srcPx[1]; // G
                    p[2] = srcPx[2]; // R
                }
            }
        }
    }

    // 0.5x dstH_05x = srcH / 2
    for (int r = 0; r < dstH_05x; ++r) {
        uint8_t* dstRowPtr = &out_05x[r * dstRow_05x];
        for (int c = 0; c < dstW_05x; ++c) {
            const int src_r = r * 2;
            const int src_c = c * 2;
            const uint8_t* srcRowPtr = &img.data[src_r * srcRow];
            const uint8_t* srcPx = &srcRowPtr[src_c * 3]; // B,G,R

            uint8_t* dstPx = &dstRowPtr[c * 3];
            // Copy B, G, R
            dstPx[0] = srcPx[0]; //img.data[0] = B
            dstPx[1] = srcPx[1];
            dstPx[2] = srcPx[2];
        }
    }
    
    // Update img to 2x size
    img.width  = dstW_2x;
    img.height = dstH_2x;
    img.data.swap(out_2x);

    bmp::BMPImage out;
    out.width  = dstW_05x;
    out.height = dstH_05x;
    out.data = std::move(out_05x);

    // write
    bmp::writeBMP("task1_bonus_2x.bmp", img);
    bmp::writeBMP("task1_bonus_0.5x.bmp", out);

    // repeat 1~3 for the resized image
    // task 1
    task1("task1_bonus_2x.bmp","task1_bonus_2x_copy.bmp");

    // task 2 
    // 270-degree rotation
    task2("task1_bonus_2x.bmp","task1_bonus_2x_rotated.bmp");

    // task 3
    task3("task1_bonus_2x_rotated.bmp","task1_bonus_2x_rotated_channel_interchanged.bmp");
}
// resize the image as 4096*4096
static void task2_bonus(const char* input,const char* output)
{
    bmp::BMPImage img = bmp::readBMP(input); //512x512

    const int srcH = img.height;
    const int srcW = img.width;
    const int srcRowByte = bmp::rowSizeBytes(srcW);

    const int dstH = img.height * 8;
    const int dstW = img.width * 8;
    const int dstRowByte = bmp::rowSizeBytes(dstW);

    std::vector<uint8_t> out(dstRowByte * dstH);
    for (int r = 0; r < dstH; ++r) {
        uint8_t* dstRowPtr = &out[r * dstRowByte];
        const int src_r = r / 8;
        for (int c = 0; c < dstW; ++c) {
            const int src_c = c / 8;
            const uint8_t* srcRowPtr = &img.data[src_r * srcRowByte];
            const uint8_t* srcPx = &srcRowPtr[src_c * 3]; // B,G,R

            uint8_t* dstPx = &dstRowPtr[c * 3];
            // Copy B, G, R
            dstPx[0] = srcPx[0]; //img.data[0] = B
            dstPx[1] = srcPx[1];
            dstPx[2] = srcPx[2];
        }
    }

    bmp::BMPImage out_img;
    out_img.width = dstW;
    out_img.height = dstH;
    out_img.data = std::move(out);
    bmp::writeBMP(output, out_img);

    // repeat 1~3 for the resized image
    // task 1
    task1(output,"task2_bonus_copy.bmp");
    // task 2
    task2(output,"task2_bonus_rotated.bmp");
    // task 3
    task3("task2_bonus_rotated.bmp","task2_bonus_rotated_channel_interchanged.bmp");

}

int main() {
    int choice;
    while (true) {
        std::cout << "\n================ Results Menu ================\n"
                  << " 1) Task 1: Read and Write BMP image\n"
                  << " 2) Task 2: Rotate BMP image 270-degree clockwise\n"
                  << " 3) Task 3: Interchange the channels of the rotated image\n"
                  << " 4) Task 1 Bonus: Resize the image as double size and one-half size\n"
                  << " 5) Task 2 Bonus: Resize the image as 4096*4096\n"
                  << " 0) Exit\n"
                  << "Enter the task number: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1 << 20, '\n');
            std::cout << "Invalid input. Please enter a number between 0 and 5.\n";
            continue;
        }
        if (choice == 0) break;

        switch (choice) {
            case 1: task1("test_image.bmp","task1.bmp"); break;
            case 2: task2("test_image.bmp","task2.bmp"); break;
            case 3: task3("task2.bmp","task3.bmp"); break;
            case 4: task1_bounus(); break;
            case 5: task2_bonus("test_image.bmp","task2_bonus.bmp"); break;
            case 0: return 0;
            default: std::cout << "Unknown selection. Try 0-4.\n"; break;
        }
    }
}
