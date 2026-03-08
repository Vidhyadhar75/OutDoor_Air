#include <HardwareSerial.h>

HardwareSerial co2Serial(1);

byte request[] = {0xFE,0x44,0x00,0x08,0x02,0x9F,0x25};

void setup() {

  Serial.begin(115200);

  co2Serial.begin(9600, SERIAL_8N1, 4, 5); // RX, TX

  Serial.println("SenseAir S8 CO2 Sensor Started");
}

void loop() {

  co2Serial.write(request,7);
  delay(50);

  if(co2Serial.available() >= 7)
  {
    byte response[7];
    co2Serial.readBytes(response,7);

    int co2 = (response[3] << 8) | response[4];

    Serial.print("CO2: ");
    Serial.print(co2);
    Serial.println(" ppm");
  }

  delay(2000);
}