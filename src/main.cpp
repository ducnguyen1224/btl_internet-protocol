#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <ESP32Servo.h>

// Define pins
#define SOIL_MOISTURE_PIN 35  // Pin connected to the soil moisture sensor
#define LDR_PIN 34            // Pin connected to the LDR
#define LIGHT_PIN  2          // Pin connected to the first LED or relay
#define PUMP_PIN  4           // Pin connected to the second LED or relay
#define FAN_PIN  5            // Pin connected to the third LED or relay
#define SERVO_PIN 13          // GPIO pin connected to the servo motor

// DHT sensor setup
const int DHT_PIN = 21; 
DHTesp dht;

// WiFi and MQTT credentials
const char* ssid = "Kreideprinz";
const char* password = "20030409";
const char* mqtt_server = "test.mosquitto.org";

// MQTT client and other variables
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
float temp = 0;
float hum = 0;
int soilMoisture = 0;
int ldrValue = 0;

// Servo object
Servo myservo;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println();

  // Control Servo via MQTT
  if (String(topic) == "/ThinkIOT/Servo") {
    if (message == "true") {
      myservo.write(90);  // Rotate servo to 90 degrees
      Serial.println("Servo rotated to 90 degrees");
    } else if (message == "false") {
      myservo.write(0);   // Rotate servo back to 0 degrees
      Serial.println("Servo rotated to 0 degrees");
    }
  }
  
  // Control Light via MQTT
  if (String(topic) == "/ThinkIOT/Light") {
    if (message == "true") {
      digitalWrite(LIGHT_PIN, HIGH);
      Serial.println("Light ON");
    } else if (message == "false") {
      digitalWrite(LIGHT_PIN, LOW);
      Serial.println("Light OFF");
    }
  }

  // Control Pump via MQTT
  if (String(topic) == "/ThinkIOT/Pump") {
    if (message == "true") {
      digitalWrite(PUMP_PIN, HIGH);
      Serial.println("Pump ON");
    } else if (message == "false") {
      digitalWrite(PUMP_PIN, LOW);
      Serial.println("Pump OFF");
    }
  }

  // Control Fan via MQTT
  if (String(topic) == "/ThinkIOT/Fan") {
    if (message == "true") {
      digitalWrite(FAN_PIN, HIGH);
      Serial.println("Fan ON");
    } else if (message == "false") {
      digitalWrite(FAN_PIN, LOW);
      Serial.println("Fan OFF");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish("/ThinkIOT/Publish", "Welcome");
      
      // Subscribe to topics
      client.subscribe("/ThinkIOT/Servo");
      client.subscribe("/ThinkIOT/Light");
      client.subscribe("/ThinkIOT/Pump");
      client.subscribe("/ThinkIOT/Fan");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Set pin modes
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  // Initialize servo on pin 13
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);  // Standard 50Hz for servo motor
  myservo.attach(SERVO_PIN, 500, 2400);  // Attach servo to pin 13

  // Begin serial communication and set up WiFi and MQTT
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.setup(DHT_PIN, DHTesp::DHT11);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    // Read and publish temperature and humidity
    TempAndHumidity data = dht.getTempAndHumidity();
    String temp = String(data.temperature, 2);
    client.publish("/ThinkIOT/temp", temp.c_str());
    String hum = String(data.humidity, 1);
    client.publish("/ThinkIOT/hum", hum.c_str());

    // Read and publish soil moisture
    soilMoisture = analogRead(SOIL_MOISTURE_PIN);
    String soilMoistureStr = String(soilMoisture);
    client.publish("/ThinkIOT/soil", soilMoistureStr.c_str());

    // Read and publish LDR value
    ldrValue = analogRead(LDR_PIN);
    String ldrValueStr = String(ldrValue);
    client.publish("/ThinkIOT/ldr", ldrValueStr.c_str());

    // Print the sensor data to the serial monitor
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(hum);
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoistureStr);
    Serial.print("LDR Value: ");
    Serial.println(ldrValueStr);
  }
}
