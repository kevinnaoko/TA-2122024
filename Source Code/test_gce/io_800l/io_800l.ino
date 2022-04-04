/* 
***************************************************
*/ 

#include <SoftwareSerial.h>
SoftwareSerial Sim800l(26, 27);
   

void setup() {
  // put your setup code here, to run once:
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
