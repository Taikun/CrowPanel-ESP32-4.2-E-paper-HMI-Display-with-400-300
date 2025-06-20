#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiClientSecure.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "secrets.h"


#define DOT_PIXEL_1X1 1

// Buffer para pantalla 400x300
uint8_t ImageBW[15000];

// Buffer de trabajo
String jsonBuffer;
int httpResponseCode;

// Datos temporales
JSONVar pools;
char buffer[128];

void clear_all() {
  EPD_Clear();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
  EPD_Full(WHITE);
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

String httpGETTrueNAS(String endpoint) {
  WiFiClientSecure client;
  client.setInsecure(); // Ignora certificado
  HTTPClient https;

  String fullUrl = String(TRUENAS_URL) + endpoint;
  https.begin(client, fullUrl);
  https.addHeader("Authorization", "Bearer " + String(TRUENAS_API_KEY));
  https.addHeader("Content-Type", "application/json");

  httpResponseCode = https.GET();
  String payload = "{}";
  if (httpResponseCode > 0) {
    payload = https.getString();
  } else {
    Serial.printf("Error HTTP: %d\n", httpResponseCode);
  }
  https.end();
  return payload;
}

void obtenerPools() {
  jsonBuffer = httpGETTrueNAS("pool");
  pools = JSON.parse(jsonBuffer);

  if (JSON.typeof(pools) == "undefined") {
    Serial.println("Error al parsear JSON de pools");
    return;
  }

  Serial.println("Pools obtenidos correctamente.");
}

void mostrarPoolsPantalla() {
  Serial.println("🧭 Starting pool status rendering...");
  EPD_GPIOInit();
  clear_all();
  EPD_Init_Fast(Fast_Seconds_1_5s);

  int y = 10;

  for (int i = 0; i < pools.length(); i++) {
    String poolName = pools[i]["name"];
    String poolStatus = pools[i]["status"];
    int poolId = (int)pools[i]["id"];

    Serial.printf("📦 Pool %d: %s (Status: %s)\n", poolId, poolName.c_str(), poolStatus.c_str());

    snprintf(buffer, sizeof(buffer), "Pool: %s [%s]", poolName.c_str(), poolStatus.c_str());
    EPD_ShowString(10, y, buffer, 16, BLACK);
    y += 20;

    // Fetch full pool details
    double size = (double)pools[i]["size"];
    double allocated = (double)pools[i]["allocated"];

    Serial.printf("📊 size: %.2f | allocated: %.2f\n", size, allocated);

    double usedPercent = 0.0;
    if (size > 0) {
      usedPercent = (allocated / size) * 100.0;
    } else {
      Serial.println("⚠️  Size is zero or undefined, skipping usage bar.");
    }

    Serial.printf("✅ Used: %.2f%%\n", usedPercent);


    // Draw usage bar (black)
    int barraX = 20;
    int barraY = y;
    int barraW = 360;
    int barraH = 10;
    int barraUsada = min((int)(barraW * (usedPercent / 100.0)), barraW);

    // 1. Dibuja toda la barra vacía (fondo blanco)
    for (int yline = barraY; yline < barraY + barraH; yline++) {
      for (int xline = barraX; xline < barraX + barraW; xline++) {
        Paint_SetPixel(xline, yline, WHITE);
      }
    }

    // 2. Dibuja solo la parte usada (relleno negro proporcional)
    for (int yline = barraY; yline < barraY + barraH; yline++) {
      for (int xline = barraX; xline < barraX + barraUsada; xline++) {
        Paint_SetPixel(xline, yline, BLACK);
      }
    }

    // 3. Dibuja marco opcional
    EPD_DrawRectangle(barraX, barraY, barraX + barraW, barraY + barraH, BLACK, DOT_PIXEL_1X1);
    
    y += 15;

    snprintf(buffer, sizeof(buffer), "Used: %.1f%%", usedPercent);
    EPD_ShowString(barraX, y, buffer, 16, BLACK);
    y += 20;

    // Get disk list (once)
    JSONVar allDisks = JSON.parse(httpGETTrueNAS("disk"));
    for (int j = 0; j < allDisks.length(); j++) {
      if ((String)allDisks[j]["pool"] == poolName) {
        String diskName = allDisks[j]["name"];
        int temp = (int)allDisks[j]["temperature"];
        snprintf(buffer, sizeof(buffer), "  %s - Temp: %d C", diskName.c_str(), temp);
        EPD_ShowString(20, y, buffer, 16, BLACK);
        y += 20;
      }
    }

    y += 10; // space between pools
  }

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Sleep();
  Serial.println("✅ Done rendering all pools.");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Iniciando conexión WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi.");
  Serial.println(WiFi.localIP());

  pinMode(7, OUTPUT); // Alimentación pantalla
  digitalWrite(7, HIGH);
}

void loop() {
  obtenerPools();
  mostrarPoolsPantalla();
  delay(1000 * 60 * 10); // Cada 10 minutos
}