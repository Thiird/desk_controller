void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  pinMode(2, INPUT_PULLUP);

}
void moveUp()
{
    byte escape = 0x1b;
    byte squareBr = 0x5b;
    byte capitalA = 0x41;
    Serial1.write(escape);
    Serial1.write(squareBr);
    Serial1.write(capitalA);
}

void moveDown()
{
    byte escape = 0x1b;
    byte squareBr = 0x5b;
    byte capitalB = 0x42;
    Serial1.write(escape);
    Serial1.write(squareBr);
    Serial1.write(capitalB);
}

void moveLeft()
{
    byte escape = 0x1b;
    byte squareBr = 0x5b;
    byte capitalD = 0x44;
    Serial1.write(escape);
    Serial1.write(squareBr);
    Serial1.write(capitalD);
}

void moveRight()
{
    byte escape = 0x1b;
    byte squareBr = 0x5b;
    byte capitalC = 0x43;
    Serial1.write(escape);
    Serial1.write(squareBr);
    Serial1.write(capitalC);
}

bool sent = false;
bool startOver = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastButtonState = HIGH;
int buttonState;

void loop() {  

  while(Serial1.available())
  {
    Serial.write(Serial1.read());
  }

  int reading = digitalRead(2);
  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
     if (reading != buttonState)
     {
        buttonState = reading;
        if (buttonState == LOW)
        {
            startOver = false;
            if(!sent)
            {
              sent = true;
              Serial1.write("Bias_ON\n");
              Serial.write("aaaa\n");
            }
        }
        else
        {
            sent = false;
            if(!startOver)
            {
              startOver = true;
              Serial1.write("Bias_OFF\n");
              Serial.write("bbb\n");
            }
        }
     }
  }

/*
//##################
  int cont = 0;
  while(true)
  {
    while(Serial.available())
    {
      byte temp = Serial.read();
      Serial.println("------------------");
      Serial1.print((char)temp);
      Serial.print(" : ");
      for (int i = 7; i >= 0; i--)
      {
          bool b = bitRead(temp, i);
          Serial.print(b);
      }
      Serial.println();
    }

    if(!digitalRead(2))
    {
      moveUp();
      delay(1000);
      moveDown();
      delay(1000);
      moveLeft();
      delay(1000);
      moveRight();
    }       
  }
//##################
*/

  
  /*if(!digitalRead(2))
  {
    Serial1.write("a");
    delay(1000);
  }

  
  byte temp = 0x0;
  while(Serial1.available())
  {
    Serial.println("------------------");
    temp = Serial1.read();
    Serial.print((char)temp);
    Serial.print(" : ");
    for (int i = 7; i >= 0; i--)
    {
        bool b = bitRead(temp, i);
        Serial.print(b);
    }
    Serial.println();
  }*/
}
