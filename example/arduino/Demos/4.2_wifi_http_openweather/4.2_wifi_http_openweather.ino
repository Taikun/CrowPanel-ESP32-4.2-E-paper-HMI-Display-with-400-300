#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "EPD.h"  // Include the EPD library for controlling the electronic ink screen (E-Paper Display)
#include "EPD_GUI.h"  // Include the EPD_GUI library for graphical user interface (GUI) operations
#include "pic.h"

// Define a black and white image array as the buffer for the e-paper display
uint8_t ImageBW[15000];  // Define the size based on the resolution of the e-paper display

// WiFi network SSID and password
const char* ssid = "yanfa_software";
const char* password = "yanfa-123456";

// OpenWeatherMap API key
String openWeatherMapApiKey = "You-API";

// City and country code to query
String city = "London";
String countryCode = "2643743";

// Timer variables
unsigned long lastTime = 0;
// Set the timer to 10 seconds (10000 milliseconds) for testing
unsigned long timerDelay = 10000;
// In production, set the timer to 10 minutes (600000 milliseconds), adjust according to API call limits
// unsigned long timerDelay = 600000;

String jsonBuffer;  // To store the JSON data retrieved from the API
int httpResponseCode;  // HTTP response code
JSONVar myObject;  // JSON data object

// Weather-related information
String weather;
String temperature;
String humidity;
String sea_level;
String wind_speed;
String city_js;
int weather_flag = 0;

// Clear all content on the display
void clear_all()
{
  EPD_Clear(); // Clear the display content
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer, fill with white
  EPD_Full(WHITE); // Fill the entire screen with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Update the display content
}

// Display weather forecast information
void UI_weather_forecast()
{
  // Create character arrays to store information
  char buffer[40];

  EPD_GPIOInit();  // Initialize the screen GPIO
  EPD_Clear();  // Clear the screen
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);  // Create a new canvas, set the canvas to white
  EPD_Full(WHITE);  // Clear the canvas, fill with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);  // Display a blank canvas

  EPD_Init_Fast(Fast_Seconds_1_5s);  // Initialize the screen, set update speed to 1.5 seconds

  // Display weather-related icons and information
  EPD_ShowPicture(7, 10, 184, 208, Weather_Num[weather_flag], WHITE);
  EPD_ShowPicture(205, 22, 184, 88, gImage_city, WHITE);
  EPD_ShowPicture(6, 238, 96, 40, gImage_wind, WHITE);
  EPD_ShowPicture(205, 120, 184, 88, gImage_hum, WHITE);
  EPD_ShowPicture(112, 238, 144, 40, gImage_tem, WHITE);
  EPD_ShowPicture(265, 238, 128, 40, gImage_visi, WHITE);

  // Draw partition lines
  EPD_DrawLine(0, 230, 400, 230, BLACK); // Draw horizontal line
  EPD_DrawLine(200, 0, 200, 230, BLACK); // Draw vertical line
  EPD_DrawLine(200, 115, 400, 115, BLACK); // Draw horizontal line

  // Display city name
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", city_js); // Format the updated city as a string
  EPD_ShowString(290, 74, buffer, 24, BLACK); // Display city name

  // Display temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s C", temperature); // Format the updated temperature as a string
  EPD_ShowString(160, 273, buffer, 16, BLACK); // Display temperature

  // Display humidity
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", humidity); // Format the humidity as a string
  EPD_ShowString(290, 171, buffer, 16, BLACK); // Display humidity

  // Display wind speed
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s m/s", wind_speed); // Format the wind speed as a string
  EPD_ShowString(54, 273, buffer, 16, BLACK); // Display wind speed

  // Display sea level pressure
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", sea_level); // Format the sea level pressure as a string
  EPD_ShowString(316, 273, buffer, 16, BLACK); // Display sea level pressure

  // Update the e-ink display content
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Refresh the screen display

  EPD_Sleep(); // Enter sleep mode to save energy
}

void setup() {
  Serial.begin(115200); // Initialize serial communication

  // Connect to the WiFi network
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Wait 0.5 seconds
    Serial.print("."); // Print a dot in the serial monitor
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP()); // Print the IP address of the WiFi connection

  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
  
  // Set the power pin of the screen to output mode and set it to high level to turn on the power
  pinMode(7, OUTPUT);  // Set GPIO 7 to output mode
  digitalWrite(7, HIGH);  // Set GPIO 7 to high level

  // Initialize the e-ink display
  EPD_GPIOInit();  // Initialize the GPIO pins of the e-ink display
}

void loop() {
  js_analysis();  // Parse JSON data (this function is not provided in the code)
  UI_weather_forecast();  // Display weather forecast information
  delay(1000*60*60); // Main loop delay for 1 hour
}

// Function js_analysis is used to parse weather data and update weather information
void js_analysis()
{
  // Check if WiFi is connected successfully
  if (WiFi.status() == WL_CONNECTED) {
    // Build the OpenWeatherMap API request URL
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

    // Loop until a successful HTTP response code of 200 is received
    while (httpResponseCode != 200)
    {
      // Send an HTTP GET request and get JSON data
      jsonBuffer = httpGETRequest(serverPath.c_str());
      Serial.println(jsonBuffer); // Print the retrieved JSON data
      myObject = JSON.parse(jsonBuffer); // Parse the JSON data

      // Check if the JSON data was parsed successfully
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!"); // If parsing fails, print error message and exit
        return;
      }
      delay(2000); // Wait 2 seconds
    }

    // Extract weather information from the JSON data
    weather = JSON.stringify(myObject["weather"][0]["main"]); // Get the main weather description
    temperature = JSON.stringify(myObject["main"]["temp"]); // Get the temperature
    humidity = JSON.stringify(myObject["main"]["humidity"]); // Get the humidity
    sea_level = JSON.stringify(myObject["main"]["sea_level"]); // Get the sea level pressure
    wind_speed = JSON.stringify(myObject["wind"]["speed"]); // Get the wind speed
    city_js = JSON.stringify(myObject["name"]); // Get the city name

    // Print the extracted weather information
    Serial.print("String weather: ");
    Serial.println(weather);
    Serial.print("String Temperature: ");
    Serial.println(temperature);
    Serial.print("String humidity: ");
    Serial.println(humidity);
    Serial.print("String sea_level: ");
    Serial.println(sea_level);
    Serial.print("String wind_speed: ");
    Serial.println(wind_speed);
    Serial.print("String city_js: ");
    Serial.println(city_js);

    // Set the weather flag based on the weather description
    if (weather.indexOf("clouds") != -1 || weather.indexOf("Clouds") != -1 ) {
      weather_flag = 1; // Cloudy weather
    } else if (weather.indexOf("clear sky") != -1 || weather.indexOf("Clear sky") != -1) {
      weather_flag = 3; // Clear sky
    } else if (weather.indexOf("rain") != -1 || weather.indexOf("Rain") != -1) {
      weather_flag = 5; // Rainy weather
    } else if (weather.indexOf("thunderstorm") != -1 || weather.indexOf("Thunderstorm") != -1) {
      weather_flag = 2; // Thunderstorm weather
    } else if (weather.indexOf("snow") != -1 || weather.indexOf("Snow") != -1) {
      weather_flag = 4; // Snowy weather
    } else if (weather.indexOf("mist") != -1 || weather.indexOf("Mist") != -1) {
      weather_flag = 0; // Mist
    }
  }
  else {
    Serial.println("WiFi Disconnected"); // If WiFi is not connected, print error message
  }
}

// Function httpGETRequest sends an HTTP GET request and returns the response content
String httpGETRequest(const char* serverName) {
  WiFiClient client; // Create a WiFi client object
  HTTPClient http; // Create an HTTP client object

  // Initialize the HTTP request with the specified URL
  http.begin(client, serverName);

  // Send the HTTP GET request
  httpResponseCode = http.GET(); // Get the HTTP response code

  String payload = "{}"; // Default response content is an empty JSON object

  // Check if the HTTP response code is greater than 0 (indicating success)
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode); // Print the HTTP response code
    payload = http.getString(); // Get the response content
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode); // Print the error code
  }
  // Free the HTTP client resources
  http.end();

  return payload; // Return the response content
}