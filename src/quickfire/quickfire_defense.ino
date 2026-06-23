/*
 * ============================================================================
 *  Terrain Scout III — QuickFire Defense Module
 * ============================================================================
 *
 *  Automated threat response system with ultrasonic proximity detection.
 *  When a target enters the defined engagement zone (<30 cm), the system
 *  triggers a rapid-fire mechanism via servo-actuated launcher.
 *
 *  Working Principle:
 *    1. Servo1 sweeps 0°–180° scanning for threats
 *    2. HC-SR04 measures distance at each angle
 *    3. If target is within engagement range → trigger Servo2
 *    4. Servo2 actuates the firing mechanism
 *    5. Auto-reset after cooldown period
 *
 *  Hardware:
 *    - Arduino UNO + L293D Motor Shield
 *    - Servo Motor MG995 (scan sweep — pin 9)
 *    - Servo Motor MG995 (fire trigger — pin 10)
 *    - HC-SR04 Ultrasonic Sensor (Trig: A0, Echo: A1)
 *
 *  Author: Harsh Yadav
 *  Project: Terrain Scout III — Multi-Operational Defence Rover
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <Servo.h>

// ─── Pin Configuration ──────────────────────────────────────────
#define TRIG_PIN    A0
#define ECHO_PIN    A1

// ─── QuickFire Parameters ───────────────────────────────────────
#define SCAN_DELAY           15     // Delay between scan steps (ms)
#define ENGAGEMENT_RANGE     30     // Fire threshold distance (cm)
#define FIRE_ANGLE           90     // Trigger servo angle (degrees)
#define FIRE_HOLD            1000   // Trigger hold time (ms)
#define COOLDOWN_PERIOD      1000   // Cooldown between shots (ms)

// ─── Hardware Objects ───────────────────────────────────────────
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Servo scanServo;      // Servo 1: Scan sweep (0°–180°)
Servo fireServo;      // Servo 2: Fire trigger actuator

void setup() {
  Serial.begin(9600);
  Serial.println("[TS-III] QuickFire Defense System Armed");

  // Initialize motor shield
  AFMS.begin();

  // Attach servos
  scanServo.attach(9);
  fireServo.attach(10);

  // Configure ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("[TS-III] Scanning for threats...");
}

void loop() {
  // ─── Forward Scan (0° → 180°) ──────────────────────────────
  for (int angle = 0; angle <= 180; angle++) {
    scanServo.write(angle);
    delay(SCAN_DELAY);

    int distance = measureDistance();
    reportThreatData(angle, distance);

    if (distance > 0 && distance <= ENGAGEMENT_RANGE) {
      engageTarget(angle, distance);
    }
  }

  // ─── Return Scan (180° → 0°) ───────────────────────────────
  for (int angle = 180; angle > 0; angle--) {
    scanServo.write(angle);
    delay(SCAN_DELAY);

    int distance = measureDistance();
    reportThreatData(angle, distance);

    if (distance > 0 && distance <= ENGAGEMENT_RANGE) {
      engageTarget(angle, distance);
    }
  }
}

/**
 * Measure distance using HC-SR04 ultrasonic sensor.
 *
 * @return Distance in centimeters
 */
int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}

/**
 * Report scan data to serial monitor.
 * Output format: "Angle: <deg>, Distance: <cm>"
 */
void reportThreatData(int angle, int distance) {
  Serial.print("Angle: ");
  Serial.print(angle);
  Serial.print(", Distance: ");
  Serial.println(distance);
}

/**
 * Engage detected target — activate firing mechanism.
 *
 * @param angle    Current scan angle where target was detected
 * @param distance Distance to target in cm
 */
void engageTarget(int angle, int distance) {
  Serial.print("[ENGAGE] Target at ");
  Serial.print(angle);
  Serial.print("°, ");
  Serial.print(distance);
  Serial.println(" cm — FIRING!");

  // Actuate fire mechanism
  fireServo.write(FIRE_ANGLE);
  delay(FIRE_HOLD);

  // Reset fire mechanism
  fireServo.write(0);
  delay(COOLDOWN_PERIOD);
}
