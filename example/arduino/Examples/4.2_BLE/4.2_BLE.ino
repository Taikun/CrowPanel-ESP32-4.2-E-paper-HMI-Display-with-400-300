#include <Arduino.h>
#include "EPD.h"                  // e-paper display library
#include "EPD_GUI.h"              // e-paper display graphical interface library
#include "BLEDevice.h"            // BLE device library
#include "BLEServer.h"            // BLE server library
#include "BLEUtils.h"             // BLE utility functions library
#include "BLE2902.h"              // BLE descriptor library

// Define BLE service and characteristic UUIDs
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // RX characteristic UUID
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // TX characteristic UUID

// Declare BLE object pointers
BLECharacteristic *pCharacteristic;
BLEServer *pServer;
BLEService *pService;

// Flag to track whether the device is connected
bool deviceConnected = false;

// BLE data buffer
char BLEbuf[32] = {0};

// BLE server callback class
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("------> BLE connect ."); // Prompt when the device connects
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("------> BLE disconnect ."); // Prompt when the device disconnects
      pServer->startAdvertising(); // Restart advertising to allow reconnection
      Serial.println("start advertising");
    }
};

// BLE characteristic callback class
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue(); // Read the value written to the characteristic

      if (rxValue.length() > 0) {
        Serial.print("------>Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]); // Print each received character
        }
        Serial.println();

        // Respond based on the received characters
        if (rxValue.find("A") != -1) {
          Serial.print("Rx A!");
        }
        else if (rxValue.find("B") != -1) {
          Serial.print("Rx B!");
        }
        Serial.println();
      }
    }
};

// e-paper display image buffer
uint8_t Image_BW[15000];

void setup() {
  pinMode(7, OUTPUT); // Set pin 7 to output for power control
  digitalWrite(7, HIGH); // Enable e-paper display power

  BLEDevice::init("CrowPanel4-2"); // Initialize BLE device and set device name
  pServer = BLEDevice::createServer(); // Create BLE server
  pServer->setCallbacks(new MyServerCallbacks()); // Set server callbacks

  pService = pServer->createService(SERVICE_UUID); // Create BLE service
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY); // Create TX characteristic
  pCharacteristic->addDescriptor(new BLE2902()); // Add notification descriptor
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE); // Create RX characteristic
  pCharacteristic->setCallbacks(new MyCallbacks()); // Set RX characteristic callbacks

  pService->start(); // Start BLE service
  pServer->getAdvertising()->start(); // Start advertising to allow device connections

  EPD_GPIOInit(); // Initialize e-paper display
}

int flag = 0;

void loop() {
  if (deviceConnected) {
    memset(BLEbuf, 0, 32); // Clear the buffer
    memcpy(BLEbuf, (char*)"Hello BLE APP!", 32); // Set the buffer value
    pCharacteristic->setValue(BLEbuf); // Update characteristic value
    pCharacteristic->notify(); // Notify connected devices

    Serial.print("*** Sent Value: ");
    Serial.print(BLEbuf); // Print the sent value
    Serial.println(" ***");

    if (flag != 2)
      flag = 1; // Update flag to indicate device connection status
  } else {
    if (flag != 4)
      flag = 3; // Update flag to indicate device disconnection status
  }

  // Update e-paper display based on connection status
  if (flag == 1) {
    char buffer[30];
    clear_all(); // Clear the display
    strcpy(buffer, "Bluetooth connected");
    strcpy(BLEbuf, "Sent Value:");
    strcat(BLEbuf, "Hello BLE APP!");
    EPD_ShowString(0, 0 + 0 * 20, buffer, 16, BLACK); // Display connection status
    EPD_ShowString(0, 0 + 1 * 20, BLEbuf, 16, BLACK); // Display sent value
    EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Update the display
    EPD_Sleep(); // Put the display into sleep mode

    flag = 2; // Update flag
  } else if (flag == 3) {
    char buffer[30];
    clear_all(); // Clear the display
    strcpy(buffer, "Bluetooth not connected!");
    EPD_ShowString(0, 0 + 0 * 20, buffer, 16, BLACK); // Display disconnection status
    EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Update the display
    EPD_Sleep(); // Put the display into sleep mode

    flag = 4; // Update flag
  }

  delay(1000); // Wait for 1 second
}

// Function to clear the e-paper display
void clear_all() {
  EPD_Clear(); // Clear the display
  Paint_NewImage(Image_BW, EPD_W, EPD_H, 0, WHITE); // Initialize a new image with a white background
  EPD_Full(WHITE); // Clear the canvas
  EPD_Display_Part(0, 0, EPD_W, EPD_H, Image_BW); // Show the cleared image
}