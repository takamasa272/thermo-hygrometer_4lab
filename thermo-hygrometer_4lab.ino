#include <Arduino.h>
#include <HTTPClient.h>
#include <time.h>

#include <WiFi.h>
#include "esp_wifi.h"
// for 802.1X authentication
#include "esp_eap_client.h"
// #include "esp_wpa2.h"
// #include <SPI.h> // for WiFiNINA.h?
// #include <WiFiNINA.h>

#include "wifi_credentials.h"     // SSID, EAP_ID, EAP_USERNAME and EAP_PASSWORD
#include "ambient_credentials.h"  // keys for ambient
#include "gas_credentials.h"      // keys for Google Apps Script

#include <CRC8.h>              // for AHT25
#include <Wire.h>              // for AHT25, SSD1306
#include <Adafruit_SSD1306.h>  // SSD1306 display
#include <Adafruit_GFX.h>      // SSD1306 display
#include <Ambient.h>

/* ENABLE/DISABLE FEATURES */
// send data to ambient
const bool ENABLE_AMBIENT = false;
// send data to google apps script
const bool ENABLE_GAS = false;
// use WPA2 Enterprise 802.1X authentication
const bool ENABLE_8021X = false;

/* FOR SENSOR AHT25 */
const int Wire_I2C_SDA = 15;
const int Wire_I2C_SCL = 4;
const byte AHT25_ADDR = 0x38;
const double ERROR_VALUE = 999.0;
CRC8 crc;
double temperature = 0.0;
double humidity = 0.0;

/* FOR OLED SSD1306 */
const byte OLED_ADDR = 0x3C;
const int OLED_WIDTH = 128;    // 幅
const int OLED_HEIGHT = 64;    // 高さ
const int Wire1_I2C_SDA = 19;  // SDA pin
const int Wire1_I2C_SCL = 18;  // SCL pin
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire1);

/* FOR W-Fi */
// wifi info (from wifi_credentials.h)
const char* ssid = WIFI_SSID;

// #define EAP_ID "youruserid" // for WPA2-EAP
// #define EAP_USERNAME "youruserid"
// #define EAP_PASSWORD "yourpassword"
esp_eap_ttls_phase2_types ttlsPhase2Type = ESP_EAP_TTLS_PHASE2_MSCHAPV2;

const char* password = WIFI_PASS;  // for WPA2-PSK

/* FOR Ambient */
unsigned int channelId = AMBIENT_CHANNELID;  // From ambient_credentials.h
const char* writeKey = AMBIENT_WRITEKEY;     // From ambient_credentials.h
WiFiClient client;
Ambient ambient;

/* FOR Google Apps Script */
const String gasUrl = GAS_URL;  // From gas_credentials.h

// ntp
struct tm timeInfo;
const char* dayofweek[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char year[4][6];
char date[20], hour_minute[20];

bool kougo;

void initAht25(void) {
  delay(100);
  Wire.beginTransmission(AHT25_ADDR);
  Wire.write(0x71);
  Wire.endTransmission();
  delay(10);

  crc.setPolynome(0x31);
  crc.setStartXOR(0xFF);
}

void updateAht25(void) {
  byte buf[7];
  uint32_t humidity_raw;
  uint32_t temperature_raw;
  byte state;

  Wire.beginTransmission(AHT25_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();
  do {
    delay(80);
    Wire.requestFrom(AHT25_ADDR, 7);
    if (Wire.available() >= 7) {
      for (int i = 0; i < 7; i++) {
        buf[i] = Wire.read();
      }
    }
  } while ((buf[0] & 0x80) != 0);

  crc.restart();
  crc.add(buf, 6);

  if (buf[6] == crc.getCRC()) {
    state = buf[0];
    humidity_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (((uint32_t)buf[3] & 0xF0) >> 4);
    temperature_raw = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | ((uint32_t)buf[5]);

    humidity = humidity_raw / 1048576.0 * 100;
    temperature = temperature_raw / 1048576.0 * 200 - 50;
  } else {
    // error
    humidity = ERROR_VALUE;
    temperature = ERROR_VALUE;
  }
}

void ShowTempHumid(float fukai, long rssi) {
  getLocalTime(&timeInfo);  //tmオブジェクトのtimeInfoに現在時刻を入れ込む
  sprintf(year[0], "%04d/", timeInfo.tm_year + 1900);
  sprintf(date, "%02d/%02d", timeInfo.tm_mon + 1, timeInfo.tm_mday);  //日付に変換
  sprintf(hour_minute, "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);

  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(year[0]);
  display.print(date);
  display.println(hour_minute);

  display.print(F("RSSI: "));
  display.print(rssi);
  display.println(F(" dBm"));

  display.setTextSize(2);
  if (temperature != ERROR_VALUE) {
    display.print(temperature, 2);  // (temp, 小数点以下桁数)

    // display "℃"
    display.print(F(" "));
    display.setTextSize(1);
    display.print(F("o"));
    display.setTextSize(2);
    display.println(F("C"));
  } else {
    display.println(F("ERROR"));
  }

  if (humidity != ERROR_VALUE) {
    display.print(humidity, 2);
    display.println(F(" %"));
  } else {
    display.println("ERROR");
  }
  display.print(F("Fukai: "));
  display.println(fukai);
  display.display();
}

void SendToAmbient(void) {
  ambient.set(1, (float)temperature);
  ambient.set(2, (float)humidity);

  ambient.send();
  Serial.println(F(" [AMBIENT] Data send to ambient"));
}

void SendToGoogleApps(void) {
  //Google Spreadsheet
  String urlFinal = gasUrl + "?temperature=" + String(temperature) + "&humidity=" + String(humidity);

  HTTPClient http;
  http.begin(urlFinal.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.print(F(" [GAS] HTTP Status Code: "));
  Serial.println(httpCode);
  //---------------------------------------------------------------------
  //getting response from google sheet
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println(" [GAS] Payload: " + payload);
  }
  //---------------------------------------------------------------------
  http.end();
  Serial.println(F(" [GAS] Send data to Google SpreadSheet"));
}

void setup(void) {
  Serial.begin(115200);
  delay(10);
  Serial.print(F("Hello! AHT25 Thermo-hygrometer"));

  // wifi connect
  Serial.println("");
  Serial.print(F("[Wi-Fi] Try connecting to "));
  Serial.println(ssid);

  WiFi.disconnect(true);
  Serial.println(F("DEBUG 1 [Wi-Fi] disconnected"));
  WiFi.mode(WIFI_STA);  //init wifi mode
  // WiFi.mode(WIFI_IF_STA);  //init wifi mode
  if (ENABLE_8021X) {
    // For "esp_wpa2.h"
    // esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)EAP_ID, strlen(EAP_ID));
    // esp_wifi_sta_wpa2_ent_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
    // esp_wifi_sta_wpa2_ent_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    // esp_wifi_sta_wpa2_ent_enable();

    // OR For "esp_eap_client.h"
    Serial.println(F("DEBUG 2 [Wi-Fi] begin to initiate EAP client"));
    esp_eap_client_set_identity((uint8_t*)EAP_ID, strlen(EAP_ID));              //provide identity
    esp_eap_client_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));  //provide username
    esp_eap_client_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));  //provide password
    esp_eap_client_set_ttls_phase2_method(ttlsPhase2Type);
    Serial.println(F("DEBUG 2.5 [Wi-Fi] korekara Set up EAP"));
    esp_wifi_sta_enterprise_enable();
    Serial.println(F("DEBUG 3 [Wi-Fi] Set up EAP"));
    WiFi.begin(ssid);
  } else {
    WiFi.begin(ssid, password);
  }
  // WiFi.beginEnterprise(ssid, EAP_USERNAME, EAP_USERNAME, EAP_ID);
  Serial.println(F("DEBUG 4 [Wi-Fi] Wifi began"));

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F(" [Wi-Fi] Try connecting to WiFi.."));
    delay(1000);
  }

  Serial.println(F(""));
  Serial.println(F(" [Wi-Fi] Connected to the WiFi network"));
  Serial.print(F("  IP: "));
  Serial.println(WiFi.localIP());
  if (ENABLE_8021X) {
    Serial.print(F("  User ID: "));
    Serial.println(EAP_USERNAME);
  }

  /* Initiate AHT25 */
  Wire.begin(Wire_I2C_SDA, Wire_I2C_SCL);
  initAht25();

  Serial.println(F(" [AHT25] AHT25 has configured"));

  // initiate OLED SSD1306
  Wire1.begin(Wire1_I2C_SDA, Wire1_I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("!ERROR: [OLED] FAILED to initiate SSD1306"));
  } else {
    Serial.println(F(" [OLED] Initiated OLED display SSD1306"));
  }

  // NTP setting
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

  Serial.println(" [NTP] NTP has configured");

  //  チャネルIDとライトキーを指定してAmbientの初期化
  if (ENABLE_AMBIENT) ambient.begin(channelId, writeKey, &client);

  Serial.println(F("<< Initialized >>"));
}

void loop() {
  // time
  if ((WiFi.status() == WL_CONNECTED)) {
    // measure Temperature and humidity
    updateAht25();
    Serial.println("[AHT25] Measured");

    delay(30);

    float fukai = 0.81 * temperature + 0.01 * humidity * (0.99 * temperature - 14.3) + 46.3;

    // wifi connected
    long rssi = WiFi.RSSI();  // rcvd sig strength indicator [dBm]
    Serial.print("[Wi-Fi] RSSI: ");
    Serial.print(rssi);
    Serial.println("dBm");

    ShowTempHumid(fukai, rssi);

    // send to ambient (temp. and humid.)
    // あんまりデータ送るとよくないので，2回に1回にする．
    if (kougo) {
      if (ENABLE_AMBIENT) SendToAmbient();
      if (ENABLE_GAS) SendToGoogleApps();
    }

    kougo = kougo ? false : true;
    delay(29965);

  } else {
    Serial.println(F("Wi-Fi not connected"));
  }
}
