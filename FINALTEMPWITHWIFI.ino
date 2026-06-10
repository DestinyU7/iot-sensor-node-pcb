#include "Arduino.h"
#include <SoftwareSerial.h>
#include "Grove_Temperature_And_Humidity_Sensor.h"

#define RESET_PIN 8          // pin used to bring ESP-01 module up in UART mode
#define UART_TX_PIN 11       // UART software serial transmit pin
#define UART_RX_PIN 10       // UART software serial receive pin

#define DHTTYPE DHT20
#define DHTPIN 2
DHT dht(DHTPIN, DHTTYPE);

// LED pins
#define RED_LED_PIN 13
#define GREEN_LED_PIN 12

SoftwareSerial espSerial(UART_RX_PIN, UART_TX_PIN);  // Create software UART to talk to the ESP8266
String IO_USERNAME = "Timmum";
String IO_KEY = "REPLACE_WITH_ADAFRUIT_IO_KEY";
String WIFI_SSID = "UD Devices";
String WIFI_PASS = "";

float num = 1.0;
float lastTemperature = 0.0;
float lastRiseTemp = 0.0;
const float TEMP_TOLERANCE = 0.1;
const float STABLE_LOW = 22.0;
const float STABLE_HIGH = 24.0;
String response;
long int time;

void setup() {
  Serial.begin(115200);
  espSerial.begin(9600);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, LOW);
  delay(1000);
  digitalWrite(RESET_PIN, HIGH);

  Wire.begin();
  dht.begin();

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  Serial.println("Setup Complete");

  espSend("wifi_ssid=" + WIFI_SSID, 2000, true);
  espSend("wifi_pass=" + WIFI_PASS, 2000, true);
  espSend("io_user=" + IO_USERNAME, 2000, true);
  espSend("io_key=" + IO_KEY, 2000, false);
  espSend("setup_io", 30000, true);
  espSend("setup_pubfeed=1,temperature-feed", 2000, false);
}

void loop() {
  delay(5000);
  float temp_hum_val[2] = {0};

  if (!dht.readTempAndHumidity(temp_hum_val)) {
    float currentTemp = temp_hum_val[1];
    Serial.print("Current Temperature: ");
    Serial.println(currentTemp);
    float tempDifference = currentTemp - lastTemperature;

    if (tempDifference > TEMP_TOLERANCE) {
      lastRiseTemp = currentTemp;
      digitalWrite(RED_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, HIGH);
      delay(250);
      digitalWrite(GREEN_LED_PIN, LOW);
      delay(250);
    } else if (tempDifference < -TEMP_TOLERANCE && currentTemp < lastRiseTemp) {
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
      delay(250);
      digitalWrite(RED_LED_PIN, LOW);
      delay(250);
    } else if (currentTemp >= STABLE_LOW && currentTemp <= STABLE_HIGH) {
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
    }

    response = espSend("send_data=1," + String(currentTemp), 2000, false);
    lastTemperature = currentTemp;
  } else {
    Serial.println("Failed to get temperature and humidity value.");
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  }
}

String espSend(String command, const int timeout, boolean debug) {
  response = "";
  espSerial.println(command);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (espSerial.available()) {
      char c = espSerial.read();
      response += c;
    }
  }
  response.trim();
  if (debug) {
    Serial.println("Resp: " + response);
  }
  return response;
}
