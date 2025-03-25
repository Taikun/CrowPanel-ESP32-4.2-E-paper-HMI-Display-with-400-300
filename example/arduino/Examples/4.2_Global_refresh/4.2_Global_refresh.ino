#include <Arduino.h>        // Include the Arduino core library for development on the Arduino platform
#include "EPD.h"            // Include the EPD library to control the electronic ink screen (E-Paper Display)
#include "EPD_GUI.h"        // Include the EPD_GUI library which provides graphical user interface functionalities
#include "pic_scenario.h"   // Include the header file containing image data

uint8_t Image_BW[15000];    // Declare an array of size 15000 bytes to store black and white image data

void setup() {
  // Initialization settings, executed only once when the program starts
  // Initialize screen power
  pinMode(7, OUTPUT);            // Set pin 7 as output mode for controlling the screen power
  digitalWrite(7, HIGH);         // Set pin 7 to high level to activate the screen power

  EPD_GPIOInit();                // Initialize the GPIO pin configuration for the EPD electronic ink screen

  // The SPI initialization part is commented out
  // SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  // SPI.begin ();

  EPD_Clear();                   // Clear the screen content and reset it to the default state
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer, set the size to EPD_W x EPD_H, and the background color to white
  EPD_Full(WHITE);              // Fill the entire canvas with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Display the image stored in the Image_BW array

  EPD_Init_Fast(Fast_Seconds_1_5s); // Quickly initialize the EPD screen and set it to fast mode with a duration of 1.5 seconds

  // Display the boot interface
  EPD_ShowPicture(0, 0, EPD_W, EPD_H, gImage_scenario_home, WHITE); // Display the gImage_scenario_home image on the screen, with the background color set to white
  // EPD_ShowPicture(0, 0, EPD_W, EPD_H, gImage_111, WHITE); // Commented out code, displays another image gImage_111
  // EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Commented out code, used to display the image in the Image_BW array
  EPD_Display_Fast(Image_BW); // Quickly display the image stored in the Image_BW array
  EPD_Sleep();                // Set the screen to sleep mode to save power

  delay(5000);                // Wait for 5000 milliseconds (5 seconds), allowing the screen to remain in sleep mode for some time

  clear_all();               // Call the clear_all function to clear the screen content
}

void loop() {
  // Main loop function, currently does not perform any actions
  // Code that needs to be executed repeatedly can be added in this function
}

void clear_all() {
  // Function to clear the screen content
  EPD_Clear();                 // Clear the screen content and reset it to the default state
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer, set the size to EPD_W x EPD_H, and the background color to white
  EPD_Full(WHITE);            // Fill the entire canvas with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Display the image stored in the Image_BW array
}