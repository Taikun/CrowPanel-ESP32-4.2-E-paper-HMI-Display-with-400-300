#include <Arduino.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include <WiFi.h>
String ssid = "yanfa_software"; // your wifi name
String password = "yanfa-123456"; // your wifi password
uint8_t Image_BW[15000];
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //Screen power supply
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
  //e-paper screen initialization
  EPD_GPIOInit();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  char buffer[40];
  clear_all();
  strcpy(buffer, "WiFi connected");
  EPD_ShowString(0, 0 + 0 * 20, buffer, 16, BLACK);
  strcpy(buffer, "IP address: ");
  strcat(buffer, WiFi.localIP().toString().c_str());
  EPD_ShowString(0, 0 + 1 * 20, buffer, 16, BLACK);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);
  EPD_Sleep();
}

void loop() {
  // put your main code here, to run repeatedly:

}
void clear_all()
{
  EPD_Clear();
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE); //Clear Canvas
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW);

}
