/* Capacitive water-level sensor — STM32 Nucleo-64
 *   RC-time measurement using one sense pin (A0) and one discharge pin (D4).
 *   Prints: raw t_us, C_pF, height_mm   every 150 ms.
 *
 * Wiring:
 *   A0 (PA0)  ← sensor plate  + 100 kΩ → 3V3
 *   D4 (PB5)  ← jumper to A0 row; software-controlled discharge
 *   Sensor return plate → GND
 */

 #define LED_PIN       LED_BUILTIN   // LD2
 #define SENSE_PIN     A0            // PA0
 #define DISCH_PIN     D4            // PB5 open-drain
 #define R_OHMS        100000.0f     // pull-up resistor (Ω)
 
 // Calibration constants (update after lab fit!)
 #define C_OFFSET_F    51e-12f       // pF → F  (C when tube empty)
 #define SENSITIVITY   15.7e-12f     // pF per mm → F/mm
 
 void setup() {
   pinMode(LED_PIN, OUTPUT);
   pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
   digitalWrite(DISCH_PIN, LOW);     // start discharged
 
   Serial.begin(115200);
   while (!Serial && millis() < 4000) {}   // wait up to 4 s for Serial
   Serial.println(F("Capacitive sensor sketch ✓"));
 }
 
 void loop() {
   float C_F  = measureCapacitance();
   float C_pF = C_F * 1e12f;
   float h_mm = (C_F - C_OFFSET_F) / SENSITIVITY;   // linear model
 
   Serial.print("t_us=");
   Serial.print(lastTime_us);
   Serial.print("  C=");
   Serial.print(C_pF, 2);
   Serial.print(" pF  h≈");
   Serial.print(h_mm, 1);
   Serial.println(" mm");
 
   digitalToggle(LED_PIN);
   delay(150);      // 6–7 Hz update
 }
 
 /* ---------- internals ---------------------------------------------------------------- */
 
 volatile uint32_t lastTime_us = 0;
 
 float measureCapacitance() {
   /* 1. DISCHARGE — hold node low ≥5τ (worst-case 400 µs; 2 ms to be safe) */
   pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
   digitalWrite(DISCH_PIN, LOW);
   delayMicroseconds(2000);
 
   /* 2. RELEASE to Hi-Z so 100 kΩ can charge sensor */
   pinMode(DISCH_PIN, INPUT);        // High-Z
 
   /* 3. Time until digital HIGH (≈0.6·VCC) */
   uint32_t t0 = micros();
   while (!digitalRead(SENSE_PIN)) { }      // wait for threshold
   lastTime_us = micros() - t0;
 
   /* 4. Re-attach discharge pin for next cycle */
   pinMode(DISCH_PIN, OUTPUT_OPEN_DRAIN);
   digitalWrite(DISCH_PIN, LOW);
 
   /* 5. Convert time to capacitance: t = 0.693·R·C  ⇒  C = t / (0.693·R) */
   float C = (float)lastTime_us * 1e-6f / (0.693f * R_OHMS);
   return C;      // farads
 }
 