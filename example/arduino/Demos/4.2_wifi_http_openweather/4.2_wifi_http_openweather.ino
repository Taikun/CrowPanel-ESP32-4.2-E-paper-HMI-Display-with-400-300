#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "time.h"
#include "EPD.h"        // Elecrow E-paper library
#include "EPD_GUI.h"
#include "pic.h"
#include "secrets.h"

// Timezone offset in seconds (adjust for daylight saving changes)
#define TIME_OFFSET 7200

// Image buffer for 400x300 resolution
uint8_t ImageBW[15000];

// Use values from secrets.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String openWeatherMapApiKey = OWM_API_KEY;
String city = OWM_CITY;
String countryCode = OWM_CITY_CODE;

char dateTimeBuffer[40]; // Holds time, city and date string
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char sunriseBuffer[6], sunsetBuffer[6], sunTimesBuffer[32];

String jsonBuffer;
int httpResponseCode;
JSONVar myObject;

// Weather information
String weather;
String temperature;
String humidity;
String sea_level;
String wind_speed;
String city_js;
int weather_flag = 0;
String temp_max;

// Clear the entire display
void clear_all()
{
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// Convert a UNIX timestamp field from JSONVar to long
long jsonVarToLong(JSONVar var) {
  String str = JSON.stringify(var);
  return atol(str.c_str());
}

// Safely retrieve integers from nested JSON
long getJsonLong(JSONVar parent, const char* field) {
  JSONVar v = parent[field];
  String s = JSON.stringify(v); // Force value to string
  return s.toInt() > 0 ? s.toInt() : atol(s.c_str());
}

// Helper to check if a timestamp (seconds) corresponds to today
bool isToday(long timestamp) {
  time_t now;
  struct tm *now_tm;
  time(&now);
  now_tm = localtime(&now);

  time_t ts_time = (time_t)timestamp;   // explicit cast
  struct tm ts_tm;
  localtime_r(&ts_time, &ts_tm);        // use address of time_t

  return (now_tm->tm_year == ts_tm.tm_year) &&
         (now_tm->tm_mon  == ts_tm.tm_mon) &&
         (now_tm->tm_mday == ts_tm.tm_mday);
}

void updateDateTimeCity() {
  // Use the ESP32 system time in UTC
  time_t now;
  time(&now);
  now += TIME_OFFSET; // Manually apply the time offset

  struct tm *tm_now = gmtime(&now);

  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

  snprintf(dateTimeBuffer, sizeof(dateTimeBuffer),
    "%02d:%02d   %s %s %02d-%s-%02d",
    tm_now->tm_hour, tm_now->tm_min,
    city_js.c_str(),
    days[tm_now->tm_wday],
    tm_now->tm_mday,
    months[tm_now->tm_mon],
    (tm_now->tm_year + 1900) % 100
  );

  Serial.print("Local hour: "); Serial.print(tm_now->tm_hour); Serial.print(":"); Serial.println(tm_now->tm_min);
  Serial.print("Weekday: "); Serial.println(days[tm_now->tm_wday]);
  Serial.print("Day of month: "); Serial.println(tm_now->tm_mday);
  Serial.print("Month: "); Serial.println(months[tm_now->tm_mon]);
  Serial.print("Year: "); Serial.println((tm_now->tm_year + 1900) % 100);
  Serial.print("City: "); Serial.println(city_js);
  Serial.print("dateTimeBuffer to show: "); Serial.println(dateTimeBuffer);
}

// Display weather information
void UI_weather_forecast()
{
  char buffer[40];

  EPD_GPIOInit();
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Init_Fast(Fast_Seconds_1_5s);

  // Icons and weather fields
  EPD_ShowPicture(7, 10, 184, 208, Weather_Num[weather_flag], WHITE);
  // EPD_ShowPicture(205, 120, 184, 88, gImage_hum, WHITE);
  // EPD_ShowPicture(10, 185, 144, 40, gImage_tem, WHITE); // temperature icon, adjust Y if needed

  // Divider lines
  EPD_DrawLine(0, 230, 400, 230, BLACK);
  EPD_DrawLine(200, 0, 200, 230, BLACK);
  EPD_DrawLine(200, 115, 400, 115, BLACK);

  // Sunrise and sunset (line above the city/date/time)
  EPD_ShowString(5, 255, sunTimesBuffer, 16, BLACK);

  // City, date and time (bottom line)
  updateDateTimeCity(); // Update the buffer before drawing
  Serial.print("Final text for city-date line: ");
  Serial.println(dateTimeBuffer);
  EPD_ShowString(5, 273, dateTimeBuffer, 16, BLACK);

  // Humidity
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", humidity.c_str());
  EPD_ShowString(290, 171, buffer, 16, BLACK);

  // Maximum temperature
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Max: %s C", temp_max.c_str());
  Serial.print("Text to display: ");
  Serial.println(buffer);  // ensure correct text
  EPD_ShowString(50, 190, buffer, 24, BLACK); // shifted to the right

  // Current temperature
  // Icon and value in the upper right corner
  Serial.print("Raw temperature value: '");
  Serial.print(temperature);
  Serial.println("' (longitud: " + String(temperature.length()) + ")");

  for (size_t i = 0; i < temperature.length(); i++) {
    Serial.print("[");
    Serial.print(i);
    Serial.print("]=");
    Serial.print((int)temperature[i]);
    Serial.print(" ");
  }
  Serial.println();
  char buffer_actual[20];
  memset(buffer_actual, 0, sizeof(buffer_actual));
  snprintf(buffer_actual, sizeof(buffer_actual), "%s C", temperature.c_str());
  // Icon (adjust X/Y if you want it closer to the corner)
  Serial.print("Buffer actual para pintar: '");
  Serial.print(buffer_actual);
  Serial.println("'");

  EPD_ShowPicture(210, 15, 24, 24, epd_bitmap_temperature_24, BLACK);
  EPD_ShowString(250, 15, buffer_actual, 24, BLACK);    // Font size 24, same as max

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  EPD_Sleep();
}

// Fetch and process weather data
// using the OpenWeatherMap API
// Specifically using the forecast endpoint
void js_analysis()
{
  if (WiFi.status() == WL_CONNECTED) {
    // 1. Use the forecast API to get today's max and min
    String serverPath = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey + "&units=metric";

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

    // Ejemplo de respuesta JSON de la API de forecast
    /*
    {
      "city": {
        "id": 2510911,
        "name": "Seville",
        "country": "ES",
        "sunrise": 1750050166,
        "sunset": 1750103194,
        ...
      },
      "list": [
        {
          "dt": 1750027200,
          "main": {
            "temp": 30.5,
            "temp_min": 29.0,
            "temp_max": 33.0,
            ...
          },
          "weather": [{"main": "Clear", ...}],
          ...
        },
        ...
      ]
    }
    */

    // Weather info from the first element (current moment)
    JSONVar list = myObject["list"];
    JSONVar first = list[0];

    weather = String((const char*)first["weather"][0]["main"]);
    
    double temp_value = (double)first["main"]["temp"];
    temperature = String(temp_value, 1);
    
    humidity = String((double)first["main"]["humidity"], 0);
    sea_level = String((double)first["main"]["sea_level"], 0);
    wind_speed = String((double)first["wind"]["speed"], 1);
    city_js = String((const char*)myObject["city"]["name"]);

    // Calculate today's max and min iterating today's data
    double tmax = -1000, tmin = 1000;
    time_t now;
    time(&now);
    struct tm *now_tm = localtime(&now);

    for (int i = 0; i < list.length(); i++) {
      JSONVar entry = list[i];
      long entryTime = atol(JSON.stringify(entry["dt"]).c_str());

      time_t ts_time = (time_t)entryTime;
      struct tm ts_tm;
      localtime_r(&ts_time, &ts_tm);

      // Only count entries from the same day
      if (now_tm->tm_year == ts_tm.tm_year &&
          now_tm->tm_mon  == ts_tm.tm_mon  &&
          now_tm->tm_mday == ts_tm.tm_mday) {
        double t = (double)entry["main"]["temp_max"];
        double tminVal = (double)entry["main"]["temp_min"];
        if (t > tmax) tmax = t;
        if (tminVal < tmin) tmin = tminVal;
      }
    }
    temp_max = String(tmax, 1);
    String temp_min = String(tmin, 1);

    Serial.print("Today's max temperature: "); Serial.println(temp_max);
    Serial.print("Today's min temperature: "); Serial.println(temp_min);

    // Sunrise and sunset from the forecast city section
    JSONVar cityObj = myObject["city"];
    String sunriseStr = JSON.stringify(cityObj["sunrise"]);
    String sunsetStr  = JSON.stringify(cityObj["sunset"]);

    long sunrise_ts = sunriseStr.toInt() > 0 ? sunriseStr.toInt() : atol(sunriseStr.c_str());
    long sunset_ts  = sunsetStr.toInt() > 0 ? sunsetStr.toInt() : atol(sunsetStr.c_str());

    Serial.print("DEBUG sunriseStr: "); Serial.println(sunriseStr);
    Serial.print("DEBUG sunsetStr: "); Serial.println(sunsetStr);
    Serial.print("DEBUG sunrise_ts: "); Serial.println(sunrise_ts);
    Serial.print("DEBUG sunset_ts: "); Serial.println(sunset_ts);

    // Apply manual time offset
    sunrise_ts += TIME_OFFSET;
    sunset_ts  += TIME_OFFSET;

    Serial.print("sunrise_ts after offset: ");
    Serial.println(sunrise_ts);
    Serial.print("sunset_ts after offset: ");
    Serial.println(sunset_ts);

    // Process sunrise first and copy immediately
    time_t sunrise_time = sunrise_ts;
    time_t sunset_time  = sunset_ts;
    struct tm tm_sunrise, tm_sunset;
    struct tm *ptm_sunrise = gmtime(&sunrise_time);
    if (ptm_sunrise) {
        memcpy(&tm_sunrise, ptm_sunrise, sizeof(struct tm)); // Copia inmediata
    }

    // Now process sunset
    struct tm *ptm_sunset = gmtime(&sunset_time);
    if (ptm_sunset) {
        memcpy(&tm_sunset, ptm_sunset, sizeof(struct tm)); // Copia inmediata
    }

    // Verify both conversions succeeded
    if (ptm_sunrise && ptm_sunset) {
        snprintf(sunriseBuffer, sizeof(sunriseBuffer), "%02d:%02d", tm_sunrise.tm_hour, tm_sunrise.tm_min);
        snprintf(sunsetBuffer, sizeof(sunsetBuffer), "%02d:%02d", tm_sunset.tm_hour, tm_sunset.tm_min);

        Serial.print("Sunrise time: "); Serial.println(sunriseBuffer);
        Serial.print("Sunset time: "); Serial.println(sunsetBuffer);
    } else {
        strcpy(sunriseBuffer, "--:--");
        strcpy(sunsetBuffer, "--:--");
    }

    snprintf(sunTimesBuffer, sizeof(sunTimesBuffer), "Sunrise: %s Sunset: %s", sunriseBuffer, sunsetBuffer);
    Serial.print("Text for sun times: ");
    Serial.println(sunTimesBuffer);

    // For debugging:
    Serial.print("String weather: ");
    Serial.println(weather);

    String weatherLower = weather;
    weatherLower.toLowerCase();

    if (weatherLower.indexOf("cloud") != -1) {
      weather_flag = 1; // Cloudy
    } else if (weatherLower.indexOf("clear") != -1) {
      weather_flag = 3; // Clear
    } else if (weatherLower.indexOf("rain") != -1) {
      weather_flag = 5; // Rain
    } else if (weatherLower.indexOf("thunder") != -1) {
      weather_flag = 2; // Thunder
    } else if (weatherLower.indexOf("snow") != -1) {
      weather_flag = 4; // Snow
    } else if (weatherLower.indexOf("mist") != -1) {
      weather_flag = 0; // Mist
    } else {
      weather_flag = 0; // Default: mist
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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Power the display
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  EPD_GPIOInit();
}

void loop() {
  js_analysis();
  UI_weather_forecast();
  delay(1000 * 60 * 60); // Refresh every hour
}