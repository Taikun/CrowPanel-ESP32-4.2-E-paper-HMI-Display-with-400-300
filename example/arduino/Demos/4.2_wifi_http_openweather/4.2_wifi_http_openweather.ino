#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "time.h"
#include "EPD.h"        // Librería Elecrow E-paper
#include "EPD_GUI.h"
#include "pic.h"
#include "secrets.h"

// 2 horas en verano, 1 hora en invierno. Cambia cuando toque horario.
#define OFFSET_HORARIO 7200

// Buffer de imagen según resolución 400x300
uint8_t ImageBW[15000];

// Usa defines de secrets.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
String openWeatherMapApiKey = OWM_API_KEY;
String city = OWM_CITY;
String countryCode = OWM_CITY_CODE;

char dateTimeBuffer[40]; // Para mostrar hora, ciudad, fecha
const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char sunriseBuffer[6], sunsetBuffer[6], sunTimesBuffer[32];

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
String temp_max;

// Limpiar pantalla
void clear_all()
{
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// Función para convertir un campo numérico UNIX timestamp desde JSONVar de forma robusta
long jsonVarToLong(JSONVar var) {
  String str = JSON.stringify(var);
  return atol(str.c_str());
}

// Función segura para obtener enteros de JSON anidados
long getJsonLong(JSONVar parent, const char* field) {
  JSONVar v = parent[field];
  String s = JSON.stringify(v); // Fuerza a string el valor interno
  return s.toInt() > 0 ? s.toInt() : atol(s.c_str());
}

// Función auxiliar para comprobar si un timestamp (segundos) es "hoy"
bool isToday(long timestamp) {
  time_t now;
  struct tm *now_tm;
  time(&now);
  now_tm = localtime(&now);

  time_t ts_time = (time_t)timestamp;   // <-- Conversión explícita
  struct tm ts_tm;
  localtime_r(&ts_time, &ts_tm);        // <-- Usar dirección de time_t

  return (now_tm->tm_year == ts_tm.tm_year) &&
         (now_tm->tm_mon  == ts_tm.tm_mon) &&
         (now_tm->tm_mday == ts_tm.tm_mday);
}

void obtenerFechaHoraCiudad() {
  // Usamos la hora del propio sistema del ESP32, que está en UTC (no en hora local)
  time_t now;
  time(&now);
  now += OFFSET_HORARIO; // Suma manualmente la diferencia horaria de España

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

  Serial.print("Hora local: "); Serial.print(tm_now->tm_hour); Serial.print(":"); Serial.println(tm_now->tm_min);
  Serial.print("Día de la semana: "); Serial.println(days[tm_now->tm_wday]);
  Serial.print("Día del mes: "); Serial.println(tm_now->tm_mday);
  Serial.print("Mes: "); Serial.println(months[tm_now->tm_mon]);
  Serial.print("Año: "); Serial.println((tm_now->tm_year + 1900) % 100);
  Serial.print("Ciudad: "); Serial.println(city_js);
  Serial.print("dateTimeBuffer a mostrar: "); Serial.println(dateTimeBuffer);
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
  // EPD_ShowPicture(205, 120, 184, 88, gImage_hum, WHITE);
  // EPD_ShowPicture(10, 185, 144, 40, gImage_tem, WHITE); // icono de temperatura, ajusta Y si hace falta

  // Líneas divisorias
  EPD_DrawLine(0, 230, 400, 230, BLACK);
  EPD_DrawLine(200, 0, 200, 230, BLACK);
  EPD_DrawLine(200, 115, 400, 115, BLACK);

  // Amanecer y Ocaso (línea justo encima de ciudad/fecha/hora)
  EPD_ShowString(5, 255, sunTimesBuffer, 16, BLACK);

  // Ciudad, fecha y hora (línea inferior)
  obtenerFechaHoraCiudad(); // Actualiza el buffer antes de pintarlo
  Serial.print("Texto final para pantalla ciudad-fecha: ");
  Serial.println(dateTimeBuffer);
  EPD_ShowString(5, 273, dateTimeBuffer, 16, BLACK);

  // Humedad
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s ", humidity.c_str());
  EPD_ShowString(290, 171, buffer, 16, BLACK);

  // Temperatura máxima
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "Max: %s C", temp_max.c_str());
  Serial.print("Texto a mostrar en pantalla: ");
  Serial.println(buffer);  // Para asegurar qué texto llega
  EPD_ShowString(50, 190, buffer, 24, BLACK); // texto desplazado a la derecha

  // Temperatura actual
  // Icono y texto de temperatura actual arriba a la derecha
  Serial.print("Valor bruto de temperature: '");
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
  // Icono (ajusta las X/Y si lo quieres más pegado a la esquina)
  Serial.print("Buffer actual para pintar: '");
  Serial.print(buffer_actual);
  Serial.println("'");

  EPD_ShowPicture(210, 15, 24, 24, epd_bitmap_temperature_24, BLACK);
  EPD_ShowString(250, 15, buffer_actual, 24, BLACK);    // Fuente tamaño 24, igual que la máxima

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  EPD_Sleep();
}

// Obtener y procesar datos meteorológicos
// usando la API de OpenWeatherMap
// Obtener y procesar datos meteorológicos usando forecast
void js_analysis()
{
  if (WiFi.status() == WL_CONNECTED) {
    // 1. Usar la API de forecast para obtener máxima y mínima del día real
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

    // Info meteo principal del primer elemento (momento actual aprox.)
    JSONVar list = myObject["list"];
    JSONVar first = list[0];

    weather = String((const char*)first["weather"][0]["main"]);
    
    double temp_value = (double)first["main"]["temp"];
    temperature = String(temp_value, 1);
    
    humidity = String((double)first["main"]["humidity"], 0);
    sea_level = String((double)first["main"]["sea_level"], 0);
    wind_speed = String((double)first["wind"]["speed"], 1);
    city_js = String((const char*)myObject["city"]["name"]);

    // Calcular máxima y mínima del día recorriendo los datos de hoy
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

      // Solo cuenta los del mismo día
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

    Serial.print("Temp máxima del día: "); Serial.println(temp_max);
    Serial.print("Temp mínima del día: "); Serial.println(temp_min);

    // Amanecer y ocaso desde la sección city del forecast
    JSONVar cityObj = myObject["city"];
    String sunriseStr = JSON.stringify(cityObj["sunrise"]);
    String sunsetStr  = JSON.stringify(cityObj["sunset"]);

    long sunrise_ts = sunriseStr.toInt() > 0 ? sunriseStr.toInt() : atol(sunriseStr.c_str());
    long sunset_ts  = sunsetStr.toInt() > 0 ? sunsetStr.toInt() : atol(sunsetStr.c_str());

    Serial.print("DEBUG sunriseStr: "); Serial.println(sunriseStr);
    Serial.print("DEBUG sunsetStr: "); Serial.println(sunsetStr);
    Serial.print("DEBUG sunrise_ts: "); Serial.println(sunrise_ts);
    Serial.print("DEBUG sunset_ts: "); Serial.println(sunset_ts);

    // Suma offset horario manual
    sunrise_ts += OFFSET_HORARIO;
    sunset_ts  += OFFSET_HORARIO;

    Serial.print("sunrise_ts tras sumar offset: ");
    Serial.println(sunrise_ts);
    Serial.print("sunset_ts tras sumar offset: ");
    Serial.println(sunset_ts);

    // SOLUCIÓN: Procesar primero el amanecer y copiarlo inmediatamente
    time_t sunrise_time = sunrise_ts;
    time_t sunset_time  = sunset_ts;
    struct tm tm_sunrise, tm_sunset;
    struct tm *ptm_sunrise = gmtime(&sunrise_time);
    if (ptm_sunrise) {
        memcpy(&tm_sunrise, ptm_sunrise, sizeof(struct tm)); // Copia inmediata
    }

    // Ahora procesar el ocaso
    struct tm *ptm_sunset = gmtime(&sunset_time);
    if (ptm_sunset) {
        memcpy(&tm_sunset, ptm_sunset, sizeof(struct tm)); // Copia inmediata
    }

    // Verificar que ambas conversiones fueron exitosas
    if (ptm_sunrise && ptm_sunset) {
        snprintf(sunriseBuffer, sizeof(sunriseBuffer), "%02d:%02d", tm_sunrise.tm_hour, tm_sunrise.tm_min);
        snprintf(sunsetBuffer, sizeof(sunsetBuffer), "%02d:%02d", tm_sunset.tm_hour, tm_sunset.tm_min);

        Serial.print("Hora amanecer: "); Serial.println(sunriseBuffer);
        Serial.print("Hora ocaso: "); Serial.println(sunsetBuffer);
    } else {
        strcpy(sunriseBuffer, "--:--");
        strcpy(sunsetBuffer, "--:--");
    }

    snprintf(sunTimesBuffer, sizeof(sunTimesBuffer), "Amanecer: %s Ocaso: %s", sunriseBuffer, sunsetBuffer);
    Serial.print("Texto sol para pantalla: ");
    Serial.println(sunTimesBuffer);

    // Para depuración:
    Serial.print("String weather: ");
    Serial.println(weather);

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

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

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