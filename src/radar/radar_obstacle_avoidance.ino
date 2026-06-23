/*
 * ============================================================================
 *  Terrain Scout III — Radar & Obstacle Avoidance Module
 * ============================================================================
 *
 *  360° scanning radar system using servo-mounted ultrasonic sensor.
 *  Performs continuous 0°–180° sweep, measuring distances at each angle.
 *  When an obstacle is detected within the threshold range, triggers
 *  an evasive response via the secondary servo actuator.
 *
 *  Features:
 *    - Continuous 180° radar sweep with distance mapping
 *    - Real-time obstacle detection (configurable threshold)
 *    - Automated evasive maneuver triggering
 *    - Serial output for radar visualization (Processing/Python)
 *
 *  Hardware:
 *    - Arduino UNO + L293D Motor Shield
 *    - Servo Motor MG995 (radar sweep — pin 9)
 *    - Servo Motor MG995 (evasion actuator — pin 10)
 *    - HC-SR04 Ultrasonic Sensor (Trig: A0, Echo: A1)
 *
 *  Author: Avishkar Jaiswal
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

// ─── Radar Parameters ───────────────────────────────────────────
#define SWEEP_DELAY       15      // Delay between angle steps (ms)
#define OBSTACLE_THRESHOLD 30     // Detection range (cm)
#define EVASION_ANGLE      90     // Evasion servo response angle
#define EVASION_HOLD       1000   // Evasion hold duration (ms)

// ─── Hardware Objects ───────────────────────────────────────────
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Servo radarServo;       // Servo 1: Radar sweep (0°–180°)
Servo evasionServo;     // Servo 2: Evasion response actuator

void setup() {
  Serial.begin(9600);
  Serial.println("[TS-III] Radar & Obstacle Avoidance System Online");

  // Initialize motor shield
  AFMS.begin();

  // Attach servos
  radarServo.attach(9);
  evasionServo.attach(10);

  // Configure ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("[TS-III] Initiating 180° radar sweep...");
}

void loop() {
  // ─── Forward Sweep (0° → 180°) ─────────────────────────────
  for (int angle = 0; angle <= 180; angle++) {
    radarServo.write(angle);
    delay(SWEEP_DELAY);

    int distance = measureDistance();
    reportRadarData(angle, distance);

    if (distance <= OBSTACLE_THRESHOLD && distance > 0) {
      triggerEvasion();
    }
  }

  // ─── Return Sweep (180° → 0°) ──────────────────────────────
  for (int angle = 180; angle > 0; angle--) {
    radarServo.write(angle);
    delay(SWEEP_DELAY);

    int distance = measureDistance();
    reportRadarData(angle, distance);

    if (distance <= OBSTACLE_THRESHOLD && distance > 0) {
      triggerEvasion();
    }
  }
}

/**
 * Measure distance using HC-SR04 ultrasonic sensor.
 *
 * Sends a 10µs trigger pulse and measures echo return time.
 * Distance = (duration × speed_of_sound) / 2
 *
 * @return Distance in centimeters (0 if no echo)
 */
int measureDistance() {
  // Send trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure echo duration
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Convert to centimeters (speed of sound ≈ 0.034 cm/µs)
  int distance = duration * 0.034 / 2;
  return distance;
}

/**
 * Output radar data to serial for visualization.
 * Format compatible with Processing radar display sketches.
 */
void reportRadarData(int angle, int distance) {
  Serial.print("Angle: ");
  Serial.print(angle);
  Serial.print(", Distance: ");
  Serial.println(distance);
}

/**
 * Trigger evasive maneuver when obstacle is detected.
 * Activates the evasion servo and holds briefly before resetting.
 */
void triggerEvasion() {
  Serial.println("[ALERT] Obstacle detected! Triggering evasion...");
  evasionServo.write(EVASION_ANGLE);
  delay(EVASION_HOLD);
  evasionServo.write(0);   // Reset evasion servo
  delay(EVASION_HOLD);     // Cooldown to prevent rapid re-trigger
}