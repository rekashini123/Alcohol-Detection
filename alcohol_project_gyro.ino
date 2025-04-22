#include <WiFi.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// WiFi credentials
const char* ssid = "Pavi";
const char* password = "Max@0925";

// ThingSpeak info (manual HTTP request)
const char* host = "api.thingspeak.com";
const char* writeAPIKey = "H6X8KBTLGX25JTIE";

// Sensor pins
#define DHTPIN 4
#define DHTTYPE DHT11
#define VIBRATION_PIN 14
#define ALCOHOL_PIN 34

DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(9600); // Changed to 9600 as requested
  dht.begin();
  pinMode(VIBRATION_PIN, INPUT);

  // Connect to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found! Check connections.");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 initialized.");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int vibration = digitalRead(VIBRATION_PIN);
  int alcoholValue = analogRead(ALCOHOL_PIN);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.println("=== Sensor Readings ===");
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Vibration Detected: "); Serial.println(vibration ? "Yes" : "No");
  Serial.print("Alcohol Level: "); Serial.println(alcoholValue);
  Serial.print("Accel X: "); Serial.print(a.acceleration.x);
  Serial.print(" | Y: "); Serial.print(a.acceleration.y);
  Serial.print(" | Z: "); Serial.println(a.acceleration.z);

  // Create HTTP GET request URL
  String url = "/update?api_key=";
  url += writeAPIKey;
  url += "&field1=" + String(temperature);
  url += "&field2=" + String(humidity);
  url += "&field3=" + String(vibration);
  url += "&field4=" + String(alcoholValue);
  url += "&field5=" + String(a.acceleration.x);
  url += "&field6=" + String(a.acceleration.y);
  url += "&field7=" + String(a.acceleration.z);

  // Send data to ThingSpeak
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection to ThingSpeak failed");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Sending data to ThingSpeak...");

  // Wait for response (optional)
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  client.stop();
  Serial.println("\nData sent!\n");

  delay(3000); // Wait 3 seconds before next reading
}
