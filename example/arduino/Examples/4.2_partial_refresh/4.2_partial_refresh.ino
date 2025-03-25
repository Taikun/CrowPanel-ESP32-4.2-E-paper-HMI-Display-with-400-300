#include <Arduino.h>        // Include the Arduino core library for Arduino platform development.
#include "EPD.h"            // Include the EPD library for controlling the Electronic Paper Display.
#include "EPD_GUI.h"        // Include the EPD_GUI library providing graphical user interface functions.
#include "pic_scenario.h"   // Include the header file containing picture data.

uint8_t Image_BW[15000];    // Declare an array of size 15000 bytes for storing black and white image data.

void setup() {
  // Initialization setup, executed only once when the program starts.
  // Configure pin 7 as output mode and set it to high level to activate the screen power.
  pinMode(7, OUTPUT);            // Set pin 7 to output mode.
  digitalWrite(7, HIGH);         // Set pin 7 to high level to activate the screen power.

  EPD_GPIOInit();                // Initialize the GPIO pin configuration of the EPD e-ink screen.

  // The SPI initialization part is commented out.
  // SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  // SPI.begin ();

  EPD_Clear();                   // Clear the screen content and restore to the default state.
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer with size EPD_W x EPD_H and background color white.
  EPD_Full(WHITE);              // Fill the entire canvas with white.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Display the image stored in the Image_BW array.

  EPD_Init_Fast(Fast_Seconds_1_5s); // Fast initialization of the EPD screen and set it to 1.5 seconds fast mode.

  EPD_ShowPicture(0, 0, 312, 152, gImage_1, WHITE); // Display picture gImage_1 with starting coordinates (0, 0), width 312 and height 152 and background color white.
  EPD_Display_Fast(Image_BW); // Fast display of the image stored in the Image_BW array.

  EPD_Sleep();                // Set the screen to sleep mode to save power.

  delay(5000);                // Wait for 5000 milliseconds (5 seconds) to let the screen stay in sleep mode for a while.

  clear_all();               // Call the clear_all function to clear the screen content.
}

void loop() {
  // Main loop function. Currently, there is no operation being performed.
  // In this function, code that needs to be repeated can be added.
}

void clear_all() {
  // Function to clear the screen content.
  EPD_Clear();                 // Clear the screen content and restore to the default state.
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer with size EPD_W x EPD_H and background color white.
  EPD_Full(WHITE);            // Fill the entire canvas with white.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Display the image stored in the Image_BW array.
}