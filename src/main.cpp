#include <WiFiS3.h>
#include "Arduino_LED_Matrix.h"
#include "ArduinoGraphics.h"
#include <Servo.h>
#include <secrets.h>

WiFiServer server(80);
ArduinoLEDMatrix matrix;
Servo myServo;

// Configuration pour la lecture de tension
const int ADC_MAX = 1023;
const float VREF = 4.66;

// Configuration du servo
const int SERVO_PIN = 9;
const int SERVO_MIN_ANGLE = 0;
const int SERVO_MAX_ANGLE = 180;

// Configuration de la LED
const int LED_PIN = 2;

void setup() {
  Serial.begin(115200);
  delay(2000);

  // Configuration ADC en 10 bits
  analogReadResolution(10);

  // Initialiser la LED rouge
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialiser la matrice LED
  matrix.begin();

  // Initialiser le servo
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_MIN_ANGLE);

  Serial.println("=== WiFiS3 status ===");
  Serial.print("Firmware: ");
  Serial.println(WiFi.firmwareVersion());

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERREUR: module Wi-Fi non detecte.");
    while (true) {}
  }

  Serial.print("Connexion a: ");
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
    Serial.println("OK: connecte !");
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());

    // Attendre l'attribution d'une IP par DHCP
    Serial.print("Attente IP DHCP");
    int ipRetries = 0;
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && ipRetries < 10) {
      delay(1000);
      Serial.print(".");
      ipRetries++;
    }
    Serial.println();

    if (WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      Serial.print("IP obtenue: ");
      Serial.println(WiFi.localIP());

      // Démarrer le serveur web
      server.begin();
      Serial.println("Serveur HTTP demarre sur port 80");
      Serial.print("Ouvrez http://");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("ERREUR: Pas d'IP apres 10s");
      Serial.println("Verifiez le serveur DHCP du routeur");
    }
  } else {
    Serial.print("ECHEC, status = ");
    Serial.println(WiFi.status());
  }
}

float readVoltage() {
  int analogValue = analogRead(A0);
  float voltage = (analogValue / (float)ADC_MAX) * VREF;
  return voltage;
}

int voltageToServoAngle(float voltage) {
  // Mapper la tension (0-VREF) vers l'angle du servo (min-max)
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
  // Clignotement de la LED rouge toutes les 167ms (six fois plus vite que l'origine)
  static unsigned long lastLedToggle = 0;
  static bool ledState = false;

  if (millis() - lastLedToggle > 167) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedToggle = millis();
  }

  // Lire et afficher la tension sur la matrice LED et contrôler le servo
  static unsigned long lastUpdate = 0;
  static unsigned long lastReset = 0;

  // Lecture continue de la tension
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

  // Retour à 0 toutes les 3 secondes
  if (millis() - lastReset > 3000) {
    float voltage = readVoltage();
    int targetAngle = voltageToServoAngle(voltage);

    // Aller à 0
    myServo.write(0);
    Serial.println("Servo: RESET a 0 deg");
    delay(500);

    // Retourner à l'angle de consigne
    myServo.write(targetAngle);
    Serial.print("Servo: retour a l'angle de consigne ");
    Serial.print(targetAngle);
    Serial.println(" deg");

    lastReset = millis();
  }

  WiFiClient client = server.available();

  if (client) {
    Serial.println("Nouveau client connecte!");

    // Attendre que des données soient disponibles
    unsigned long timeout = millis();
    while (client.connected() && !client.available() && millis() - timeout < 3000) {
      delay(1);
    }

    // Lire la première ligne de la requête
    String currentLine = "";
    bool requestComplete = false;

    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Double retour à la ligne = fin des headers
            requestComplete = true;
            break;
          }
          currentLine = "";
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Lire la tension et calculer l'angle du servo
    float voltage = readVoltage();
    int servoAngle = voltageToServoAngle(voltage);

    // Envoyer la réponse HTTP
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=utf-8");
    client.println("Connection: close");
    client.println();

    // Page HTML
    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<meta http-equiv='refresh' content='1'>");
    client.println("<title>Tension A0</title>");
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
    client.println("<h1>Mesures Arduino</h1>");
    client.println("<div class='label'>TENSION (A0)</div>");
    client.print("<div class='voltage'>");
    client.print(voltage, 2);
    client.println(" V</div>");
    client.println("<div class='label'>ANGLE SERVO</div>");
    client.print("<div class='servo'>");
    client.print(servoAngle);
    client.println(" &deg;</div>");
    client.println("<div class='info'>Actualisation automatique: 1 seconde</div>");
    client.println("</div>");
    client.println("</body></html>");

    // Petite pause pour que le client reçoive toutes les données
    delay(1);
    client.stop();

    Serial.print("Reponse envoyee - Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V | Servo: ");
    Serial.print(servoAngle);
    Serial.println(" deg");
  }
}
