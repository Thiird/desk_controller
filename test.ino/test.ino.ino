void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
}
void loop() {  
  
  while(Serial1.available())
  {
    Serial.write((char)Serial1.read());
  }
}
