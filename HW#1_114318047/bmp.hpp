#pragma once
#include <cstdint> // for uint16_t, uint32_t
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>

namespace bmp {

// note:
// #pragma is for diable member allignment
//EX:
/*
    struct Example {
        uint16_t a; // 2 bytes
        uint32_t b; // 4 bytes
    };
    without #pargma, the total size is 8 bytes (a(2+2, cause padding) + b(4))
    with #pragma, the total size is 6 bytes (a(2) + b(4))
    and total size matters in the later file read/write

    the alternative way of #pragma is to use __attribute__((packed)) like
    struct __attribute__((packed)) BMPHeader {
        uint16_t bfType;
        uint32_t bfSize;
        uint16_t bfReserved1;
        uint16_t bfReserved2;
        uint32_t bfOffBits;
    };

    Usage:
    #pragma pack(push, 1) 

    struct BMPHeader {
        ...
    };
    #pragma pack(pop)
*/

// BMP file header (14 bytes) + DIB header (40 bytes)
#pragma pack(push, 1)

// BMP file header (14 bytes)
/*
based on WM68d3b04ca3853.pdf, we can know the size of each member in BMPHeader
*/
struct BMPHeader {
    uint16_t bfType;      //2 bytes
    uint32_t bfSize;      
    uint16_t bfReserved1; 
    uint16_t bfReserved2; 
    uint32_t bfOffBits;   
};

// DIB header (BITMAPINFOHEADER, 40 bytes)
struct BMPInfoHeader {
    uint32_t biSize;          
    int32_t  biWidth;   // define as int32_t to support negative height (top-down)     
    int32_t  biHeight;  // same as above
    uint16_t biPlanes;        
    uint16_t biBitCount;      
    uint32_t biCompression;   
    uint32_t biSizeImage;     
    int32_t  biXPelsPerMeter; 
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;       
    uint32_t biClrImportant;  
};
#pragma pack(pop)

struct BMPImage {
    int width = 0;
    int height = 0;                 
    std::vector<uint8_t> data;      // BGR pixel data (with row padding)
};

// readBMP will be defined in bmp.cpp
BMPImage readBMP(const char* filename);

// writeBMP will be defined in bmp.cpp
void writeBMP(const char* filename, const BMPImage& img);

inline int rowSizeBytes(int width) {
    int rowSize = width * 3;
    int padding = (4 - (rowSize % 4)) % 4; 
    return rowSize += padding; // each row padded to multiple of 4 bytes
}
/*
    rowSize % 4 => how many bytes left to be multiple of 4
    4 - (rowSize % 4) => how many bytes needed to be multiple of 4
    (4 - (rowSize % 4)) % 4 => if rowSize is already multiple of 4, then padding should be 0, not 4
*/

} // namespace bmp
