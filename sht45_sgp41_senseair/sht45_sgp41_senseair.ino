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

HardwareSerial co2Serial(1);

uint16_t conditioning_s = 10;
char errorMessage[128];

byte request[] = {0xFE,0x44,0x00,0x08,0x02,0x9F,0x25};

void setup() {

  Serial.begin(115200);
  delay(1000);

  // I2C
  Wire.begin(6,7);

  sht4x.begin(Wire, SHT40_I2C_ADDR_44);
  sgp41.begin(Wire);

  sht4x.softReset();
  delay(10);

  // CO2 UART
  co2Serial.begin(9600, SERIAL_8N1, 4, 5);

  Serial.println("Air Quality System Started");
}

void loop() {

  uint16_t error;

  float temperature = 0;
  float humidity = 0;

  uint16_t srawVoc = 0;
  uint16_t srawNox = 0;

  uint16_t compensationRh;
  uint16_t compensationT;

  int co2ppm = 0;

  delay(1000);

  // -------- SHT4x --------
  error = sht4x.measureHighPrecision(temperature, humidity);

  if (error) {
    Serial.print("SHT4x Error: ");
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
    return;
  }

  compensationT = (uint16_t)((temperature + 45) * 65535 / 175);
  compensationRh = (uint16_t)(humidity * 65535 / 100);

  // -------- SGP41 --------
  if (conditioning_s > 0) {
    error = sgp41.executeConditioning(compensationRh, compensationT, srawVoc);
    conditioning_s--;
  } 
  else {
    error = sgp41.measureRawSignals(compensationRh, compensationT, srawVoc, srawNox);
  }

  if (error) {
    Serial.print("SGP41 Error: ");
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
    return;
  }

  int32_t voc_index = voc_algorithm.process(srawVoc);
  int32_t nox_index = nox_algorithm.process(srawNox);

  // -------- SenseAir S8 CO2 --------
  co2Serial.write(request,7);
  delay(50);

  if(co2Serial.available() >= 7)
  {
    byte response[7];
    co2Serial.readBytes(response,7);
    co2ppm = (response[3] << 8) | response[4];
  }

  // -------- Print Data --------
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | RH: ");
  Serial.print(humidity);

  Serial.print(" | VOC Index: ");
  Serial.print(voc_index);

  Serial.print(" | NOx Index: ");
  Serial.print(nox_index);

  Serial.print(" | CO2: ");
  Serial.print(co2ppm);
  Serial.println(" ppm");
}