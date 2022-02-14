This is my adaption of Venice's tty2oled program to take advantange of a colour 160x80 screen I had laying around.  I've added some scaling code to retain compatibility with existing 256x64 XBM & GSC images.  I've also added support for native 160x80 images in the following formats

XBM - 1,600 bytes recognised as 160x80 B&W image
GSC - 6,400 bytes recognised as 160x80 Greyscale image
IDX - 13,312 bytes recognised as 8bpp (256 colour) indexed image.  First 12,800 bytes are pixel data, last 512 bytes are the 256 colour values in 16 bit little endian format
COL - 25,600 bytes recognised as 16bpp image.  Pixel data is 16 bit 565 colour values in 16 bit little endian format

The ttyoled.ini and ttyoled.sh have been updated to scan for the COL and IDX folders and filetypes

None of this would be possible without the excellent work done on tty2oled by Venice for which I am vedry grateful.

The display driver library I use is https://github.com/prenticedavid/Adafruit_GC9102_kbv as main ST7735 has issues with the right hand side of the display not working.

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_113402.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_114244.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_114347.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_114405.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_195855.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_202447.jpg)

![alt text](https://github.com/dave18/MiSTer_tty2oled/blob/main/MiSTer_ST7735_Colour/IMG_20220214_202515.jpg)




