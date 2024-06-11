#include <Adafruit_SSD1306.h>  // SSD1306 display
#include <Adafruit_GFX.h>      // SSD1306 display

/* FOR OLED SSD1306 */
const byte OLED_ADDR = 0x3C;
const int OLED_WIDTH = 128;    // 幅
const int OLED_HEIGHT = 64;    // 高さ
const int Wire1_I2C_SDA = 19;  // SDA pin
const int Wire1_I2C_SCL = 18;  // SCL pin
#define OLED_RESET -1          // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire1, OLED_RESET);

void initOLED(void) {
  Wire1.begin(Wire1_I2C_SDA, Wire1_I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("!ERROR: [OLED] FAILED to initiate SSD1306"));
  } else {
    Serial.println(F(" [OLED] Initiated OLED display SSD1306"));
    display.display();    // initial logo
    display.cp437(true);  // for bug cp437
  }
}

void handleOLED(long rssi) {
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
  display.print(F(" "));
  display.println(hour_minute);

  display.print(F("Wi-Fi RSSI: "));
  display.print(rssi);
  display.println(F(" dBm"));

  display.setCursor(0, 20);
  display.setTextSize(2);
  if (temperature != ERROR_VALUE) {
    display.print(temperature, 2);  // (temp, 小数点以下桁数)
    display.print(F(" "));
    display.write(0xF8);  // degree sign
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

  float fukai = 0.81 * temperature + 0.01 * humidity * (0.99 * temperature - 14.3) + 46.3;
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(F("Fukai index: "));
  display.println((int16_t)fukai);

  display.display();
  Serial.println(F("[OLED] Display updated"));
}