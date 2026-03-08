#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSps30.h>
#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>

#define NO_ERROR 0

SensirionI2cSps30 sps30;
SensirionI2cSht4x sht4x;
SensirionI2CSgp41 sgp41;

VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;

HardwareSerial co2Serial(1);

static char errorMessage[128];
static int16_t error;

uint16_t conditioning_s = 10;

byte request[] = {0xFE,0x44,0x00,0x08,0x02,0x9F,0x25};

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println("Air Quality System Start");

  // I2C BUS (ALL SENSORS)
  Wire.begin(6,7);

  // -------- SPS30 --------
  sps30.begin(Wire, SPS30_I2C_ADDR_69);
  sps30.stopMeasurement();

  int8_t serialNumber[32] = {0};
  int8_t productType[8] = {0};

  sps30.readSerialNumber(serialNumber, 32);
  Serial.print("SPS30 Serial: ");
  Serial.println((const char*)serialNumber);

  sps30.readProductType(productType, 8);
  Serial.print("Product: ");
  Serial.println((const char*)productType);

  Serial.println("Starting SPS30 measurement...");
  sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);

  // -------- SHT4x + SGP41 --------
  sht4x.begin(Wire, SHT40_I2C_ADDR_44);
  sgp41.begin(Wire);
  sht4x.softReset();

  // -------- CO2 SENSOR --------
  co2Serial.begin(9600, SERIAL_8N1, 4, 5);

  Serial.println("Sensor warming up...");
  delay(10000);
}

void loop() {

  // -------- VARIABLES --------
  uint16_t dataReadyFlag = 0;

  uint16_t mc1p0, mc2p5, mc4p0, mc10p0;
  uint16_t nc0p5, nc1p0, nc2p5, nc4p0, nc10p0;
  uint16_t typicalParticleSize;

  float temperature = 0;
  float humidity = 0;

  uint16_t srawVoc = 0;
  uint16_t srawNox = 0;

  uint16_t compensationRh;
  uint16_t compensationT;

  int co2ppm = 0;

  // -------- SPS30 --------
  error = sps30.readDataReadyFlag(dataReadyFlag);

  if (error != NO_ERROR) {
    Serial.print("DataReadyFlag error: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
  }

  if (dataReadyFlag) {

    error = sps30.readMeasurementValuesUint16(
        mc1p0, mc2p5, mc4p0, mc10p0,
        nc0p5, nc1p0, nc2p5, nc4p0,
        nc10p0, typicalParticleSize);

    if (error != NO_ERROR) {
      Serial.print("Read error: ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
    }
  }

  // -------- SHT4x --------
  error = sht4x.measureHighPrecision(temperature, humidity);

  if (error) {
    Serial.print("SHT4x Error: ");
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
  }

  compensationT = (uint16_t)((temperature + 45) * 65535 / 175);
  compensationRh = (uint16_t)(humidity * 65535 / 100);

  // -------- SGP41 --------
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
  }

  int32_t voc_index = voc_algorithm.process(srawVoc);
  int32_t nox_index = nox_algorithm.process(srawNox);

  // -------- CO2 --------
  co2Serial.write(request,7);
  delay(50);

  if(co2Serial.available() >= 7)
  {
    byte response[7];
    co2Serial.readBytes(response,7);
    co2ppm = (response[3] << 8) | response[4];
  }

  // -------- PRINT DATA --------
  Serial.println("------ AIR QUALITY ------");

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | RH: ");
  Serial.println(humidity);

  Serial.print("VOC Index: ");
  Serial.println(voc_index);

  Serial.print("NOx Index: ");
  Serial.println(nox_index);

  Serial.print("CO2: ");
  Serial.print(co2ppm);
  Serial.println(" ppm");

  Serial.print("PM1.0 : "); Serial.println(mc1p0);
  Serial.print("PM2.5 : "); Serial.println(mc2p5);
  Serial.print("PM4.0 : "); Serial.println(mc4p0);
  Serial.print("PM10  : "); Serial.println(mc10p0);

  Serial.println("-------------------------");

  delay(2000);
}