#define MAX_DELAY_MS_GEOLOC 10000
#define MAX_DELAY_MS_TIME 1000
#define LOOP_DELAY_MS 2 

#include <SoftwareSerial.h>
SoftwareSerial Sim800l(26, 27);

// fx
void buttonHandler();
void getLocation(int* param);
void getCurrentTime(char* currentTime);
int checkGsm();

// vars
int jb = 0;

int isButton1Pushed;
int isButton2Pushed;
int isButtonModePushed;
long lastPressed;

int getTimeCmd = 0;
int getLocCmd = 0;

const byte numChars = 128;
char receivedChars[numChars]; // an array to store the received data

boolean newData = false;

int dataNumber = 0; // new for this version

// interrupt functions
void IRAM_ATTR Ext_INT1_ISR()
{
    isButton1Pushed = 1;
}

void IRAM_ATTR Ext_INT2_ISR()
{
    isButton2Pushed = 1;
}

void IRAM_ATTR Ext_INT3_ISR()
{
    isButtonModePushed = 1;
}

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);
    Sim800l.begin(9600);

    // interrupt pins
    int btn1_pin = 23;
    int btn2_pin = 19;
    int btnMode_pin = 18;
    pinMode(btn1_pin, INPUT_PULLUP);
    pinMode(btn2_pin, INPUT_PULLUP);
    pinMode(btnMode_pin, INPUT_PULLUP);
    attachInterrupt(btn1_pin, Ext_INT1_ISR, RISING);
    attachInterrupt(btn2_pin, Ext_INT2_ISR, RISING);
    attachInterrupt(btnMode_pin, Ext_INT3_ISR, RISING);
}

void loop()
{
    char stringHolder[128];  
    int locDetails[5]; 

    buttonHandler();

    if (getLocCmd == 1)
    {
        getLocation(locDetails);
        Serial.println("OUT: ");
        //Serial.println(info);
        for (int i = 0; i < 4; i++) {
            Serial.println(locDetails[i]);
        } 
        getLocCmd = 0;
    }

    if (getTimeCmd == 1)
    {
        getCurrentTime(stringHolder);
        Serial.print("OUT: ");
        //Serial.println(info2);
        Serial.println(stringHolder);
 
        getTimeCmd = 0;
    }


}

void getLocation(int* param)
{
    if (checkGsm() == 0){
        Serial.println("SIM800L offline, cannot get CGI");
        return;
    }
    
    int intercharTime = 0;

    Serial.println("Get nearby antenna info ...");
    
    Sim800l.print("AT+CNETSCAN=1\r");
    delay(1000);

    Sim800l.print("AT+CNETSCAN\r");
    delay(100);

    String buffer2; 

    while ((intercharTime) < MAX_DELAY_MS_GEOLOC)
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

    int str_len = buffer2.length() + 1;
    char strProcess[str_len];
    buffer2.toCharArray(strProcess, str_len);

    // parsing
    /* get the first token */
    char sep[2] = {',', ':'};
    int CGI[5];

    String processing[10];
    char tempProcessing[50];
    char *token;
    char *token2;

    // get op
    token = strtok(strProcess, ","); 
    if (token == NULL){
        Serial.println("\nBuffer error, force quit fx\n");
        return;
    }

    // Parsing
    processing[0] = token;
    for (int i = 1; i < 10; i++){
        if (token == NULL){
            break;
        }
        processing[i] = strtok(NULL, ",");
    }

    // get mcc, mnc, lac, cellid
    int index = 0;
    for (int i = 1; i < 7; i++){
        // skip untuk row 3, 5, 7
        if (i == 3 || i == 5 || i == 7){
            continue;
        }

        // convert String to char[]
        char strBuf[50];
        processing[i].toCharArray(strBuf, 50);
        token2 = strtok(strBuf, ":");

        // mcc, mnc int
        if (index < 2){
            CGI[index] = atoi(strtok(NULL, ":"));
        }
        // lac cellid hex
        else{
            CGI[index] = (int)strtol(strtok(NULL, ":"), NULL, 16);
        }

        //masukkan nilai ke pointer args
        param[index] = CGI[index];
        index++;
    } 
    
    return;
}
 

void getCurrentTime(char* currentTime)
{
    if (checkGsm() == 0){
        Serial.println("SIM800L offline, cannot get time");
        return;
    }
    
    int intercharTime = 0; 
    Serial.println("Get time info ..."); 
    Sim800l.print("AT+CCLK?\r");  
    String buffer2;

    while (Sim800l.available()){
        Serial.write(Sim800l.read());
    }

    while ((intercharTime) < MAX_DELAY_MS_TIME){
        if (Sim800l.available()){
            int c = Sim800l.read();  
            //Serial.print((char)c);
            buffer2.concat((char)c); 
            intercharTime = 0; 
        }
        else{ 
            intercharTime += LOOP_DELAY_MS;
        }

        delay(LOOP_DELAY_MS);
    }

    int str_len = buffer2.length() + 1;
    char strProcess[str_len];
    buffer2.toCharArray(strProcess, str_len);

    // Parsing (+CCLK: "03/01/01,00:01:58+28")
    char *token; 
    token = strtok(strProcess, "\""); 
    token = strtok(NULL, "+"); 
    if (token == NULL){
        Serial.println("\nBuffer error, force quit fx\n");
        return;
    }
    strcpy(currentTime, token);
    return;
}

int checkGsm(){
    String buffer2;
    int intercharTime = 0;
    Sim800l.print("AT\r");

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

//    Serial.print("buf:");
//    Serial.println(buffer2);
//    Serial.println("DONE");

    if (buffer2.indexOf("OK")>0){
        return 1;        
    }
    else{
        return 0;
    }
}
 
void buttonHandler()
{
    if (isButton1Pushed == 0 && isButton2Pushed == 0 && isButtonModePushed == 0){
        return;
    }

    if (millis() - lastPressed < 500){
        return;
    }

    lastPressed = millis();

    if (isButton1Pushed == 1){
        getTimeCmd = 1;
    }

    if (isButton2Pushed == 1){
        getLocCmd = 1;
    }

    if (isButtonModePushed == 1){
        
    }

    isButton1Pushed = 0;
    isButton2Pushed = 0;
    isButtonModePushed = 0;
}
