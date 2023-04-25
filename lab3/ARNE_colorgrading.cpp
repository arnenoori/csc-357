#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include <semaphore.h>
#include "bmp.h"

void color_grade(unsigned char* data, int width, int row_padded, float red, float green, float blue, int start_row, int end_row);
void read_bitmap(const char* filename, Bitmap& bmp);
void write_bitmap(const char* filename, const Bitmap& bmp);

sem_t sem_parent;
sem_t sem_child;

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " [IMAGEFILE] [RED] [GREEN] [BLUE] [OUTPUTFILE]" << std::endl; // what we type to enter the values
        return 1;
    }

    const char* input_filename = argv[1];
    float red = std::stof(argv[2]);
    float green = std::stof(argv[3]);
    float blue = std::stof(argv[4]);
    const char* output_filename = argv[5];

    // Perform color grading without fork
    Bitmap bmp_without_fork;
    read_bitmap(input_filename, bmp_without_fork);
    
    auto start_without_fork = std::chrono::high_resolution_clock::now();

    color_grade(bmp_without_fork.data, bmp_without_fork.width, bmp_without_fork.row_padded, red, green, blue, 0, bmp_without_fork.height);

    auto end_without_fork = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_without_fork = end_without_fork - start_without_fork;
    std::cout << "Elapsed time (without fork): " << elapsed_without_fork.count() << " seconds" << std::endl;

    // Save the result without fork
    write_bitmap(output_filename, bmp_without_fork);

    // Perform color grading with fork
    Bitmap bmp;
    read_bitmap(input_filename, bmp);

    auto start = std::chrono::high_resolution_clock::now();

    /// creating shared memory
    size_t size = bmp.height * bmp.row_padded;
    unsigned char* shared_data = (unsigned char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memcpy(shared_data, bmp.data, size);

    // create semaphores for parent and child
    sem_init(&sem_parent, 1, 0);
    sem_init(&sem_child, 1, 0);

    // fork process
    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Error: fork() failed" << std::endl; // if process fails
        return 1;
    }

    int start_row, end_row;
    if (pid == 0) { // child process
        sem_post(&sem_parent); // signal parent process
        start_row = bmp.height / 2;
        end_row = bmp.height;
    } else { // parent process
        sem_wait(&sem_parent); // wait for child process to start
        start_row = 0;
        end_row = bmp.height / 2;
    }

    // perform color grading
    color_grade(shared_data, bmp.width, bmp.row_padded, red, green, blue, start_row, end_row);

    if (pid == 0) { // child process
        sem_post(&sem_child); // signal parent process that child is done
        munmap(shared_data, size);
        sem_destroy(&sem_parent);
        sem_destroy(&sem_child);
        exit(0);
    } else { // parent process
        sem_wait(&sem_child); // wait for child process to finish
        memcpy(bmp.data, shared_data, size);
        munmap(shared_data, size);
        sem_destroy(&sem_parent);
        sem_destroy(&sem_child);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time (with fork): " << elapsed.count() << " seconds" << std::endl;

    // save the result
    write_bitmap(output_filename, bmp);

    munmap(shared_data, size);

    return 0;
}


void color_grade(unsigned char* data, int width, int row_padded, float red, float green, float blue, int start_row, int end_row) {
    for (int row = start_row; row < end_row; ++row) {
        for (int col = 0; col < width; ++col) {
            int index = row * row_padded + col * 3;
            float normalized_blue = data[index] / 255.0f;
            float normalized_green = data[index + 1] / 255.0f;
            float normalized_red = data[index + 2] / 255.0f;

            data[index] = std::min(255, static_cast<int>(normalized_blue * blue * 255));
            data[index + 1] = std::min(255, static_cast<int>(normalized_green * green * 255));
            data[index + 2] = std::min(255, static_cast<int>(normalized_red * red * 255));
        }
    }
}


void read_bitmap(const char* filename, Bitmap& bmp) {
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        exit(1);
    }

    // read file header
    file.read((char*)&bmp.file_header, sizeof(BITMAPFILEHEADER));

    // read info header
    file.read((char*)&bmp.info_header, sizeof(BITMAPINFOHEADER));

    // validate that it's a BMP file
    if (bmp.file_header.bfType != 0x4D42) {
        std::cerr << "Error: Invalid BMP file: " << filename << std::endl;
        exit(1);
    }

    // read the file data
    bmp.width = bmp.info_header.biWidth;
    bmp.height = bmp.info_header.biHeight;
    bmp.row_padded = (bmp.width * 3 + 3) & (~3);
    bmp.data = new unsigned char[bmp.height * bmp.row_padded]; // if they're padded
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

// How to run: 
// cd ~/Documents/Github/csc-357/lab3
// g++ -std=c++11 -o colorgrading ARNE_colorgrading.cpp
// ./colorgrading input.bmp red green blue output.bmp (Example)
// ./colorgrading lion.bmp 0.8 1.0 0.8 result.bmp

// Initial bug fixing
// test child and process, delete greens on 1 end and blues on one end, debug by changing the values of some