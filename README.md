# ESP32-C6 Gas Monitor System

This is a gas and environment monitoring project I built using an ESP32-C6 board. It reads two gas sensors and a temperature/humidity sensor, sends the readings to ThingsBoard over MQTT, and also raises an instant alarm if gas levels cross a danger threshold.

## What it does

- Reads an MQ6 sensor (detects LPG/butane) and an MQ9 sensor (detects CO) and converts the readings to approximate ppm values
- Reads temperature and humidity from a DHT11 sensor
- Sends all this data to ThingsBoard over MQTT every couple of seconds
- Each gas sensor module also has a built-in digital alarm pin. If gas crosses the sensor's own threshold, it triggers a hardware interrupt that sends an alarm message over MQTT right away, separate from the normal data updates
- Reconnects WiFi on its own if the connection drops while running

## Hardware used

- ESP32-C6 board
- MQ6 gas sensor (analog pin + digital alarm pin)
- MQ9 gas sensor (analog pin + digital alarm pin)
- DHT11 temperature and humidity sensor

Pin connections are set in `config.h`:
- MQ6 analog: pin 1
- MQ9 analog: pin 3
- MQ6 digital alarm: pin 5
- MQ9 digital alarm: pin 6
- DHT11: pin 7

## Files in this project

- **ESP32-GAS-Detection.ino** – Main file. Sets everything up (WiFi, MQTT, sensors, interrupts) and starts three background tasks that keep running on their own.
- **config.h** – All the settings in one place: WiFi name/password, ThingsBoard host and token, pin numbers, timing intervals, and gas alarm thresholds. Ships with placeholder values, fill in your own before flashing.
- **sensor.h / sensor.cpp** – Reads the gas sensors and DHT11, and converts the raw gas sensor readings into approximate ppm values.
- **mqtt_handler.h / mqtt_handler.cpp** – Handles connecting to ThingsBoard over MQTT and sending the data.

## How it works (the task structure)

The program runs three separate background tasks at the same time using FreeRTOS (built into the ESP32 core), instead of doing everything one after another in a single loop:

1. **sensor_task** – reads all sensors once every second and stores the latest reading
2. **alarm_task** – sits idle until a gas sensor's digital alarm pin triggers an interrupt, then immediately sends an alarm message over MQTT
3. **mqtt_task** – every 2 seconds, makes sure WiFi is still connected and sends the latest sensor reading to ThingsBoard

## How to use it

1. Open `ESP32-GAS-Detection.ino` in the Arduino IDE
2. Install the required libraries: WiFi, PubSubClient, DHT11 (search these by name in the Library Manager)
3. Select ESP32-C6 under Tools > Board
4. Edit `config.h` with your own WiFi details and ThingsBoard host/token
5. Wire up the sensors to the pins listed in `config.h`
6. Upload, then open the Serial Monitor at 115200 baud to see live readings
