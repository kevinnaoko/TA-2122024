/* 
***************************************************
*/ 

#include <SoftwareSerial.h>
SoftwareSerial Sim800l(26, 27);
//int MODEM_RX = 27;
//int MODEM_TX = 26;

#define Sim800l Serial1 

void setup() {
  // put your setup code here, to run once:
  
//  Sim800l.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX); 

  Serial.begin(9600);
  Sim800l.begin(9600);
 

  
}

void loop() {  
//  Serial.println("HAI");
  
  while(Sim800l.available()){
    Serial.write(Sim800l.read());
  }
 
  while(Serial.available()){
    Sim800l.write(Serial.read());
  }

} 
