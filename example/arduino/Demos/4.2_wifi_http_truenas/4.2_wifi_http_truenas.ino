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

  // --- OPTIMIZACIÓN: Obtener datos de discos UNA SOLA VEZ ---
  Serial.println("Fetching disk data once...");
  JSONVar allDisks = JSON.parse(httpGETTrueNAS("disk"));
  if (JSON.typeof(allDisks) == "undefined") {
      Serial.println("❌ Error parsing JSON from /disk");
  } else {
      Serial.println("✅ Disk data fetched correctly.");
  }

  for (int i = 0; i < pools.length(); i++) {
    String poolName = (const char*)pools[i]["name"];
    String poolStatus = (const char*)pools[i]["status"];

    double size = (double)pools[i]["size"];
    double allocated = (double)pools[i]["allocated"];
    double usedPercent = 0.0;
    if (size > 0) {
      usedPercent = (allocated / size) * 100.0;
    }

    Serial.printf("Pool: %s, Used: %.1f%%\n", poolName.c_str(), usedPercent);

    int barraX = 20;
    int barraY = y + 20;
    int barraW = 360;
    int barraH = 15;
    int barraUsadaW = min((int)(barraW * (usedPercent / 100.0)), barraW);

    // 🖋️ Texto: nombre y estado
    snprintf(buffer, sizeof(buffer), "Pool: %s [%s]", poolName.c_str(), poolStatus.c_str());
    EPD_ShowString(10, y, buffer, 16, BLACK);

    // --- LÓGICA DE DIBUJO CORREGIDA ---

    // 1. 🧱 Dibuja el borde exterior de la barra (rectángulo VACÍO)
    //    Usamos el modo 0 para que solo dibuje el contorno.
    EPD_DrawRectangle(barraX, barraY, barraX + barraW, barraY + barraH, BLACK, 0);

    // 2. ⬛ Dibuja la parte usada (rectángulo RELLENO)
    //    Usamos el modo 1 para rellenar.
    //    Solo dibujamos si el ancho es mayor que 0.
    if (barraUsadaW > 0) {
      EPD_DrawRectangle(barraX, barraY, barraX + barraUsadaW, barraY + barraH, BLACK, 1);
    }
    
    // --- FIN DE LA LÓGICA CORREGIDA ---

    // 📊 Texto porcentaje
    y = barraY + barraH + 5;
    snprintf(buffer, sizeof(buffer), "Used: %.1f%%", usedPercent);
    EPD_ShowString(barraX, y, buffer, 16, BLACK);
    y += 20;

    // 💽 Temperaturas de discos (usando los datos ya descargados)
    if (JSON.typeof(allDisks) != "undefined") {
        for (int j = 0; j < allDisks.length(); j++) {
          if (allDisks[j]["pool"] != NULL && (String)allDisks[j]["pool"] == poolName) {
            String diskName = (const char*)allDisks[j]["name"];
            int temp = (int)allDisks[j]["temperature"];
            snprintf(buffer, sizeof(buffer), "  %s - Temp: %d C", diskName.c_str(), temp);
            EPD_ShowString(20, y, buffer, 16, BLACK);
            y += 20;
          }
        }
    }
    
    y += 15; // Espacio entre pools
  }

  // Envía el búfer completo a la pantalla
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
  
  // Pone la pantalla en modo de bajo consumo
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