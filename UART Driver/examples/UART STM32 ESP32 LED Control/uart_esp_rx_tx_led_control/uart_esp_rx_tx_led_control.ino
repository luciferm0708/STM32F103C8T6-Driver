#define STM_BAUD 9600
#define STM_RX_PIN 16
#define STM_TX_PIN 17
#define LED_PIN 2

void setup(){
  Serial.begin(115200);
  Serial2.begin(STM_BAUD, SERIAL_8N1, STM_RX_PIN, STM_TX_PIN);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("ESP32 ready. Type led_on / led_off to control STM32's LED.");
}

void loop(){
  if(Serial.available()){
    String typed = Serial.readStringUntil('\n');
    typed.trim();

    if(typed == "led_on"){
      Serial2.print("led_on\r\n");
      Serial.println("Sent to STM32: led_on");
    }else if(typed == "led_off"){
      Serial2.print("led_off\r\n");
      Serial.println("Sent to STM32: led_off");
    }else{
      Serial.println("Unknown command. Try: led_on, led_off");
    }
  }

  if(Serial2.available()){
    String line = Serial2.readStringUntil('\n');
    line.trim();

    if(line.length() > 0){
      Serial.print("Recieved from STM32: ");
      Serial.println(line);

      if(line == "led_on"){
        digitalWrite(LED_PIN, HIGH);
        Serial.println("ESP32 LED ON");
      }else if(line == "led_off"){
        digitalWrite(LED_PIN, LOW);
        Serial.println("ESP32 LED OFF");
      }
    }
  }
}