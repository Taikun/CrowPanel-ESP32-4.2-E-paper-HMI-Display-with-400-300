#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"        // Librería Elecrow E-paper
#include "EPD_GUI.h"
#include "pic.h"
#include "secrets.h"

// Buffer de imagen según resolución 400x300
uint8_t ImageBW[15000];

// Usa defines de secrets.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String openWeatherMapApiKey = OWM_API_KEY;
String city = OWM_CITY;
String countryCode = OWM_CITY_CODE;

// Variables de control
String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

// Información meteo
String weather;
String temperature;
String humidity;
String sea_level;
String wind_speed;
String city_js;
int weather_flag = 0;

// Limpiar pantalla
void clear_all()
{
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// Mostrar info meteorológica
void UI_weather_forecast()
{
  char buffer[40];

  EPD_GPIOInit();
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Init_Fast(Fast_Seconds_1_5s);

  // Iconos y campos meteorológicos
  EPD_ShowPicture(7, 10, 184, 208, Weather_Num[weather_flag], WHITE);
  EPD_ShowPicture(205, 22, 184, 88, gImage_city, WHITE);
  EPD_ShowPicture(6, 238, 96, 40, gImage_wind, WHITE);
  EPD_ShowPicture(205, 120, 184, 88, gImage_hum, WHITE);
  EPD_ShowPicture(112, 238, 144, 40, gImage_tem, WHITE);
  EPD_ShowPicture(265, 238, 128, 40, gImage_visi, WHITE);

  // Líneas divisorias
  EPD_DrawLine(0, 230, 400, 230, BLACK);
  EPD_DrawLine(200, 0, 200, 230, BLACK);
  EPD_DrawLine(200, 115, 400, 115, BLACK);

  // Ciudad
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", city_js.c_str());
  EPD_ShowString(290, 74, buffer, 24, BLACK);

  // Temperatura
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s C", temperature.c_str());
  EPD_ShowString(160, 273, buffer, 16, BLACK);

  // Humedad
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", humidity.c_str());
  EPD_ShowString(290, 171, buffer, 16, BLACK);

  // Viento
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s m/s", wind_speed.c_str());
  EPD_ShowString(54, 273, buffer, 16, BLACK);

  // Presión a nivel del mar
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", sea_level.c_str());
  EPD_ShowString(316, 273, buffer, 16, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  EPD_Sleep();
}

// Obtener y procesar datos meteorológicos
void js_analysis()
{
  if (WiFi.status() == WL_CONNECTED) {
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

    do {
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer);
      myObject = JSON.parse(jsonBuffer);

      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      delay(2000);
    } while (httpResponseCode != 200);

    // ***** CAMBIO CRÍTICO AQUÍ *****
    // Usamos String((const char*)myObject["weather"][0]["main"]) para evitar las comillas dobles
    weather = String((const char*)myObject["weather"][0]["main"]);
    temperature = String((const char*)myObject["main"]["temp"]);
    humidity = String((const char*)myObject["main"]["humidity"]);
    sea_level = String((const char*)myObject["main"]["sea_level"]);
    wind_speed = String((const char*)myObject["wind"]["speed"]);
    city_js = String((const char*)myObject["name"]);

    // Para depuración:
    Serial.print("String weather: ");
    Serial.println(weather);

    // ***** Nueva lógica robusta *****
    String weatherLower = weather;
    weatherLower.toLowerCase();

    if (weatherLower.indexOf("cloud") != -1) {
      weather_flag = 1; // Nublado
    } else if (weatherLower.indexOf("clear") != -1) {
      weather_flag = 3; // Soleado
    } else if (weatherLower.indexOf("rain") != -1) {
      weather_flag = 5; // Lluvia
    } else if (weatherLower.indexOf("thunder") != -1) {
      weather_flag = 2; // Tormenta
    } else if (weatherLower.indexOf("snow") != -1) {
      weather_flag = 4; // Nieve
    } else if (weatherLower.indexOf("mist") != -1) {
      weather_flag = 0; // Niebla
    } else {
      weather_flag = 0; // Default: niebla
    }
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

// HTTP GET para obtener el JSON
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);

  httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");

  // Alimentar la pantalla
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  EPD_GPIOInit();
}

void loop() {
  js_analysis();
  UI_weather_forecast();
  delay(1000 * 60 * 60); // Refresca cada hora
}