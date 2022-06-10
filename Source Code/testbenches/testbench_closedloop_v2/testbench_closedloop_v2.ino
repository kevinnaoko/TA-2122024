#include <Adafruit_ADS1X15.h>

#define CLOSED_LOOP
//#define OPEN_LOOP

Adafruit_ADS1015 ads1015;

int PWM_FREQUENCY = 23000;
int PWM_CHANNEL = 0;
int PWM_RESOUTION = 12;
int GPIOPIN = 4;
int NPNPIN = 17;
int REF_VOLTAGE_PIN = 2;
int RELAY_PIN = 25;
int TOGGLE_RELAY_PIN = 14;

int peweem = 0;
int limitPwm = 1800; 
int isRedFlag = 0;

const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;
volatile byte state = LOW;

/* create a hardware timer */
hw_timer_t *timer = NULL;

/*Interrupt routine for Timer overflow event*/
void IRAM_ATTR onTimer()
{
    state = !state;
    digitalWrite(LED_BUILTIN, state);
}

void setup()
{
    Serial.begin(9600);

    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
    ledcAttachPin(GPIOPIN, PWM_CHANNEL);
    pinMode(REF_VOLTAGE_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(TOGGLE_RELAY_PIN, OUTPUT);

    digitalWrite(TOGGLE_RELAY_PIN, HIGH);

    digitalWrite(REF_VOLTAGE_PIN, HIGH);

    // timer
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
    ads1015.begin(0x48);
}

int current;
int temp;
int refCurrent = 0;

int target_current = 600; // mA
int vcc = 5121;           // mV
int qov = vcc / 2;        // mV
int max_current = 20000;  // mA
int readAdc;
int driftVoltage = -1;
float sensitivity = 0.066;
long lastPrint;

void loop()
{
    delay(10);
    /* relay enable */
    int relayPinStatus = digitalRead(TOGGLE_RELAY_PIN);
    if (relayPinStatus == LOW){
        digitalWrite(RELAY_PIN, HIGH);
    }
    else{
        digitalWrite(RELAY_PIN, LOW);
    }
        
    /* sensing task */
    int readAdcCurrent = ads1015.readADC_SingleEnded(0);
//    int readAdcVoltage = ads1015.readADC_SingleEnded(1);
//    int readAdcTemp = ads1015.readADC_SingleEnded(2);

//    int readAdcCurrent = 0;
    int readAdcVoltage = 0;
    int readAdcTemp = 0;
    
    int voltage = readAdcVoltage * 3;
    float current = ((float((readAdcCurrent * 3 + driftVoltage)) - qov)) / sensitivity;
    float temperature = ((float((readAdcTemp * 3))) / 10);

    /* read serial */
    static byte ndx = 0;
    char endMarker = '\n';
    char rc;

    while (Serial.available() > 0 && newData == false)
    {
        rc = Serial.read();

        if (rc != endMarker)
        {
            receivedChars[ndx] = rc;
            ndx++;
            if (ndx >= numChars)
            {
                ndx = numChars - 1;
            }
        }
        else
        {
            receivedChars[ndx] = '\0'; // terminate the string
            ndx = 0;
            newData = true;
        }
    }
    // insert serial to variable
    if (newData == true)
    {
        newData = false;
        #ifdef OPEN_LOOP
            peweem = atoi(receivedChars);       // insert serial info to duty cycle
        #endif

        #ifdef CLOSED_LOOP
            refCurrent =  atoi(receivedChars);  // insert serial info to reference current
        #endif

    }

    /* CL current control, berjalan hanya bila relay ON */
    #ifdef CLOSED_LOOP
        if (relayPinStatus == LOW){
            if (current > -100){
                if (current > refCurrent)
                {
                    if (peweem > 0){
                        peweem--;
                    }
                }
                else
                {
                    if (peweem < limitPwm)
                    {
                        peweem++;
                    }
                }
            }
        }
        else{
            peweem = 0;
        }
    #endif
    
    /* hardlimit & set PWM */
    if (peweem > limitPwm)
    {
        peweem = limitPwm;
    }
    
    ledcWrite(PWM_CHANNEL, peweem);
    digitalWrite(NPNPIN, LOW);

    /* Serial prints */
    //if (millis() - lastPrint > 500){
        lastPrint = millis();
        Serial.print("Volt: ");
        Serial.print(voltage);
        Serial.print("\tCurrent: ");
        Serial.print(current);
    
        
        Serial.print("\t\tADC Current: ");
        Serial.print(readAdcCurrent*3);
        
        Serial.print("\tRef Current: ");
        Serial.print(refCurrent);
    
        Serial.print("\t\tPWM: ");
        Serial.println(peweem);
    //}
}
