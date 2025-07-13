#include <Arduino.h>

/* ----------  force a Serial port on USART2 (PA2 TX, PA3 RX) ---------- */
HardwareSerial Serial_STLINK(PA3, PA2);   // PA3 = RX, PA2 = TX

/* ----------  user-editable sensor constants -------------------------- */
#define LED_PIN       LED_BUILTIN
#define SENSE_PIN     A0            // PA0 / TIM2
#define DISCH_PIN     D4            // PB5 open-drain
#define R_OHMS        100000.0f     // pull-up resistor (Ω)

#define C_OFFSET_F    51e-12f       // dry-tube capacitance
#define SENSITIVITY   15.7e-12f     // F per mm

volatile uint32_t lastTime_us = 0;  // latest timing result

/* ---------- forward decl. ---------- */
float measureCapacitance();

/* --------------------------- setup ----------------------------------- */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DISCH_PIN, LOW);

  Serial_STLINK.begin(115200);      // start USART2 right away
  Serial_STLINK.println(F("Cap-sensor sketch (USART2)"));

  // quick LED flash to show reset
  digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
}

/* --------------------------- main loop ------------------------------- */
void loop() {
  float C_F  = measureCapacitance();
  float C_pF = C_F * 1e12f;
  float h_mm = (C_F - C_OFFSET_F) / SENSITIVITY;

  Serial_STLINK.print(F("t_us="));  Serial_STLINK.print(lastTime_us);
  Serial_STLINK.print(F("  C="));   Serial_STLINK.print(C_pF, 2);
  Serial_STLINK.print(F(" pF  h≈"));Serial_STLINK.print(h_mm, 1);
  Serial_STLINK.println(F(" mm"));

  digitalToggle(LED_PIN);           // heartbeat
  delay(500);
}

/* -------------------- RC-time measurement ---------------------------- */
float measureCapacitance() {
  pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DISCH_PIN, LOW);
  delayMicroseconds(2000);          // ≥5 τ

  pinMode(DISCH_PIN, INPUT);        // Hi-Z → start charge
  uint32_t t0 = micros();
  while (!digitalRead(SENSE_PIN)) {}
  lastTime_us = micros() - t0;

  pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
  digitalWrite(DISCH_PIN, LOW);

  return (float)lastTime_us * 1e-6f / (0.693f * R_OHMS);
}
