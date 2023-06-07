#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>
#define BMP_H

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;

#pragma pack(push, 1)
typedef struct tagBITMAPFILEHEADER
    {
    WORD bfType; //specifies the file type
    DWORD bfSize; //specifies the size in bytes of the bitmap file
    WORD bfReserved1; //reserved; must be 0
    WORD bfReserved2; //reserved; must be 0
    DWORD bfOffBits; //species the offset in bytes from the bitmapfileheader to the bitmap bits
    } BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
    {
    DWORD biSize; //specifies the number of bytes required by the struct
    LONG biWidth; //specifies width in pixels
    LONG biHeight; //species height in pixels
    WORD biPlanes; //specifies the number of color planes, must be 1 
    WORD biBitCount; //specifies the number of bit per pixel
    DWORD biCompression;//spcifies the type of compression 
    DWORD biSizeImage; //size of image in bytes
    LONG biXPelsPerMeter; //number of pixels per meter in x axis 
    LONG biYPelsPerMeter; //number of pixels per meter in y axis 
    DWORD biClrUsed; //number of colors used by th ebitmap 
    DWORD biClrImportant; //number of colors that are important
    } BITMAPINFOHEADER;

#pragma pack(pop)

typedef struct tagRGB_VALUES
    {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
    } RGB_VALUES;

typedef struct tagRGB_VALUES_FLOAT
    {
    float rgbtBlue;
    float rgbtGreen;
    float rgbtRed;
    } RGB_VALUES_FLOAT;


RGB_VALUES get_color(RGB_VALUES *image, int x, int y, int width)
    {
    return image[y * width + x];
    }


void write_bmp(char *filename, BITMAPFILEHEADER *bf, BITMAPINFOHEADER *bi, RGB_VALUES *image)
{
    FILE *file = fopen(filename, "wb");

    fwrite(bf, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(bi, sizeof(BITMAPINFOHEADER), 1, file);
    int padding = (4 - (bi->biWidth * sizeof(RGB_VALUES)) % 4) % 4;

    unsigned char pad[3] = {0, 0, 0};
        for (int i = 0; i < bi->biHeight; i++)
        {
            fwrite(&image[i * bi->biWidth], sizeof(RGB_VALUES), bi->biWidth, file);
            fwrite(pad, 1, padding, file);
        }
        fclose(file);
}


void read_image(const char *filename, BITMAPFILEHEADER *fileheader, BITMAPINFOHEADER *infoheader, RGB_VALUES **image)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error: Unable to open file %s\n", filename);
        exit(1);
    }

    fread(fileheader, sizeof(BITMAPFILEHEADER), 1, file);
    fread(infoheader, sizeof(BITMAPINFOHEADER), 1, file);

    *image = malloc(sizeof(RGB_VALUES) * infoheader->biWidth * infoheader->biHeight);
    if (*image == NULL)
    {
        printf("Error: Unable to allocate memory for image\n");
        fclose(file);
        exit(1);
    }

    fread(*image, sizeof(RGB_VALUES), infoheader->biWidth * infoheader->biHeight, file);

    fclose(file);

    printf("Read image %s\n", filename);
}


void normalize_colors(RGB_VALUES *image, RGB_VALUES_FLOAT *image_f, int width, int height)
{
    for (int i = 0; i < width*height; i++)
    {
        // printf("Before normalization: R=%u, G=%u, B=%u\n", image[i].rgbtRed, image[i].rgbtGreen, image[i].rgbtBlue);
        image_f[i].rgbtRed = image[i].rgbtRed / 255.0;
        image_f[i].rgbtGreen = image[i].rgbtGreen / 255.0;
        image_f[i].rgbtBlue = image[i].rgbtBlue / 255.0;
        // printf("After normalization: R=%f, G=%f, B=%f\n", image_f[i].rgbtRed, image_f[i].rgbtGreen, image_f[i].rgbtBlue);
    }
    // printf("Colors normalized.\n");
}


void scale_and_cast_to_byte(RGB_VALUES_FLOAT *image, RGB_VALUES *result, int width, int height)
{
    // loop over each pixel in the image then multiply the colors by 0.03 and then by 255, and cast to a byte
    for (int i = 0; i < width*height; i++)
    {
        // printf("Before scaling: R=%f, G=%f, B=%f\n", image[i].rgbtRed, image[i].rgbtGreen, image[i].rgbtBlue);
        result[i].rgbtRed = (unsigned char)(image[i].rgbtRed * 0.03 * 255);
        result[i].rgbtGreen = (unsigned char)(image[i].rgbtGreen * 0.03 * 255);
        result[i].rgbtBlue = (unsigned char)(image[i].rgbtBlue * 0.03 * 255);
        // printf("After scaling: R=%u, G=%u, B=%u\n", result[i].rgbtRed, result[i].rgbtGreen, result[i].rgbtBlue);
    }
    // printf("Colors scaled and cast to byte.\n");
}


void matrix_multiply(RGB_VALUES_FLOAT *image1, RGB_VALUES_FLOAT *image2, RGB_VALUES_FLOAT *result, int width, int height, int par_id, int par_count, int *ready)
{
    int start_row = (height / par_count) * par_id;
    int end_row = (par_id == par_count - 1) ? height : start_row + (height / par_count);

    for (int i = start_row; i < end_row; i++)
    {
        for (int j = 0; j < width; j++)
        {
            result[i*width + j].rgbtRed = 0;
            result[i*width + j].rgbtGreen = 0;
            result[i*width + j].rgbtBlue = 0;

            for (int k = 0; k < width; k++)
            {
                /*
                printf("Before multiplication: R1=%f, G1=%f, B1=%f, R2=%f, G2=%f, B2=%f\n", 
                image1[i*width + k].rgbtRed, image1[i*width + k].rgbtGreen, image1[i*width + k].rgbtBlue,
                image2[k*width + j].rgbtRed, image2[k*width + j].rgbtGreen, image2[k*width + j].rgbtBlue);
                */

                result[i*width + j].rgbtRed += image1[i*width + k].rgbtRed * image2[k*width + j].rgbtRed;
                result[i*width + j].rgbtGreen += image1[i*width + k].rgbtGreen * image2[k*width + j].rgbtGreen;
                result[i*width + j].rgbtBlue += image1[i*width + k].rgbtBlue * image2[k*width + j].rgbtBlue;

                /*
                printf("After multiplication: R=%f, G=%f, B=%f\n", 
                result[i*width + j].rgbtRed, result[i*width + j].rgbtGreen, result[i*width + j].rgbtBlue);
                */
            }
        }
    }
    ready[par_id]++;
    while (1) {
        int done = 1;
        for (int i = 0; i < par_count; i++) {
            if (ready[i] < 1) {
                done = 0;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    // printf("Matrix multiplication completed by process %d\n", par_id);
}


void synch(int par_id, int par_count, int *ready, int sync)
{
    ready[par_id] = sync + 1;

    // Wait for all instances to reach this point
    while (1)
    {
        int done = 1;
        for (int i = 0; i < par_count; i++)
        {
            if (ready[i] < sync + 1)
            {
                done = 0;
                break;
            }
        }
        if (done)
        {
            break;
        }
    }

    ready[par_id] = sync + 2;

    // Wait for all instances to finish waiting
    while (1)
    {
        int done = 1;
        for (int i = 0; i < par_count; i++)
        {
            if (ready[i] < sync + 2)
            {
                done = 0;
                break;
            }
        }
        if (done)
        {
            break;
        }
    }
}



int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <parallel id> <parallel count>\n", argv[0]);
        return 1;
    }

    int par_id = atoi(argv[1]);
    int par_count = atoi(argv[2]);

    // error handling
    if (par_id < 0 || par_id >= par_count)
    {
        printf("Error: Invalid parallel id or count\n");
        return 1;
    }

    clock_t start, end;
    start = clock();

    BITMAPFILEHEADER fileheader1, fileheader2;
    BITMAPINFOHEADER infoheader1, infoheader2;
    RGB_VALUES *image1, *image2;
    RGB_VALUES_FLOAT *image_f1, *image_f2;
    read_image("f1.bmp", &fileheader1, &infoheader1, &image1);
    read_image("f2.bmp", &fileheader2, &infoheader2, &image2);

    image_f1 = malloc(sizeof(RGB_VALUES_FLOAT) * infoheader1.biWidth * infoheader1.biHeight);
    image_f2 = malloc(sizeof(RGB_VALUES_FLOAT) * infoheader2.biWidth * infoheader2.biHeight);

    if (image_f1 == NULL || image_f2 == NULL)
    {
        printf("Error: Unable to allocate memory for float images\n");
        return 1;
    }

    normalize_colors(image1, image_f1, infoheader1.biWidth, infoheader1.biHeight);
    normalize_colors(image2, image_f2, infoheader2.biWidth, infoheader2.biHeight);

    RGB_VALUES_FLOAT *result_f = malloc(sizeof(RGB_VALUES_FLOAT) * infoheader1.biWidth * infoheader1.biHeight);
    RGB_VALUES *result = malloc(sizeof(RGB_VALUES) * infoheader1.biWidth * infoheader1.biHeight);
    if (result_f == NULL || result == NULL)
    {
        printf("Error: Unable to allocate memory for result image\n");
        return 1;
    }

    int shm_fd = shm_open("ready", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(int) * par_count);
    int *ready = mmap(0, sizeof(int) * par_count, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    for (int i = 0; i < par_count; i++)
    {
        ready[i] = 0;
    }

    matrix_multiply(image_f1, image_f2, result_f, infoheader1.biWidth, infoheader1.biHeight, par_id, par_count, ready);

    synch(par_id, par_count, ready, 0);

    scale_and_cast_to_byte(result_f, result, infoheader1.biWidth, infoheader1.biHeight);

    write_bmp("output.bmp", &fileheader1, &infoheader1, result);

    free(image1);
    free(image2);
    free(result_f);
    free(result);

    if (par_id == 0)
    {
        shm_unlink("ready");
    }

    end = clock();
    double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
    printf("Matrix multiplication took %f seconds to execute \n", time_taken);

    return 0;
}

/*

Testing:
./mpi ./matrix 1
./mpi ./matrix 2
./mpi ./matrix 4
./mpi ./matrix 8

*/