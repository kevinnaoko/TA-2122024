
#include <Adafruit_ADS1X15.h>

//Adafruit_ADS1015 ads1015;    // Construct an ads1015 


int PWM_FREQUENCY = 50000; 
int PWM_CHANNEL = 0; 
int PWM_RESOUTION = 8; 
int GPIOPIN = 5 ; 
int NPNPIN = 19 ; 
int peweem = 240;

const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;

void setup() {
  Serial.begin(112500);
  //ads init
//  ads1015.begin(0x49); 
  
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(GPIOPIN, PWM_CHANNEL);
  pinMode(NPNPIN, OUTPUT);
}

int current;
int target_current = 600;   //mA
int vcc = 5111;   //mV
int qov = vcc/2;    //mV
int max_current = 20000;   //mA
int readAdc;

void loop() {
//  int readAdc = ads1015.readADC_SingleEnded(0);
//  float current = ((float(  (readAdc*3)  ) - qov) / (vcc/2) ) * max_current;

  
//  if (current > target_current){
//    peweem++;
//  }
//  else{
//    if (peweem > 210){
//      peweem--;
//    }
//    
//  }
  
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  while (Serial.available() > 0 && newData == false) {
      rc = Serial.read();

      if (rc != endMarker) {
          receivedChars[ndx] = rc;
          ndx++;
          if (ndx >= numChars) {
              ndx = numChars - 1;
          }
      }
      else {
          receivedChars[ndx] = '\0'; // terminate the string
          ndx = 0;
          newData = true;
      }
  }

  if (newData == true){
    newData = false;
    peweem = atoi(receivedChars);
  }

  
//  
//  Serial.print("Volt: ");
//  Serial.print(readAdc*3);
//  
//  Serial.print("   Current: ");
//  Serial.print(current);

  Serial.print("   PWM: ");
  Serial.println(peweem);
  
  ledcWrite(PWM_CHANNEL, peweem);
  digitalWrite(NPNPIN, LOW);
}
 
 
