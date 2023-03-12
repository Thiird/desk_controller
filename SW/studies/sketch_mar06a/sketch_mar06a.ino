const int BUTTON_PIN = 2;        // the number of the pushbutton pin
const int DEBOUNCE_DELAY = 50;   // the debounce time; increase if the output flickers

int lastSteadyState = LOW;       // the previous steady state from the input pin
int lastFlickerableState = LOW;  // the previous flickerable state from the input pin
int currentState;                // the current reading from the input pin

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

void setup() {
  Serial.begin(115200, SERIAL_8N1);
  Serial1.begin(115200, SERIAL_8N1);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  currentState = digitalRead(BUTTON_PIN);
  if (currentState != lastFlickerableState) {
    lastDebounceTime = millis();
    lastFlickerableState = currentState;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (lastSteadyState == HIGH && currentState == LOW)
    {      
      Serial1.write("Bias_ON\n");
    }
    else if (lastSteadyState == LOW && currentState == HIGH)
    {
      Serial1.write("Bias_OFF\n");
    }

    // save the the last steady state
    lastSteadyState = currentState;
  }

  while(Serial1.available())
  {
    Serial.write(Serial1.read());
  }
}
