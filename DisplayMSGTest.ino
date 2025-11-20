// ===== ANCHOR with OLED: ESP32 UWB Pro with Display v1.4 - ESP-NOW RECEIVER =====
#include <SPI.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include "DW1000Ranging.h"
#include "DW1000.h"
#include <esp_wifi.h>  // Add this for WiFi channel functions
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// UWB pins for Pro with display (from Makerfabs docs)
#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23

#define UWB_SS    21   // chip select
#define UWB_RST   27   // reset
#define UWB_IRQ   34   // irq

// OLED I2C pins (on-board SSD1306)
#define I2C_SDA   4
#define I2C_SCL   5
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Anchor address (must be different from tag's)
char ANCHOR_ADDR[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x01};

float lastDistance = 0.0f;
String lastReceivedMessage = "Waiting...";
unsigned long lastMessageTime = 0;

// Structure to receive data (must match sender)
typedef struct struct_message {
  char message[50];
  float distance;
} struct_message;

struct_message incomingData;

// Updated callback signature for newer ESP32 core (ESP-IDF 5.x)
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingDataPtr, int len) {
  Serial.println("=== ESP-NOW DATA RECEIVED ===");
  
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  lastReceivedMessage = String(incomingData.message);
  lastMessageTime = millis();
  
  Serial.print("Message: '");
  Serial.print(lastReceivedMessage);
  Serial.print("' (distance from sender: ");
  Serial.print(incomingData.distance);
  Serial.println(" m)");
  
  // Update display with new message
  drawDistanceAndMessage(lastDistance, lastReceivedMessage);
}

void drawDistanceAndMessage(float d, String msg) {
  display.clearDisplay();
  
  // Draw message at top
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(msg);
  
  // Draw a line separator
  display.drawLine(0, 12, SCREEN_WIDTH, 12, SSD1306_WHITE);
  
  // Draw distance
  display.setTextSize(1);
  display.setCursor(0, 18);
  display.println("Distance:");
  display.setTextSize(3);
  display.setCursor(0, 32);
  display.print(d, 2);
  display.println("m");
  
  display.display();
}

void newRange() {
  DW1000Device *dev = DW1000Ranging.getDistantDevice();
  if (!dev) return;
  lastDistance = dev->getRange();   // meters

  //Serial.print("UWB Range: ");
  //Serial.print(lastDistance);
  //Serial.println(" m");

  drawDistanceAndMessage(lastDistance, lastReceivedMessage);
}

void newDevice(DW1000Device *dev) {
  Serial.print("Tag joined: ");
  Serial.println(dev->getShortAddress(), HEX);
}

void inactiveDevice(DW1000Device *dev) {
  Serial.println("Tag inactive");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("ANCHOR with ESP-NOW Receiver");

  // Init WiFi in STA mode for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Set WiFi to channel 1 explicitly
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  Serial.print("ANCHOR MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  String mac = WiFi.macAddress();
  if (mac == "00:00:00:00:00:00") {
    Serial.println("ERROR: Invalid MAC address! Restarting...");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("Copy this MAC address to the TAG code!");

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    delay(1000);
    ESP.restart();
  }
  
  Serial.println("ESP-NOW initialized successfully");
  
  // Register callback for received data
  if (esp_now_register_recv_cb(OnDataRecv) != ESP_OK) {
    Serial.println("Failed to register receive callback");
  } else {
    Serial.println("Receive callback registered successfully");
  }

  // SPI for UWB
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // I2C + display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (1) delay(10);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("UWB Anchor");
  display.println("ESP-NOW Ready");
  display.display();
  delay(2000);

  // UWB
  DW1000Ranging.initCommunication(UWB_RST, UWB_SS,  UWB_IRQ);
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  // Start as ANCHOR
  DW1000Ranging.startAsAnchor(ANCHOR_ADDR,
                              DW1000.MODE_LONGDATA_RANGE_ACCURACY,
                              false);
  
  Serial.println("Setup complete!");
  Serial.println("Listening for ESP-NOW messages...");
}

void loop() {
  DW1000Ranging.loop();
  
  // Debug: print if no message received in a while
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 5000) {
    Serial.println("Still listening for ESP-NOW...");
    lastDebug = millis();
  }
}