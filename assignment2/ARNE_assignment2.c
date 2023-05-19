#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct tagRGBTRIPLE
    {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
    } RGBTRIPLE;


RGBTRIPLE get_pixel(RGBTRIPLE *image, int x, int y, int width)
    {
    return image[y * width + x];
    }


void set_pixel(RGBTRIPLE *image, int x, int y, int width, RGBTRIPLE color)
    {
    image[y * width + x] = color;
    }


RGBTRIPLE blend_colors(RGBTRIPLE color1, RGBTRIPLE color2, float ratio)
    {
    RGBTRIPLE result;
    result.rgbtBlue = color1.rgbtBlue * ratio + color2.rgbtBlue * (1 - ratio);
    result.rgbtGreen = color1.rgbtGreen * ratio + color2.rgbtGreen * (1 - ratio);
    result.rgbtRed = color1.rgbtRed * ratio + color2.rgbtRed * (1 - ratio);
    return result;
    }


RGBTRIPLE bilinear_interpolation(RGBTRIPLE *image, float x, float y, int width, int height)
    {
    int x1 = (int)x;
    int y1 = (int)y;
    int x2 = x1 + 1 < width ? x1 + 1 : x1;
    int y2 = y1 + 1 < height ? y1 + 1 : y1;

    RGBTRIPLE q11 = get_pixel(image, x1, y1, width);
    RGBTRIPLE q12 = get_pixel(image, x1, y2, width);
    RGBTRIPLE q21 = get_pixel(image, x2, y1, width);
    RGBTRIPLE q22 = get_pixel(image, x2, y2, width);

    float r1 = x2 == x1 ? 0 : x2 - x;
    float r2 = x2 == x1 ? 1 : x - x1;
    float t1 = y2 == y1 ? 0 : y2 - y;
    float t2 = y2 == y1 ? 1 : y - y1;

    RGBTRIPLE result;
    result.rgbtBlue = (q11.rgbtBlue * r1 + q21.rgbtBlue * r2) * t1 + (q12.rgbtBlue * r1 + q22.rgbtBlue * r2) * t2;
    result.rgbtGreen = (q11.rgbtGreen * r1 + q21.rgbtGreen * r2) * t1 + (q12.rgbtGreen * r1 + q22.rgbtGreen * r2) * t2;
    result.rgbtRed = (q11.rgbtRed * r1 + q21.rgbtRed * r2) * t1 + (q12.rgbtRed * r1 + q22.rgbtRed * r2) * t2;

    return result;
    }


RGBTRIPLE *read_bmp(char *filename, BITMAPFILEHEADER *bf, BITMAPINFOHEADER *bi)
    {
    FILE *file = fopen(filename, "rb");

    fread(bf, sizeof(BITMAPFILEHEADER), 1, file);
    fread(bi, sizeof(BITMAPINFOHEADER), 1, file);

    // print the bfType and biBitCount values
    printf("bfType: %u\n", bf->bfType);
    printf("biBitCount: %u\n", bi->biBitCount);

    if (bf->bfType != 0x4D42 || bi->biBitCount != 24)
        {
        printf("Unsupported file format.\n");
        fclose(file);
        return NULL;
        }

    int padding = (4 - (bi->biWidth * sizeof(RGBTRIPLE)) % 4) % 4;

    RGBTRIPLE *image = malloc(bi->biWidth * bi->biHeight * sizeof(RGBTRIPLE));

    fseek(file, bf->bfOffBits, SEEK_SET);
    for (int i = 0; i < bi->biHeight; i++)
        {
            fread(&image[i * bi->biWidth], sizeof(RGBTRIPLE), bi->biWidth, file);
            fseek(file, padding, SEEK_CUR);
        }

    fclose(file);
    return image;
    }


void write_bmp(char *filename, BITMAPFILEHEADER *bf, BITMAPINFOHEADER *bi, RGBTRIPLE *image)
    {
    FILE *file = fopen(filename, "wb");

    fwrite(bf, sizeof(BITMAPFILEHEADER), 1, file);
    fwrite(bi, sizeof(BITMAPINFOHEADER), 1, file);
    int padding = (4 - (bi->biWidth * sizeof(RGBTRIPLE)) % 4) % 4;

    unsigned char pad[3] = {0, 0, 0};
        for (int i = 0; i < bi->biHeight; i++)
        {
            fwrite(&image[i * bi->biWidth], sizeof(RGBTRIPLE), bi->biWidth, file);
            fwrite(pad, 1, padding, file);
        }
        fclose(file);
    }


int main(int argc, char *argv[]) 
    {
    if (argc != 5) 
        {
        printf("Usage: %s [imagefile1] [imagefile2] [ratio] [outputfile]\n", argv[0]);
        return 1;
        }

    char *imagefile1 = argv[1];
    char *imagefile2 = argv[2];
    float ratio = atof(argv[3]);
    char *outputfile = argv[4];

    BITMAPFILEHEADER bf1, bf2;
    BITMAPINFOHEADER bi1, bi2;
    RGBTRIPLE *image1 = read_bmp(imagefile1, &bf1, &bi1);
    RGBTRIPLE *image2 = read_bmp(imagefile2, &bf2, &bi2);

    // compute the dimensions of the output image
    int out_width = bi1.biWidth > bi2.biWidth ? bi1.biWidth : bi2.biWidth;
    int out_height = bi1.biHeight > bi2.biHeight ? bi1.biHeight : bi2.biHeight;

    float scale_x1 = (float) bi1.biWidth / out_width;
    float scale_y1 = (float) bi1.biHeight / out_height;
    float scale_x2 = (float) bi2.biWidth / out_width;
    float scale_y2 = (float) bi2.biHeight / out_height;

    // allocate memory for the output image
    RGBTRIPLE *out_image = malloc(out_width * out_height * sizeof(RGBTRIPLE));
    if (!out_image)
        {
        printf("Could not allocate memory for output image.\n");
        return 1;
        }

    // loop over the pixels in the output image
    for (int y = 0; y < out_height; y++)
        {
        for (int x = 0; x < out_width; x++)
            {
            // calculate the corresponding coordinates in the input images
            float x1 = x * scale_x1;
            float y1 = y * scale_y1;
            float x2 = x * scale_x2;
            float y2 = y * scale_y2;

            // get the colors from the input images using bilinear interpolation
            RGBTRIPLE color1 = bilinear_interpolation(image1, x1, y1, bi1.biWidth, bi1.biHeight);
            RGBTRIPLE color2 = bilinear_interpolation(image2, x2, y2, bi2.biWidth, bi2.biHeight);

            // blend the colors and set the pixel in the output image
            RGBTRIPLE out_color = blend_colors(color1, color2, ratio);
            set_pixel(out_image, x, y, out_width, out_color);
            }
        }


    // prepare the headers for the output file
    BITMAPFILEHEADER out_bf;
    BITMAPINFOHEADER out_bi;
    memcpy(&out_bf, &bf1, sizeof(BITMAPFILEHEADER));
    memcpy(&out_bi, &bi1, sizeof(BITMAPINFOHEADER));
    out_bi.biWidth = out_width;
    out_bi.biHeight = out_height;
    out_bi.biSizeImage = out_width * out_height * sizeof(RGBTRIPLE);
    out_bf.bfSize = out_bi.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    write_bmp(outputfile, &out_bf, &out_bi, out_image);

    free(image1);
    free(image2);
    free(out_image);
    return 0;

}

    /*

    60% if your program works at least for images with different size but without line padding (e.g.
    lion, jarjar).

    ./ARNE_assignment2 images/lion.bmp images/jar.bmp 0.5 images/merged/merged1.bmp

    25% if your program works for images with line padding (e.g. flowers)

    ./ARNE_assignment2 images/flowers.bmp images/wolf.bmp 0.5 images/merged/merged2.bmp

    25% if your program works with arbitrary resolutions and bilinear interpolation.

    ./ARNE_assignment2 images/jar.bmp images/1UP.bmp 0.6 images/merged/merged3.bmp
    
    */

