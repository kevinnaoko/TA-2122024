//#include <SoftwareSerial.h>
#define Sim800l Serial1 


#define txGSM 26    //25
#define rxGSM 27    //26

//SoftwareSerial mySerial(26,27); // RX, TX 
void setup()  
{
  // Open serial communication
  Serial.begin(9600);

  // set the data rate for the SoftwareSerial port
  
  Sim800l.begin(9600, SERIAL_8N1, rxGSM, txGSM); 

  delay(1000);
  Serial.println("Testing SIM800L module");
  Serial.println();
  Serial.print("Sizeof(mySerial) = "); Serial.println(sizeof(Sim800l));
  Serial.println();

}

void loop() // run over and over
{

  if( Sim800l.available() )
  {
    char c = Sim800l.read();
    Serial.print(c);
  }

  if(Serial.available())
  {
    String Arsp = Serial.readString();

    Serial.println("Serial available");
    Serial.println(Arsp);
    Sim800l.println(Arsp);
    Serial.println("Serial available end");
  }

}
