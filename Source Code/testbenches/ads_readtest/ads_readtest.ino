#include <Adafruit_ADS1X15.h>


Adafruit_ADS1015 ads1015_01;    // Construct an ads1015 


void setup() {
  Serial.begin(112500); 

  /* ads init */
  ads1015_01.begin(0x48); //suhu
}


void loop() {  
  // read current
  //int readAdcCurrent = ads1015_02.readADC_SingleEnded(0);
  //float current = ((float(  (readAdcCurrent*3 + driftVoltage)  ) - qov) ) / 0.155 ;


  //readTemp
  int readAdcTemp = ads1015_01.readADC_SingleEnded(0);
  float temperature = ((float(  (readAdcTemp*3)  ) ) / 10 );

  
 
  Serial.print("ADC: ");
  Serial.print(readAdcTemp);

  Serial.print("Temp: ");
  Serial.println(temperature);
  
}
 
