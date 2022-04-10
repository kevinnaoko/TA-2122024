/* 
***************************************************
*/ 
#define MAX_DELAY_MS_GEOLOC 10000
#define MAX_DELAY_MS_TIME 1000
#define LOOP_DELAY_MS 2 
#include <SoftwareSerial.h>
SoftwareSerial Sim800l(26, 27);
   

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Sim800l.begin(9600);
  
  
}

void loop() {    
    int serialusage = 1;
    if (serialusage == 1){
        while(Sim800l.available()){ 
            Serial.write(Sim800l.read());
        }
     
        while(Serial.available()){
            Sim800l.write(Serial.read());
        }
    }
    else{
        int res = checkGsmNetwork();
        Serial.print("Res: ");
        Serial.println(res);
        delay(5000);
    }
} 

int checkGsmNetwork(){
    String buffer2;
    int intercharTime = 0;
    Sim800l.print("AT+CREG?\r");

    while ((intercharTime) < 100)
    {
        if (Sim800l.available())
        {
            int c = Sim800l.read(); 
            // Serial.print((char)c);
            buffer2.concat((char)c); 
            intercharTime = 0; 
        }
        else
        { 
            intercharTime += LOOP_DELAY_MS;
        }

        delay(LOOP_DELAY_MS);
    }
    
//    while (Sim800l.available()){
//        Serial.write(Sim800l.read());
//    }

    Serial.print("buf:");
    Serial.println(buffer2);
    Serial.println("DONE");

    if (buffer2.indexOf("+CREG: 0,1")>0){
        return 1;        
    }
    else{
        return 0;
    }
}
