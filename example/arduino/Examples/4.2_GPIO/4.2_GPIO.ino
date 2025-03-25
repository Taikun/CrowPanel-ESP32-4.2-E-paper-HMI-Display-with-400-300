#include <Arduino.h>
#include "EPD.h"      // Include the e-paper display library
#include "EPD_GUI.h"  // Include the e-paper display graphical interface library

// Image buffer for the e-paper display
uint8_t Image_BW[15000];

// Function to judge the GPIO status and display the result on the e-paper display
void judgement_function(int* pin) {
  char buffer[30];  // Buffer to store the output string

  clear_all();  // Clear the e-paper display

  // Iterate through all GPIO pins
  for (int i = 0; i < 12; i++) {
    int state = digitalRead(pin[i]);  // Read the current pin state

    if (state == HIGH) {
      // If the pin state is high, write the status information to the buffer
      int length = sprintf(buffer, "GPIO%d : on", pin[i]);
      buffer[length] = '\0';  // Ensure the buffer ends with '\0'
      EPD_ShowString(0, 0 + i * 20, buffer, 16, BLACK);  // Display the pin status on the e-paper display

    } else {
      // If the pin state is low, write the status information to the buffer
      int length = sprintf(buffer, "GPIO%d : off", pin[i]);
      buffer[length] = '\0';  // Ensure the buffer ends with '\0'
      EPD_ShowString(0, 0 + i * 20, buffer, 16, BLACK);  // Display the pin status on the e-paper display
    }
  }

  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);  // Update the e-paper display content
  EPD_Sleep();  // Put the display into sleep mode to save power
}

// Define the numbers of 12 GPIO pins
int pin_Num[12] = {8, 3, 14, 9, 16, 15, 18, 17, 20, 19, 38, 21};

void setup() {
  Serial.begin(115200);  // Initialize serial communication

  // Initialize e-paper display power
  pinMode(7, OUTPUT);  // Set pin 7 to output mode
  digitalWrite(7, HIGH);  // Set pin 7 to high level to turn on the display power

  // Set 12 GPIO pins to output mode and set their levels to high
  for (int i = 0; i < 12; i++) {
    pinMode(pin_Num[i], OUTPUT);  // Set each pin to output mode
    digitalWrite(pin_Num[i], HIGH);  // Set the level of each pin to high
  }

  // Initialize the e-paper display
  EPD_GPIOInit();
  
  // Call the function to display the GPIO status
  judgement_function(pin_Num);
}

void loop() {
  // Code in the main loop is executed once per second
  delay(1000);  // Wait for 1 second
}

// Function to clear the e-paper display
void clear_all() {
  EPD_Clear();  // Clear the e-paper display
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE);  // Initialize the image buffer with a white background
  EPD_Full(WHITE);  // Fill the entire canvas with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);  // Update the display content
}