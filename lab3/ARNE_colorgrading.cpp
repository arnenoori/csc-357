#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include "bmp.h"

void color_grade(Bitmap& bmp, float red, float green, float blue, int start_row, int end_row);
void read_bitmap(const char* filename, Bitmap& bmp);
void write_bitmap(const char* filename, const Bitmap& bmp);

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " [IMAGEFILE] [COLOR_GRADING] [OUTPUTFILE]" << std::endl;
        return 1;
    }

    const char* input_filename = argv[1];
    float red = std::stof(argv[2]);
    float green = std::stof(argv[3]);
    float blue = std::stof(argv[4]);
    const char* output_filename = argv[5];

    Bitmap bmp;
    read_bitmap(input_filename, bmp);

    auto start = std::chrono::high_resolution_clock::now();

    // Create shared memory
    size_t size = bmp.height * bmp.row_padded;
    unsigned char* shared_data = (unsigned char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memcpy(shared_data, bmp.data, size);
    // Fork process
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Error: fork() failed" << std::endl;
        return 1;
    }

    int start_row, end_row;
    if (pid == 0) { // Child process
        start_row = bmp.height / 2;
        end_row = bmp.height;
    } else { // Parent process
        start_row = 0;
        end_row = bmp.height / 2;
    }

    // Perform color grading
    color_grade(bmp, red, green, blue, start_row, end_row);

    // Wait for child process to finish
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time (with fork): " << elapsed.count() << " seconds" << std::endl;

    // Save the result
    write_bitmap(output_filename, bmp);

    // Cleanup
    munmap(shared_data, size);

    return 0;
}

void color_grade(Bitmap& bmp, float red, float green, float blue, int start_row, int end_row) {
    for (int row = start_row; row < end_row; ++row) {
        for (int col = 0; col < bmp.width; ++col) {
            int index = row * bmp.row_padded + col * 3;
            float normalized_blue = bmp.data[index] / 255.0f;
            float normalized_green = bmp.data[index + 1] / 255.0f;
            float normalized_red = bmp.data[index + 2] / 255.0f;

            bmp.data[index] = std::min(255, static_cast<int>(normalized_blue * blue * 255));
            bmp.data[index + 1] = std::min(255, static_cast<int>(normalized_green * green * 255));
            bmp.data[index + 2] = std::min(255, static_cast<int>(normalized_red * red * 255));
        }
    }
}

void read_bitmap(const char* filename, Bitmap& bmp) {
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        exit(1);
    }

    // Read file header
    file.read((char*)&bmp.file_header, sizeof(BITMAPFILEHEADER));

    // Read info header
    file.read((char*)&bmp.info_header, sizeof(BITMAPINFOHEADER));

    // Validate that it's a BMP file
    if (bmp.file_header.bfType != 0x4D42) {
        std::cerr << "Error: Invalid BMP file: " << filename << std::endl;
        exit(1);
    }

    // Read the data
    bmp.width = bmp.info_header.biWidth;
    bmp.height = bmp.info_header.biHeight;
    bmp.row_padded = (bmp.width * 3 + 3) & (~3);
    bmp.data = new unsigned char[bmp.height * bmp.row_padded];
    file.read((char*)bmp.data, bmp.height * bmp.row_padded);

    file.close();
}

void write_bitmap(const char* filename, const Bitmap& bmp) {
    std::ofstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        exit(1);
    }

    // Write file header
    file.write((char*)&bmp.file_header, sizeof(BITMAPFILEHEADER));

    // Write info header
    file.write((char*)&bmp.info_header, sizeof(BITMAPINFOHEADER));

    // Write the data
    file.write((char*)bmp.data, bmp.height * bmp.row_padded);

    file.close();
}

// cd ~/Documents/Github/csc-357/lab3
// g++ -o colorgrading ARNE_colorgrading.cpp
// ./colorgrading input.bmp red green blue output.bmp
