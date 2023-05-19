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

typedef struct tagRGB_VALUES
    {
    unsigned char rgbtBlue;
    unsigned char rgbtGreen;
    unsigned char rgbtRed;
    } RGB_VALUES;


RGB_VALUES get_color(RGB_VALUES *image, int x, int y, int width)
    {
    return image[y * width + x];
    }


void set_pixel(RGB_VALUES *image, int x, int y, int width, RGB_VALUES color)
    {
    image[y * width + x] = color;
    }


RGB_VALUES blend_colors(RGB_VALUES color1, RGB_VALUES color2, float ratio)
    {
    RGB_VALUES result;
    result.rgbtBlue = color1.rgbtBlue * ratio + color2.rgbtBlue * (1 - ratio);
    result.rgbtGreen = color1.rgbtGreen * ratio + color2.rgbtGreen * (1 - ratio);
    result.rgbtRed = color1.rgbtRed * ratio + color2.rgbtRed * (1 - ratio);
    return result;
    }


RGB_VALUES bilinear_interpolation(RGB_VALUES *image, float x, float y, int width, int height)
    {
    int x1 = (int)x;
    int y1 = (int)y;
    int x2 = x1 + 1 < width ? x1 + 1 : x1;
    int y2 = y1 + 1 < height ? y1 + 1 : y1;

    RGB_VALUES color_left_lower = get_color(image, x1, y1, width);
    RGB_VALUES color_left_upper = get_color(image, x1, y2, width);
    RGB_VALUES color_right_lower = get_color(image, x2, y1, width);
    RGB_VALUES color_right_upper = get_color(image, x2, y2, width);

    float dx1 = x2 == x1 ? 0 : x2 - x;
    float dx2 = x2 == x1 ? 1 : x - x1;
    float dy1 = y2 == y1 ? 0 : y2 - y;
    float dy2 = y2 == y1 ? 1 : y - y1;

    RGB_VALUES result;
    result.rgbtBlue = (color_left_lower.rgbtBlue * dx1 + color_right_lower.rgbtBlue * dx2 ) * dy1 + (color_left_upper.rgbtBlue * dx1 + color_right_upper.rgbtBlue * dx2 ) * dy2;
    result.rgbtGreen = (color_left_lower.rgbtGreen * dx1 + color_right_lower.rgbtGreen * dx2 ) * dy1 + (color_left_upper.rgbtGreen * dx1 + color_right_upper.rgbtGreen * dx2 ) * dy2;
    result.rgbtRed = (color_left_lower.rgbtRed * dx1 + color_right_lower.rgbtRed * dx2 ) * dy1 + (color_left_upper.rgbtRed * dx1 + color_right_upper.rgbtRed * dx2 ) * dy2;

    return result;
    }


RGB_VALUES *read_bmp(char *filename, BITMAPFILEHEADER *bf, BITMAPINFOHEADER *bi)
    {
    FILE *file = fopen(filename, "rb");

    fread(bf, sizeof(BITMAPFILEHEADER), 1, file);
    fread(bi, sizeof(BITMAPINFOHEADER), 1, file);

    // printf("bfType: %u\n", bf->bfType);
    // printf("biBitCount: %u\n", bi->biBitCount);

    // tests if image is working
    if (bf->bfType != 0x4D42 || bi->biBitCount != 24)
        {
        printf("Unsupported file format.\n");
        fclose(file);
        return NULL;
        }

    int padding = (4 - (bi->biWidth * sizeof(RGB_VALUES)) % 4) % 4;
    RGB_VALUES *image = malloc(bi->biWidth * bi->biHeight * sizeof(RGB_VALUES));
    fseek(file, bf->bfOffBits, SEEK_SET);

    for (int i = 0; i < bi->biHeight; i++)
        {
            fread(&image[i * bi->biWidth], sizeof(RGB_VALUES), bi->biWidth, file);
            fseek(file, padding, SEEK_CUR);
        }

    fclose(file);
    return image;
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


int main(int argc, char *argv[]) 
    {
    // makes sure user runs it correctly
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
    RGB_VALUES *image1 = read_bmp(imagefile1, &bf1, &bi1);
    RGB_VALUES *image2 = read_bmp(imagefile2, &bf2, &bi2);

    // compute the dimensions of the output image
    int out_width = bi1.biWidth > bi2.biWidth ? bi1.biWidth : bi2.biWidth;
    int out_height = bi1.biHeight > bi2.biHeight ? bi1.biHeight : bi2.biHeight;

    float scale_x1 = (float) bi1.biWidth / out_width;
    float scale_y1 = (float) bi1.biHeight / out_height;
    float scale_x2 = (float) bi2.biWidth / out_width;
    float scale_y2 = (float) bi2.biHeight / out_height;

    // allocate memory for the output image
    RGB_VALUES *out_image = malloc(out_width * out_height * sizeof(RGB_VALUES));

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

            // get the colors from the two input images using bilinear interpolation
            RGB_VALUES color1 = bilinear_interpolation(image1, x1, y1, bi1.biWidth, bi1.biHeight);
            RGB_VALUES color2 = bilinear_interpolation(image2, x2, y2, bi2.biWidth, bi2.biHeight);

            // blend the colors and set the pixel in the output image
            RGB_VALUES out_color = blend_colors(color1, color2, ratio);
            set_pixel(out_image, x, y, out_width, out_color);
            }
        }


    // prepare for the output file
    BITMAPFILEHEADER out_bf;
    BITMAPINFOHEADER out_bi;
    memcpy(&out_bf, &bf1, sizeof(BITMAPFILEHEADER));
    memcpy(&out_bi, &bi1, sizeof(BITMAPINFOHEADER));
    out_bi.biWidth = out_width;
    out_bi.biHeight = out_height;
    out_bi.biSizeImage = out_width * out_height * sizeof(RGB_VALUES);
    out_bf.bfSize = out_bi.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    write_bmp(outputfile, &out_bf, &out_bi, out_image);

    // free allocated memory
    free(image1);
    free(image2);
    free(out_image);
    return 0;

}

    /*

    assignment grading details:

    60% if your program works at least for images with different size but without line padding (e.g.
    lion, jarjar).

    ./ARNE_assignment2 images/lion.bmp images/jar.bmp 0.5 images/merged/merged1.bmp

    20% if your program works for images with line padding (e.g. flowers)

    ./ARNE_assignment2 images/flowers.bmp images/wolf.bmp 0.5 images/merged/merged2.bmp

    20% if your program works with arbitrary resolutions and bilinear interpolation.

    ./ARNE_assignment2 images/jar.bmp images/1UP.bmp 0.6 images/merged/merged3.bmp
    
    */

