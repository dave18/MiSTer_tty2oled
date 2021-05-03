/*
  By Venice
  Get CORENAME from MiSTer via Serial TTY Device and show CORENAME related text, Pictures or Logos
  Using the universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
  Problem: 
  Sometimes after Power-On "MENU" is written instead of the logo shown.
  So i added an "first transmission" without doing anything.
  2021-02
  -Change serial Speed to 57600
  -Adding SD Support for ESP32 using an dedicated SPI Interface
   Use now an TTGO-T8  ESP32 with integrated SD Micro Slot https://github.com/LilyGO/TTGO-T8-ESP32 .
   All Logos or Picture have to be 256x64 Pixel (Display Size) in XBM Format.
   Place the Pictures to the root of the FAT or FAT32 formatted SD Card an give them the name like the core is named with extension (Lunar Lander = llander.xbm).
   Use Code from "XBM Read from FS" from https://github.com/lbernstone/eyes
  -Add "#define XDEBUG" for Debugging, can be Uncommented for more Infos over Serial
  2021-02-11
  -Trying to make it more universal in case of used display sizes. Using more "DispHeight" and "DispWidth" instead of fixed values.
  2021-02-12
  -Adding some Menu Effects
  2021-02-20/21
  -Using the Effects for all Picture/Logo Transistions (not only for the Menu)
  -"MENU" Picture moved from Code to the SD, makes it easier to change (if you like).
  -Modified Text Position calculation (see U8G2 Documentation for Details). 
   Change text reference position from TOP to BASELINE.
   Using now "getAscent" instead of "getMaxCharHeight" for Text Y-Position calculation.
  -Show a small "SD-Icon" on Startup Screen instead of the "dot" if SD Card was detected.
  2021-02-16 (not released)
  -Make it more Universal 
   Add DispLineBytes , using more DispHeight & DispWidth instead of fixed Values
  2021-03-06
  -Adding some Menu Effects
  -Code cleanup
  2021-03-07
  -Adding some Menu Effects
  -Code cleanup
  2021-03-14
  -Using/Testing SD_MMC Mode (Should not matter in case of SD Access speed because of too small files. 
   Board 1: Lolin32 Board with Adafruit Micro SD SPI/SDIO Adapter. SD Access possible using SPI or 1/4.Bit SD Bus Mode.
   Board 2: TTGO_T8 v1.7.1 with integrated SD Slot. SD Access possible using SPI or 1.Bit SD Bus Mode.
   
   SD Adapter Excample Connections:
   SD Card | ESP32 4-Bit | ESP32 1-Bit (TTGO-T8)
   ----------------------------------------------
     D0       2              2
     D1       4              
     D2       12             
     D3       13             13
     CLK      14             14
     CMD      15             15
     VSS      GND            GND
     VDD      3.3V           3.3V
  -Using #ifdef...#endif for my different HW-Configs (LOLIN32/TTGOT8)
   Choose ONLY one USE_xxxx Board definition at the beggining of the Sketch
  -Adding more Debug Infos
*/

// Uncomment to get some Debugging Infos over Serial especially for SD Debugging
//#define XDEBUG

// Uncomment ONLY one Board !!
//#define USE_LOLIN32           // LOLIN32, Arduino: WEMOS LOLIN32
#define USE_TTGOT8          // TTGO-T8, Arduino: ESP32 Dev Module, xx MB Flash, def. Part. Schema

#include <Arduino.h>
#include <U8g2lib.h>    // Display Library
#include "logo.h"       // The Pics in XMB Format

// SD Stuff 
#include "FS.h"
#include "SD_MMC.h"

// ------------ Objects -----------------
// Display Constructor HW-SPI ESP32-Board (Lolin32) with Adafruit SD_MMC Adapter, 180° Rotation => U8G2_R2
// Using VSPI SCLK = 18, MISO = 19, MOSI = 23 and...
#ifdef USE_LOLIN32
U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 5);  // Enable U8G2_16BIT in u8g2.h <= !! I M P O R T A N T !!
#endif

// Display Constructor HW-SPI ESP32-Board (TTGO T8 OLED & integrated SD Card) 180° Rotation => U8G2_R2
// Using VSPI SCLK = 18, MISO = 19, MOSI = 23 and...
#ifdef USE_TTGOT8 
//U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 26, /* dc=*/ 25, /* reset=*/ 27);  // Enable U8G2_16BIT in u8g2.h <= !! I M P O R T A N T !!
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 26, /* dc=*/ 25, /* reset=*/ 27);  // Enable U8G2_16BIT in u8g2.h <= !! W I C H T I G !!
#endif

// Display Constructor HW-SPI Mighty Core ATMega 1284 0° Rotation => U8G2_R0
//U8G2_SSD1322_NHD_256X64_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 17, /* dc=*/ 16, /* reset=*/ 18);  // Enable U8G2_16BIT in u8g2.h <= !! I M P O R T A N T !!

// ------------ Variables ----------------

// Strings
//String newCore = "";             // Received Text, from MiSTer without "\n\r" currently (2021-01-11)
//String oldCore = "";             // Buffer String for Text change detection
char newCore[256]="";
char oldCore[256]="";
char newCoreChar[256];  
char filename[260];             
char cbuffer[256];

bool halfsize;

// Display Vars
u8g2_uint_t DispWidth, DispHeight, DispLineBytes;

// SD Vars
uint8_t sdCardType;
bool sdCardOK = false;
bool sdPicShown = false;

File imagefile;

// ================ SETUP ==================
void setup(void) {
  // Init Serial
  //Serial.begin(9600);              // 9600 for MiSTer ttyUSBx Device FTDI Chip or manual set Speed
  //Serial.begin(57600);           // 57600 Common Modem Speed :-)
  Serial.begin(115200);          // 115200 for MiSTer ttyUSBx Device CP2102 Chip on ESP32

  //Init Random Generator with empty Analog Port value
  randomSeed(analogRead(34));

  // Init Display
  u8g2.begin();
  u8g2.setDrawColor(1);                      // Set font color to White (default value)
  u8g2.setFontDirection(0);                  // Set drawing direction of all strings or glyphs (default value)
  u8g2.setFontPosBaseline();                 // Set font Reference Position to BASELINE (default value)
  u8g2.setFontRefHeightText();               // Set the calculation method for the ascent and descent of the current font (default value)
  u8g2.setFont(u8g2_font_tenfatguys_tr);     // 10 Pixel Font

  // Get Display Dimensions
  DispWidth = u8g2.getDisplayWidth();
  DispHeight = u8g2.getDisplayHeight();
  //DispLineBytes = DispWidth / 8;             // How many Bytes each Dipslay Line (SSD1322: 256Pixel/8Bit = 32Bytes each Line)
  //DispLineBytes = 256 / 8;             // How many Bytes each Dipslay Line (SSD1322: 256Pixel/8Bit = 32Bytes each Line)

// LOLIN32 SD Bus Init
#ifdef USE_LOLIN32
  // SD_MMC Init
  if(!SD_MMC.begin()) {                      // 4 Bit SD Bus Mode works for Lolin32 with Adafruit SD Breakout Board
    sdCardOK = false;
#ifdef XDEBUG
    Serial.println("4-Bit SD Bus Mode Start failed");
#endif
  }
  else {
    sdCardOK = true;
#ifdef XDEBUG
    Serial.println("4-Bit SD Bus Mode Start OK");
#endif
  }
#endif

// TTGO-T8 SD Bus Init
#ifdef USE_TTGOT8
  if (!SD_MMC.begin("/sdcard", true)) {              // 1 Bit SD Bus Mode for TTGO_T8 v1.7.1
  // SD_MMC Init
#ifdef XDEBUG
    Serial.println("First 1-Bit SD Bus-Mode Start failed, trying second run.");
#endif
    if(!SD_MMC.begin("/sdcard", true)) {         // ... but need 2x SD_MMC.begins
      sdCardOK = false;
#ifdef XDEBUG
      Serial.println("Second 1-Bit SD Bus-Mode Start failed");
#endif
    }
    else {
      sdCardOK = true;
#ifdef XDEBUG
      Serial.println("Second 1-Bit SD Bus-Mode Start OK");
#endif
    }
  }
  else {
    sdCardOK = true;
#ifdef XDEBUG
    Serial.println("First 1-Bit SD Bus-Mode Start OK");
#endif
  }
#endif
  
// **** Debugging Code ****
#ifdef XDEBUG
  //if (sdCardOK) {
  //  Serial.println("Card Mount OK");
  //}
  //else {
  //  Serial.println("Card Mount Failed");
  //}

  // Read Card Type
  sdCardType = SD_MMC.cardType();

  if(sdCardType == CARD_NONE){
    Serial.println("No SD card attached");
  }
  else {
    Serial.print("SD Card Type: ");
    if(sdCardType == CARD_MMC){
      Serial.println("MMC");
    } 
    else if(sdCardType == CARD_SD){
       Serial.println("SDSC");
    } 
    else if(sdCardType == CARD_SDHC){
      Serial.println("SDHC");
    } 
    else {
      Serial.println("UNKNOWN");
    }
    Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));
  }
#endif
// **** Debbuging Code ****

  // OLED Startup with Some Text
  oled_mistertext();
}

// ================ MAIN LOOP ===================
void loop(void) {

  if (Serial.available()) {
    //newCore = Serial.readStringUntil('\n');                  // Read string from serial until NewLine "\n" (from MiSTer's echo command) is detected or timeout (1000ms) happens.
    my_readSerial();
#ifdef XDEBUG
    Serial.printf("Received Corename: %s\n", newCore);
    //Serial.print("Received Corename: ");
    //Serial.println(newCore);
#endif
  }  // end serial available
    
  if (strcmp(newCore,oldCore)) {                                    // Proceed only if Core Name changed
    // Many if an elses as switch/case is not working (maybe later with an array).

    // -- First Transmission --
    if (strcmp(newCore,"QWERTZ")==0) {                        // TESTING: Process first Transmission after PowerOn/Reboot.
        // Do nothing, just receive one string to clear the buffer.
    }                    

    // -- Menu Core(s) => now part of the Logos --
    //else if (newCore=="MENU")         oled_misterlogo(random(0,7));  // MiSTer Logo Effects 0-3
    
    // Show Menu Effects manually
    else if (strcmp(newCore,"MENU0")==0)        sd2oled_readndrawlogo("MENU",0);   // Show MiSTer MENU Logo with no Effect
    else if (strcmp(newCore,"MENU1")==0)        sd2oled_readndrawlogo("MENU",1);   // Show MiSTer MENU Logo with Particle Effect
    else if (strcmp(newCore,"MENU2")==0)        sd2oled_readndrawlogo("MENU",2);   // Show MiSTer MENU Logo drawn from left to right
    else if (strcmp(newCore,"MENU3")==0)        sd2oled_readndrawlogo("MENU",3);   // Show MiSTer MENU Logo drawn from top to bottom
    else if (strcmp(newCore,"MENU4")==0)        sd2oled_readndrawlogo("MENU",4);   // Show MiSTer MENU Logo drawn from right to left
    else if (strcmp(newCore,"MENU5")==0)        sd2oled_readndrawlogo("MENU",5);   // Show MiSTer MENU Logo drawn from bottom to top
    else if (strcmp(newCore,"MENU6")==0)        sd2oled_readndrawlogo("MENU",6);   // Show MiSTer MENU Logo drawn from left to right but diagonally
    else if (strcmp(newCore,"MENU7")==0)        sd2oled_readndrawlogo("MENU",7);   // Show MiSTer MENU Logo drawn even line left to right / odd line right to left
    else if (strcmp(newCore,"MENU8")==0)        sd2oled_readndrawlogo("MENU",8);   // Show MiSTer MENU Logo drawn top part left to right / bottom part right to left 
    else if (strcmp(newCore,"MENU9")==0)        sd2oled_readndrawlogo("MENU",9);   // Show MiSTer MENU Logo drawn four parts left to right to left to right
    else if (strcmp(newCore,"MENU10")==0)       sd2oled_readndrawlogo("MENU",10);  // Show MiSTer MENU Logo drawn in Circles(Snake)
    else if (strcmp(newCore,"MENU11")==0)       sd2oled_readndrawlogo("MENU",11);  // Show MiSTer MENU Logo drawn in 4 Parts
    
    // -- Test Commands --
    else if (strcmp(newCore,"cls")==0)          u8g2.clear();
    else if (strcmp(newCore,"sorg")==0)         oled_mistertext();
    else if (strcmp(newCore,"bye")==0)          oled_drawlogo64h(sorgelig_icon64_width, sorgelig_icon64);
    
    // -- Unidentified Core Name , search SD  --
    else {
      if (sdCardOK) {
        sdPicShown = sd2oled_readndrawlogo(newCore, random(1,12));  // => 1..11
      } // end if sdCardOK
      
      if (!sdCardOK || !sdPicShown) {
        // Get Font
        const uint8_t *old_font = u8g2.getU8g2()->font;
        strncpy(newCoreChar,newCore,255);
        //newCoreChar = (char*)newCore.c_str();
        u8g2.clearBuffer();
        // Set font
        //u8g2.setFont(u8g2_font_bubble_tr);                        // 18 Pixel Font
        //u8g2.setFont(u8g2_font_maniac_tr);                          // 23 Pixel Font
        u8g2.setFont(u8g2_font_amstrad_cpc_extended_8f);
        u8g2.drawStr(DispWidth/2-(u8g2.getStrWidth(newCoreChar)/2), ( DispHeight - u8g2.getAscent() ) / 2 + u8g2.getAscent(), newCoreChar);  // Write Corename to Display
        //u8g2.setCursor(0,20);
        //u8g2.print(u8g2.getMaxCharHeight());
        u8g2.sendBuffer(); 
        // Set font back
        u8g2.setFont(old_font);
      } // end if !sdCardOK
    }  // end ifs
    strncpy(oldCore,newCore,255);         // Update Buffer
  } // end newCore!=oldCore
} // End Main Loop

//=================== Functions =========================

void my_readSerial(void)
{
  char inchar;
  int index=0;
  delay(100);     //give the serial buffer time to fill up!
  inchar=Serial.read();
  //Serial.printf("Serial pointer=%d Char=%c \n",index,inchar);
  while ((inchar!='\n') && (Serial.available()>0))
  {
    newCore[index]=inchar;
    index++;
    inchar=Serial.read();
    //Serial.printf("Serial pointer=%d Char=%c \n",index,inchar);
  }
  newCore[index]='\0';
}

void my_readLine(void)
{
  char inchar;
  int index=0;  
  inchar=imagefile.read();
  //Serial.printf("Serial pointer=%d Char=%c \n",index,inchar);
  while ((inchar!='\n') && (imagefile.available()>0))
  {
    cbuffer[index]=inchar;
    index++;
    inchar=imagefile.read();
    //Serial.printf("Serial pointer=%d Char=%c \n",index,inchar);
  }
  cbuffer[index]='\0';
}



// ---- oled_mistertext -- Show the Start-Up Text ----
void oled_mistertext(void) {
  u8g2.clearBuffer();
  u8g2.setCursor(DispWidth/2-(u8g2.getStrWidth("MiSTer FPGA")/2), ( DispHeight/2 - u8g2.getAscent() ) / 2 + u8g2.getAscent() + 12 );
  u8g2.print("MiSTer FPGA");
  u8g2.setCursor(DispWidth/2-(u8g2.getStrWidth("by Sorgelig")/2), ( DispHeight/2 - u8g2.getAscent() ) / 2 + u8g2.getAscent() + DispHeight/2 +8 );
  u8g2.print("by Sorgelig");

  // If SD Card detected print "."
  if (sdCardOK) {
    u8g2.drawXBMP(DispWidth-sdcard_icon16_width-1, 0, sdcard_icon16_width, sdcard_icon16_height, sdcard_icon16);
  }
  u8g2.sendBuffer();
}


// ---- oled_drawlogo64h -- Draw Pictures with an height of 64 Pixel centered ----
void oled_drawlogo64h(int w, const uint8_t *bitmap) {
  u8g2.clearBuffer();

  //w=256;
  //int wd=256;

  #ifdef XDEBUG
            Serial.printf("Width passed to oled_drawlogo64h is %d \n",w);
  #endif

  //char str[255];

  //sprintf(str,"drawlogo64h Width %d %d",w,DispWidth);

  //Serial.write(str);
  //Serial.println();
  if (DispWidth < w)  
  {
      //w=128;    
     w/=2;
     #ifdef XDEBUG
      Serial.printf("Calling drawXBMPHalf with w value of %d \n",w);
     #endif
      u8g2.drawXBMPHalf(DispWidth/2-w/2, 0, w, 64, bitmap);  
  }
  else 
  {
    #ifdef XDEBUG
      Serial.printf("Calling drawXBMP with w value of %d \n",w);
     #endif
    u8g2.drawXBMP(DispWidth/2-w/2, 0, w, 64, bitmap);
  }
  u8g2.sendBuffer();
} // end oled_drawlogo64h


// --- sd2oled_readndrawlogo -- Read and Draw XBM Picture from SD with some effect, Picture must be sized for the display ----
int sd2oled_readndrawlogo(char* corename, int effect) {
  
  // Returns 1 if OK or 0 in case of an Error
  // Let's try to Read the Picture first
  // u8g2.clearBuffer();                       // Line commented = Overwrite the Pixels
  //String xbmFile = String("/" + corename + ".xbm");  // => "lander.xbm"
  
  //char *filename = (char*)xbmFile.c_str();
  strcpy(filename,"/");
  strcat(filename,corename);
  strcat(filename,".xbm");
  
  if (!filename) return 0;                     // No Filename given function end here
#ifdef XDEBUG
  Serial.print("Reading File: ");
  Serial.println(filename);
#endif
  if (!SD_MMC.exists(filename)) {                  // No File found on SD function end here
#ifdef XDEBUG
    Serial.println("File not found");
#endif
    return 0;
  }
  imagefile = SD_MMC.open(filename);
  //String xbm;
  char *next;
  u8g2_uint_t imageWidth=0, imageHeight=0;  
  //uint8_t imageBits[DispWidth * DispHeight / 8];  // Make it more universal
  uint8_t imageBits[2048];  // Make it more universal
  uint16_t pos = 0;
  const char CR = 10;                             // "\CR"
  const char comma = 44;                          // ","
  const char curly = 125;                         // "}"
  char sepchar;
  int res;
  uint16_t bytepos;

  // Get first #define (width)
  my_readLine();
  Serial.println(cbuffer);
  if (strstr(cbuffer,"#define")!=NULL)
  {
    next=strstr(cbuffer,"_width");
    #ifdef XDEBUG
    Serial.println(next);
    #endif
    if (next!=NULL)
    {
      sscanf(next,"%*6s %i",&imageWidth);
    }
    else
    {
      #ifdef XDEBUG
      Serial.println("Invalid file format - Width _width not found");
      #endif
      return 0;
    }
  }
  else
  {
    #ifdef XDEBUG
    Serial.println("Invalid file format - Width #define not found");
    #endif
    return 0;
  }
  #ifdef XDEBUG
  Serial.printf("Image Width=%d \n",imageWidth);
  #endif

  if (imageWidth > 256) {
  #ifdef XDEBUG
    Serial.println("Image too large for screen");
  #endif
  return 0;
  }

  if (imageWidth > 128) halfsize=true; else halfsize=false;
  
  // Get second #define (height)
  my_readLine();
  #ifdef XDEBUG
  Serial.println(cbuffer);
  #endif
  if (strstr(cbuffer,"#define")!=NULL)
  {
    next=strstr(cbuffer,"_height");
    #ifdef XDEBUG
    Serial.println(next);
    #endif
    if (next!=NULL)
    {
      sscanf(next,"%*7s %i",&imageHeight);
    }
    else
    {
      #ifdef XDEBUG
      Serial.println("Invalid file format - Heigth _heigth not found");
      #endif
      return 0;
    }
  }
  else
  {
    #ifdef XDEBUG
    Serial.println("Invalid file format - Height #define not found");
    #endif
    return 0;
  }
  #ifdef XDEBUG
  Serial.printf("Image Height=%d \n",imageHeight);
  #endif

  if (imageHeight > DispHeight) {
  #ifdef XDEBUG
    Serial.println("Image too large for screen");
  #endif
  return 0;
  }

  my_readLine();    //ditch next line - should be the static char[] declaration
  //** could put some more robust checks in here **//

  res=1;
  // Now read in all the bytes
  bytepos=0;
  while ((res) && (imagefile.available()))
  {
    pos=0;
    sepchar='\0';
    while ((sepchar!=comma) && (sepchar!=curly) && (sepchar!=CR))
    {
      sepchar=imagefile.read();      
      cbuffer[pos++]=sepchar;
     // printf("sepchar is %c \n",sepchar);
    }
    if (pos>1)
    {
      cbuffer[pos]='\0';
      if (sepchar==curly) res=0;
      sscanf(cbuffer,"%hhx",&imageBits[bytepos]);
    /*  #ifdef XDEBUG
      printf("Byte read at pos %d is %#x based on cbuffer %s\n",bytepos,imageBits[bytepos],cbuffer);
      #endif*/
      bytepos++;
    }
  }


  imagefile.close();  // close File

  #ifdef XDEBUG
    Serial.println("Finished Reading File");
  #endif

  #ifdef XDEBUG
    if (halfsize) Serial.println("Image over 128 pixels wide so will integer scale by 2"); else Serial.println("Image <= 128 pixels wide so will fit ok");
  #endif
  
  // Now let's draw the Picture
  //const int logoBytes = 256 * DispHeight / 8; // Make it more universal
  const int logoBytes = imageWidth * imageHeight / 8; // Make it more universal
  int logoByte;
  unsigned char logoByteValue;
  //int a,i,x,y,x2;
  int w,x,y,x2;

  DispLineBytes = imageWidth / 8;
  #ifdef XDEBUG
        Serial.printf("DispLineByte=%d \n",DispLineBytes);
  #endif

  // -------------------- Effects -----------------------
  switch (effect) {
    case 1:                                      // Particle Effect
      #ifdef XDEBUG
            Serial.println("Case 1");
      #endif
      for (w=0; w<10000; w++) {
        logoByte = random(logoBytes); // Value 2048 => Get 0..2047

   /*    #ifdef XDEBUG
        sprintf(cbuffer,"LogoByte %d",logoByte);
        Serial.println(cbuffer);
       #endif
*/
        
        logoByteValue = imageBits[logoByte];
        if (halfsize) x = (logoByte % DispLineBytes) * 4; else x = (logoByte % DispLineBytes) * 8;
        y = logoByte / DispLineBytes;
        if (halfsize) drawFourBit(x, y, logoByteValue); else drawEightBit(x, y, logoByteValue);

        /*x = (logoByte % 32) * 4;
        y = logoByte / 32;
        drawFourBit(x, y, logoByteValue);*/

        
        // For different speed
        // if ((w % (w/10)) == 0) u8g2.sendBuffer();
        if (w<=1000) {
          if ((w % 25)==0) u8g2.sendBuffer();
        }
        if ((w>1000) && (w<=2000)) {
          if ((w % 50)==0) u8g2.sendBuffer();
        }
        if ((w>2000) && (w<=5000)) { 
          if ((w % 200)==0) u8g2.sendBuffer();
        }
        if (w>5000) { 
          if ((w % 400)==0) u8g2.sendBuffer();
        }
      }
      // Finally overwrite the Screen with fill Size Picture
      //u8g2.drawXBM(0, 0, imageWidth, imageHeight, imageBits);
      //u8g2.sendBuffer();
      oled_drawlogo64h(imageWidth,imageBits);
    break;  // end case 1
  
    case 2:                                        // Left to Right
      #ifdef XDEBUG
            Serial.println("Case 2");
      #endif
      for (x=0; x<DispLineBytes; x++) {
        for (y=0; y<imageHeight; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 2
    
    case 3:                                       // Top to Bottom
      #ifdef XDEBUG
            Serial.println("Case 3");
      #endif
      for (y=0; y<imageHeight; y++) {
        for (x=0; x<DispLineBytes; x++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 3

    case 4:                                        // Right to Left
      #ifdef XDEBUG
            Serial.println("Case 4");
      #endif
      for (x=DispLineBytes-1; x>=0; x--) {
        for (y=0; y<imageHeight; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 4

    case 5:                                       // Bottom to Top
      #ifdef XDEBUG
            Serial.println("Case 5");
      #endif
      for (y=imageHeight-1; y>=0; y--) {
        for (x=0; x<DispLineBytes; x++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 5

    case 6:                                        // Left to Right Diagonally
      #ifdef XDEBUG
            Serial.println("Case 6");
      #endif
      for (x=0; x<imageWidth+imageHeight; x++) {
        for (y=0; y<imageHeight; y++) {
          // x2 calculation = Angle
          //x2=x-y;                                // Long Diagonal
          //x2=x-y/2;                              // Middle Diagonal
          x2=x-y/4;                                // Short Diagonal
          if ((x2>=0) && (x2<DispLineBytes)) {
            logoByteValue = imageBits[x2+y*DispLineBytes];
            if (halfsize) drawFourBit(x2*4, y, logoByteValue); else drawEightBit(x2*8, y, logoByteValue);
          }  // end for x2
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 6
    
    case 7:                                     // Even Line Left to Right / Odd Line Right to Left
      #ifdef XDEBUG
            Serial.println("Case 7");
      #endif
      for (x=0; x<DispLineBytes; x++) {
        for (y=0; y<imageHeight; y++) {
          if ((y % 2) == 0) {
            x2 = x;
          }
          else {
            x2 = x*-1 + DispLineBytes -1;
          }
          logoByteValue = imageBits[x2+y*DispLineBytes];
          if (halfsize) drawFourBit(x2*4, y, logoByteValue); else drawEightBit(x2*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 7
    
    case 8:                                     // Top Part Left to Right / Bottom Part Right to Left
      #ifdef XDEBUG
            Serial.println("Case 8");
      #endif
      for (x=0; x<DispLineBytes; x++) {
        for (y=0; y<imageHeight; y++) {
          if (y < DispLineBytes) {
            x2 = x;
          }
          else {
            x2 = x*-1 + DispLineBytes -1;
          }
          logoByteValue = imageBits[x2+y*DispLineBytes];
          if (halfsize) drawFourBit(x2*4, y, logoByteValue); else drawEightBit(x2*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 8
    
    case 9:                                     // Four Parts Left to Right to Left to Right...
      #ifdef XDEBUG
            Serial.println("Case 9");
      #endif
      for (w=0; w<4; w++) {
        for (x=0; x<DispLineBytes; x++) {
          for (y=0; y<imageHeight/4; y++) {
            if ((w%2) == 0) {
              x2 = x;
            }
            else {
              x2 = x*-1 + DispLineBytes -1;
            }
            logoByteValue = imageBits[x2+(y+w*16)*DispLineBytes];            
            if (halfsize) drawFourBit(x2*4, y+w*16, logoByteValue); else drawEightBit(x2*8, y+w*16, logoByteValue);
          }  // end for y
          u8g2.sendBuffer();
        }  // end for x
      }
      break;  // end case 9

      case 10:                                     // Circles(Snake)
      #ifdef XDEBUG
            Serial.println("Case 10");
      #endif
      for (w=0; w<4; w++) {
        // To the right
        for (x=0+w; x<DispLineBytes-w; x++) {
          for (y=0; y<8; y++) {
            logoByteValue = imageBits[x+(y+w*8)*DispLineBytes];
            if (halfsize) drawFourBit(x*4, y+w*8, logoByteValue); else drawEightBit(x*8, y+w*8, logoByteValue);
          }
        u8g2.sendBuffer();
        } 
        // Down
        x=DispLineBytes-1-w;
        for (y=(1+w)*8; y<imageHeight-(1+w)*8; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
          if ((y+1) % 4 == 0) u8g2.sendBuffer();              // Modulo Factor = Speed, 2(slow), 4(middle), 8(fast), none(Hyperspeed)
        }
        // To the left
        for (x=DispLineBytes-1-w; x>=0+w; x--) {
          for (y=0; y<8; y++) {
            logoByteValue = imageBits[x+(imageHeight-1-y-w*8)*DispLineBytes];
            if (halfsize) drawFourBit(x*4, DispHeight-1-y-w*8, logoByteValue); else drawEightBit(x*8, DispHeight-1-y-w*8, logoByteValue);
          }
        u8g2.sendBuffer();
        } 
        // Up
        x=0+w;
        for (y=imageHeight-1-(1+w)*8; y>=0+(1+w)*8; y--) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
          if (y % 4 == 0) u8g2.sendBuffer();                  // Modulo Factor = Speed, 2(slow), 4(middle), 8(fast), none(Hyperspeed)
        }
      }  // end for w
      break;  // end case 10

    case 11:                                     // 4 Parts, Top-Left => Bottom-Right => Top-Right => Bottom-Left
      #ifdef XDEBUG
            Serial.println("Case 11");
      #endif
      // Part 1 Top Left
      for (x=0; x<DispLineBytes/2; x++) {
        for (y=0; y<imageHeight/2; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      // Part 2 Bottom Right
      for (x=DispLineBytes/2; x<DispLineBytes; x++) {
        for (y=imageHeight/2; y<imageHeight; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      // Part 3 Top Right
      //for (x=DispLineBytes/2; x<DispLineBytes; x++) {
      for (x=DispLineBytes-1; x>=DispLineBytes/2; x--) {
        for (y=0; y<imageHeight/2; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      // Part 4 Bottom Left
      //for (x=0; x<DispLineBytes/2; x++) {
      for (x=DispLineBytes/2-1; x>=0; x--) {
        for (y=imageHeight/2; y<imageHeight; y++) {
          logoByteValue = imageBits[x+y*DispLineBytes];
          if (halfsize) drawFourBit(x*4, y, logoByteValue); else drawEightBit(x*8, y, logoByteValue);
        }  // end for y
        u8g2.sendBuffer();
      }  // end for x
      break;  // end case 11

    default:                                     // Just overwrite the whole screen
      #ifdef XDEBUG
            Serial.println("Case Default");
      #endif
      //u8g2.clearBuffer();
      u8g2.drawXBM(0, 0, imageWidth, imageHeight, imageBits);
      u8g2.sendBuffer();
      break;  // endcase default
  }  // end switch effect
  
  return 1;                      // Everything OK
}  // end sd2oled_readndrawlogo

// Draw one XBM Byte, called from the Effects in function sd2oled_readndrawlogo
void drawEightBit(int x, int y, unsigned char b) {
  for (int i=0; i<8; i++){
    if (bitRead(b, i)) {
      // Set Pixel
      u8g2.drawPixel(x+i,y);
    }
    else {
      // Clear Pixel
      u8g2.setDrawColor(0);
      u8g2.drawPixel(x+i,y);
      u8g2.setDrawColor(1);        
    }  // end bit read
  }  // end for j
}

// Draw one XBM Byte, called from the Effects in function sd2oled_readndrawlogo
void drawFourBit(int x, int y, unsigned char b) {
  for (int i=0; i<=3; i++){
    if (bitRead(b, i*2)) {
      // Set Pixel
      u8g2.drawPixel(x+i,y);
    }
    else {
      // Clear Pixel
      u8g2.setDrawColor(0);
      u8g2.drawPixel(x+i,y);
      u8g2.setDrawColor(1);        
    }  // end bit read
  }  // end for j
}

//========================== The end ================================
