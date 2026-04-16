#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";
const char* API_URL = "http://192.168.1.10:8000/metrics";

const unsigned long FETCH_INTERVAL_MS = 10000;
const unsigned long PAGE_INTERVAL_MS = 5000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

float cpuPercent = 0.0f;
float diskFreeGb = 0.0f;
float powerWatts = 0.0f;
int containersRunning = 0;
int containersInactive = 0;

unsigned long lastFetchMs = 0;
unsigned long lastPageChangeMs = 0;
unsigned long lastWifiAttemptMs = 0;
uint8_t currentPage = 0;

// Bitmap XBM de exemplo 32x32.
const unsigned char PROGMEM logoBitmap[] = {
  0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07,
  0xf0, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xfe, 0x7f,
  0xfe, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f,
  0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07,
  0xf0, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f, 0xfe, 0x7f,
  0xfe, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f,
  0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00
};

void drawHeader(const char* title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

void drawLogoPage() {
  drawHeader("Tela 1 - Logo");
  display.drawBitmap(48, 18, logoBitmap, 32, 32, SSD1306_WHITE);
  display.display();
}

void drawSystemPage() {
  drawHeader("Tela 2 - Sistema");
  display.setCursor(0, 18);
  display.setTextSize(2);
  display.printf("CPU: %.1f%%\n", cpuPercent);
  display.setTextSize(1);
  display.printf("Livre: %.2f GB", diskFreeGb);
  display.display();
}

void drawContainersPage() {
  drawHeader("Tela 3 - Compose");
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.printf("%d / %d", containersRunning, containersInactive);
  display.setTextSize(1);
  display.setCursor(0, 46);
  display.println("Ativos / Inativos");
  display.display();
}

void drawEnergyPage() {
  drawHeader("Tela 4 - Energia");
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.printf("%.1f W", powerWatts);
  display.setTextSize(1);
  display.setCursor(0, 46);
  display.println("Estimativa instantanea");
  display.display();
}

void renderCurrentPage() {
  switch (currentPage) {
    case 0:
      drawLogoPage();
      break;
    case 1:
      drawSystemPage();
      break;
    case 2:
      drawContainersPage();
      break;
    case 3:
      drawEnergyPage();
      break;
  }
}

void startWifiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void fetchMetrics() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, API_URL)) {
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(1536);
    DeserializationError error = deserializeJson(doc, http.getString());

    if (!error) {
      cpuPercent = doc["cpu"]["percent"] | 0.0f;
      diskFreeGb = doc["disk"]["free_gb"] | 0.0f;
      powerWatts = doc["energy_estimate"]["instant_power_watts"] | 0.0f;
      containersRunning = doc["containers"]["running"] | 0;
      containersInactive = doc["containers"]["inactive"] | 0;
    }
  }

  http.end();
}

void setup() {
  Wire.begin(D2, D1);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  startWifiConnection();
  renderCurrentPage();
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED && now - lastWifiAttemptMs >= WIFI_RETRY_INTERVAL_MS) {
    lastWifiAttemptMs = now;
    startWifiConnection();
  }

  if (now - lastFetchMs >= FETCH_INTERVAL_MS) {
    lastFetchMs = now;
    fetchMetrics();
    renderCurrentPage();
  }

  if (now - lastPageChangeMs >= PAGE_INTERVAL_MS) {
    lastPageChangeMs = now;
    currentPage = (currentPage + 1) % 4;
    renderCurrentPage();
  }
}
