/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cloud-mqtt-broker-sim800l/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  https://github.com/vshymanskyy/TinyGSM/blob/master/examples/AllFunctions/AllFunctions.ino
  
*/
#define MAX_DELAY_MS_GEOLOC 10000
#define MAX_DELAY_MS_TIME 1000
#define LOOP_DELAY_MS 2 

// Select your modem:
#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L

// Set serial for debug console (to the Serial Monitor, default speed 115200)
//#define SerialMon Serial
// Set serial for AT commands
#define SerialAT Serial1

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG Serial

#define MODEM_TX             27
#define MODEM_RX             26
// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";

// SIM card PIN (leave empty, if not defined)
const char simPIN[]   = ""; 

// MQTT details
const char* broker = "34.101.49.52";                   

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS

#include <Wire.h>
#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

#include <PubSubClient.h> 

TinyGsmClient clientelle(modem);
PubSubClient mqtt(clientelle);

uint32_t lastReconnectAttempt = 0;

// I2C for SIM800 (to keep it running when powered from battery)
TwoWire I2CPower = TwoWire(0);

TwoWire I2CBME = TwoWire(1);
//Adafruit_BME280 bme;

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

float temperature = 0;
float humidity = 0;
long lastMsg = 0;

//fx
void getCurrentTime(char* currentTime);
int checkGsm();

bool setPowerBoostKeepOn(int en){
  I2CPower.beginTransmission(IP5306_ADDR);
  I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    I2CPower.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    I2CPower.write(0x35); // 0x37 is default reg value
  }
  return I2CPower.endTransmission() == 0;
}

void mqttCallback(char* topic, byte* message, unsigned int len) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < len; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp/output1, you check if the message is either "true" or "false". 
  // Changes the output state according to the message
  if (String(topic) == "esp/output1") {
    Serial.print("Changing output to ");
    if(messageTemp == "true"){
      Serial.println("true");
    }
    else if(messageTemp == "false"){
      Serial.println("false");
    }
  }
  else if (String(topic) == "esp/output2") {
    Serial.print("Changing output to ");
    if(messageTemp == "true"){
      Serial.println("true");
    }
    else if(messageTemp == "false"){
      Serial.println("false");
    }
  }
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.print(broker);

  // Connect to MQTT Broker without username and password
  boolean status = mqtt.connect("GsmClientN"); 

  if (status == false) {
    Serial.println(" fail");
    ESP.restart();
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe("sys/commands"); 

  return mqtt.connected();
}


void setup() {
  // Set console baud rate
  Serial.begin(115200);
  delay(10);
    
  // Keep power when running from battery
  //  bool isOk = setPowerBoostKeepOn(1);
  //  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  Serial.println("Wait...");

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  //  modem.restart();
  modem.init();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

  // Unlock your SIM card with a PIN if needed
//  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
//    modem.simUnlock(GSM_PIN);
//  }

  Serial.print("Connecting to APN: ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn)) {
    Serial.println(" fail");
    // try again in 30 secs
    ESP.restart();
  }
  else {
    Serial.println(" -- OK");
  }
  
  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }

  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  if (!mqtt.connected()) {
    Serial.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;
    
    char blank[1] = "";
    char stringHolder[30];
      
    getCurrentTime(stringHolder);
    Serial.print("hey:");
    Serial.println(stringHolder);
    
    char tempString[12] = "test esp32";
    mqtt.publish("sys/test", stringHolder);
    
    strcpy(stringHolder, blank);
 
  }

  mqtt.loop();
}

int checkGsm(){
    String buffer2;
    int intercharTime = 0;
    SerialAT.print("AT\r");

    while ((intercharTime) < 100)
    {
        if (SerialAT.available())
        {
            int c = SerialAT.read(); 
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

void getCurrentTime(char* currentTime)
{
    if (checkGsm() == 0){
        Serial.println("SIM800L offline, cannot get time");
        return;
    }
    
    int intercharTime = 0; 
    Serial.println("Get time info ..."); 
    SerialAT.print("AT+CCLK?\r");  
    String buffer2;

    while (SerialAT.available()){
        Serial.write(SerialAT.read());
    }

    while ((intercharTime) < MAX_DELAY_MS_TIME){
        if (SerialAT.available()){
            int c = SerialAT.read();  
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
    // the snprintf solution
    char timeString[str_len+2];
    snprintf(timeString, sizeof timeString, "\"%s\"", token);
    strcpy(currentTime, timeString);
    return;
}
