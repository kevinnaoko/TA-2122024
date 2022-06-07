//int GPIOPIN = 9; 
//
//int peweem = 0;
//int limitPwm = 255;
//int isRedFlag = 0;
//
//const byte numChars = 32;
//char receivedChars[numChars];
//boolean newData = false;
//volatile byte state = LOW;
//
//// Pins D9 and D10 - 31.4 kHz
//
//void setup() {
//    
//    TCCR1A = 0b00000001; // 8bit
//    TCCR1B = 0b00000001; // x1 phase correct
//
//  Serial.begin(112500); 
//  Serial.println("START");
//  pinMode(GPIOPIN, OUTPUT);
//}
//
//int target_current = 600;   //mA
//int vcc = 5111;   //mV
//int qov = vcc/2;    //mV
//int max_current = 20000;   //mA
//int readAdc;
//int driftVoltage = 17;
//
//void loop() {
//  //hardlimit PWM
//  if (peweem > limitPwm){
//    Serial.println("Limitpwm");
//    peweem = limitPwm;
//  }
//  
//  Serial.print("   PWM: ");
//  Serial.println(peweem);
// 
//  analogWrite(GPIOPIN, 50);
// 
//  
//}
 
/////////////////////
//char buf[80];
//int peweem = 0;
//int limitPwm = 200;
//
//int readline(int readch, char *buffer, int len) {
//  static int pos = 0;
//  int rpos;
//
//  if (readch > 0) {
//      switch (readch) {
//          case '\r': // Ignore CR
//              break;
//          case '\n': // Return on new-line
//              rpos = pos;
//              pos = 0;  // Reset position index ready for next time
//              return rpos;
//          default:
//              if (pos < len-1) {
//                  buffer[pos++] = readch;
//                  buffer[pos] = 0;
//              }
//      }
//  }
//  return 0;
//}
//
//void setup() {
//  Serial.begin(115200);
//  Serial.println("start");
//
//  TCCR1A = _BV(COM1A1) | _BV(COM1B1);
//  TCCR1B = _BV(WGM13) | _BV(CS10);
//  ICR1 = 400 ; // 10 bit resolution
//  OCR1A = 0; // vary this value between 0 and 400 for 10-bit precision
//  OCR1B = 328; // vary this value between 0 and 400 for 10-bit precision 
//
//  pinMode(9, OUTPUT);
//}
//
//void loop() {
//    if (readline(Serial.read(), buf, 80) > 0) {
//        Serial.print("You entered: >");
//        Serial.print(buf);
//        Serial.println("<");  
//        peweem = atoi(buf);
//    }
//    
//    if (peweem > limitPwm){
//      Serial.println("Limitpwm");
//      peweem = limitPwm; 
//    }
//
//    Serial.print("PWM:");
//    Serial.println(peweem);
//    // OCR1B/*pin 10*/ = peweem/* speed 0 to 400 */; 
//    OCR1A/*pin 9*/ = peweem/* speed 0 to 400 */;
//}
////////////

//void setup() 
//{ 
//  Serial.begin(9600);
//
//  TCCR1A = _BV(COM1A1) | _BV(COM1B1);
//  TCCR1B = _BV(WGM13) | _BV(CS10);
//  ICR1 = 400 ; // 10 bit resolution
//  OCR1A = 0; // vary this value between 0 and 400 for 10-bit precision
//  OCR1B = 328; // vary this value between 0 and 400 for 10-bit precision      
//
//  pinMode (10, OUTPUT);
//  pinMode (9, OUTPUT);
//}
//
//void loop() {
//
// //--- Note:  to set speed, just do (eg) this:  
// //         OCR1B/*pin 10*/ = 328/* speed 0 to 400 */;
// //--- or
//          OCR1A/*pin 9*/ = 200/* speed 0 to 400 */;
//
//}


///////////////
char buf[80];
int peweem = 0;
int limitPwm = 50;

const byte OC1A_PIN = 9;
const byte OC1B_PIN = 10;

const word PWM_FREQ_HZ = 23000; //Adjust this value to adjust the frequency (Frequency in HZ!) (Set currently to 25kHZ)
const word TCNT1_TOP = 16000000/(2*PWM_FREQ_HZ);

int readline(int readch, char *buffer, int len) {
  static int pos = 0;
  int rpos;

  if (readch > 0) {
      switch (readch) {
          case '\r': // Ignore CR
              break;
          case '\n': // Return on new-line
              rpos = pos;
              pos = 0;  // Reset position index ready for next time
              return rpos;
          default:
              if (pos < len-1) {
                  buffer[pos++] = readch;
                  buffer[pos] = 0;
              }
      }
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("start");

  pinMode(OC1A_PIN, OUTPUT);

  // Clear Timer1 control and count registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // Set Timer1 configuration
  // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
  // COM1B(1:0) = 0b00   (Output B normal operation)
  // WGM(13:10) = 0b1010 (Phase correct PWM)
  // ICNC1      = 0b0    (Input capture noise canceler disabled)
  // ICES1      = 0b0    (Input capture edge select disabled)
  // CS(12:10)  = 0b001  (Input clock select = clock/1)
  
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);
  ICR1 = TCNT1_TOP;
}

void loop() {
    if (readline(Serial.read(), buf, 80) > 0) {
        Serial.print("You entered: >");
        Serial.print(buf);
        Serial.println("<");  
        peweem = atoi(buf);
    }
    
    if (peweem > limitPwm){
      Serial.println("Limitpwm");
      peweem = limitPwm; 
    }

    Serial.print("DUTY:");
    Serial.print(peweem);
    Serial.println( "%");
    // OCR1B/*pin 10*/ = peweem/* speed 0 to 400 */; 
    // OCR1A/*pin 9*/ = peweem/* speed 0 to 400 */;

    setPwmDuty(peweem);
}

void setPwmDuty(byte duty) {
  OCR1A = (word) (duty*TCNT1_TOP)/100;
}
