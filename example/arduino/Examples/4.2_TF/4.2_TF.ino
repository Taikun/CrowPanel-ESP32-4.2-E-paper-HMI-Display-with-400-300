#include <Arduino.h>
#include "EPD.h"
#include "EPD_GUI.h"

#include "SD.h"

#define SD_MOSI 40
#define SD_MISO 13
#define SD_SCK 39
#define SD_CS 10
SPIClass SD_SPI = SPIClass(HSPI);
uint8_t Image_BW[15000];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Screen power.
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Initialize e-ink screen.
  EPD_GPIOInit();

  // Turn on SD card power.
  pinMode(42, OUTPUT);
  digitalWrite(42, HIGH);
  delay(10);

  // SD card.
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, SD_SPI, 80000000))
  {
    Serial.println(F("ERROR: File system mount failed!"));
  }
  else
  {
    Serial.printf("SD Size： %lluMB \n", SD.cardSize() / (1024 * 1024));
    char buffer[30]; // Assume no more than 50 characters here. Adjust size according to actual situation.
    // Use sprintf to output formatted string to buffer.
    int length = sprintf(buffer, "SD Size:%lluMB", SD.cardSize() / (1024 * 1024));
    buffer[length] = '\0';
    clear_all();
    Serial.println(buffer);

    EPD_ShowString(0, 0, buffer, 16, BLACK);
    EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);
    EPD_Sleep();
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10);
}

void clear_all()
{
  EPD_Clear();
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE); // Clear canvas.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);

}