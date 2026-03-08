
# 🌍 Outdoor Air Quality Monitoring System

A **solar-powered IoT outdoor air monitoring station** built using **ESP32-C6** that measures particulate matter, CO₂, temperature, humidity, VOC, and NOx.
The system publishes real-time environmental data to an **MQTT broker** and displays system status on a **TFT dashboard**.

Designed for **smart city deployments, environmental monitoring, and edge IoT analytics**.

---

# 📊 System Overview

The system integrates multiple high-precision sensors from **Sensirion** and **Senseair**, powered by a **solar charging system**, and connected to the cloud via WiFi.

### Sensors Used

| Sensor              | Measurement            |
| ------------------- | ---------------------- |
| Sensirion **SPS30** | PM1, PM2.5, PM4, PM10  |
| Sensirion **SHT4x** | Temperature & Humidity |
| Sensirion **SGP41** | VOC & NOx Gas Index    |
| **Senseair S8**     | CO₂ Concentration      |

---

# ⚡ Hardware Architecture

Main Controller:

**ESP32-C6**

Features:

* WiFi connectivity
* Low power IoT MCU
* Multiple communication buses
* Edge processing capability

---

# 🖥 System Diagram

Below is the wiring architecture of the full system.

<img width="1920" height="1080" alt="Circuit Diagram" src="https://github.com/user-attachments/assets/7e9ae420-acaf-4ddf-9490-05b11078a9e7" />


*(Use the diagram image in your repository docs folder)*

---

# 🔌 Hardware Connections

## ESP32-C6 Pin Mapping

### TFT Display (ILI9341 SPI)

| TFT Pin | ESP32-C6 Pin | Description    |
| ------- | ------------ | -------------- |
| VCC     | 3.3V         | Power          |
| GND     | GND          | Ground         |
| CS      | GPIO10       | Chip Select    |
| DC      | GPIO8        | Data / Command |
| RST     | GPIO9        | Reset          |
| MOSI    | GPIO7        | SPI Data       |
| SCLK    | GPIO6        | SPI Clock      |
| LED     | 3.3V         | Backlight      |
| MISO    | Not used     | Optional       |

---

### I²C Sensors (Shared Bus)

| Sensor | SDA   | SCL   |
| ------ | ----- | ----- |
| SPS30  | GPIO1 | GPIO2 |
| SHT4x  | GPIO1 | GPIO2 |
| SGP41  | GPIO1 | GPIO2 |

All sensors share the same **I²C bus**.

---

### CO₂ Sensor (UART)

| Signal          | ESP32 Pin |
| --------------- | --------- |
| CO₂ TX → ESP RX | GPIO4     |
| CO₂ RX → ESP TX | GPIO5     |
| VCC             | 5V        |
| GND             | GND       |

---

# 🔋 Power System

The station runs on **solar energy**.

Components:

| Component               | Purpose          |
| ----------------------- | ---------------- |
| Solar Panel             | Generates power  |
| Solar Charge Controller | Battery charging |
| 12V Battery             | Energy storage   |
| Buck Converter          | Step-down to 5V  |
| ESP32 Power Input       | System supply    |

This makes the station **fully deployable outdoors**.

---

# 📡 MQTT Data Topics

The system publishes data to MQTT topics:

| Topic          | Data        |
| -------------- | ----------- |
| `air/temp`     | Temperature |
| `air/humidity` | Humidity    |
| `air/co2`      | CO₂         |
| `air/voc`      | VOC Index   |
| `air/nox`      | NOx Index   |
| `air/pm1`      | PM1         |
| `air/pm25`     | PM2.5       |
| `air/pm4`      | PM4         |
| `air/pm10`     | PM10        |

---

# 📟 TFT Display

The TFT dashboard shows:

* Temperature
* Humidity
* CO₂ levels
* VOC index
* NOx index
* PM1 / PM2.5 / PM4 / PM10

With **color-coded air quality indicators**.

---

# 🚀 Features

✔ Solar Powered
✔ Real-time Air Quality Monitoring
✔ Edge IoT with ESP32-C6
✔ MQTT Data Streaming
✔ Multi-sensor Fusion
✔ TFT Dashboard
✔ Outdoor Deployment Ready

---

# 🧠 Sensor Data Processing

The system uses **Sensirion gas index algorithms**:

```
VOCGasIndexAlgorithm
NOxGasIndexAlgorithm
```

These convert raw gas sensor signals into **standardized air quality indices**.

---

# 🛠 Installation

### 1️⃣ Install Arduino Libraries

Required libraries:

```
PubSubClient
Adafruit_GFX
Adafruit_ILI9341
Sensirion SPS30
Sensirion SHT4x
Sensirion SGP41
VOCGasIndexAlgorithm
NOxGasIndexAlgorithm
```

---

### 2️⃣ Configure WiFi

Update credentials:

```
const char* ssid = "YOUR_WIFI";
const char* password = "PASSWORD";
```

---

### 3️⃣ Configure MQTT

```
const char* mqtt_server = "BROKER_IP";
```

---

### 4️⃣ Upload Code

Flash the firmware using **Arduino IDE** or **PlatformIO**.

---

# 📊 Example Serial Output

```
------ AIR QUALITY ------

Temp: 29.4 C
Humidity: 60 %

VOC Index: 73
NOx Index: 2

CO2: 420 ppm

PM1.0 : 5
PM2.5 : 8
PM4.0 : 10
PM10  : 12
```

---

# 🌎 Applications

Smart City Monitoring
Environmental Research
Outdoor Pollution Tracking
Industrial Air Quality Monitoring
IoT Environmental Networks

---

# 🔮 Future Improvements

* AQI calculation
* LoRaWAN connectivity
* Edge AI pollution prediction
* Weather station integration
* Cloud dashboards
* OTA firmware updates

---

# 🧑‍💻 Author

**Vidhyadhara Rao Kotagiri**
B.Tech IoT — KL University

---

# ⭐ If you like this project

Give the repo a **star ⭐ on GitHub**.

---

