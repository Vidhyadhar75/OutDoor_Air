#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include <SensirionI2cSps30.h>
#include <SensirionI2cSht4x.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>

#define NO_ERROR 0

/* TFT PINS (same as your working code) */

#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9
#define TFT_MOSI 7
#define TFT_SCLK 6

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/* WIFI */

const char* ssid = "KLEF";
const char* password = "";

/* MQTT */

const char* mqtt_server = "10.32.5.57";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long wifiRetry = 0;
unsigned long mqttRetry = 0;
unsigned long screenTimer = 0;

/* Sensors */

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

/* WIFI CHECK */

void checkWiFi() {

  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - wifiRetry > 10000) {
    wifiRetry = millis();
    Serial.println("Trying WiFi...");
    WiFi.begin(ssid, password);
  }
}

/* MQTT CHECK */

void checkMQTT() {

  if (WiFi.status() != WL_CONNECTED) return;

  if (client.connected()) return;

  if (millis() - mqttRetry > 5000) {
    mqttRetry = millis();
    Serial.println("Trying MQTT...");
    client.connect("ESP32_AIR_NODE");
  }
}

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println("Air Quality System Start");

  /* TFT INIT */

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
SPI.setFrequency(20000000);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(80,10);
  tft.print("AIR MONITOR");

  /* I2C */

  Wire.begin(1,2);

  /* SPS30 */

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

  sps30.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_UINT16);

  /* SHT4x + SGP41 */

  sht4x.begin(Wire, SHT40_I2C_ADDR_44);
  sgp41.begin(Wire);
  sht4x.softReset();

  /* CO2 */

  co2Serial.begin(9600, SERIAL_8N1, 4, 5);

  /* WIFI + MQTT */

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);

  Serial.println("Sensor warming up...");
  delay(10000);
}

void loop() {

  checkWiFi();
  checkMQTT();
  client.loop();

  uint16_t dataReadyFlag = 0;

  uint16_t mc1p0=0, mc2p5=0, mc4p0=0, mc10p0=0;
  uint16_t nc0p5, nc1p0, nc2p5, nc4p0, nc10p0;
  uint16_t typicalParticleSize;

  float temperature = 0;
  float humidity = 0;

  uint16_t srawVoc = 0;
  uint16_t srawNox = 0;

  uint16_t compensationRh;
  uint16_t compensationT;

  int co2ppm = 0;

  /* SPS30 */

  error = sps30.readDataReadyFlag(dataReadyFlag);

  if (dataReadyFlag) {

    error = sps30.readMeasurementValuesUint16(
        mc1p0, mc2p5, mc4p0, mc10p0,
        nc0p5, nc1p0, nc2p5, nc4p0,
        nc10p0, typicalParticleSize);
  }

  /* SHT4x */

  error = sht4x.measureHighPrecision(temperature, humidity);

  if (error) {
    errorToString(error, errorMessage, sizeof(errorMessage));
    Serial.println(errorMessage);
  }

  compensationT = (uint16_t)((temperature + 45) * 65535 / 175);
  compensationRh = (uint16_t)(humidity * 65535 / 100);

  /* SGP41 */

  if (conditioning_s > 0) {

    error = sgp41.executeConditioning(compensationRh, compensationT, srawVoc);
    conditioning_s--;

  } else {

    error = sgp41.measureRawSignals(compensationRh, compensationT, srawVoc, srawNox);
  }

  int32_t voc_index = voc_algorithm.process(srawVoc);
  int32_t nox_index = nox_algorithm.process(srawNox);

  /* CO2 */

  co2Serial.write(request,7);
  delay(50);

  if(co2Serial.available() >= 7)
  {
    byte response[7];
    co2Serial.readBytes(response,7);
    co2ppm = (response[3] << 8) | response[4];
  }

  /* SERIAL PRINT */

  Serial.println("------ AIR QUALITY ------");

  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" C | RH: "); Serial.println(humidity);

  Serial.print("VOC Index: "); Serial.println(voc_index);
  Serial.print("NOx Index: "); Serial.println(nox_index);

  Serial.print("CO2: "); Serial.print(co2ppm);
  Serial.println(" ppm");

  Serial.print("PM1.0 : "); Serial.println(mc1p0);
  Serial.print("PM2.5 : "); Serial.println(mc2p5);
  Serial.print("PM4.0 : "); Serial.println(mc4p0);
  Serial.print("PM10  : "); Serial.println(mc10p0);

  Serial.println("-------------------------");

  /* TFT DISPLAY */
/* TFT DISPLAY */

tft.fillScreen(ILI9341_BLACK);

/* Header Bar */

tft.fillRect(0,0,240,30,ILI9341_BLUE);

tft.setTextColor(ILI9341_WHITE);
tft.setTextSize(2);
tft.setCursor(40,8);
tft.print("AIR QUALITY");

/* Temperature */

tft.setTextColor(ILI9341_CYAN);
tft.setCursor(10,40);
tft.print("Temp: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(temperature);

/* Humidity */

tft.setTextColor(ILI9341_BLUE);
tft.setCursor(10,65);
tft.print("Humidity: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(humidity);

/* CO2 */

tft.setTextColor(ILI9341_ORANGE);
tft.setCursor(10,90);
tft.print("CO2: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(co2ppm);

/* VOC */

tft.setTextColor(ILI9341_MAGENTA);
tft.setCursor(10,115);
tft.print("VOC: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(voc_index);

/* NOx */

tft.setTextColor(ILI9341_YELLOW);
tft.setCursor(10,140);
tft.print("NOx: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(nox_index);

/* PM1 */

tft.setCursor(10,165);
tft.setTextColor(ILI9341_GREEN);
tft.print("PM1: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(mc1p0);

/* PM2.5 with color indicator */

tft.setCursor(10,190);

if(mc2p5 < 12)
tft.setTextColor(ILI9341_GREEN);
else if(mc2p5 < 35)
tft.setTextColor(ILI9341_YELLOW);
else
tft.setTextColor(ILI9341_RED);

tft.print("PM2.5: ");
tft.setTextColor(ILI9341_WHITE);
tft.print(mc2p5);

/* PM4 */

tft.setCursor(10,215);
tft.setTextColor(ILI9341_GREEN);
tft.print("PM4: ");

tft.setTextColor(ILI9341_WHITE);
tft.print(mc4p0);

/* PM10 */

tft.setCursor(10,240);

if(mc10p0 < 50)
tft.setTextColor(ILI9341_GREEN);
else if(mc10p0 < 100)
tft.setTextColor(ILI9341_YELLOW);
else
tft.setTextColor(ILI9341_RED);

tft.print("PM10: ");
tft.setTextColor(ILI9341_WHITE);
tft.print(mc10p0);
  /* MQTT */

  if(client.connected()) {

  char msg[20];

  dtostrf(temperature,4,2,msg);
  client.publish("air/temp",msg);

  dtostrf(humidity,4,2,msg);
  client.publish("air/humidity",msg);

  sprintf(msg,"%d",voc_index);
  client.publish("air/voc",msg);

  sprintf(msg,"%d",nox_index);
  client.publish("air/nox",msg);

  sprintf(msg,"%d",co2ppm);
  client.publish("air/co2",msg);

  sprintf(msg,"%d",mc1p0);
  client.publish("air/pm1",msg);

  sprintf(msg,"%d",mc2p5);
  client.publish("air/pm25",msg);

  sprintf(msg,"%d",mc4p0);
  client.publish("air/pm4",msg);

  sprintf(msg,"%d",mc10p0);
  client.publish("air/pm10",msg);
  }

  delay(2000);
}