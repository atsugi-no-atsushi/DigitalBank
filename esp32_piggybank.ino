#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>  // LEDテープ用（WS2812B想定）
#include "driver/i2s.h"

// ===== Wi-Fi設定 =====
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== サーバー設定 =====
const char* serverUrl = "https://your-server.com/api/latest-savings?deviceId=demo-01";

// ===== LED設定 =====
#define LED_PIN  5
#define LED_COUNT 16
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ===== I2S (MAX98357A) 設定 =====
#define I2S_BCLK  26
#define I2S_LRC   25
#define I2S_DIN   22

long lastCheckMs = 0;
long checkIntervalMs = 5000;  // 5秒ごとにサーバー確認
int lastEventId = 0;          // 前回処理したイベントID


// ===== WiFi 接続 =====
void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

// ===== I2S セットアップ =====
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DIN,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ===== LED 初期化 =====
void setupLeds() {
  strip.begin();
  strip.show();
}

// ===== チャリン音（ダミー） =====
void playCharinSound() {
  // 本気で WAV 再生をしたいなら後で実装する
}

// ===== LED アニメーション（花のように光る） =====
void runLedAnimation() {
  for (int j = 0; j < 3; j++) {
    for (int b = 0; b < 255; b += 10) {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(b, b, 0)); // 黄色
      }
      strip.show();
      delay(20);
    }

    for (int b = 255; b >= 0; b -= 10) {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(b, b, 0));
      }
      strip.show();
      delay(20);
    }
  }
}

// ===== サーバー確認 =====
void checkServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(serverUrl);
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    int eventId = payload.toInt();

    if (eventId > lastEventId) {
      Serial.printf("New event! id=%d\n", eventId);
      playCharinSound();
      runLedAnimation();
      lastEventId = eventId;
    }

  } else {
    Serial.printf("HTTP error: %d\n", code);
  }

  http.end();
}

// ===== setup =====
void setup() {
  Serial.begin(115200);
  setupWiFi();
  setupI2S();
  setupLeds();
}

// ===== loop =====
void loop() {
  long now = millis();
  if (now - lastCheckMs > checkIntervalMs) {
    checkServer();
    lastCheckMs = now;
  }
}
