#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"
#include "ArduinoGraphics.h"
#include <Servo.h>
#include <secrets.h>

WiFiServer server(80);
ArduinoLEDMatrix matrix;
Servo myServo;

// Configuration for voltage reading
const int ADC_MAX = 1023;
const float VREF = 4.66;

// Servo configuration
const int SERVO_PIN = 9;
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;

// LED configuration
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Configure ADC to 10 bits
  analogReadResolution(10);

  // Initialize the red LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize the LED matrix
  matrix.begin();

  // Initialize the servo
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_MIN_ANGLE);

  Serial.println("=== WiFiS3 status ===");
  Serial.print("Firmware: ");
  Serial.println(WiFi.firmwareVersion());

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERROR: Wi-Fi module not detected.");
    while (true) {}
  }

  Serial.print("Connecting to: ");
  Serial.println(ssid);

  int status = WiFi.begin(ssid, pass);

  unsigned long t0 = millis();
  while (status != WL_CONNECTED && millis() - t0 < 20000) {
    delay(1000);
    status = WiFi.status();
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("OK: connected!");
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    // Wait for DHCP IP assignment
    Serial.print("Waiting for DHCP IP");
    int ipRetries = 0;
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && ipRetries < 10) {
      delay(1000);
      Serial.print(".");
      ipRetries++;
    }
    Serial.println();

    if (WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      Serial.print("IP obtained: ");
      Serial.println(WiFi.localIP());

      // Start the web server
      server.begin();
      Serial.println("HTTP server started on port 80");
      Serial.print("Open http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("ERROR: No IP after 10s");
      Serial.println("Check the router's DHCP server");
    }
  } else {
    Serial.print("FAILED, status = ");
    Serial.println(WiFi.status());
  }
}

float readVoltage() {
  int analogValue = analogRead(A0);
  float voltage = (analogValue / (float)ADC_MAX) * VREF;
  return voltage;
}

int voltageToServoAngle(float voltage) {
  // Map voltage (0-VREF) to servo angle (min-max)
  int angle = map((int)(voltage * 100), 0, (int)(VREF * 100), SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  angle = constrain(angle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
  return angle;
}

void updateLEDMatrix(float voltage) {
  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(String(voltage, 2) + "V");
  matrix.endText();
  matrix.endDraw();
}

void loop() {
  // Red LED blink every 167ms (six times faster than original)
  static unsigned long lastLedToggle = 0;
  static bool ledState = false;

  if (millis() - lastLedToggle > 167) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedToggle = millis();
  }

  // Read and display voltage on LED matrix and control the servo
  static unsigned long lastUpdate = 0;
  static unsigned long lastReset = 0;

  // Continuous voltage reading
  if (millis() - lastUpdate > 500) {
    float voltage = readVoltage();
    int servoAngle = voltageToServoAngle(voltage);

    updateLEDMatrix(voltage);
    myServo.write(servoAngle);

    Serial.print("Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | Servo: ");
    Serial.print(servoAngle);
    Serial.println(" deg");

    lastUpdate = millis();
  }

  // Reset to 0 every 3 seconds
  if (millis() - lastReset > 3000) {
    float voltage = readVoltage();
    int targetAngle = voltageToServoAngle(voltage);

    // Go to 0
    myServo.write(0);
    Serial.println("Servo: RESET to 0 deg");
    delay(500);

    // Return to target angle
    myServo.write(targetAngle);
    Serial.print("Servo: return to target angle ");
    Serial.print(targetAngle);
    Serial.println(" deg");

    lastReset = millis();
  }

  WiFiClient client = server.available();

  if (client) {
    Serial.println("New client connected!");

    // Wait for data to be available
    unsigned long timeout = millis();
    while (client.connected() && !client.available() && millis() - timeout < 3000) {
      delay(1);
    }

    // Read the first line of the request
    String currentLine = "";
    bool requestComplete = false;

    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Double newline = end of headers
            requestComplete = true;
            break;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Read voltage and calculate servo angle
    float voltage = readVoltage();
    int servoAngle = voltageToServoAngle(voltage);

    // Send the HTTP response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();

    // HTML page
    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<meta http-equiv='refresh' content='1'>");
    client.println("<title>Voltage A0</title>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }");
    client.println(".container { max-width: 600px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
    client.println("h1 { color: #00979D; text-align: center; }");
    client.println(".voltage { font-size: 72px; font-weight: bold; text-align: center; color: #00979D; margin: 30px 0; }");
    client.println(".servo { font-size: 48px; font-weight: bold; text-align: center; color: #FF6B35; margin: 20px 0; }");
    client.println(".info { text-align: center; color: #666; margin-top: 20px; }");
    client.println(".label { text-align: center; color: #999; font-size: 14px; margin-bottom: 10px; }");
    client.println("</style>");
    client.println("</head><body>");
    client.println("<div class='container'>");
    client.println("<h1>Arduino Readings</h1>");
    client.println("<div class='label'>VOLTAGE (A0)</div>");
    client.print("<div class='voltage'>");
    client.print(voltage, 2);
    client.println(" V</div>");
    client.println("<div class='label'>SERVO ANGLE</div>");
    client.print("<div class='servo'>");
    client.print(servoAngle);
    client.println(" &deg;</div>");
    client.println("<div class='info'>Auto refresh: 1 second</div>");
    client.println("</div>");
    client.println("</body></html>");

    // Small delay to let the client receive all data
    delay(1);
    client.stop();

    Serial.print("Response sent - Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | Servo: ");
    Serial.print(servoAngle);
    Serial.println(" deg");
  }
}
