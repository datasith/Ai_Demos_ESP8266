uint8_t relay_pin = D1;

void setup()
{
  pinMode(relay_pin, OUTPUT); 
  digitalWrite(relay_pin, LOW);
}

void loop()
{
  digitalWrite(relay_pin, !digitalRead(relay_pin));
  delay(4000);
}
