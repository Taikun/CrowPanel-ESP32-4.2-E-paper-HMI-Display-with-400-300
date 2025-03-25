#include <WiFi.h>  // Include the WiFi library for connecting to the WiFi network.
#include <Arduino.h>  // Include the Arduino library to provide basic functionality.
#include "EPD.h"  // Include the EPD library for controlling the Electronic Paper Display.
#include "EPD_GUI.h"  // Include the EPD_GUI library for graphical user interface operations.
#include <ArduinoJson.h>  // Include the ArduinoJson library for parsing JSON data.
#include <HTTPClient.h>  // Include the HTTPClient library for sending HTTP requests.
#include "pic.h"  // Include pic.h which presumably contains image data for display.

const char * ID = "yanfa_software";  // WiFi network name.
const char * PASSWORD = "yanfa-123456";  // WiFi password.

// Weather related parameters.
String API = "SBy5V5m7FkAn1e_vi";  // Weather API key.
String WeatherURL = "";  // Used to store the generated weather API request URL.
String CITY = "Shenzhen";  // Name of the city to query.
String url_xinzhi = "";  // Used to store the specific weather API request URL.
String Weather = "0";  // Stores the obtained weather information.

long sum = 0;  // Spare variable, currently not in use.

// Define a black and white image array as a buffer for the e-paper display.
uint8_t ImageBW[15000];

// Create an instance of HTTPClient.
HTTPClient http;

// Generate the weather API request URL.
String GitURL(String api, String city)
{
  url_xinzhi =  "https://api.seniverse.com/v3/weather/now.json?key=";
  url_xinzhi += api;  // Add API key.
  url_xinzhi += "&location=";
  url_xinzhi += city;  // Add city name.
  url_xinzhi += "&language=zh-Hans&unit=c";  // Set language to simplified Chinese and unit to Celsius.
  return url_xinzhi;  // Return the generated URL.
}

// Parse weather data.
void ParseWeather(String url)
{
  DynamicJsonDocument doc(1024);  // Allocate 1024 bytes of memory to store JSON data.
  http.begin(url);  // Initialize HTTP request and start accessing the specified URL.

  int httpGet = http.GET();  // Send GET request and get response status code.
  if (httpGet > 0)  // Request successful.
  {
    Serial.printf("HTTPGET is %d", httpGet);  // Print HTTP response status code.

    if (httpGet == HTTP_CODE_OK)  // Request successful and response code is 200.
    {
      String json = http.getString();  // Get the response JSON string.
      Serial.println(json);  // Print JSON string.

      deserializeJson(doc, json);  // Parse the JSON string into the DynamicJsonDocument object.

      Weather = doc["results"][0]["now"]["text"].as<String>();  // Get weather information.
    }
    else
    {
      Serial.printf("ERROR1!!");  // Print error message.
    }
  }
  else
  {
    Serial.printf("ERROR2!!");  // Print error message.
  }
  http.end();  // End HTTP request.
}

// Create a character array for displaying information.
char buffer[40];

void setup()
{
  Serial.begin(115200);  // Initialize serial communication with baud rate set to 115200.

  // ==================WiFi connection==================
  Serial.println("WiFi:");
  Serial.println(ID);  // Print WiFi network name.
  Serial.println("PASSWORLD:");
  Serial.println(PASSWORD);  // Print WiFi password.

  WiFi.begin(ID, PASSWORD);  // Try to connect to the specified WiFi network.

  while (WiFi.status()!= WL_CONNECTED)  // Wait until WiFi connection is successful.
  {
    delay(500);  // Check every 500 milliseconds.
    Serial.println("Connecting...");  // Print connection status.
  }

  Serial.println("Connected successfully!");  // Print connection success message.
  // ==================WiFi connection==================

  WeatherURL = GitURL(API, CITY);  // Generate the weather API request URL.

  // Set the screen power pin as output mode and set it to high level to turn on the power.
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  EPD_GPIOInit();  // Initialize the screen GPIO.
  EPD_Clear();  // Clear the screen.
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);  // Create a new canvas and set it to white.
  EPD_Full(WHITE);  // Clear the canvas and fill it with white.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);  // Display the blank canvas.

  EPD_Init_Fast(Fast_Seconds_1_5s);  // Initialize the screen and set the update speed to 1.5 seconds.

  Serial.println("Connected successfully!");  // Print connection success message.

}

void loop()
{
    ParseWeather(WeatherURL);  // Get weather information and parse it.

  PW(WeatherURL);  
  delay(1000*60*60); // Main loop delay for 1 hour.
}

// Clear all content on the display.
void clear_all()
{
  EPD_Clear(); // Clear the display content.
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer and fill it with white.
  EPD_Full(WHITE); // Fill the entire screen with white.
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Update the display content.
}
void PW(String url)
{
  DynamicJsonDocument doc(1024); // Allocate memory dynamically.
  http.begin(url); // Start HTTP request.

  int httpGet = http.GET(); // Send GET request.
  if (httpGet > 0) // Request successful.
  {
    Serial.printf("HTTPGET is %d\n", httpGet);

    if (httpGet == HTTP_CODE_OK) // HTTP response code is 200.
    {
      String json = http.getString(); // Get response content.
      Serial.println(json);

      deserializeJson(doc, json); // Parse JSON data.

      // Parse and extract each element.
      String location = doc["results"][0]["location"]["name"].as<String>();
      String weatherText = doc["results"][0]["now"]["text"].as<String>();
      String temperature = doc["results"][0]["now"]["temperature"].as<String>();
      String humidity = doc["results"][0]["now"]["humidity"].as<String>();
      String windSpeed = doc["results"][0]["now"]["wind"]["speed"].as<String>();

      String Country = doc["results"][0]["location"]["country"].as<String>();
      String Timezone = doc["results"][0]["location"]["timezone"].as<String>();
      String last_update = doc["results"][0]["last_update"].as<String>();

      // Create a character array for storing information.
      char buffer[40];

      // Clear the image and initialize the e-ink screen.
      clear_all(); // Clear the screen content.
      EPD_ShowPicture(0, 0, 32, 32, home_2, WHITE); // Display the home icon.

      // Update time UI.
      EPD_ShowPicture(0, 230, 32, 32, weather, WHITE); // Display the weather icon.
      EPD_ShowString(50, 245, "Last Time :", 16, BLACK); // Display "Last Time :".

      memset(buffer, 0, sizeof(buffer)); // Clear the character array.
      snprintf(buffer, sizeof(buffer), "%s ", last_update.c_str()); // Format the time.
      EPD_ShowString(10, 270, buffer, 16, BLACK); // Display the update time.

      // Update city UI.
      EPD_ShowString(55, 60, "City :", 16, BLACK); // Display "City :".
      EPD_ShowPicture(0, 50, 48, 48, city, WHITE); // Display the city icon.

      memset(buffer, 0, sizeof(buffer)); // Clear the character array.
      if (strcmp(location.c_str(), "Shenzhen") == 0) // Check if the city is "Shenzhen".
      {
        snprintf(buffer, sizeof(buffer), " %s", "Sheng Zhen"); // Set the city name.
      } else
      {
        snprintf(buffer, sizeof(buffer), "%s", "Null"); // Unknown city.
      }
      EPD_ShowString(10, 90, buffer, 16, BLACK); // Display the city name.

      // Update timezone UI.
      EPD_ShowString(55, 160, "time zone :", 16, BLACK); // Display "time zone :".
      EPD_ShowPicture(0, 140, 48, 48, time_zone, WHITE); // Display the timezone icon.

      memset(buffer, 0, sizeof(buffer)); // Clear the character array.
      snprintf(buffer, sizeof(buffer), "%s ", Timezone.c_str()); // Format the timezone.
      EPD_ShowString(0, 190, buffer, 16, BLACK); // Display the timezone.

      // Update temperature UI.
      EPD_ShowString(300, 230, "Temp :", 24, BLACK); // Display "Temp :".
      EPD_ShowPicture(260, 240, 32, 32, temp, WHITE); // Display the temperature icon.

      memset(buffer, 0, sizeof(buffer)); // Clear the character array.
      snprintf(buffer, sizeof(buffer), "%s C", temperature.c_str()); // Format the temperature.
      EPD_ShowString(320, 260, buffer, 24, BLACK); // Display the temperature value.

      // Update weather UI.
      memset(buffer, 0, sizeof(buffer)); // Clear the character array.
      if (strcmp(weatherText.c_str(), "heavy rain") == 0) // Check weather condition.
      {
        snprintf(buffer, sizeof(buffer), "Weather: %s", "heavy rain"); // Set weather description.
        EPD_ShowPicture(250, 60, 80, 80, heavy_rain, WHITE); // Display heavy rain icon.
      } else if (strcmp(weatherText.c_str(), "cloudy") == 0)
      {
        snprintf(buffer, sizeof(buffer), "Weather: %s", "Cloudy"); // Set weather description.
        EPD_ShowPicture(250, 60, 80, 80, cloudy, WHITE); // Display cloudy icon.
      }
      else if (strcmp(weatherText.c_str(), "small rain") == 0)
      {
        snprintf(buffer, sizeof(buffer), "Weather: %s", "small rain"); // Set weather description.
        EPD_ShowPicture(250, 60, 80, 80, small_rain, WHITE); // Display small rain icon.
      } else if (strcmp(weatherText.c_str(), "clear day") == 0)
      {
        snprintf(buffer, sizeof(buffer), "Weather: %s", "clear day"); // Set weather description.
        EPD_ShowPicture(250, 60, 80, 80, clear_day, WHITE); // Display clear day icon.

      EPD_ShowString(240, 160, buffer, 16, BLACK); // Display weather description.

      // Update partition and draw line UI.
      EPD_DrawLine(0, 40, 400, 40, BLACK); // Draw a dividing line.
      EPD_DrawLine(230, 40, 230, 300, BLACK); // Draw a dividing line.
      EPD_DrawLine(0, 220, 400, 220, BLACK); // Draw a dividing line.
      EPD_DrawLine(0, 130, 230, 130, BLACK); // Draw a dividing line.

      // Update the e-ink screen display content.
      EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Refresh the screen display.

      EPD_Sleep(); // Enter sleep mode to save energy.

      // Update the Weather variable.
      Weather = weatherText; // Save the current weather.
    }
    else
    {
      Serial.println("ERROR1!!"); // Output error message.
    }
  }
  else
  {
    Serial.println("ERROR2!!"); // Output error message.
  }
  http.end(); // End HTTP request.
}