#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

#pragma pack(pop)

void change_contrast(unsigned char *data, LONG width, LONG height, float factor) {
    for (LONG i = 0; i < width * height * 3; i++) {
        float pixel_normalized = data[i] / 255.0;
        data[i] = (unsigned char)round(pow(pixel_normalized, factor) * 255.0);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s [imagefile1] [outputfile] [contrastfactor]\n", argv[0]);
        return 1;
    }

    char *input_filename = argv[1];
    char *output_filename = argv[2];
    float contrast_factor = atof(argv[3]);

    FILE *input_file = fopen(input_filename, "rb");
    if (!input_file) {
        perror("Error opening input file");
        return 1;
    }

    BITMAPFILEHEADER file_header;
    fread(&file_header, sizeof(BITMAPFILEHEADER), 1, input_file);

    BITMAPINFOHEADER info_header;
    fread(&info_header, sizeof(BITMAPINFOHEADER), 1, input_file);

    LONG width = info_header.biWidth;
    LONG height = info_header.biHeight;
    DWORD image_size = width * height * 3;

    unsigned char *data = (unsigned char *)malloc(image_size);

    fread(data, 1, image_size, input_file);
    fclose(input_file);

    change_contrast(data, width, height, contrast_factor);

    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Error opening output file");
        return 1;
    }

    fwrite(&file_header, sizeof(BITMAPFILEHEADER), 1, output_file);
    fwrite(&info_header, sizeof(BITMAPINFOHEADER), 1, output_file);
    fwrite(data, 1, image_size, output_file);
    fclose(output_file);

    free(data);

    return 0;
}

/*
cd ~/Documents/Github/csc-357/lab2/contrast.c

*/