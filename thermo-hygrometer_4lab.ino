#include <Arduino.h>

/*****************************************
  ENABLE/DISABLE FEATURES
******************************************/
// send data to ambient
const bool ENABLE_AMBIENT = false;
// send data to google apps script
const bool ENABLE_GAS = false;
// use WPA2 Enterprise (802.1X authentication)
const bool ENABLE_8021X = true;

void setup(void) {
  Serial.begin(115200);
  delay(10);
  Serial.println(F("\n*****************************************************"));
  Serial.println(F("*    ESP32 + AHT25 + SSD1306 Thermo-hygrometer      *"));
  Serial.println(F("*****************************************************\n"));

  /* Wi-Fi connect */
  initWiFi();

  /* Initiate AHT25 */
  initAht25();

  /* initiate OLED SSD1306 */
  initOLED();

  /* initiate NTP setting */
  initNTP();

  /* initiate Ambient */
  if (ENABLE_AMBIENT) { initAmbient(); }

  Serial.println(F("[SYSTEM] Initialized"));
}

void loop() {
  /* measure Temperature and humidity */
  updateAht25();

  /* fetch Wi-Fi RSSI */
  long rssi = fetchRSSI();

  /* Update OLED Display */
  handleOLED(rssi);

  /* send to ambient/GAS (temp. and humid.) */
  // あんまりデータ送るとよくないので，5minに一回したい
  if (false) {
    if (ENABLE_AMBIENT) sendToAmbient();
    if (ENABLE_GAS) sendToGoogleApps();
  }

  delay(29920);
}
