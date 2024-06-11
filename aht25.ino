#include <CRC8.h>              // for AHT25
#include <Wire.h>              // for AHT25, SSD1306 (I2C)

/* FOR SENSOR AHT25 */
const int Wire_I2C_SDA = 15;
const int Wire_I2C_SCL = 4;
const byte AHT25_ADDR = 0x38;
const double ERROR_VALUE = 999.0;
CRC8 crc;
double temperature = 0.0;
double humidity = 0.0;

void initAht25(void) {
  Wire.begin(Wire_I2C_SDA, Wire_I2C_SCL);
  delay(100);
  Wire.beginTransmission(AHT25_ADDR);
  Wire.write(0x71);
  Wire.endTransmission();
  delay(10);

  crc.setPolynome(0x31);
  crc.setInitial(0xFF);

  Serial.println(F(" [AHT25] AHT25 has configured"));
}

void updateAht25(void) {
  byte buf[7];
  uint32_t humidity_raw;
  uint32_t temperature_raw;

  Wire.beginTransmission(AHT25_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  Wire.endTransmission();
  do {
    delay(80);
    Wire.requestFrom((uint8_t)AHT25_ADDR, (uint8_t)7);
    if (Wire.available() >= 7) {
      for (int i = 0; i < 7; i++) {
        buf[i] = Wire.read();
      }
    }
  } while ((buf[0] & 0x80) != 0);

  crc.restart();
  crc.add(buf, 6);

  if (buf[6] == crc.calc()) {
    humidity_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (((uint32_t)buf[3] & 0xF0) >> 4);
    temperature_raw = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | ((uint32_t)buf[5]);

    humidity = humidity_raw / 1048576.0 * 100;
    temperature = temperature_raw / 1048576.0 * 200 - 50;
  } else {
    // error
    humidity = ERROR_VALUE;
    temperature = ERROR_VALUE;
  }

  Serial.print(F("[AHT25] Measured: "));
  Serial.print(temperature);
  Serial.print(F(" oC, "));
  Serial.print(humidity);
  Serial.println(F(" %"));
}