#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiClientSecure.h>
#include "EPD.h"
#include "EPD_GUI.h"
#include "secrets.h"

// --- Constantes y Buffers ---
#define DOT_PIXEL_1X1 1
uint8_t ImageBW[15000];
char buffer[128];

// --- Variables Globales ---
int paginaActual = 0;

// --- Funciones Auxiliares ---
void clear_all() {
  EPD_Clear();
  memset(ImageBW, 0xFF, sizeof(ImageBW));
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
}

String httpGETTrueNAS(String endpoint) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String fullUrl = String(TRUENAS_URL) + endpoint;
  https.begin(client, fullUrl);
  https.addHeader("Authorization", "Bearer " + String(TRUENAS_API_KEY));
  
  int httpResponseCode = https.GET();
  String payload = ""; // Devolvemos vacío en caso de error
  if (httpResponseCode > 0) {
    payload = https.getString();
    // Si la respuesta es muy larga, esto puede fallar.
    if (payload.length() > 10000) {
      Serial.println("WARN: Payload is very large, potential for memory issues.");
    }
  } else {
    Serial.printf("Error HTTP en %s: %d\n", endpoint.c_str(), httpResponseCode);
  }
  https.end();
  return payload;
}

// --- PANTALLA 1: ESTADO DE LOS POOLS ---
void mostrarPoolsPantalla() {
  Serial.println("🧭 Mostrando estado de Pools...");
  EPD_GPIOInit();
  clear_all(); 
  EPD_Init_Fast(Fast_Seconds_1_5s);
  
  JSONVar pools = JSON.parse(httpGETTrueNAS("pool"));
  if (JSON.typeof(pools) == "undefined") {
    EPD_ShowString(10, 10, "Error: No se pudo obtener Pools", 16, BLACK);
  } else {
    int y = 10;
    for (int i = 0; i < pools.length(); i++) {
      String poolName = (const char*)pools[i]["name"];
      String poolStatus = (const char*)pools[i]["status"];
      double size = (double)pools[i]["size"];
      double allocated = (double)pools[i]["allocated"];
      double usedPercent = (size > 0) ? (allocated / size) * 100.0 : 0.0;
      
      int barraX = 20, barraY = y + 20, barraW = 360, barraH = 15;
      int barraUsadaW = min((int)(barraW * (usedPercent / 100.0)), barraW);

      snprintf(buffer, sizeof(buffer), "Pool: %s [%s]", poolName.c_str(), poolStatus.c_str());
      EPD_ShowString(10, y, buffer, 16, BLACK);

      EPD_DrawRectangle(barraX, barraY, barraX + barraW, barraY + barraH, BLACK, 0);
      if (barraUsadaW > 0) {
        EPD_DrawRectangle(barraX, barraY, barraX + barraUsadaW, barraY + barraH, BLACK, 1);
      }

      y = barraY + barraH + 5;
      snprintf(buffer, sizeof(buffer), "Used: %.1f%%", usedPercent);
      EPD_ShowString(barraX, y, buffer, 16, BLACK);
      y += 35;
    }
  }
  
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Sleep();
  Serial.println("✅ Fin de renderizado de Pools.");
}

// --- PANTALLA 2: ALERTAS ACTIVAS ---
void mostrarAlertas() {
  Serial.println("🚨 Mostrando Alertas Activas...");
  EPD_GPIOInit();
  clear_all(); 
  EPD_Init_Fast(Fast_Seconds_1_5s);
  
  String jsonAlerts = httpGETTrueNAS("alert/list");
  if (jsonAlerts == "") {
      EPD_ShowString(10, 10, "Error: No se pudo obtener Alertas", 16, BLACK);
  } else {
    JSONVar alerts = JSON.parse(jsonAlerts);
    if (JSON.typeof(alerts) == "undefined") {
      EPD_ShowString(10, 10, "Error: Fallo al parsear Alertas", 16, BLACK);
    } else {
      EPD_ShowString(10, 10, "--- Active Alerts ---", 24, BLACK);
      int y = 50, activeAlertCount = 0;
      for (int i = 0; i < alerts.length(); i++) {
        bool isDismissed = (bool)alerts[i]["dismissed"];
        if (isDismissed) continue;

        String level = (String)alerts[i]["level"];
        if (level == "CRITICAL" || level == "WARNING" || level == "ERROR") {
          activeAlertCount++;
          String alertKlass = (const char*)alerts[i]["klass"];
          snprintf(buffer, sizeof(buffer), "[%.4s] %s", level.c_str(), alertKlass.c_str());
          EPD_ShowString(10, y, buffer, 16, BLACK);
          y += 20;
          if (y > (EPD_H - 30)) { EPD_ShowString(10, y, "...", 16, BLACK); break; }
        }
      }
      if (activeAlertCount == 0) EPD_ShowString(10, y, "✅ No active critical alerts.", 24, BLACK);
    }
  }

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Sleep();
  Serial.println("✅ Fin de renderizado de Alertas.");
}

void mostrarSystemInfo() {
  Serial.println("ℹ️ Mostrando Info del Sistema (Modo Depuración)...");
  EPD_GPIOInit();
  clear_all(); 
  EPD_Init_Fast(Fast_Seconds_1_5s);

  // 1. Obtener /system/info para carga CPU y total RAM
  String jsonInfo = httpGETTrueNAS("system/info");
  if (jsonInfo == "") {
    EPD_ShowString(10, 10, "Error al obtener SysInfo", 16, BLACK);
    return;
  }
  JSONVar sysInfo = JSON.parse(jsonInfo);
  if (JSON.typeof(sysInfo) == "undefined") {
    EPD_ShowString(10, 10, "Error al parsear SysInfo", 16, BLACK);
    return;
  }

  double total_mem_bytes = (double)sysInfo["physmem"];
  double total_mem_gb = total_mem_bytes / 1024.0 / 1024.0 / 1024.0;

  double load1 = (double)sysInfo["loadavg"][0];
  double load5 = (double)sysInfo["loadavg"][1];
  double load15 = (double)sysInfo["loadavg"][2];
  String uptime_str = (const char*)sysInfo["uptime"];

  // 2. Obtener memoria libre desde reporting/get_data
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  String url = String(TRUENAS_URL) + "reporting/get_data";
  https.begin(client, url);
  https.addHeader("Authorization", "Bearer " + String(TRUENAS_API_KEY));
  https.addHeader("Content-Type", "application/json");
  String body = "{\"graphs\": [{\"name\": \"memory\"}]}";

  int code = https.POST(body);
  Serial.printf("[DEBUG] Código HTTP: %d\n", code);
  if (code <= 0) {
    snprintf(buffer, sizeof(buffer), "Error HTTP memoria: %d", code);
    EPD_ShowString(10, 80, buffer, 16, BLACK);
    https.end();
    return;
  }

  String payload = https.getString();
  https.end();
  JSONVar memJson = JSON.parse(payload);
  if (JSON.typeof(memJson) == "undefined" || memJson.length() == 0) {
    EPD_ShowString(10, 80, "Error al parsear memoria", 16, BLACK);
    return;
  }

  JSONVar aggregations = memJson[0]["aggregations"];
  JSONVar mean = aggregations["mean"];
  double mem_free = (double)mean["available"];

  // 3. Cálculo de uso
  double mem_used = total_mem_bytes - mem_free;
  double mem_used_gb = mem_used / 1024.0 / 1024.0 / 1024.0;
  double mem_percent = (mem_used / total_mem_bytes) * 100.0;

  // 4. Mostrar en pantalla
  EPD_ShowString(10, 10, "--- System Status ---", 24, BLACK);
  snprintf(buffer, sizeof(buffer), "Load CPU: %.2f %.2f %.2f", load1, load5, load15);
  EPD_ShowString(10, 50, buffer, 16, BLACK);

  snprintf(buffer, sizeof(buffer), "RAM: %.1f/%.1f GB (%.0f%%)", mem_used_gb, total_mem_gb, mem_percent);
  EPD_ShowString(10, 80, buffer, 16, BLACK);

  // Barra de RAM
  int barraX = 10, barraY = 110, barraW = 360, barraH = 15;
  int barraUsadaW = (int)(barraW * (mem_percent / 100.0));
  EPD_DrawRectangle(barraX, barraY, barraX + barraW, barraY + barraH, BLACK, 0);
  if (barraUsadaW > 0) {
    EPD_DrawRectangle(barraX, barraY, barraX + barraUsadaW, barraY + barraH, BLACK, 1);
  }

  snprintf(buffer, sizeof(buffer), "Uptime: %s", uptime_str.c_str());
  EPD_ShowString(10, 140, buffer, 16, BLACK);

  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  EPD_Sleep();
  Serial.println("✅ Fin de renderizado de SysInfo.");
}

// --- SETUP Y LOOP PRINCIPAL ---
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("📶 Iniciando conexión WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a WiFi.");
  Serial.println(WiFi.localIP());

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);
}

void loop() {
  switch (paginaActual) {
    case 0:
      mostrarPoolsPantalla();
      break;
    case 1:
      mostrarAlertas();
      break;
    case 2:
      mostrarSystemInfo();
      break;
  }

  paginaActual++;
  if (paginaActual > 2) { // 3 páginas (0, 1, 2)
    paginaActual = 0;
  }
  
  Serial.printf("Durmiendo 5 minutos antes de mostrar la página %d...\n", paginaActual);
  delay(1000 * 60 * 5);
}