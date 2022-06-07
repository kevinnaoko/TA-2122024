#include <Adafruit_ADS1X15.h>

Adafruit_ADS1015 ads1015;

int PWM_FREQUENCY = 23000;
int PWM_CHANNEL = 0;
int PWM_RESOUTION = 12;
int GPIOPIN = 15;
int NPNPIN = 17;

int peweem = 0;
int limitPwm = 2000; 
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
    Serial.begin(112500);

    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
    ledcAttachPin(GPIOPIN, PWM_CHANNEL);
    pinMode(NPNPIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

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
int vcc = 5111;           // mV
int qov = vcc / 2;        // mV
int max_current = 20000;  // mA
int readAdc;
int driftVoltage = 17;

void loop()
{
    /* sensing task */
    int readAdcCurrent = ads1015.readADC_SingleEnded(0);
    int readAdcVoltage = ads1015.readADC_SingleEnded(1);
    int readAdcTemp = ads1015.readADC_SingleEnded(2);

    int voltage = readAdcVoltage * 3;
    float current = ((float((readAdcCurrent * 3 + driftVoltage)) - qov)) / 0.155;
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
        peweem = atoi(receivedChars);       // insert serial info to duty cycle
        // refCurrent =  atoi(receivedChars);  // insert serial info to reference current
    }

    /* CL current control */
    if (current > refCurrent)
    {
        peweem--;
    }
    else
    {
        if (peweem < limitPwm)
        {
            peweem++;
        }
    }

    
    /* hardlimit & set PWM */
    if (peweem > limitPwm)
    {
        peweem = limitPwm;
    }
    
    ledcWrite(PWM_CHANNEL, peweem);
    digitalWrite(NPNPIN, LOW);

    /* Serial prints */
    Serial.print("Volt: ");
    Serial.print(voltage);
    Serial.print("   Current: ");
    Serial.println(current);
    
    Serial.print("Ref Current: ");
    Serial.print(refCurrent);

    Serial.print("   PWM: ");
    Serial.println(peweem);

}
