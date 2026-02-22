#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= WIFI =================
const char* ssid = "JALANTH iPhone";
const char* password = "jalanth9342";

const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;

// ================= MQTT TOPICS =================
const char* flow_topic  = "water/flow";
const char* valve_topic = "water/valve";

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= PINS =================
#define IN1 32
#define IN2 33
#define ENA 25
#define FLOW_PIN 26
#define RELAY_PIN 27

// ================= VALVE STATES =================
#define VALVE_OPEN_STATE HIGH
#define VALVE_CLOSED_STATE LOW

// ================= PWM =================
const int pwmChannel = 0;
const int pwmFreq = 20000;
const int pwmResolution = 8;

// ================= FLOW =================
volatile unsigned long pulseCount = 0;
unsigned long lastCalcTime = 0;
float flowRate = 0.0;
float calibrationFactor = 7.8;

// ================= MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

// ================= STATES =================
bool leakActive = false;
bool manualValveOverride = false;
unsigned long leakShutdownTime = 0;
const unsigned long restartDelay = 10000;

// ================= ISR =================
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ================= VALVE + PUMP =================
void setValve(bool open) {
  if (open) {
    digitalWrite(RELAY_PIN, VALVE_OPEN_STATE);
    ledcWrite(pwmChannel, 255);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(RELAY_PIN, VALVE_CLOSED_STATE);
    ledcWrite(pwmChannel, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
}

// ================= MQTT CALLBACK =================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == valve_topic) {
    manualValveOverride = true;
    if (msg == "1") setValve(true);
    else setValve(false);
  }
}

// ================= WIFI =================
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

// ================= MQTT =================
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_FlowSystem")) {
      client.subscribe(valve_topic);
    }
    delay(2000);
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, VALVE_CLOSED_STATE);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannel);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, RISING);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

// ================= LOOP =================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - lastCalcTime >= 1000) {
    lastCalcTime = millis();

    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    flowRate = pulses / calibrationFactor;
    static float smooth = 0;
    smooth = 0.2 * flowRate + 0.8 * smooth;
    flowRate = smooth < 0.15 ? 0 : smooth;

    if (!manualValveOverride && flowRate > 16 && !leakActive) {
      setValve(false);
      leakActive = true;
      leakShutdownTime = millis();
    }

    if (leakActive && millis() - leakShutdownTime > restartDelay) {
      setValve(true);
      leakActive = false;
    }

    char payload[10];
    dtostrf(flowRate, 4, 2, payload);
    client.publish(flow_topic, payload);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("ValveX");
    display.setCursor(0, 20);
    display.print(flowRate);
    display.print(" L/m");
    display.display();
  }
}
