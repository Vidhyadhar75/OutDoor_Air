#include <Arduino.h>
#include <Wire.h>

#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>

SensirionI2cSht4x sht4x;
SensirionI2CSgp41 sgp41;

VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;

uint16_t conditioning_s = 10;
char errorMessage[128];

void setup() {

  Serial.begin(115200);
  delay(1000);

  // ✅ Correct I2C pins for XIAO ESP32-S3
  Wire.begin(5, 6);

  // Start sensors
  sht4x.begin(Wire, SHT40_I2C_ADDR_44);
  sgp41.begin(Wire);

  sht4x.softReset();
  delay(10);

  Serial.println("SHT4x + SGP41 Started...");
}

void loop() {

  uint16_t error;

  float temperature = 0;
  float humidity = 0;

  uint16_t srawVoc = 0;
  uint16_t srawNox = 0;

  uint16_t compensationRh;
  uint16_t compensationT;

  delay(1000);   // 1Hz required

  // 🔹 1️⃣ Read Temperature & Humidity
  error = sht4x.measureHighPrecision(temperature, humidity);

  if (error) {
    Serial.print("SHT4x Error: ");
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
    return;
  }

  // 🔹 2️⃣ Convert to ticks for SGP41 compensation
  compensationT = (uint16_t)((temperature + 45) * 65535 / 175);
  compensationRh = (uint16_t)(humidity * 65535 / 100);

  // 🔹 3️⃣ Read SGP41
  if (conditioning_s > 0) {
    error = sgp41.executeConditioning(compensationRh, compensationT, srawVoc);
    conditioning_s--;
  } else {
    error = sgp41.measureRawSignals(compensationRh, compensationT, srawVoc, srawNox);
  }

  if (error) {
    Serial.print("SGP41 Error: ");
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
    return;
  }

  // 🔹 4️⃣ Process Gas Index
  int32_t voc_index = voc_algorithm.process(srawVoc);
  int32_t nox_index = nox_algorithm.process(srawNox);

  // 🔹 5️⃣ Print Everything
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" °C | RH: ");
  Serial.print(humidity);
  Serial.print(" % | VOC Index: ");
  Serial.print(voc_index);
  Serial.print(" | NOx Index: ");
  Serial.println(nox_index);
}