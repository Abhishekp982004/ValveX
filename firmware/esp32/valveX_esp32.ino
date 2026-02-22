t#include <WiFi.h>
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
const char* valve_topic = "water/valve";   // 🔴 ADDED

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

// ================= VALVE LOGIC STATES =================
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

// ================= DEMO LEAK =================
// #define DEMO_LEAK_MODE true
// #define DEMO_LEAK_VALUE 8.5
// #define DEMO_LEAK_INTERVAL 15000
// unsigned long lastDemoLeak = 0;

// ================= MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

// ================= LEAK STATE =================
bool leakActive = false;
unsigned long leakShutdownTime = 0;
const unsigned long restartDelay = 10000;

// ================= MANUAL VALVE CONTROL =================
bool manualValveOverride = false;   // 🔴 ADDED

// ================= FUNCTION DECL =================
void calculateFlow();
void setValve(bool open);

// ================= ISR =================
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// ================= VALVE + PUMP CONTROL FUNCTION =================
void setValve(bool open) {
  if (open) {
    digitalWrite(RELAY_PIN, VALVE_OPEN_STATE);

    // Start pump
    ledcWrite(pwmChannel, 255);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

  } else {
    digitalWrite(RELAY_PIN, VALVE_CLOSED_STATE);

    // Stop pump
    ledcWrite(pwmChannel, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
}

// ================= MQTT CALLBACK =================
void mqttCallback(char* topic, byte* payload, unsigned int length) {   // 🔴 ADDED
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == valve_topic) {
    manualValveOverride = true;   // UI has control

    if (message == "1") {
      // OPEN valve
      setValve(true);
      leakActive = false;

      Serial.println("🟢 VALVE OPENED FROM UI — PUMP STARTED");

    } else {
      // CLOSE valve
      setValve(false);
      leakActive = false;

      Serial.println("🔴 VALVE CLOSED FROM UI — PUMP STOPPED");
    }
  }
}

// ================= WIFI =================
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// ================= MQTT =================
void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_FlowSystem")) {
      client.subscribe(valve_topic);   // 🔴 ADDED
    }
    delay(2000);
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, VALVE_CLOSED_STATE);   // CLOSED

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);   // 🔴 ADDED

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannel);

  // Pump OFF initially since valve is CLOSED
  ledcWrite(pwmChannel, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, RISING);

  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
}

// ================= LOOP =================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  calculateFlow();
}

// ================= FLOW + LEAK LOGIC =================
void calculateFlow() {

  if (millis() - lastCalcTime >= 1000) {

    lastCalcTime = millis();

    // ---- DEMO LEAK DISABLED ----
    {
      noInterrupts();
      unsigned long pulses = pulseCount;
      pulseCount = 0;
      interrupts();

      flowRate = pulses / calibrationFactor;
      static float smooth = 0;
      smooth = 0.2 * flowRate + 0.8 * smooth;
      flowRate = smooth;
      if (flowRate < 0.15) flowRate = 0.0;
    }

    // ---- AUTO LEAK SHUT (ONLY IF NO MANUAL OVERRIDE) ----
    if (!manualValveOverride && flowRate > 16.0 && !leakActive) {

      setValve(false);

      leakActive = true;
      leakShutdownTime = millis();
    }

    if (!manualValveOverride && leakActive &&
        millis() - leakShutdownTime >= restartDelay) {

      setValve(true);  // OPEN valve

      leakActive = false;
    }

    // ---- MQTT FLOW ----
    char payload[20];
    dtostrf(flowRate, 4, 2, payload);
    client.publish(flow_topic, payload);

    // ---- OLED ----
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("EmbedX");
    display.setCursor(78, 0);
    display.print("ValveX");

    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(flowRate, 2);
    display.print(" L/m");

    display.setTextSize(1);
    display.setCursor(0, 48);
    display.print("Valve: ");
    display.print(digitalRead(RELAY_PIN) == VALVE_OPEN_STATE ? "OPEN" : "CLOSED");

    display.display();
  }
}
