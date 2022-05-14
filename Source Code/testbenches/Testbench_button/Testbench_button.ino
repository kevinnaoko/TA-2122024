#define buttonR 32
#define buttonL 33

void setup(){
    Serial.begin(9600);
    pinMode(buttonR, INPUT_PULLUP);
    pinMode(buttonL, INPUT_PULLUP);
}

void loop(){
    byte haha = digitalRead(buttonR);
    byte hehe = digitalRead(buttonL);

    Serial.print("R: ");
    Serial.print(haha);
    Serial.print("   L: ");
    Serial.println(hehe);
}
