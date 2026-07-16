/**
 * ESP32 <-> STM32 round-trip test, now with manual typing from the
 * Arduino IDE Serial Monitor forwarded to the STM32.
 *
 * Serial  (USB, 115200) -> Arduino IDE Serial Monitor. Type here to send
 *          a message to the STM32, and see whatever the STM32 sends back.
 * Serial2 (GPIO16=RX2, GPIO17=TX2, 9600) -> wired link to the STM32.
 */
#define STM_BAUD     9600
#define STM_RX_PIN   16   // ESP32 GPIO16 = RX2 <- STM32 PA2 (TX)
#define STM_TX_PIN   17   // ESP32 GPIO17 = TX2 -> STM32 PA3 (RX)

void setup() {
  Serial.begin(115200);
  Serial2.begin(STM_BAUD, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);
  Serial.println("ESP32 ready. Type a message below to send to STM32.");
}

void loop() {
  // Typed into the Arduino IDE Serial Monitor
  if (Serial.available()) {
    String typed = Serial.readStringUntil('\n');
    typed.trim();
    // Forwards to STM32's USART2
    if (typed.length() > 0) {
      Serial2.print(typed); 
      Serial2.print("\r\n");
      Serial.print("Sent to STM32: ");
      Serial.println(typed);
    }
  }

  // Whatever arrives from the STM32 over Serial2, shows it in serial monitor
  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.print("Received from STM32: ");
      Serial.println(line);
    }
  }
}