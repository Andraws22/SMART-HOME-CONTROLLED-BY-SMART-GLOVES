/**
 * ============================================================
 *  SMART HOME UNIT — Firmware
 *  Smart Home Controlled by Smart Gloves
 * ============================================================
 *  B.Sc. Graduation Project — Mechatronics Engineering
 *  Benha National University, Faculty of Engineering
 *  Academic Year 2025/2026
 * ============================================================
 *
 *  Description:
 *    Hosts a Wi-Fi Access Point and an HTTP web server.
 *    Receives gesture codes (0–7) from the Smart Glove and
 *    actuates home devices accordingly.
 *
 *  Pin Assignment:
 *    GPIO 33  →  Relay CH1 — Fan      (2-channel relay module)
 *    GPIO 25  →  Relay CH2 — Lamp     (2-channel relay module)
 *    GPIO 19  →  DC Motor IN1 (Curtain direction A)
 *    GPIO 18  →  DC Motor IN2 (Curtain direction B)
 *    GPIO 26  →  SG90 Servo PWM (Door)
 *    GPIO 2   →  Onboard Heartbeat LED
 *
 *  2-Channel Relay Wiring:
 *    IN1 → GPIO 33 → Fan   (220V AC via Relay CH1)
 *    IN2 → GPIO 25 → Lamp  (220V AC via Relay CH2)
 *    VCC → 5V,  GND → GND
 *    LOW  = device ON  (relay active-low)
 *    HIGH = device OFF (relay active-high / released)
 *
 *  Gesture-to-Command Map (Thumb–Index–Pinky):
 *    000 (0) → Fan  ON
 *    001 (1) → Fan  OFF
 *    010 (2) → Lamp ON
 *    011 (3) → Lamp OFF
 *    100 (4) → Curtain OPEN
 *    101 (5) → Curtain CLOSE
 *    110 (6) → Door OPEN   (Servo → OPEN angle)
 *    111 (7) → Door CLOSE  (Servo → CLOSE angle)
 *
 *  Dependencies:
 *    - ESP32 Arduino Core (Espressif)
 *    - WiFi.h, WebServer.h  (included with ESP32 core)
 *    - ESP32Servo            (install via Arduino Library Manager)
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ============================================================
//  CONFIGURATION
// ============================================================
const char* AP_SSID     = "SmartHome_AP";
const char* AP_PASSWORD = "smarthome123";

// ============================================================
//  PIN DEFINITIONS
// ============================================================
#define RELAY_FAN   33   // Relay CH1 → Fan   (active-LOW)
#define LAMP_PIN    25   // Relay CH2 → Lamp  (active-LOW)
#define MOTOR_IN1   19   // DC Motor direction A — Curtain OPEN
#define MOTOR_IN2   18   // DC Motor direction B — Curtain CLOSE
#define SERVO_PIN   26   // SG90 Servo signal   — Door
#define STATUS_LED   2   // Onboard heartbeat LED

// ============================================================
//  RELAY LOGIC
//  Most 2-channel relay modules are ACTIVE-LOW:
//    LOW  signal → relay energised → device ON
//    HIGH signal → relay released  → device OFF
//  If your module is active-HIGH, swap the defines below.
// ============================================================
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ============================================================
//  SERVO ANGLES — adjust to match your door mechanism
// ============================================================
const int SERVO_DOOR_OPEN  = 90;    // Degrees when door is open
const int SERVO_DOOR_CLOSE = 0;     // Degrees when door is closed

// ============================================================
//  MOTOR TIMING
// ============================================================
const int CURTAIN_TRAVEL_MS = 2500;   // Time for full curtain travel (ms)

// ============================================================
//  DEVICE STATE
// ============================================================
bool stateFan     = false;   // false = OFF
bool stateLamp    = false;   // false = OFF
bool stateCurtain = false;   // false = CLOSED, true = OPEN
bool stateDoor    = false;   // false = CLOSED, true = OPEN

// ============================================================
//  OBJECTS
// ============================================================
WebServer server(80);
Servo doorServo;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n====================================");
  Serial.println("  SMART HOME — Booting...");
  Serial.println("====================================");

  // Configure output pins
  pinMode(RELAY_FAN,  OUTPUT);
  pinMode(LAMP_PIN,   OUTPUT);
  pinMode(MOTOR_IN1,  OUTPUT);
  pinMode(MOTOR_IN2,  OUTPUT);
  pinMode(STATUS_LED, OUTPUT);

  // Safe defaults — everything OFF
  digitalWrite(RELAY_FAN,  RELAY_OFF);
  digitalWrite(LAMP_PIN,   RELAY_OFF);
  motorStop();
  digitalWrite(STATUS_LED, LOW);

  // Attach servo — door starts CLOSED
  doorServo.attach(SERVO_PIN);
  doorServo.write(SERVO_DOOR_CLOSE);
  stateDoor = false;
  Serial.println("[Servo]  Door → CLOSED");

  // Start Wi-Fi Access Point
  startAccessPoint();

  // Register HTTP routes
  server.on("/cmd",    handleCommand);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("[Server] HTTP server started — port 80");
  Serial.println("[Server] http://192.168.4.1/cmd?val=0..7");
  Serial.println("====================================\n");
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  server.handleClient();

  // Heartbeat LED — blinks every second to confirm system is alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    lastBlink = millis();
  }
}

// ============================================================
//  HTTP HANDLER — /cmd?val=N
// ============================================================
void handleCommand() {
  if (!server.hasArg("val")) {
    server.send(400, "text/plain", "Missing parameter: val");
    return;
  }

  int code = server.arg("val").toInt();
  Serial.printf("\n[CMD] Received code: %d\n", code);

  if (code < 0 || code > 7) {
    server.send(400, "text/plain", "Invalid code. Use 0–7.");
    return;
  }

  // ── Command Dispatch ─────────────────────────────────────
  switch (code) {

    case 0:   // 000 — Thumb bent, Index bent, Pinky bent
      Serial.println("[CMD] 000 → Fan ON");
      setFan(true);
      break;

    case 1:   // 001 — Thumb bent, Index bent, Pinky extended
      Serial.println("[CMD] 001 → Fan OFF");
      setFan(false);
      break;

    case 2:   // 010 — Thumb bent, Index extended, Pinky bent
      Serial.println("[CMD] 010 → Lamp ON");
      setLamp(true);
      break;

    case 3:   // 011 — Thumb bent, Index extended, Pinky extended
      Serial.println("[CMD] 011 → Lamp OFF");
      setLamp(false);
      break;

    case 4:   // 100 — Thumb extended, Index bent, Pinky bent
      Serial.println("[CMD] 100 → Curtain OPEN");
      moveCurtain(true);
      break;

    case 5:   // 101 — Thumb extended, Index bent, Pinky extended
      Serial.println("[CMD] 101 → Curtain CLOSE");
      moveCurtain(false);
      break;

    case 6:   // 110 — Thumb extended, Index extended, Pinky bent
      Serial.println("[CMD] 110 → Door OPEN");
      setDoor(true);
      break;

    case 7:   // 111 — Thumb extended, Index extended, Pinky extended
      Serial.println("[CMD] 111 → Door CLOSE");
      setDoor(false);
      break;
  }

  printStatus();
  server.send(200, "application/json", buildStatusJson());
}

// ============================================================
//  HTTP HANDLER — /status
// ============================================================
void handleStatus() {
  server.send(200, "application/json", buildStatusJson());
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found. Use /cmd?val=0..7 or /status");
}

// ============================================================
//  ACTUATOR FUNCTIONS
// ============================================================

void setFan(bool on) {
  stateFan = on;
  // 2-ch relay is active-LOW: LOW = ON, HIGH = OFF
  digitalWrite(RELAY_FAN, on ? RELAY_ON : RELAY_OFF);
  Serial.printf("[Relay1] Fan  → %s\n", on ? "ON" : "OFF");
}

void setLamp(bool on) {
  stateLamp = on;
  digitalWrite(LAMP_PIN, on ? RELAY_ON : RELAY_OFF);
  Serial.printf("[Relay2] Lamp → %s\n", on ? "ON" : "OFF");
}

void setDoor(bool open) {
  stateDoor = open;
  doorServo.write(open ? SERVO_DOOR_OPEN : SERVO_DOOR_CLOSE);
  Serial.printf("[Servo]  Door → %s (%d deg)\n",
                open ? "OPEN" : "CLOSED",
                open ? SERVO_DOOR_OPEN : SERVO_DOOR_CLOSE);
  delay(600);   // Allow servo to reach position
}

void moveCurtain(bool open) {
  stateCurtain = open;
  Serial.printf("[Motor]  Curtain → %s\n", open ? "OPENING..." : "CLOSING...");

  if (open) {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  } else {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
  }

  delay(CURTAIN_TRAVEL_MS);
  motorStop();

  Serial.printf("[Motor]  Curtain → %s (done)\n", open ? "OPEN" : "CLOSED");
}

void motorStop() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

// ============================================================
//  UTILITIES
// ============================================================

String buildStatusJson() {
  String j = "{";
  j += "\"fan\":"     + String(stateFan     ? "true"  : "false") + ",";
  j += "\"lamp\":"    + String(stateLamp    ? "true"  : "false") + ",";
  j += "\"curtain\":" + String(stateCurtain ? "\"open\""   : "\"closed\"") + ",";
  j += "\"door\":"    + String(stateDoor    ? "\"open\""   : "\"closed\"");
  j += "}";
  return j;
}

void printStatus() {
  Serial.println("[Status] ─────────────────────────");
  Serial.printf("  Fan:     %s\n", stateFan     ? "ON"     : "OFF");
  Serial.printf("  Lamp:    %s\n", stateLamp    ? "ON"     : "OFF");
  Serial.printf("  Curtain: %s\n", stateCurtain ? "OPEN"   : "CLOSED");
  Serial.printf("  Door:    %s\n", stateDoor    ? "OPEN"   : "CLOSED");
  Serial.println("[Status] ─────────────────────────");
}

// ============================================================
//  WI-FI ACCESS POINT
// ============================================================
void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);

  Serial.printf("[WiFi] AP started — SSID: %s\n", AP_SSID);
  Serial.printf("[WiFi] IP: %s\n", WiFi.softAPIP().toString().c_str());
}
