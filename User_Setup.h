// Configuration file for TFT_eSPI library for ESP32 with ILI9341 display
// This file must be placed in the same directory as TFT_eSPI.h

#define USER_SETUP_INFO "User Setup"

// Define the pins you're using to connect to your display
#define TFT_MISO 19 // SPI MISO pin
#define TFT_MOSI 23 // SPI MOSI pin
#define TFT_SCLK 18 // SPI Clock pin
#define TFT_CS   5  // Chip select pin
#define TFT_DC   2  // Data Command control pin
#define TFT_RST  4  // Reset pin (or connect to ESP32 RST, set to -1 if directly connected)

// Define the display controller (uncomment one below based on your display)
#define ILI9341_DRIVER

// Define the colors depth (16 bits is the most common for these displays)
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// Optional: Set orientation
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// Touch screen configuration (if applicable)
//#define TOUCH_CS 21 // Touch screen CS pin (if you're using a touch display)

// Color depth
#define COLOR_DEPTH 16