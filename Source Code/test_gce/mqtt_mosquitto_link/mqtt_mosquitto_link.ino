#define MAX_DELAY_MS_GEOLOC 10000
#define MAX_DELAY_MS_TIME 1000
#define LOOP_DELAY_MS 2 
#define brokerAddress "34.101.49.52"
#define brokerPort 1883


#define Sim800l Serial1

#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L

#include <String.h>
#include <WiFi.h>

#include <Wire.h>
#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(Sim800l, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(Sim800l);
#endif

#include <PubSubClient.h>
 
#define TINY_GSM_DEBUG Serial

#define MODEM_TX             27
#define MODEM_RX             26

const char apn[] = "internet";

//#include <SoftwareSerial.h>
//SoftwareSerial Sim800l(26, 27);

//WiFiClient espClient;
//PubSubClient client(espClient);

TinyGsmClient clientelle(modem);
PubSubClient client(clientelle);

// functions
void reconnectmqttserver();
void callback(char* topic, byte* payload, unsigned int length);

void buildStringBatteryInfo(int battSelect, char* output);
void getCurrentTime(char* currentTime);
void getLocation(int* param);
int checkGsm();
int getSoC(int battSelect);
void getSN(int battSelect, char* serialNumber);
int getChargingStatus(int battSelect);
int eventHandler();
void buttonHandler();

// global variables

// global mock variables
int isChargingBatt1 = 0;
int isChargingBatt2 = 0;
int chargingMode = 0;
 
int oldIsChargingBatt1 = 0; 
int oldIsChargingBatt2 = 0;
int oldChargingMode = 0;

int currentTime;
int soc = 75;
char SN[17] = "ABCDE12345FGHIJK";

// program variables 
char msgmqtt[128];
long  i;

//char topics[3][20] = {
//    "sys/charger1/s1",
//    "sys/charger1/s2",
//    "sys/charger1/device", 
//};
 

char topics[3][20] = {
  "sys/charger2/s1",
  "sys/charger2/s2",
  "sys/charger2/device", 
};

 
int isButton1Pushed;
int isButton2Pushed;
int isButtonModePushed;
long lastPressed;

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

// Main Program
void setup()
{
  i = 0;
//  Serial.begin(115200);
  // put your setup code here, to run once:
  Serial.begin(9600);
//  Sim800l.begin(9600);
  Sim800l.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX); 
  delay(3000);
  
  // wifi connection
//  WiFi.disconnect();
//  delay(3000);
//  Serial.println("start");
//  WiFi.begin("ThinQ","kentnaoko");
//  while ((!(WiFi.status() == WL_CONNECTED))){
//    delay(300);
//    Serial.print("...");
//  }
//  Serial.println("Connected");
//  Serial.println("Your IP is");
//  Serial.println((WiFi.localIP()));

  // gprs connection
  gprsConnectFuction();

  client.setServer(brokerAddress, brokerPort);
  client.setCallback(callback);

  //interrupt pins
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
  // fungsi connect ke broker
  if (!client.connected()) {
    reconnectmqttserver();
  }
  // fungsi untuk keberjalanan modul PubSubClient.h
  client.loop();

//    char temp[128] = "{\"Sensor_ID\": \"Dummy-1\", \"Date\": \"10-Mar-2022 14:59:25:833428\", \"Humidity\": 56.41}";
//    
//    snprintf (msgmqtt, 128, "%s ", temp);
//    Serial.println(msgmqtt);
//
//    char sendTopic[24];
//    strcpy(sendTopic, topicPrefix);
//    strcat(sendTopic, topics[0]);
//    
//    client.publish(sendTopic, msgmqtt); 
//    delay(5000);

  buttonHandler(); 
  
  // fungsi eventHandler berperan dalam deteksi perubahan state pada slot atau mode pengisian daya
  int uploadSelect = eventHandler();   
  char holder[256];
  char blank[1] = "";
  
  switch( uploadSelect ){
    case 1:  
      buildStringBatteryInfo(0, holder);
      Serial.println(holder);  
      client.publish(topics[0], holder); 
      
      strcpy(holder, blank);
      break;
    case 2:
      buildStringBatteryInfo(1, holder);
      Serial.println(holder); 
      client.publish(topics[1], holder); 
      strcpy(holder, blank);
      break;
    case 3:
      buildStringDeviceInfo(holder);
      Serial.println(holder); 
      client.publish(topics[2], holder); 
      strcpy(holder, blank);
      break;
    default:
      break;
  }
  delay (100);
 
}

void gprsConnectFuction(){
  Serial.println("Initializing modem...");
  modem.init();

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

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
    lastGprsAttempt = 1;
  }
}

void reconnectmqttserver() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("sys/commands");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String MQTT_DATA = "";
  
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
    MQTT_DATA += (char)payload[i];
  }
  Serial.println();
}

void buildStringBatteryInfo(int battSelect, char* holder){
  char dbIndex[6][20] = {
      "\"Status\": ", "\"Serial Number\": ", "\"Time\": ", "\"SoC\": "  
  };

  char sep[3] = ", ";  
  char blank[1] = "";
  char ob = '{';
  char cb = '}';
  
  char stringHolder[30];  
  //Openbracket 
  strncat(holder, &ob , 1);
  //Status
  strcat(holder, dbIndex[0]);   
  sprintf(stringHolder, "%d", getChargingStatus(battSelect));
  strcat(holder, stringHolder);  
  strcpy(stringHolder, blank);
  strcat(holder, sep);
  
  //Serial Number  
  strcat(holder, dbIndex[1]);  
  getSN(battSelect, stringHolder);  
  strcat(holder, stringHolder);  
  strcpy(stringHolder, blank);
  strcat(holder, sep);
  
  //Time
  strcat(holder, dbIndex[2]);  
  getCurrentTime(stringHolder);  
  strcat(holder, stringHolder);  
  strcpy(stringHolder, blank);
  strcat(holder, sep);
  
  //SOC
  strcat(holder, dbIndex[3]); 
  sprintf(stringHolder, "%d", getSoC(battSelect));
  strcat(holder, stringHolder);  
  strcpy(stringHolder, blank); 
 
  //closebracket
  strncat(holder, &cb , 1);
}

void buildStringDeviceInfo(char* holder){
  char dbIndex[6][20] = { 
      "\"Charging Mode\": ", "\"MCC\": ", "\"MNC\": ", "\"LAC\": ", "\"Cell ID\": " 
  };
  char sep[3] = ", ";  
  char blank[1] = "";
  char ob = '{';
  char cb = '}';
  
  char stringHolder[30];  
  int locDetails[5];
  
  //Openbracket 
  strncat(holder, &ob , 1);
  //Charging Mode
  strcat(holder, dbIndex[0]);   
  sprintf(stringHolder, "%d", chargingMode);
  strcat(holder, stringHolder);  
  strcpy(stringHolder, blank);
  strcat(holder, sep);
  
  //Location 
  getLocation(locDetails);
  
  for (int i = 0; i < 4; i++){
    strcat(holder, dbIndex[i+1]); 
    sprintf(stringHolder, "%d", locDetails[i]);
    strcat(holder, stringHolder);  
    strcpy(stringHolder, blank);
    if (i < 3){
      strcat(holder, sep);
    } 
  }

  
  //closebracket 
  strncat(holder, &cb , 1);
  
}

//void getLocation(int* param){
//  //mock values
//  //mcc, mnc, lac, cellid
////  param[0] = 510;
////  param[1] = 11;
////  param[2] = 35117;
////  param[3] = 593639;
//
//  param[0] = 510;
//  param[1] = 1 ;
//  param[2] = 9147;
//  param[3] = 36665; 
//}

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
 
        index++;
    } 
    
    //masukkan nilai ke pointer args
    param[0] = CGI[0];
    param[1] = CGI[1];
    param[2] = CGI[3];
    param[3] = CGI[2];
    
    return;
}

//void getCurrentTime(char* currentTime){
//  strcpy(currentTime, "\"10-Mar-2022 14:59\"");  
//}

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
    // the snprintf solution
    char timeString[str_len+2];
    snprintf(timeString, sizeof timeString, "\"%s\"", token);
    strcpy(currentTime, timeString);
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

    if (buffer2.indexOf("OK")>0){
        return 1;        
    }
    else{
        return 0;
    }
}

int getSoC(int battSelect){
  int soc;
  if (battSelect == 0){
    soc = 75; 
  }
  else if (battSelect == 1){
    soc = 55; 
  }
   
  return soc;
}

int getChargingStatus(int battSelect){
  int battStatus;
  
  if (battSelect == 0)
    battStatus = isChargingBatt1;
  else if (battSelect == 1)
    battStatus = isChargingBatt2;
    
  return battStatus;
}

void getSN(int battSelect, char* serialNumber){
  //fungsi dapetin SN, sementara mock
  if (battSelect == 0){
    strcpy(serialNumber, "\"0123456789ABCDEF\"");
  }
  else if (battSelect == 1){
    strcpy(serialNumber, "\"ABCDEF9876543210\"");
  } 
}

int eventHandler(){
  /*
   * Return value untuk tipe data yang diupload
   * 0 = tidak upload apa apa
   * 1 = upload kondisi start/finish charging Batt1
   * 2 = upload kondisi start/finish charging Batt2
   * 3 = upload perubahan switch mode daya 
   */
  int uploadSelect = 0;

  if (isChargingBatt1 != oldIsChargingBatt1){
    uploadSelect = 1;
    oldIsChargingBatt1 = isChargingBatt1;
  }

  else if (isChargingBatt2 != oldIsChargingBatt2){
    uploadSelect = 2;
    oldIsChargingBatt2 = isChargingBatt2;
  }
   
  else if (chargingMode != oldChargingMode){
    uploadSelect = 3;
    oldChargingMode = chargingMode;
  }

  return uploadSelect; 
}

void buttonHandler(){
  if (isButton1Pushed == 0 && isButton2Pushed == 0 && isButtonModePushed == 0){
    return;
  }

  if (millis() - lastPressed < 500){
    return;
  }
  
  Serial.println("HALO"); 
  lastPressed = millis();
  
  if (isButton1Pushed == 1){
    if (isChargingBatt1 == 1){
      isChargingBatt1 = 0; 
    }
    else if (isChargingBatt1 == 0){
      isChargingBatt1 = 1; 
    } 
  }

  if (isButton2Pushed == 1){
    if (isChargingBatt2 == 1){
      isChargingBatt2 = 0; 
    }
    else if (isChargingBatt2 == 0){
      isChargingBatt2 = 1; 
    } 
  }

  if (isButtonModePushed == 1){ 
    if (chargingMode == 1){
      chargingMode = 0; 
    }
    else if (chargingMode == 0){
      chargingMode = 1; 
    } 
  }
 
  isButton1Pushed = 0;
  isButton2Pushed = 0;
  isButtonModePushed = 0;
}
