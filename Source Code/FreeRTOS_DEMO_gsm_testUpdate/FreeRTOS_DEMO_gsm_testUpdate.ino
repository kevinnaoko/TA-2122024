/*  Filename: FreeRTOS_TA
 *  Description: Program for Control Subsystem
 *  Contributors:
 *    Danu Ihza Pamungkas
 *    Dhanurangga Al Fadh
 *    Kevin Naoko
 *    Eren
 */ 
#include <WiFi.h> 
//#include <SoftwareSerial.h>
//SoftwareSerial Sim800l(26, 27);

#define PRODUCT_ID 3
#define TINY_GSM_MODEM_SIM800 // Modem is SIM800L 
#include <TinyGsmClient.h>
#define Sim800l Serial1 
TinyGsm modem(Sim800l);


#include <PubSubClient.h> 
//WiFiClient espClient;
//PubSubClient client(espClient);

TinyGsmClient clientelle(modem);
PubSubClient client(clientelle);

// Library for ADS1115
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// delay definition 
#define MAX_DELAY_MS_GEOLOC 10000
#define MAX_DELAY_MS_TIME 1000
#define LOOP_DELAY_MS 2 

// State Definition
#define IDLE 0
#define RETRIEVE_SERIAL 1
#define TRIGGER 2
#define INIT_CHARGING 3
#define CHARGING 5
#define FINISH_CHARGING 4

// Pin Definition
// Right Side
#define triggerButtonR 5
#define batteryButtonR 35
#define chargerSW1R 32
#define chargerSW2R 33
#define redR 15
#define greenR 2
#define blueR 14

// Left Side
#define triggerButtonL 18
#define batteryButtonL 13
#define chargerSW1L 23
#define chargerSW2L 25
#define redL 4
#define greenL 16
#define blueL 17

// Others
#define chargingModePin 19
#define SDA 21
#define SCL 22
#define txBMS 12
#define rxBMS 14
#define txGSM 26
#define rxGSM 27

#define delayChargingState 60000

// mqtt
//#define brokerIP "192.168.1.102"
#define brokerIP "34.101.49.52"

// wifi param
#define wifi_ssid "G7"
#define wifi_password "1234567890"

//#define wifi_ssid "ThinQ"
//#define wifi_password "kentnaoko"

// sim800 + gprs param
#define MODEM_TX 27
#define MODEM_RX 26
const char apn[] = "internet";

// Global Variables
// Right Side
byte stateR = 0;
int PWM_FREQUENCYR = 50000;
int PWM_CHANNELR = 0;
int PWM_RESOUTIONR = 8;
unsigned short SoHR;
unsigned short remainingCapacityR;
unsigned short totalCapacityR;
double SoCR;
double voltR;
char msgR[200];
char serialNumberR[16] = "";
byte cmd_sendSlotR = 0;
// Adafruit_ADS1115 adsVoltageCurrent;

// Left Side
byte stateL = 0;
int PWM_FREQUENCYL = 50000;
int PWM_CHANNELL = 0;
int PWM_RESOUTIONL = 8;
unsigned short SoHL;
unsigned short remainingCapacityL;
unsigned short totalCapacityL;
double SoCL;
double voltL;
char msgL[200];
char serialNumberL[16] = "";
byte cmd_sendSlotL = 0;

// Others
int locDetails[5];
int chargingMode = 0;
byte isGetLocation = 0;
long lastGprsAttempt = 0;
long lastDebugUpload = 0;
char topics[3][20] = {
    "sys/s1",
    "sys/s2",
    "sys/device", 
};
char commandTopic[25] = "sys/charger3/commands";
// char topics[3][20]; 
// char commandTopic[25];
// sprintf(commandTopic, "sys/charger%d/commands", PRODUCT_ID); 
 
void setup()
{
    // Right Side Pin Configuration
    pinMode(triggerButtonR, INPUT_PULLUP);
    pinMode(batteryButtonR, INPUT_PULLUP);
    pinMode(chargerSW1R, OUTPUT);
    pinMode(chargerSW2R, OUTPUT);
    pinMode(redR, OUTPUT);
    pinMode(greenR, OUTPUT);
    pinMode(blueR, OUTPUT);

    // Left Side Pin Configuration
    pinMode(triggerButtonL, INPUT_PULLUP);
    pinMode(batteryButtonL, INPUT_PULLUP);
    pinMode(chargerSW1L, OUTPUT);
    pinMode(chargerSW2L, OUTPUT);
    pinMode(redL, OUTPUT);
    pinMode(greenL, OUTPUT);
    pinMode(blueL, OUTPUT);

    // Other pins configuration
    pinMode(chargingModePin, INPUT_PULLUP);

    // Initialize serial communication
    Serial.begin(9600); 
    Sim800l.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX); 

    // Connect to WiFi
//    WiFi.disconnect();
//    delay(3000);
//    Serial.println("start"); 
//    WiFi.begin(wifi_ssid, wifi_password);
//    while ((!(WiFi.status() == WL_CONNECTED))){
//        delay(300);
//        Serial.print("...");
//    }
//    Serial.println("Connected");
//    Serial.println("Your IP is");
//    Serial.println((WiFi.localIP()));

    // Connect to GPRS
    if (checkGsmNetwork() == 1){
        gprsConnectFuction();     
    } 
    else{
        Serial.println("Network unavailable, retrying GPRS in 30secs");
    }

    // mqtt params
    client.setServer(brokerIP, 1883);
    client.setCallback(callback);

    // untuk wifi
//    reconnectmqttserver();
     
    // Task Idle R and L
//    xTaskCreatePinnedToCore(LeftCode, "Left", 10000, NULL, 1, NULL, 0);
////    xTaskCreatePinnedToCore(SamplingTemperature, "SamplingTemperature", 10000, NULL, 1, NULL, 0);
//    xTaskCreatePinnedToCore(RightCode, "Right", 10000, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(MQTTLoop, "MQTTLoop", 10000, NULL, 2, NULL, 1);
//    xTaskCreatePinnedToCore(GsmTask, "GsmTask", 10000, NULL, 1, NULL, 0);    

}

void loop()
{
}

//void SamplingTemperature(void *parameter)
//{
//    for (;;)
//    { 
//    }
//}

void MQTTLoop(void *parameter)
{
    for (;;)
    { 
        char blank[1] = ""; 
        char stringHolder[256];

        int gsmStatus = checkGsmNetwork();
        int modemStatus = modem.isGprsConnected();

        // Setiap 15 detik, apabila koneksi GPRS belum ada, dipanggil fungsi koneksi GPRS 
        if (millis() - lastGprsAttempt > 15000 && gsmStatus == 1 && modemStatus == 0){
            if (modem.isGprsConnected()){
                Serial.println("GPRS Connected");
            }
            else{ 
                Serial.println("GPRS not connected, connecting..."); 
                lastGprsAttempt = millis();
                gprsConnectFuction();  
                if (modem.isGprsConnected()){
                      
                }
            }
        }
        
        // else{
        //     Serial.println("Network unavailable, cant connect to GPRS & broker");
        // }

        // if (!modem.isGprsConnected() && millis() - lastGprsAttempt > 30000){
        //     lastGprsAttempt = millis();
        //     gprsConnectFuction();
        // }

        // Bila sudah terkoneksi GPRS namun belum terkoneksi ke broker MQTT dipanggil fungsi koneksi ke broker
        if (!client.connected() && modem.isGprsConnected()){
            reconnectmqttserver();
        } 
        
        // fungsi untuk keberjalanan modul PubSubClient.h
        client.loop(); 
        
        // test doang, delete nanti 
         if (millis() - lastDebugUpload > 5000){
//             Serial.print("Network status: ");
//             Serial.println(gsmStatus);
//             Serial.print("GPRS status: ");
//             Serial.println(modemStatus);
             lastDebugUpload = millis();
             Serial.println("debug set cmd_sendSlotL = 1");
//             cmd_sendSlotL = 1;

            strcpy(stringHolder, blank);
            buildStringBatteryInfo(0, stringHolder);
            Serial.println(stringHolder);

            strcpy(stringHolder, blank);
            buildStringDeviceInfo(stringHolder);
            Serial.println(stringHolder);

         }

        // Sampling lokasi apabila informasi belum didapatkan 
        if (!(isGetLocation))
        {   
            if (checkGsm() == 1){
                getLocation(locDetails);
                Serial.println("Loc:");
                for (int l = 0; l < 4; l++){
                    Serial.println(locDetails[l]);
                }  
                
                isGetLocation = 1;
            }
                
        }

        // Proses bila sudah terkoneksi ke broker
        // terdapat beberapa variabel command yang diset 1 pada state tertentu di Task Left dan Right battery
        // proses publish harus dilakukan terpisah dengan Task Left dan Right
        // karena proses komunikasi dengan SIM800L harus ada di task yang level prioritas nya lebih tinggi
        if (client.connected()){
            // command publsih data slot kiri (slot 1)
            if (cmd_sendSlotL == 1){  
                strcpy(stringHolder, blank);
                buildStringBatteryInfo(0, stringHolder);
                Serial.println(stringHolder);
                client.publish(topics[0], stringHolder);
                strcpy(stringHolder, blank);
                buildStringDeviceInfo(stringHolder);
                Serial.println(stringHolder);
                client.publish(topics[2], stringHolder);
    
                cmd_sendSlotL = 0;
            }
            // command publsih data slot kiri (slot 2)
            if (cmd_sendSlotR == 1){  
                strcpy(stringHolder, blank);
                buildStringBatteryInfo(1, stringHolder);
                Serial.println(stringHolder);
                client.publish(topics[1], stringHolder);
                strcpy(stringHolder, blank);
                buildStringDeviceInfo(stringHolder);
                Serial.println(stringHolder);
                client.publish(topics[2], stringHolder);
    
                cmd_sendSlotR = 0;
            }
        }

        // bila koneksi ke broker belum ada, command publish ditahan dulu
        // publish dilakukan setelah koneksi ke broker berhasil dilakukan  
        else{
            Serial.println("Can't upload, MQTT / GPRS unavailable");
            Serial.print("Network status: ");
            Serial.println(gsmStatus);
            Serial.print("GPRS status: ");
            Serial.println(modemStatus);
        }
                  
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
}

void GsmTask(void *parameter){
    for (;;)
    {
        vTaskDelay(3 / portTICK_PERIOD_MS);
    }
}

void LeftCode(void *parameter)
{
    for (;;)
    {
        if (stateL == IDLE)
        {
            stateIdleL();
            idleTransitionL();
        }
        else if (stateL == RETRIEVE_SERIAL)
        {
            stateRetrieveSerialL();
        }
        else if (stateL == TRIGGER)
        {
            stateTriggerBMSL();
        }
        else if (stateL == INIT_CHARGING)
        {
            initChargingL();
        }
        else if (stateL == FINISH_CHARGING)
        {
            stateFinishChargeL();
        }
        else if (stateL == CHARGING)
        {
            stateChargingL();
        }
    }
}

void RightCode(void *parameter)
{
    for (;;)
    {
        if (stateR == IDLE)
        {
            stateIdleR();
            idleTransitionR();
        }
        else if (stateR == RETRIEVE_SERIAL)
        {
            stateRetrieveSerialR();
        }
        else if (stateR == TRIGGER)
        {
            stateTriggerBMSR();
        }
        else if (stateR == INIT_CHARGING)
        {
            initChargingR();
        }
        else if (stateR == FINISH_CHARGING)
        {
            stateFinishChargeR();
        }
        else if (stateR == CHARGING)
        {
            stateChargingR();
        }
    }
}

// -----Left Functions----- //
void LED_1L(int red_light_value_1, int green_light_value_1, int blue_light_value_1)
{
    digitalWrite(redL, red_light_value_1);
    digitalWrite(greenL, green_light_value_1);
    digitalWrite(blueL, blue_light_value_1);
}

void idleTransitionL()
{

    // Serial.println(stateL);

    byte triggerDetectedL;                      // Untuk mendeteksi trigger button ditekan
    byte battSWL = digitalRead(batteryButtonL); // Untuk mendeteksi switch pada dudukan baterai ditekan
    int16_t adc;
    float vBattery = 0;

    // Apabila terdeteksi ada baterai, pindah ke state 1. Selain itu, pindah ke state 2
    if (battSWL == 0)
    {
        stateL = RETRIEVE_SERIAL;
    }
    else
    {
        triggerDetectedL = digitalRead(triggerButtonL);
        if (triggerDetectedL == 0)
        {
            stateL = TRIGGER;
        }
    }
}

void stateIdleL()
{
    //  Serial.print("L Idle\n");

    // Membuka transistor SW2
    digitalWrite(chargerSW2L, HIGH);

    // Memberika indikator LED merah
    LED_1L(HIGH, LOW, LOW);

    // Delay
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void stateRetrieveSerialL()
{
    // Memberikan indikator LED warna kuning
    LED_1L(HIGH, HIGH, LOW);
    int readIdx = 0;
    int msgIdx = 0;
    int serialNumberIdx = 0;
    int plusCount = 0;
    int serialNumberCount = 0;
    byte battSWL = digitalRead(batteryButtonL);

    // Mengirim request serial number. request dikirimkan dua kali karena ada delay respon dari BMS sebesar satu request
    Serial.write("LSerial_Number");

    // Pada titik ini, biasanya respon yang dikirimkan dari BMS bukan respon dari request di atas, melainkan request sebelumnya lagi
    // Jadi, ini untuk menghilangkan respon dari request sebelumnya yang sebenarnya tidak dibutuhkan lagi
    while (Serial.available())
    {
        Serial.read();
    }

    vTaskDelay(800 / portTICK_PERIOD_MS);

    // Mengirimkan request lagi
    Serial.write("LSerial_Number");
    Serial.flush();

    // Membaca respon dari request
    while (Serial.available() > 0)
    {
        msgL[readIdx] = Serial.read();
        readIdx++;
    }

    // Menghitung karakter '+'
    while (msgIdx < readIdx + 1)
    {
        if (plusCount == 1)
        {
            serialNumberCount++;
        }

        if (msgL[msgIdx] == '+')
        {
            plusCount++;
            if (plusCount == 1)
            {
                serialNumberIdx = msgIdx + 1;
            }
        }

        msgIdx++;
    }

    msgIdx = 0;

    // Memindahkan nomor serial baterai ke variabel serialNumberL
    while (msgL[serialNumberIdx] != '+' && serialNumberIdx < readIdx + 1)
    {
        serialNumberL[msgIdx] = msgL[serialNumberIdx];
        serialNumberIdx++;
        msgIdx++;
    }

    // Terminasi string
    serialNumberL[msgIdx] = '\0';

    // Untuk troubleshooting
    Serial.println(serialNumberL);

    // Mengirim request serial number. request dikirimkan dua kali karena ada delay respon dari BMS sebesar satu request
    Serial.write("LBatt_Info");

    // Pada titik ini, biasanya respon yang dikirimkan dari BMS bukan respon dari request di atas, melainkan request sebelumnya lagi
    // Jadi, ini untuk menghilangkan respon dari request sebelumnya yang sebenarnya tidak dibutuhkan lagi
    while (Serial.available())
    {
        Serial.read();
    }

    vTaskDelay(800 / portTICK_PERIOD_MS);

    // Mengirimkan request lagi
    Serial.write("LBatt_Info");
    Serial.flush();

    // Membaca respon dari request
    while (Serial.available() > 0)
    {
        msgL[readIdx] = Serial.read();
        readIdx++;
    }

    SoHL = (msgL[80] << 8) | (msgL[81]);
    remainingCapacityL = (msgL[76] << 8) | (msgL[77]);
    totalCapacityL = (msgL[78] << 8) | (msgL[79]);
    SoCL = (double) remainingCapacityL / (double) totalCapacityL;

    Serial.println(SoHL);
    Serial.println(remainingCapacityL);
    Serial.println(totalCapacityL);
    Serial.println(SoCL);

    // Transisi state
    if (battSWL == 0)
    {
        if (plusCount == 2 && serialNumberCount > 1)
        {
            stateL = INIT_CHARGING;
        }
        else
        {
            stateL = RETRIEVE_SERIAL;
        }
    }
    else
    {
        stateL = IDLE;
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void stateTriggerBMSL()
{
    //  Serial.print("L Triggered\n");
    byte battSWL = digitalRead(batteryButtonL);
    LED_1L(HIGH, HIGH, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1L(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1L(HIGH, HIGH, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1L(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1L(HIGH, HIGH, LOW);

    digitalWrite(chargerSW2L, HIGH);

    if (battSWL == 0)
    {
        stateL = RETRIEVE_SERIAL;
    }
    else
    {
        stateL = IDLE;
    }
}

void initChargingL()
{
    int readIdx = 0;
    int msgIdx = 0;
    byte battSWL = digitalRead(batteryButtonL);
    char stringHolder[256];
    char blank[1] = "";

    // Indikator blink biru
    LED_1L(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1L(LOW, LOW, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Meminta request dan membaca respon yang didapat
    Serial.write("LCharge");
    while (Serial.available() > 0)
    {
        msgL[readIdx] = Serial.read();
        readIdx++;
    }

    // Terminasi string
    msgL[readIdx] = '\0';

    // Untuk troubleshooting
    Serial.println(msgL);

    // Apabila respon yang didapat adalah "Lok", pindah ke state 5. Selain itu, kirimkan request.
    // Tapi apabila baterai dilepas, kembali ke state idle
    if (battSWL == 0)
    {
        if (strncmp(msgL, "Lok", 3) == 0)
        {
            stateL = CHARGING;
            cmd_sendSlotL = 1;
        }
        else
        {
            stateL = INIT_CHARGING;
        }
    }
    else
    {
        stateL = IDLE;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void stateChargingL()
{
    //  Serial.print("L Charging\n");
    byte battSWL = digitalRead(batteryButtonL);
    LED_1L(LOW, LOW, HIGH);
    float tegangan;
    float arus;

    vTaskDelay(delayChargingState / portTICK_PERIOD_MS); // Nanti diubah

    if (battSWL == 0)
    {
        stateL = 4;
        cmd_sendSlotL = 1;
    }
    else
    {
        stateL = 0;
        cmd_sendSlotL = 1;
    }
}

void stateFinishChargeL()
{
    //  Serial.print("L Finish Charging\n");
    byte battSWL = digitalRead(batteryButtonL);

    LED_1L(LOW, HIGH, LOW);

    if (battSWL == 0)
    {
        stateL = 4;
    }
    else if (battSWL == 1)
    {
        stateL = 0;
        cmd_sendSlotL = 1;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

// -----Right Functions----- //
void LED_1R(int red_light_value_1, int green_light_value_1, int blue_light_value_1)
{
    digitalWrite(redR, red_light_value_1);
    digitalWrite(greenR, green_light_value_1);
    digitalWrite(blueR, blue_light_value_1);
}

void idleTransitionR()
{
    byte triggerDetectedR;
    byte battSWR = digitalRead(batteryButtonR);
    int16_t adc;
    float vBattery;

    if (battSWR == 0)
    {
        stateR = 1;
    }
    else
    {
        triggerDetectedR = digitalRead(triggerButtonR);
        if (triggerDetectedR == 0)
        {
            stateR = 2;
        }
    }
}

void stateIdleR()
{
    //  Serial.print("R Idle\n");
    digitalWrite(chargerSW2R, HIGH);

    LED_1R(HIGH, LOW, LOW);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void stateRetrieveSerialR()
{
    LED_1R(HIGH, HIGH, LOW);
    int readIdx = 0;
    int msgIdx = 0;
    int serialNumberIdx = 0;
    int plusCount = 0;
    int serialNumberCount = 0;
    byte battSWR = digitalRead(batteryButtonR);

    Serial.write("RSerial_Number");
    while (Serial.available())
    {
        Serial.read();
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);

    Serial.write("RSerial_Number");
    while (Serial.available() > 0)
    {
        msgR[readIdx] = Serial.read();
        readIdx++;
    }

    while (msgIdx < readIdx + 1)
    {
        if (plusCount == 1)
        {
            serialNumberCount++;
        }

        if (msgR[msgIdx] == '+')
        {
            plusCount++;
            if (plusCount == 1)
            {
                serialNumberIdx = msgIdx + 1;
            }
        }

        msgIdx++;
    }

    msgIdx = 0;

    while (msgR[serialNumberIdx] != '+' && serialNumberIdx < readIdx + 1)
    {
        serialNumberR[msgIdx] = msgR[serialNumberIdx];
        serialNumberIdx++;
        msgIdx++;
    }

    serialNumberR[msgIdx] = '\0';

    Serial.println(serialNumberR);

    // Mengirim request serial number. request dikirimkan dua kali karena ada delay respon dari BMS sebesar satu request
    Serial.write("RBatt_Info");

    // Pada titik ini, biasanya respon yang dikirimkan dari BMS bukan respon dari request di atas, melainkan request sebelumnya lagi
    // Jadi, ini untuk menghilangkan respon dari request sebelumnya yang sebenarnya tidak dibutuhkan lagi
    while (Serial.available())
    {
        Serial.read();
    }

    vTaskDelay(800 / portTICK_PERIOD_MS);

    // Mengirimkan request lagi
    Serial.write("RBatt_Info");
    Serial.flush();

    // Membaca respon dari request
    while (Serial.available() > 0)
    {
        msgR[readIdx] = Serial.read();
        readIdx++;
    }

    SoHR = (msgR[80] << 8) | (msgR[81]);
    remainingCapacityR = (msgR[76] << 8) | (msgR[77]);
    totalCapacityR = (msgR[78] << 8) | (msgR[79]);
    SoCR = (double) remainingCapacityR / (double) totalCapacityR;

    Serial.println(SoHR);
    Serial.println(remainingCapacityR);
    Serial.println(totalCapacityR);
    Serial.println(SoCR);

    if (battSWR == 0)
    {
        if (plusCount == 2 && serialNumberCount > 1)
        {
            stateR = 3;
        }
        else
        {
            stateR = 1;
        }
    }
    else
    {
        stateR = 0;
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void stateTriggerBMSR()
{
    //  Serial.print("R Triggered\n");
    byte battSWR = digitalRead(batteryButtonR);
    LED_1R(HIGH, HIGH, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1R(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1R(HIGH, HIGH, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1R(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1R(HIGH, HIGH, LOW);

    digitalWrite(chargerSW2R, HIGH);

    if (battSWR == 0)
    {
        stateR = 1;
    }
    else
    {
        stateR = 0;
    }
}

void initChargingR()
{
    int readIdx = 0;
    int msgIdx = 0;
    byte battSWR = digitalRead(batteryButtonR);
    char stringHolder[256];
    char blank[1] = "";

    LED_1R(LOW, LOW, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    LED_1R(LOW, LOW, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS);

    Serial.write("RCharge");
    while (Serial.available() > 0)
    {
        msgR[readIdx] = Serial.read();
        readIdx++;
    }

    msgR[readIdx] = '\0';

    Serial.println(msgR);

    if (battSWR == 0)
    {
        if (strncmp(msgR, "Rok", 3) == 0)
        {
            stateR = 5;
            cmd_sendSlotR = 1;
        }
        else
        {
            stateR = 3;
        }
    }
    else
    {
        stateR = 0;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void stateChargingR()
{
    //  Serial.print("R Charging\n");
    byte battSWR = digitalRead(batteryButtonR);
    LED_1R(LOW, LOW, HIGH);

    vTaskDelay(delayChargingState / portTICK_PERIOD_MS); // Nanti dihapus

    if (battSWR == 0)
    {
        stateR = 4;
        cmd_sendSlotR = 1;
    }
    else
    {
        stateR = 0;
        cmd_sendSlotR = 1;
    }
}

void stateFinishChargeR()
{
    //  Serial.print("R Finish Charging\n");
    byte battSWR = digitalRead(batteryButtonR);

    LED_1R(LOW, HIGH, LOW);

    if (battSWR == 0)
    {
        stateR = 4;
    }
    else if (battSWR == 1)
    {
        stateR = 0;
        cmd_sendSlotR = 1;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void reconnectmqttserver()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            client.subscribe(commandTopic);
            
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
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

void buildStringBatteryInfo(int battSelect, char *holder)
{
    char dbIndex[8][20] = {
        "\"Product ID\": ", "\"Status\": ", "\"Serial Number\": ", "\"Time\": ", "\"SoC\": ", "\"SoH\": ", " \"Volt\": "};

    char sep[3] = ", ";
    char buildSN[18];
    char blank[1] = "";
    char ob = '{';
    char cb = '}';

    char stringHolder[30];
    // Openbracket
    strncat(holder, &ob, 1);

    // Product ID
    strcat(holder, dbIndex[0]);
    sprintf(stringHolder, "%d", PRODUCT_ID);
    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);
    
    // Status
    strcat(holder, dbIndex[1]);
    if (battSelect == 0)
    {
        if(stateL == CHARGING){
            sprintf(stringHolder, "%d", 1);
        }
        else if(stateL == FINISH_CHARGING){
            sprintf(stringHolder, "%d", 2);
        }
        else{
            sprintf(stringHolder, "%d", 0);
        }
    }
    else
    {
        if(stateR == CHARGING){
            sprintf(stringHolder, "%d", 1);
        }
        else if(stateR == FINISH_CHARGING){
            sprintf(stringHolder, "%d", 2);
        }
        else{
            sprintf(stringHolder, "%d", 0);
        }
    }

    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);

    // Serial Number
    strcat(holder, dbIndex[2]);
    if (battSelect == 0)
    {
        sprintf(stringHolder, "\"%s\"", serialNumberL);
        strcat(holder, stringHolder);
    }
    else
    {
        sprintf(stringHolder, "\"%s\"", serialNumberR);
        strcat(holder, stringHolder);
    }
    strcpy(stringHolder, blank);
    strcat(holder, sep);

    // Time
    strcat(holder, dbIndex[3]);
    strcpy(stringHolder, "\"\"");
    getCurrentTime(stringHolder);
    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);

    // SOC
    strcat(holder, dbIndex[4]);
    if (battSelect == 0)
    {
        sprintf(stringHolder, "%f", SoCL);
    }
    else
    {
        sprintf(stringHolder, "%f", SoCR);
    }
    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);
    
     // SOH
     strcat(holder, dbIndex[5]);
     if (battSelect == 0)
     {
         sprintf(stringHolder, "%d", SoHL);
     }
     else
     {
         sprintf(stringHolder, "%d", SoHR);
     }
     strcat(holder, stringHolder);
     strcpy(stringHolder, blank); 
     strcat(holder, sep);
     
     // Volt
     strcat(holder, dbIndex[6]);
     if (battSelect == 0)
     {
         sprintf(stringHolder, "%d", voltL);
     }
     else
     {
         sprintf(stringHolder, "%d", voltR);
     }
     strcat(holder, stringHolder);
     strcpy(stringHolder, blank);

    // closebracket
    strncat(holder, &cb, 1);
}

void buildStringDeviceInfo(char *holder)
{
    char dbIndex[6][20] = {
        "\"Product ID\": ", "\"Charging Mode\": ", "\"MCC\": ", "\"MNC\": ", "\"LAC\": ", "\"Cell ID\": "};
    char sep[3] = ", ";
    char blank[1] = "";
    char ob = '{';
    char cb = '}';

    char stringHolder[30]; 

    // Openbracket
    strncat(holder, &ob, 1);

    // Product ID
    strcat(holder, dbIndex[0]);
    sprintf(stringHolder, "%d", PRODUCT_ID);
    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);
    
    // Charging Mode
    strcat(holder, dbIndex[1]);
    sprintf(stringHolder, "%d", chargingMode);
    strcat(holder, stringHolder);
    strcpy(stringHolder, blank);
    strcat(holder, sep);

    // Location
    // getLocation(locDetails);

    for (int i = 0; i < 4; i++)
    {
        strcat(holder, dbIndex[i + 2]);
        sprintf(stringHolder, "%d", locDetails[i]);
        strcat(holder, stringHolder);
        strcpy(stringHolder, blank);
        if (i < 3)
        {
            strcat(holder, sep);
        }
    }

    // closebracket
    strncat(holder, &cb, 1);
}

void getLocation(int *param)
{
    if (checkGsm() == 0)
    {
        Serial.println("SIM800L offline, cannot get CGI");
        return;
    }

    int intercharTime = 0;

    Serial.println("Get nearby antenna info ...");

    Sim800l.print("AT+CNETSCAN=1\r");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    Sim800l.print("AT+CNETSCAN\r");
    vTaskDelay(100 / portTICK_PERIOD_MS);

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
        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); 
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
    if (token == NULL)
    {
        Serial.println("\nBuffer error, force quit fx\n");
        return;
    }

    // Parsing
    processing[0] = token;
    for (int i = 1; i < 10; i++)
    {
        if (token == NULL)
        {
            break;
        }
        processing[i] = strtok(NULL, ",");
    }

    // get mcc, mnc, lac, cellid
    int index = 0;
    for (int i = 1; i < 7; i++)
    {
        // skip untuk row 3, 5, 7
        if (i == 3 || i == 5 || i == 7)
        {
            continue;
        }

        // convert String to char[]
        char strBuf[50];
        processing[i].toCharArray(strBuf, 50);
        token2 = strtok(strBuf, ":");

        // mcc, mnc int
        if (index < 2)
        {
            CGI[index] = atoi(strtok(NULL, ":"));
        }
        // lac cellid hex
        else
        {
            CGI[index] = (int)strtol(strtok(NULL, ":"), NULL, 16);
        }

        index++;
    }

    // masukkan nilai ke pointer args
    param[0] = CGI[0];
    param[1] = CGI[1];
    param[2] = CGI[3];
    param[3] = CGI[2];

    return;
}

void getCurrentTime(char *currentTime)
{
    if (checkGsm() == 0)
    {
        Serial.println("SIM800L offline, cannot get time");
        return;
    }

    int intercharTime = 0;
    Serial.println("Get time info ...");
    Sim800l.print("AT+CCLK?\r");
    String buffer2;

    while (Sim800l.available())
    {
        Serial.write(Sim800l.read());
    }

    while ((intercharTime) < MAX_DELAY_MS_TIME)
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
        vTaskDelay(LOOP_DELAY_MS / portTICK_PERIOD_MS); 
    }

    int str_len = buffer2.length() + 1;
    char strProcess[str_len];
    buffer2.toCharArray(strProcess, str_len);

    // Parsing (+CCLK: "03/01/01,00:01:58+28")
    char *token;
    token = strtok(strProcess, "\"");
    token = strtok(NULL, "+");
    if (token == NULL)
    {
        Serial.println("\nBuffer error, force quit fx\n");
        return;
    }
    // the snprintf solution
    char timeString[str_len + 2];
    snprintf(timeString, sizeof timeString, "\"%s\"", token);
    strcpy(currentTime, timeString);
    return;
}

int checkGsm()
{
    String buffer2;
    int intercharTime = 0; 
    Sim800l.print("AT\r");
    
    while ((intercharTime) < 100)
    {     
        if (Sim800l.available())
        {
            int c = Sim800l.read();
            //Serial.print((char)c);
            buffer2.concat((char)c);
            intercharTime = 0;
        }
        else
        {
            intercharTime += 1;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS); 
    }

    //    while (Sim800l.available()){
    //        Serial.write(Sim800l.read());
    //    }

    //    Serial.print("buf:");
    //    Serial.println(buffer2);
    //    Serial.println("DONE");

    if (buffer2.indexOf("OK") > 0)
    {
        return 1;
    }
    else
    {
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

//    Serial.print("buf:");
//    Serial.println(buffer2);
//    Serial.println("DONE");

    if (buffer2.indexOf("+CREG: 0,1")>0){
        return 1;        
    }
    else{
        return 0;
    }
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
        // ESP.restart();
    }
    else {
        Serial.println(" -- OK");
    }
    
    if (modem.isGprsConnected()) {
        Serial.println("GPRS connected"); 
    }
}
