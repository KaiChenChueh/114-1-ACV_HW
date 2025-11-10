#include "bmp.hpp" //define BMPImage, readBMP, writeBMP
#include <iostream>
#include <cmath>

// readBMP, writeBMP
namespace bmp {

BMPImage readBMP(const char* filename) {
    
    // open file
    FILE* input_file = fopen(filename, "rb");
    if (!input_file) {
        throw std::runtime_error("Cannot open file: " + std::string(filename));
    }

    BMPHeader header{}; //{} to initialize all members to zero BMPHeader
    BMPInfoHeader info{}; //BMPInfoHeader

    fread(&header, sizeof(header), 1, input_file);
    fread(&info, sizeof(info), 1, input_file);

    const int width = info.biWidth;
    // biHeight < 0 means top-down bitmap
    int height = info.biHeight;
    // height could be negative when the bitmap is top-down(BGR data stored from top to bottom)
    /*
        top-down: bottom-up:
        row0      row2
        row1      row1
        row2      row0

        it just the difference of the order of rows, the result are the same
    */
    const int absHeight = std::abs(height);
    const int rowSize = rowSizeBytes(width); //define in bmp.hpp, rowSizeBytes is for calculating the row size with padding(has to be multiple of 4 bytes)
    const int dataSize = rowSize * absHeight;

    std::vector<uint8_t> data(dataSize);

    fseek(input_file, header.bfOffBits, SEEK_SET); // BfOffBits is the offset to the pixel data (First byte of pixel data)
    fread(data.data(), 1, dataSize, input_file); // SEEK_SET means from the beginning of the file
    fclose(input_file);                          // SEEK_SET:reference point, header.bfOffBits:offset, SEEK_SET:from the beginning of the file

    // if height is negative, means the data is stored in top-down order, we need to flip it to bottom-up order
    if (height < 0) {
        // new buffer to store flipped data
        std::vector<uint8_t> flipped(dataSize);

        for (int y = 0; y < absHeight; ++y) {

            // pointer variables src, *src point to the beginning of each row
            const uint8_t* src = &data[y * rowSize];
            // absHeight - 1 - y: flip the row order
            // EX:total 3 rows, y=0
            // original row0 -> flipped row(absHeight - 1 - 0) = row(absHeight - 1) = row2
            // dst point to the beginning of each row
            uint8_t* dst = &flipped[(absHeight - 1 - y) * rowSize];
            std::copy(src, src + rowSize, dst); 
            // data is the original data, flipped is the new buffer to store flipped data
            // src+rowSize: copy the whole row
            //std::copy will copy the data from src to dst
        }
        data.swap(flipped); // tmp = data; data = flipped; flipped = tmp;
        height = absHeight; // Convert to positive value
    }

    BMPImage out;
    out.width = width;
    out.height = height;
    out.data = std::move(data); // std::move(data)(move to the new BMPImage object)

    return out;
}

void writeBMP(const char* filename, const BMPImage &img) {
    const int rowSize = rowSizeBytes(img.width);
    const int expectedSize = rowSize * img.height;
    
    BMPHeader header{};
    BMPInfoHeader info{};

    header.bfType = 0x4D42; //BMP 'BM' Ascii 'B' = 0x42, 'M' = 0x4D
    header.bfOffBits = sizeof(BMPHeader) + sizeof(BMPInfoHeader);//14+40=54
    header.bfSize = header.bfOffBits + expectedSize;
    header.bfReserved1 = 0;
    header.bfReserved2 = 0;

    info.biSize = sizeof(BMPInfoHeader);
    info.biWidth = img.width;
    info.biHeight = img.height; 
    info.biPlanes = 1;
    info.biBitCount = 24; // 24 bits per pixel (3 bytes: BGR)
    info.biCompression = 0;
    info.biSizeImage = expectedSize;
    info.biXPelsPerMeter = 2835;
    info.biYPelsPerMeter = 2835;
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    // std::ofstream file(filename, std::ios::binary);
    // if (!file)
    //     throw std::runtime_error("Cannot open for write: " + filename);

    // file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    // file.write(reinterpret_cast<const char*>(&info), sizeof(info));
    // file.write(reinterpret_cast<const char*>(img.data.data()), expectedSize);

    // if (!file)
    //     throw std::runtime_error("Failed to write BMP file: " + filename);

    // file.close();
    FILE* output_file = fopen(filename, "wb");

    fwrite(&header, sizeof(header), 1, output_file);
    fwrite(&info, sizeof(info), 1, output_file);
    fwrite(img.data.data(), 1, expectedSize, output_file);
    fclose(output_file);
}

} // namespace bmp
