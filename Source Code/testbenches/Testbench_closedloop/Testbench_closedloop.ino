#include <Adafruit_ADS1X15.h>


Adafruit_ADS1015 ads1015_01;    // Construct an ads1015 
Adafruit_ADS1015 ads1015_02;

int PWM_FREQUENCY = 20000; 
int PWM_CHANNEL = 0; 
int PWM_RESOUTION = 12; 
int GPIOPIN = 5 ; 
int NPNPIN = 19 ; 
//int peweem = 240;
//int limitPwm = 235;
//int maxPwm = 255;

int peweem = 3854;
int limitPwm = 3770;
int maxPwm = 4095;
int isRedFlag = 0;

const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;
volatile byte state = LOW;

/* create a hardware timer */
hw_timer_t * timer = NULL;

/*Interrupt routine for Timer overflow event*/
void IRAM_ATTR onTimer() {
  state = !state;
  digitalWrite(LED_BUILTIN, state);
}

void setup() {
  Serial.begin(112500); 
  
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(GPIOPIN, PWM_CHANNEL);
  pinMode(NPNPIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  //timer
  /* Use 1st timer of 4 */
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  timer = timerBegin(0, 80, true);

  /* Attach onTimer function to our timer */
  timerAttachInterrupt(timer, &onTimer, true);

  /* Set alarm to call onTimer function every second 1 tick is 1us
  => 1 second is 1000000us */
  /* Repeat the alarm (third parameter) */
  timerAlarmWrite(timer, 500000, true);

  /* Start an alarm */
  timerAlarmEnable(timer);
  Serial.println("start timer");

  
  /* ads init */
  ads1015_01.begin(0x48); //suhu
  ads1015_02.begin(0x49); //arus
}

int current;
int temp;
int refCurrent;

int target_current = 600;   //mA
int vcc = 5111;   //mV
int qov = vcc/2;    //mV
int max_current = 20000;   //mA
int readAdc;
int driftVoltage = 17;

void loop() {  
  // read current
  int readAdcCurrent = ads1015_02.readADC_SingleEnded(0);
  float current = ((float(  (readAdcCurrent*3 + driftVoltage)  ) - qov) ) / 0.155 ;


  //readTemp
  int readAdcTemp = ads1015_01.readADC_SingleEnded(0);
  float temperature = ((float(  (readAdcTemp*3)  ) ) / 10 );

  // CL current control
  if (current > refCurrent){
    peweem++;
  }
  else{
    if (peweem > 210){
      peweem--;
    }
  }

  // read serial
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
  // insert serial to refCurrent
  if (newData == true){
    newData = false;
    refCurrent = atoi(receivedChars);
  }

  //hardlimit PWM
  if (peweem < limitPwm){
    peweem = limitPwm;
  }
  
  else if (peweem > maxPwm){
    peweem = maxPwm;
  }
  
 
  //temp Limit
  if (isRedFlag == 0 && temperature > 50.00){
    isRedFlag = 1;
  }

  if (isRedFlag == 1 && temperature < 45.00){
    isRedFlag = 0;
  }

  if (isRedFlag == 1){
    peweem = maxPwm;
  }

//  Serial.print("Volt: ");
//  Serial.print(readAdc*3);
//  Serial.print("   Current: ");
//  Serial.print(current);
 
  Serial.print("Temp NMOS: ");
  Serial.print(temperature);
 
  Serial.print("   RefCurrent: ");
  Serial.print(refCurrent);

  
  Serial.print("   PWM: ");
  Serial.print(peweem);
  
  Serial.print("   Current: ");
  Serial.println(current);
 
  
  ledcWrite(PWM_CHANNEL, peweem);
  digitalWrite(NPNPIN, LOW);
 
  
}
 
