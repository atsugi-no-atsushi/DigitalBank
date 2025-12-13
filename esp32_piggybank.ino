/*
 * Production-ready firmware for the DigitalBank piggy bank.
 *
 * Responsibilities (as described in the project spec)
 *   - Connect to Wi‑Fi and stay connected, attempting reconnection if
 *     the network drops.
 *   - Periodically poll a backend API (e.g. `/api/latest-savings?deviceId=<id>`)
 *     to check for new savings events. When a new event ID is returned
 *     that is higher than the last processed ID, trigger a sound
 *     ("charin") and LED animation.
 *   - Persist the last seen event ID in non‑volatile storage so that
 *     repeated events aren’t replayed after a restart or power loss.
 *   - Initialise hardware peripherals: Wi‑Fi, I2S audio output for
 *     the MAX98357A DAC, and an addressable LED strip (WS2812B).
 *
 * Compared to the prototype, this sketch includes several
 * improvements:
 *   • Wi‑Fi reconnection handling and a configurable reconnect
 *     interval.
 *   • Storage of the last event ID using the Preferences API.
 *   • A basic beep implementation for the charin sound using I2S
 *     without an external WAV file.
 *   • HTTPS support via WiFiClientSecure when the server URL is
 *     secure (starts with https). The root CA certificate should be
 *     provided if certificate validation is required.
 *   • Configurable server URL and polling interval via constants.
 *   • Improved LED animation that simulates a simple bloom effect.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "driver/i2s.h"
#include <math.h>

// ==== Wi‑Fi configuration ====
// Replace with your actual SSID and password. For security in
// production, consider storing these in a separate header or secure
// credential store.
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Reconnection settings
const unsigned long wifiReconnectIntervalMs = 30000; // 30 seconds between reconnection attempts
unsigned long lastWifiReconnectAttempt = 0;

// ==== Server configuration ====
// The base URL of the API endpoint. Include your device ID as a
// query parameter. You can also store the device ID separately and
// append it in code if preferred.
const char* serverUrl = "https://your-server.com/api/latest-savings?deviceId=demo-01";
// HTTP client timeout (ms)
const uint16_t httpTimeoutMs = 5000;

// ==== LED configuration ====
#define LED_PIN   5
#define LED_COUNT 16
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ==== I2S (MAX98357A) configuration ====
#define I2S_BCLK  26
#define I2S_LRC   25
#define I2S_DIN   22
// I2S sample rate for the beep tone
const uint32_t SAMPLE_RATE = 44100;

// ==== Polling settings ====
const unsigned long checkIntervalMs = 5000; // Poll every 5 seconds
unsigned long lastCheckTime = 0;

// ==== Persistent storage ====
Preferences preferences;
int lastEventId = 0; // Will be initialised from preferences in setup()

// ==== Forward declarations ====
void setupWiFi();
void reconnectWiFiIfNeeded();
void setupI2S();
void setupLeds();
void playCharinSound();
void runLedAnimation();
void checkServer();

void setup() {
  Serial.begin(115200);
  // Open preferences namespace for storing lastEventId persistently
  preferences.begin("piggybank", false);
  lastEventId = preferences.getInt("lastEventId", 0);
  Serial.printf("Last known event ID: %d\n", lastEventId);
  setupWiFi();
  setupI2S();
  setupLeds();
}

void loop() {
  // Reconnect Wi‑Fi if necessary
  reconnectWiFiIfNeeded();
  // Check the server periodically
  unsigned long now = millis();
  if (now - lastCheckTime >= checkIntervalMs) {
    checkServer();
    lastCheckTime = now;
  }
  // Optionally, sleep or do other work here
}

// ===== Wi‑Fi connection and reconnection =====
void setupWiFi() {
  delay(100);
  Serial.printf("Connecting to Wi‑Fi SSID: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWi‑Fi connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWi‑Fi connection failed");
  }
}

void reconnectWiFiIfNeeded() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - lastWifiReconnectAttempt < wifiReconnectIntervalMs) {
    return;
  }
  lastWifiReconnectAttempt = now;
  Serial.println("Attempting to reconnect to Wi‑Fi...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
}

// ===== I2S setup =====
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  // Mute output initially by writing zeros
  size_t bytes_written;
  int16_t zero = 0;
  for (int i = 0; i < 200; ++i) {
    i2s_write(I2S_NUM_0, &zero, sizeof(zero), &bytes_written, portMAX_DELAY);
  }
}

// ===== LED setup =====
void setupLeds() {
  strip.begin();
  strip.show();
  strip.setBrightness(50); // Lower initial brightness to avoid glare
}

// ===== Play a simple beep via I2S =====
void playCharinSound() {
  // Generate a short sine wave beep on the fly. For a richer sound or
  // music, preload audio data into a buffer or play from SPIFFS. The
  // MAX98357A expects 16‑bit signed PCM samples at the configured
  // sample rate.
  const float frequency = 2000.0f; // 2 kHz
  const int durationMs = 300;      // 0.3 seconds
  const int totalSamples = (SAMPLE_RATE * durationMs) / 1000;
  const float twoPiF = 2.0f * PI * frequency;
  size_t bytes_written;
  for (int i = 0; i < totalSamples; ++i) {
    float t = (float)i / (float)SAMPLE_RATE;
    float sample = sinf(twoPiF * t);
    int16_t val = (int16_t)(sample * 32767);
    i2s_write(I2S_NUM_0, &val, sizeof(val), &bytes_written, portMAX_DELAY);
  }
}

// ===== LED animation (simple bloom) =====
void runLedAnimation() {
  // Fade in and out a warm yellow color several times to simulate
  // a coin drop glow.
  for (int cycle = 0; cycle < 3; ++cycle) {
    // Fade in
    for (int brightness = 0; brightness <= 255; brightness += 10) {
      for (int i = 0; i < LED_COUNT; ++i) {
        strip.setPixelColor(i, strip.Color(brightness, brightness, 0));
      }
      strip.show();
      delay(15);
    }
    // Fade out
    for (int brightness = 255; brightness >= 0; brightness -= 10) {
      for (int i = 0; i < LED_COUNT; ++i) {
        strip.setPixelColor(i, strip.Color(brightness, brightness, 0));
      }
      strip.show();
      delay(15);
    }
  }
  // Turn off after animation
  for (int i = 0; i < LED_COUNT; ++i) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

// ===== HTTP polling =====
void checkServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi‑Fi not connected; skipping server check");
    return;
  }

  HTTPClient http;
  http.setTimeout(httpTimeoutMs);
  // Use secure client if URL is HTTPS; otherwise default WiFiClient
  if (String(serverUrl).startsWith("https://")) {
    // In a production environment you should load your CA certificate
    // here. Without a CA, setInsecure() disables certificate validation.
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure();
    http.begin(*client, serverUrl);
  } else {
    http.begin(serverUrl);
  }
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    int eventId = payload.toInt();
    if (eventId > lastEventId) {
      Serial.printf("New event detected: %d (previous %d)\n", eventId, lastEventId);
      playCharinSound();
      runLedAnimation();
      lastEventId = eventId;
      preferences.putInt("lastEventId", lastEventId);
    }
  } else if (httpCode > 0) {
    Serial.printf("HTTP error %d: %s\n", httpCode, http.errorToString(httpCode).c_str());
  } else {
    Serial.printf("HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}