# Image Contrast Adjuster

This program is designed to adjust the contrast of 24-bit BMP images. It reads the input image, changes its contrast based on a provided contrast factor, and then saves the result as a new BMP image.

## How it works

The program reads the input BMP image and processes its headers and pixel data. It then calls the `change_contrast` function to modify the contrast of the image based on a given contrast factor. A contrast factor greater than 1 will increase the contrast, while a contrast factor between 0 and 1 will decrease it.

The `change_contrast` function works by iterating through each color channel of every pixel, normalizing the color intensity, applying the contrast factor, denormalizing the intensity, and then storing the result back into the image data.

## How to run the program

1. Open a terminal and navigate to the folder containing the `contrast.c` file:

cd ~/Documents/Github/csc-357/lab2

2. Compile the program:

gcc contrast.c -o contrast -lm

3. Run the program with the desired input image, output image, and contrast factor:

./contrast blend\ images/flowers.bmp flowers_output.bmp 3.2 \
./contrast blend\ images/jar.bmp jar_output.bmp 3.2 \
./contrast blend\ images/lion.bmp lion_output.bmp 3.2 \
./contrast blend\ images/tunnel.bmp tunnel_output.bmp 3.2 \
./contrast blend\ images/wolf.bmp wolf_output.bmp 3.2


## Example

Here are some example images before and after adjusting the contrast:

**Before:**

![Flowers before](flowers.bmp)

**After (with a contrast factor of 3.2):**

![Flowers after](flowers_output.bmp)
