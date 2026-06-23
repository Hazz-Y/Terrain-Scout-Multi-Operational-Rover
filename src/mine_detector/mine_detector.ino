/*
 * ============================================================================
 *  Terrain Scout III — Mine Detector Module
 * ============================================================================
 *
 *  Capacitive metal detection system using Arduino Nano.
 *  Detects metallic objects (mines, buried ordnance) by measuring
 *  capacitance changes caused by nearby conductive materials.
 *
 *  Working Principle:
 *    - Pulses charge a capacitor through a coil/antenna
 *    - Nearby metal objects alter the electromagnetic field
 *    - Change in capacitance is measured via analog read
 *    - LED indicator flashes proportionally to signal strength
 *
 *  Hardware:
 *    - Arduino Nano
 *    - Sensing coil (connected to A0/A1)
 *    - Indicator LED (pin 12)
 *
 *  Author: Avishkar Jaiswal
 *  Project: Terrain Scout III — Multi-Operational Defence Rover
 * ============================================================================
 */

// ─── Configuration ──────────────────────────────────────────────
const byte NUM_PULSES     = 12;     // Pulses to charge capacitor per measurement
const int  NUM_MEASURES   = 256;    // Measurements per detection cycle

// ─── Pin Assignments ────────────────────────────────────────────
const byte PIN_PULSE = A0;    // Pulse output to charge coil
const byte PIN_CAP   = A1;    // Capacitance measurement input
const byte PIN_LED   = 12;    // Metal detection indicator LED

// ─── Runtime Variables ──────────────────────────────────────────
long int sumsum        = 0;        // Running sum (64-sample moving average)
long int skip          = 0;        // Skipped sample counter
long int diff          = 0;        // Delta between current and average
long int flash_period  = 0;        // LED flash period (ms)
unsigned long prev_flash = 0;      // Last flash timestamp

void setup() {
  // Initialize pulse output
  pinMode(PIN_PULSE, OUTPUT);
  digitalWrite(PIN_PULSE, LOW);

  // Initialize capacitance sensing input
  pinMode(PIN_CAP, INPUT);

  // Initialize LED indicator
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  int minval = 2000;
  int maxval = 0;

  // ─── Measurement Cycle ──────────────────────────────────────
  unsigned long sum = 0;

  for (int imeas = 0; imeas < NUM_MEASURES + 2; imeas++) {
    // Reset capacitor charge
    pinMode(PIN_CAP, OUTPUT);
    digitalWrite(PIN_CAP, LOW);
    delayMicroseconds(20);
    pinMode(PIN_CAP, INPUT);

    // Apply charge pulses through coil
    for (int ipulse = 0; ipulse < NUM_PULSES; ipulse++) {
      digitalWrite(PIN_PULSE, HIGH);   // ~3.5µs rise
      delayMicroseconds(3);
      digitalWrite(PIN_PULSE, LOW);    // ~3.5µs fall
      delayMicroseconds(3);
    }

    // Read accumulated charge (capacitance measurement)
    int val = analogRead(PIN_CAP);     // ~104µs per read
    minval = min(val, minval);
    maxval = max(val, maxval);
    sum += val;

    // ─── LED Flash Control ──────────────────────────────────
    unsigned long timestamp = millis();
    byte ledstat = 0;

    if (timestamp < prev_flash + 12) {
      if (diff > 0) ledstat = 1;       // Metal detected (positive shift)
      if (diff < 0) ledstat = 2;       // Metal detected (negative shift)
    }
    if (timestamp > prev_flash + flash_period) {
      if (diff > 0) ledstat = 1;
      if (diff < 0) ledstat = 2;
      prev_flash = timestamp;
    }
    if (flash_period > 1000) ledstat = 0;  // No significant detection

    // Apply LED state
    switch (ledstat) {
      case 0:  digitalWrite(PIN_LED, LOW);  break;
      case 1:  digitalWrite(PIN_LED, LOW);  break;   // Subtle indicator
      case 2:  digitalWrite(PIN_LED, HIGH); break;    // Active alert
    }
  }

  // ─── Signal Processing ────────────────────────────────────
  // Remove min/max outliers to reduce spike noise
  sum -= minval;
  sum -= maxval;

  // Exponential moving average filter
  if (sumsum == 0) sumsum = sum << 6;     // Initialize to expected value
  long int avgsum = (sumsum + 32) >> 6;
  diff = sum - avgsum;

  if (abs(diff) < (avgsum >> 10)) {       // Small delta → adjust baseline
    sumsum = sumsum + sum - avgsum;
    skip = 0;
  } else {
    skip++;                               // Large delta → skip (potential metal)
  }

  if (skip > 64) {                        // Prolonged deviation → reset baseline
    sumsum = sum << 6;
    skip = 0;
  }

  // Calculate flash frequency (proportional to signal strength)
  // 1‰ change → 2 flashes/second
  if (diff == 0) flash_period = 1000000;
  else flash_period = avgsum / (2 * abs(diff));
}
