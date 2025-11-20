// ===== ANCHOR with OLED: ESP32 UWB Pro with Display v1.4 =====
#include <SPI.h>
#include <Wire.h>
#include "DW1000Ranging.h"
#include "DW1000.h"

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

// Anchor address (must be different from tag’s)
char ANCHOR_ADDR[8] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x01};

float lastDistance = 0.0f;

void drawDistance(float d) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Distance");
  display.setTextSize(3);
  display.setCursor(0, 30);
  display.print(d, 2);
  display.println(" m");
  display.display();
}

void newRange() {
  DW1000Device *dev = DW1000Ranging.getDistantDevice();
  if (!dev) return;
  lastDistance = dev->getRange();   // meters

  Serial.print("Range: ");
  Serial.print(lastDistance);
  Serial.println(" m");

  drawDistance(lastDistance);
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

  // SPI for UWB
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // I2C + display
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (1) delay(10);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("UWB Anchor");
  display.display();

  // UWB
  DW1000Ranging.initCommunication(UWB_RST, UWB_SS,  UWB_IRQ);
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  // Start as ANCHOR
  DW1000Ranging.startAsAnchor(ANCHOR_ADDR,
                              DW1000.MODE_LONGDATA_RANGE_ACCURACY,
                              false);  // false = no random delays
}

void loop() {
  DW1000Ranging.loop();
}
