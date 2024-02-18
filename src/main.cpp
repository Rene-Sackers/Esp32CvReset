#include <Arduino.h>
#include <EspMQTTClient.h>
#include <ESP32Servo.h>
#include <Preferences.h>
#include <settings.h>

#define SERVO_PIN 18

#define PREFERENCE_NAMESPACE "CV_RESET"
#define PREFERENCE_IS_SET "IS_SET"
#define PREFERENCE_DEFAULT_ANGLE "DEF"
#define PREFERENCE_ACTUATION_ANGLE "ACT"

EspMQTTClient client(
  WIFI_SSID,
  WIFI_PASSWORD,
  MQTT_ADDRESS,  // MQTT Broker server ip
  MQTT_USER,   // Can be omitted if not needed
  MQTT_PASS,   // Can be omitted if not needed
  "Esp32CvResetServo"      // Client name that uniquely identify your device
);

Preferences preferences;
int defaultAngle = 90;
int actuationAngle = 25;

Servo servoMotor;

void pressResetButton() {
  servoMotor.write(defaultAngle + actuationAngle);
  delay(1000);
  servoMotor.write(defaultAngle);

  client.publish("cvketel/was_reset", "true");
}

void readPreferences() {
  preferences.begin(PREFERENCE_NAMESPACE, false);
  bool isEepromSet = preferences.getBool(PREFERENCE_IS_SET);

  if (!isEepromSet)
  {
    preferences.end();
    return;
  }

  defaultAngle = preferences.getInt(PREFERENCE_DEFAULT_ANGLE);
  actuationAngle = preferences.getInt(PREFERENCE_ACTUATION_ANGLE);
  preferences.end();

  Serial.println("Preferences read");
  Serial.println("Default angle: " + defaultAngle);
  Serial.println("Actuation angle: " + actuationAngle);
}

void storePreferences() {
  preferences.begin(PREFERENCE_NAMESPACE, false);
  preferences.putInt(PREFERENCE_DEFAULT_ANGLE, defaultAngle);
  preferences.putInt(PREFERENCE_ACTUATION_ANGLE, actuationAngle);
  preferences.putBool(PREFERENCE_IS_SET, true);
  preferences.end();

  Serial.println("Preferences stored");
}

void setup() {
  Serial.begin(9600);
  Serial.println("ESP32 MQTT Servo starting...");

  readPreferences();

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(defaultAngle);
}

void onConnectionEstablished() {
  Serial.println("Connection established");

  client.subscribe("cvketel/reset", [] (const String &payload) {
    Serial.println("Reset triggered");
    pressResetButton();
  });

  client.subscribe("cvketel/default_angle", [] (const String &payload) {
    int degrees = payload.toInt();
    Serial.println("Default angle: " + degrees);
    defaultAngle = degrees;
    servoMotor.write(defaultAngle);
    storePreferences();
  });

  client.subscribe("cvketel/actuate_angle", [] (const String &payload) {
    int degrees = payload.toInt();
    Serial.println("Set actuation angle to: " + degrees);
    actuationAngle = degrees;
    pressResetButton();
    storePreferences();
  });
}

void loop() {
  client.loop();
}