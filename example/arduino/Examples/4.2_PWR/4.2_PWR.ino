#include <Arduino.h>
#include "EPD.h"
#include "EPD_GUI.h"
uint8_t Image_BW[15000];
// Home key.
#define HOME_KEY 2
int HOME_NUM = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Screen power.
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  // POWER indicator light.
  pinMode(41, OUTPUT);

  pinMode(HOME_KEY, INPUT);
  // Initialize e-ink screen.
  EPD_GPIOInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  int flag = 0;
  if (digitalRead(HOME_KEY) == 0)
  {
    delay(100);
    if (digitalRead(HOME_KEY) == 1)
    {
      Serial.println("HOME_KEY");
      HOME_NUM =!HOME_NUM;

      flag = 1;
    }
  }
  if (flag == 1)
  {
    char buffer[30];

    clear_all();
    if (HOME_NUM == 1)
    {
      digitalWrite(41, HIGH);
      strcpy(buffer, "PWR:on");
    } else
    {
      digitalWrite(41, LOW);
      strcpy(buffer, "PWR:off");
    }
    EPD_ShowString(0, 0 + 0 * 20, buffer, 16, BLACK);
    EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);
    EPD_Sleep();
  }
}

void clear_all()
{
  EPD_Clear();
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE); // Clear canvas.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);

}