/**
 * ============================================================
 *  SMART GLOVE UNIT — Firmware
 *  Smart Home Controlled by Smart Gloves
 * ============================================================
 *  B.Sc. Graduation Project — Mechatronics Engineering
 *  Benha National University, Faculty of Engineering
 *  Academic Year 2025/2026
 * ============================================================
 *
 *  Description:
 *    Reads 3 flex sensors on the Thumb, Index, and Pinky fingers.
 *    Classifies each sensor as BENT (0) or EXTENDED (1).
 *    Assembles a 3-bit code (0–7) and sends it to the Smart Home
 *    ESP32 via HTTP GET over a direct Wi-Fi Access Point link.
 *
 *  Voltage Divider (per sensor):
 *    3.3V ---[ FLEX ]---+---[10kΩ]--- GND
 *                       |
 *                   ESP32 ADC PIN
 *
 *    When finger is BENT   → Flex resistance ↑ → ADC voltage ↑ → classified as 0
 *    When finger is EXTENDED → Flex resistance ↓ → ADC voltage ↓ → classified as 1
 *
 *  Pin Assignment:
 *    GPIO 39  →  Flex Sensor — Thumb   [ADC1_CH3] (input only)
 *    GPIO 34  →  Flex Sensor — Index   [ADC1_CH6] (input only)
 *    GPIO 32  →  Flex Sensor — Pinky   [ADC1_CH4]
 *    GPIO 2   →  Onboard Status LED
 *
 *  Gesture Map (Thumb–Index–Pinky):
 *    000 → Fan ON
 *    001 → Fan OFF
 *    010 → Lamp ON
 *    011 → Lamp OFF
 *    100 → Curtain OPEN
 *    101 → Curtain CLOSE
 *    110 → Door OPEN
 *    111 → Door CLOSE
 *
 *  Dependencies:
 *    - ESP32 Arduino Core (Espressif)
 *    - WiFi.h     (included with ESP32 core)
 *    - HTTPClient.h (included with ESP32 core)
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>

// ============================================================
//  CONFIGURATION — Edit these values
// ============================================================

// Wi-Fi credentials (must match the Smart Home AP settings)
const char* WIFI_SSID     = "SmartHome_AP";
const char* WIFI_PASSWORD = "smarthome123";

// Smart Home ESP32 IP address (default ESP32 AP gateway)
const char* HOME_IP = "192.168.4.1";

// ============================================================
//  PIN DEFINITIONS
// ============================================================
#define THUMB_PIN  39   // Flex Sensor — Thumb  (ADC1_CH3, input only)
#define INDEX_PIN  34   // Flex Sensor — Index  (ADC1_CH6, input only)
#define PINKY_PIN  32   // Flex Sensor — Pinky  (ADC1_CH4)

#define STATUS_LED  2   // Onboard LED

// ============================================================
//  CALIBRATION
//  Open Serial Monitor at 115200 baud.
//  Fully EXTEND each finger → note the ADC value  (lower value)
//  Fully BENT  each finger → note the ADC value  (higher value)
//  Set THRESHOLD to the midpoint between the two readings.
//
//  Classification:
//    ADC < THRESHOLD  →  finger EXTENDED  →  bit = 1
//    ADC >= THRESHOLD →  finger BENT      →  bit = 0
// ============================================================
const int THRESHOLD[3] = {2200, 2200, 2200};
//                         Thumb  Index  Pinky

// ============================================================
//  TIMING
// ============================================================
const int SAMPLE_INTERVAL_MS = 20;    // Sensor read interval (ms)
const int DEBOUNCE_DELAY_MS  = 80;    // Stable hold before transmitting
const int WIFI_RETRY_DELAY   = 500;   // Delay between Wi-Fi retry attempts (ms)

// ============================================================
//  GLOBALS
// ============================================================
int lastCode    = -1;   // Last successfully transmitted code
int pendingCode = -1;   // Candidate code, awaiting debounce
unsigned long debounceStart = 0;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n====================================");
  Serial.println("  SMART GLOVE — Booting...");
  Serial.println("====================================");

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // 12-bit ADC (0–4095)
  analogReadResolution(12);

  connectToWiFi();
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  // Reconnect if Wi-Fi dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost — reconnecting...");
    connectToWiFi();
  }

  // ── Read raw ADC values ──────────────────────────────────
  int raw[3];
  raw[0] = analogRead(THUMB_PIN);
  raw[1] = analogRead(INDEX_PIN);
  raw[2] = analogRead(PINKY_PIN);

  // ── Classify: EXTENDED(1) or BENT(0) ────────────────────
  //    ADC < threshold  → low voltage → finger EXTENDED → bit = 1
  //    ADC >= threshold → high voltage → finger BENT    → bit = 0
  int bit[3];
  for (int i = 0; i < 3; i++) {
    bit[i] = (raw[i] < THRESHOLD[i]) ? 1 : 0;
  }

  // ── Assemble 3-bit code (Thumb=MSB, Pinky=LSB) ──────────
  int code = (bit[0] << 2) | (bit[1] << 1) | bit[2];   // 0–7

  // ── Serial debug ────────────────────────────────────────
  Serial.printf("[Sensors] Thumb=%4d(%d)  Index=%4d(%d)  Pinky=%4d(%d)  → Code=%d\n",
                raw[0], bit[0],
                raw[1], bit[1],
                raw[2], bit[2],
                code);

  // ── Debounce then transmit on change ────────────────────
  if (code != pendingCode) {
    pendingCode   = code;
    debounceStart = millis();
  } else if ((millis() - debounceStart >= DEBOUNCE_DELAY_MS) && (code != lastCode)) {
    sendCommand(code);
    lastCode = code;
  }

  delay(SAMPLE_INTERVAL_MS);
}

// ============================================================
//  SEND HTTP COMMAND
// ============================================================
void sendCommand(int code) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Not connected — skipping.");
    return;
  }

  String url = String("http://") + HOME_IP + "/cmd?val=" + String(code);
  Serial.printf("[HTTP] Sending command %d → %s\n", code, url.c_str());

  digitalWrite(STATUS_LED, HIGH);   // Blink to show transmission

  HTTPClient http;
  http.begin(url);
  http.setTimeout(2000);

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("[HTTP] Response %d: %s\n", httpCode, http.getString().c_str());
  } else {
    Serial.printf("[HTTP] Error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  digitalWrite(STATUS_LED, LOW);
}

// ============================================================
//  WI-FI CONNECTION
// ============================================================
void connectToWiFi() {
  Serial.printf("[WiFi] Connecting to \"%s\"", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    delay(WIFI_RETRY_DELAY);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(STATUS_LED, HIGH);
    Serial.println("\n[WiFi] Connected!");
    Serial.printf("[WiFi] IP:   %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    delay(300);
    digitalWrite(STATUS_LED, LOW);
  } else {
    digitalWrite(STATUS_LED, LOW);
    Serial.println("\n[WiFi] Failed — will retry in loop.");
  }
}
