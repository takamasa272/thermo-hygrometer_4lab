#include <HTTPClient.h>
#include <time.h>
#include <WiFi.h>
#include <Ambient.h>

/* headerfile for secrets */
#include "wifi_credentials.h"     // SSID, EAP_ID, EAP_USERNAME and EAP_PASSWORD
#include "ambient_credentials.h"  // keys for ambient
#include "gas_credentials.h"      // keys for Google Apps Script

/* FOR W-Fi */
// wifi info (from wifi_credentials.h)
const char* ssid = WIFI_SSID;
// #define EAP_ID "youruserid" // for WPA2-EAP
// #define EAP_USERNAME "youruserid"
// #define EAP_PASSWORD "yourpassword"
const char* password = WIFI_PASS;  // for WPA2-PSK

/* FOR Ambient */
unsigned int channelId = AMBIENT_CHANNELID;  // From ambient_credentials.h
const char* writeKey = AMBIENT_WRITEKEY;     // From ambient_credentials.h
WiFiClient client;
Ambient ambient;

/* FOR Google Apps Script */
const String gasUrl = GAS_URL;  // From gas_credentials.h

/* FOR ntp */
struct tm timeInfo;
char year[4][6];
char date[20], hour_minute[20];

void initWiFi(void) {
  Serial.print(F("[Wi-Fi] Try connecting to "));
  Serial.println(ssid);

  WiFi.disconnect();
  WiFi.mode(WIFI_MODE_STA);  //init wifi mode

  if (ENABLE_8021X) {
    // WPA2 Enterprise
    WiFi.begin(ssid, WPA2_AUTH_PEAP, EAP_ID, EAP_USERNAME, EAP_PASSWORD);  //without CERTIFICATE
  } else {
    // WPA2 Personal
    WiFi.begin(ssid, password);
  }
  Serial.println(F(" [Wi-Fi] Try connecting to WiFi.."));
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    ++i;
    if (i % 50 != 0) {
      Serial.print(F("."));
    } else {
      Serial.println(F("."));
    }
    delay(10);
  }

  Serial.println(F(""));
  Serial.println(F(" [Wi-Fi] Connected to the WiFi network"));
  Serial.print(F("  IP: "));
  Serial.println(WiFi.localIP());
  if (ENABLE_8021X) {
    Serial.print(F("  User ID: "));
    Serial.println(EAP_USERNAME);
  }
}

void initNTP(void) {
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
  Serial.println(F(" [NTP] NTP has configured"));
}

long fetchRSSI(void) {
  Serial.print(F("[Wi-Fi] RSSI: "));
  Serial.print(WiFi.RSSI());
  Serial.println(F("dBm"));

  return WiFi.RSSI();
}

void initAmbient(void) {
  ambient.begin(channelId, writeKey, &client);
}

void sendToAmbient(void) {
  ambient.set(1, (float)temperature);
  ambient.set(2, (float)humidity);

  bool result = ambient.send();
  if (result) {
    Serial.println(F(" [AMBIENT] Data send to ambient"));
  } else {
    Serial.println(F(" [AMBIENT] !ERROR: FAILED to send Data to ambient"));
  }
}

void sendToGoogleApps(void) {
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
    Serial.print(F(" [GAS] Payload: "));
    Serial.println(payload);
  }
  //---------------------------------------------------------------------
  http.end();
  Serial.println(F(" [GAS] Send data to Google SpreadSheet"));
}