#define MAX_MSG 200
#define serial_len 48

char msg[MAX_MSG];
char serial_num[16];
char charge_mode[10];
char temp;
char LorR;
int idx = 0;
int idx_max = 0;
long t, t2;
int readIdx = 0;
int msgIdx = 0;
short current = 0;
byte state = 1;
unsigned short voltageTemp = 0;
unsigned short soh;
unsigned short remainingCapacity;
unsigned short totalCapacity;
double soc;
int battVoltage = 0;

void get_serial(char LorR){
  t2 = millis();
  
  if(LorR == 'R'){
    Serial.write("RSerial_Number");
  }
  else{
    Serial.write("LSerial_Number");
  }

  // Apabila kondisi ini terpenuhi, data yang masuk tidak sesuai
  while(Serial.available() < 10 && (millis() - t2 < 1000)){
  }

  if(millis() - t2 > 1000){
    Serial.println("Timeout");
  }
  
  while(Serial.available() > 0){
    temp = Serial.read();
    if(temp == '+'){
      for(int i = 0; i<15; i++){
        serial_num[i] = Serial.read();
      } 
    }
  }

  temp = '\0';
   
  serial_num[15] = '\0';
  Serial.flush();
  Serial.println(serial_num);
}

void set_charge(char LorR){
  if(LorR == 'R'){
    Serial.write("RCharge");
  }
  else{
    Serial.write("LCharge");
  }

  
}

void setup() {
  Serial.begin(9600);
  t = millis();
  t2 = millis();
  LorR = 'R';
}

void loop(){
  if(state == 0){
    Serial.write("RCharge");
    while(Serial.available() > 0){
      msg[readIdx] = Serial.read();
      readIdx++;
    }
  
    msg[readIdx] = '\0';
  
    Serial.println(msg);
  
    if(strncmp(msg, "Rok", 3) == 0){
        state = 1;
    }
    else{
      state = 0;
    } 
  }
  else{
    readIdx = 0;
    msgIdx = 0;
    
    Serial.write("RBatt_Info");
    while(Serial.available() > 0){
      msg[readIdx] = Serial.read();
      readIdx++;
    }
  
    while(msgIdx < readIdx+1){
      Serial.print(msg[msgIdx]);
      Serial.print(" , ");
      Serial.println(msg[msgIdx], HEX);
  
      msgIdx++;
    }
  
    current = (msg[12]<<8) | (msg[13]);

    remainingCapacity = (msg[76] << 8) | (msg[77]);
    totalCapacity = (msg[78] << 8) | (msg[79]);
    soc = (double) remainingCapacity / (double) totalCapacity;
    soh = (msg[80] << 8) | (msg[81]);

    for(int i = 0; i < 38; i+=2){
      voltageTemp = (msg[14+i]<<8) | (msg[14+i+1]);
      battVoltage += voltageTemp;
      Serial.print("Cell ");
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(voltageTemp);
    }

    Serial.print("Voltage: ");
    Serial.println(battVoltage);
 
    Serial.print("Current: ");
    Serial.println(current);

    Serial.print("SoH: ");
    Serial.println(soh);

    Serial.print("Remaining Capacity: ");
    Serial.println(remainingCapacity);

    Serial.print("Total Capacity: ");
    Serial.println(totalCapacity);

    Serial.print("SoC: ");
    Serial.println(soc);  
  }
  battVoltage = 0;
  Serial.print("Keluar");
  delay(3000);
}
