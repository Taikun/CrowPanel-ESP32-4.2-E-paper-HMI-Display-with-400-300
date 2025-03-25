#include <Arduino.h>           // Include the Arduino core library
#include "EPD.h"               // Include the EPD library for controlling the electronic ink screen (E-Paper Display)
#include "EPD_GUI.h"           // Include the EPD_GUI library for graphical user interface (GUI) operations
#include "Ap_29demo.h"         // Include the Ap_29demo library, which may be a custom application library
#include <WiFi.h>              // Include the WiFi library for Wi-Fi functionality
#include <Ticker.h>            // Include the Ticker library for timer tasks
#include <WebServer.h>         // Include the WebServer library for creating a web server
#include "FS.h"                // Include the file system library for file operations
#include "SPIFFS.h"            // Include the SPIFFS library for the SPIFFS file system

// Define an array to store image data, with a size of 15000 bytes
uint8_t ImageBW[15000];

// Define the sizes of txt and pre image data
#define txt_size 3808
#define pre_size 4576

// Clear all content on the display
void clear_all()
{
  EPD_Clear(); // Clear the display content
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE); // Create a new image buffer, filled with white
  EPD_Full(WHITE); // Fill the entire screen with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW); // Update the display content
}

// Create a WebServer instance, listening on port 80
WebServer server(80);

// Define the hotspot name
const char *AP_SSID = "ESP32_Config"; // Configure the ESP32 hotspot name

// Define the HTML form for uploading files
String HTML_UPLOAD = "<form method=\"post\" action=\"ok\" enctype=\"multipart/form-data\">\
<input type=\"file\" name=\"msg\">\
<input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submit\">\
</form>";

// Handle root directory requests, return the upload form
void handle_root()
{
  server.send(200, "text/html", HTML_UPLOAD); // Send the upload form to the client
}

// HTML page after successful upload
String HTML_OK = "<!DOCTYPE html>\
<html>\
<body>\
<h1>OK</h1>\
</body>\
</html>";

// File object for storing uploaded files
File fsUploadFile;  // File object used to store the uploaded file

// Define an array for storing uploaded image data
unsigned char test1[15000]; // Temporary array for storing image data
unsigned char price_formerly[pre_size]; // Array for storing price image data
unsigned char txt_formerly[txt_size]; // Array for storing text image data

String filename; // String to store the filename

// Flags to mark different image data
int flag_txt = 0; // Flag to mark whether text image data exists
int flag_pre = 0; // Flag to mark whether price image data exists

// Function to handle file upload requests
void okPage()
{
  server.send(200, "text/html", HTML_OK); // Return the page indicating successful upload
  HTTPUpload &upload = server.upload(); // Get the uploaded file

  // Note: The default size of upload.buf may not be sufficient to handle large files
  // Please refer to the definition of HTTP_UPLOAD_BUFLEN in WebServer.h, adjust the initial size to support large file transfers
  if (upload.status == UPLOAD_FILE_END) // If file upload is complete
  {
    Serial.println("draw file"); // Print debug information: file upload complete
    Serial.println(upload.filename); // Print the uploaded filename
    Serial.println(upload.totalSize); // Print the size of the uploaded file
    if (upload.totalSize == txt_size) // Check if the uploaded file size matches the predefined txt_size
      filename = "txt.bin"; // Set the filename to txt.bin
    else
      filename = "pre.bin"; // Otherwise, set the filename to pre.bin
    if (!filename.startsWith("/")) filename = "/" + filename; // If the filename does not start with '/', add '/' prefix
    fsUploadFile = SPIFFS.open(filename, FILE_WRITE); // Open the file for writing
    fsUploadFile.write(upload.buf, upload.totalSize); // Write the uploaded file data to the file
    fsUploadFile.close(); // Close the file
    Serial.println("Saved successfully"); // Print the message that the save was successful
    Serial.printf("Saved: "); // Print the saved filename
    Serial.println(filename);
    
    // Copy the uploaded file data to the corresponding buffer array
    if (upload.totalSize == txt_size)
    {
      for (int i = 0; i < txt_size; i++) {
        txt_formerly[i] = upload.buf[i]; // Copy data to txt_formerly
      }
      Serial.println("txt_formerly OK"); // Print the message that txt_formerly has been successfully updated
      flag_txt = 1; // Set the flag_txt to 1
    } else
    {
      for (int i = 0; i < pre_size; i++) {
        price_formerly[i] = upload.buf[i]; // Copy data to price_formerly
      }
      Serial.println(" price_formerly OK"); // Print the message that price_formerly has been successfully updated
      flag_pre = 1; // Set the flag_pre to 1
    }

    clear_all();  // Clear all content on the display

    // Display the background image
    EPD_ShowPicture(0, 0, EPD_W, 40, background_top, WHITE); // Display the background image on the screen, with the background color as white

    if (upload.totalSize != txt_size) // If the uploaded file size does not match txt_size
    {
      if (flag_txt == 1) // If flag_txt is 1
      {
        // Display images from txt_formerly and price_formerly
        EPD_ShowPicture(20, 60, 272, 112, txt_formerly, WHITE); // Display the txt_formerly image
        EPD_ShowPicture(20, 190, 352, 104, price_formerly, WHITE); // Display the price_formerly image
      } else
      {
        // Display only the price_formerly image
        EPD_ShowPicture(20, 190, 352, 104, price_formerly, WHITE); // Display the price_formerly image
      }
    } else
    {
      // When the uploaded file size equals txt_size, process different display content
      if (flag_pre == 1) // If flag_pre is 1
      {
        // Display images from price_formerly and txt_formerly
        EPD_ShowPicture(20, 190, 352, 104, price_formerly, WHITE); // Display the price_formerly image
        EPD_ShowPicture(20, 60, 272, 112, txt_formerly, WHITE); // Display the txt_formerly image
      } else
      {
        // Display only the txt_formerly image
        EPD_ShowPicture(20, 60, 272, 112, txt_formerly, WHITE); // Display the txt_formerly image
      }
    }
    
    // Quickly display the image stored in the Image_BW array
    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

    EPD_Sleep(); // Enter sleep mode
  }
}

void setup() {
  // Initialize serial communication, set baud rate to 115200
  Serial.begin(115200);

  // Start the SPIFFS file system
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Started.");
  } else {
    // If SPIFFS fails to start, try formatting the SPIFFS partition
    if (SPIFFS.format()) {
      Serial.println("SPIFFS partition formatted successfully");
      ESP.restart(); // Restart the device after successful formatting
    } else {
      Serial.println("SPIFFS partition format failed");
    }
    return;
  }

  // Print connection information
  Serial.println("Try Connecting to ");

  // Set WiFi to Access Point (AP) mode
  WiFi.mode(WIFI_AP);
  // Start the WiFi hotspot, AP_SSID is the SSID of the hotspot, and the password is empty
  boolean result = WiFi.softAP(AP_SSID, "");
  if (result) {
    IPAddress myIP = WiFi.softAPIP();
    // Print the IP address and MAC address of the hotspot
    Serial.println("");
    Serial.print("Soft-AP IP address = ");
    Serial.println(myIP);
    Serial.println(String("MAC address = ") + WiFi.softAPmacAddress().c_str());
    Serial.println("waiting ...");
  } else {
    // If starting the hotspot fails, output error information
    Serial.println("WiFiAP Failed");
    delay(3000);
  }

  // Set up routes and handler functions for the HTTP server
  server.on("/", handle_root);
  server.on("/ok", okPage);
  server.begin();
  Serial.println("HTTP server started");
  delay(100);

  // Configure the screen power
  pinMode(7, OUTPUT); // Set GPIO 7 to output mode
  digitalWrite(7, HIGH); // Turn on the screen power
  EPD_GPIOInit();  // Initialize the screen GPIO
  EPD_Clear();     // Clear the screen
  Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);  // Create a new canvas, set the canvas to white
  EPD_Full(WHITE); // Clear the canvas, fill with white
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);    // Display the blank canvas

  EPD_Init_Fast(Fast_Seconds_1_5s);  // Initialize the screen, set update speed to 1.5 seconds

  UI_price();  // Display the price interface
}

void loop() {
  // Handle client requests
  server.handleClient();
}

// UI_price function: Update the price display interface
void UI_price() {
  clear_all();  // Clear all display content
  //
  EPD_ShowPicture(0, 0, EPD_W, 40, background_top, WHITE); // Display the top background image on the screen, with the background color as white

  if (SPIFFS.exists("/txt.bin")) {
    // If the file exists, read the file content
    File file = SPIFFS.open("/txt.bin", FILE_READ);
    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }
    // Read data from the file into the array
    size_t bytesRead = file.read(txt_formerly, txt_size);

    Serial.println("File content:");
    while (file.available()) {
      Serial.write(file.read()); // Print the file content
    }
    file.close();

    EPD_ShowPicture(20, 60, 272, 112, txt_formerly, WHITE); // Display the read image on the screen, with the background color as white
    flag_txt = 1; // Set the flag
  }

  if (SPIFFS.exists("/pre.bin")) {
    // If the file exists, read the file content
    File file = SPIFFS.open("/pre.bin", FILE_READ);
    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }
    // Read data from the file into the array
    size_t bytesRead = file.read(price_formerly, pre_size);

    Serial.println("File content:");
    while (file.available()) {
      Serial.write(file.read()); // Print the file content
    }
    file.close();

    EPD_ShowPicture(20, 190, 352, 104, price_formerly, WHITE); // Display the read image on the screen, with the background color as white
    flag_pre = 1; // Set the flag
  }

  // Display the image stored in the Image_BW array
  EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

  EPD_Sleep(); // Enter sleep mode to conserve energy
}