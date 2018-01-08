

//i2c add 0x76      BMP280  (adafruit lib edited to change add)
//        0x57 0x68 TOD 
//        0x23      BH1750


#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "ds3231.h"
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>  //lcd display removed
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h> 
//#include <Datalogger_fj.h>  
#include <DHT.h>  //DHT22
#include <DHT_U.h>
#include <BH1750.h>


// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 53;
bool runLogger=false;
String curDayLogName;
String fullLogName;
int loopDly=10;
struct ts t;

#include "CommandLine.h"  //i need this after declare of card/volume/root used in CommandLine.h

#define BUFF_MAX 128
#define DHTPIN            48         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)

BH1750 lightMeter;
Adafruit_BMP280 bmp; 
DHT_Unified dht(DHTPIN, DHTTYPE);

char recv[BUFF_MAX];
uint8_t time[8];

char buff[BUFF_MAX];
String readstring;
//String sensorData;

int SD_SDSC = 53;      
int LCD_DC = 49;
int LCD_CECS = 48;
int LCD_RST = 47;

char *cmd;

String bmp280_temp;
String bmp280_press;
String bmp280_alt;
String dht22_temp;
String dht22_humidity;
uint16_t lux;
String bh_lux;
int rain;

char read_char;

bool received=false;
int rc;
int oldDayOfWeek;
int curDayOfWeek;
int curRwSensorValue = 0;
int oldRwSensorValue = 0;
int rwSensorPin=22; //red weevill detector pin
int rwCount=0;
int curRwCount=0;


  void    setup() {
    Serial.begin(9600);
    Serial.print("\nInitializing SD card...");

    if (!card.init(SPI_HALF_SPEED, chipSelect)) {
      Serial.println("initialization failed");   
      return;
    } else {
      Serial.println("SD card is present.");
    }

    Wire.begin();
    DS3231_init(DS3231_INTCN);
    memset(recv, 0, BUFF_MAX);
    getTime();
    printTime();

    oldDayOfWeek=t.wday;
    
    curDayLogName=String(t.year,DEC)+String(t.mon,DEC)+String(t.mday,DEC)+".csv"; //WARNING:name of file must be 12 car max
    Serial.print("Current Log Name : ");
    Serial.println(curDayLogName);
 
    Serial1.begin(9600); //serial interface to bluetooth

    if (!bmp.begin()) {
      Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
    }

    dht.begin();
    lightMeter.begin();
    
    SD.begin(chipSelect);   //53 

    fullLogName = "full_log.csv";
    Serial.println(F("Waiting for startLog command to start logging")); 
  }

  void  loop() {
    
    getTime();
    curDayLogName=String(t.year,DEC)+String(t.mon,DEC)+String(t.mday,DEC)+".csv"; //WARNING:name of file must be 12 car max
    curDayOfWeek=t.wday;
    if (oldDayOfWeek !=  curDayOfWeek) {     //i need to write the date in full log when day changes
       oldDayOfWeek = curDayOfWeek;
       rc=writeDayToFullLog(); if (rc) {Serial.println("error opening full log file in write");}; 
    }
      
    while(runLogger==false) {                 // start stop the loop
       received = getCommandLineFromSerialPort(CommandLine);      //global CommandLine is defined in CommandLine.h
       if (received) DoMyCommand(CommandLine);
       received = getCommandLineFromBT(CommandLine);              //from bluetooth
       if (received) DoMyCommand(CommandLine);
    }

    rc=writeSampleToCurDayLog(); if (rc) {Serial.println("error opening current day log file in write");};
    rc=writeSampleToFullLog();   if (rc) {Serial.println("error opening full log file in write");};
   
    for (int mn=1; mn<=loopDly;mn++) {  //for  loopDly in minutes
      for (int sec=1; sec<=60;sec++) {  //for 1mn 
        Serial.print(sec);
        Serial.print(" ");
        curRwCount=readRwSensor();  //timed 1sec
        received = getCommandLineFromSerialPort(CommandLine);      //i need to probe for a command when in wait loop
        if (received) DoMyCommand(CommandLine);
        received = getCommandLineFromBT(CommandLine);    
        if (received) DoMyCommand(CommandLine);
     }
     Serial.println(" ");
   }
 }   




int writeSampleToCurDayLog() {
  File dataFile = SD.open(curDayLogName, FILE_WRITE);  //open log file 
  getTime();
  String dataString = String(t.hour,DEC) + ',' + String(t.min,DEC) + ',' + String(t.sec,DEC) + ','; // time stamp to logged data
  dataString += readSensors();  //add sensor logged data
  printData(); //print data to serial screen
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    return 0; } else { return 99; }
}


int writeSampleToFullLog() {
  File dataFile = SD.open(fullLogName, FILE_WRITE);  //open log file 
  getTime();
  String dataString = String(t.hour,DEC) + ',' + String(t.min,DEC) + ',' + String(t.sec,DEC) + ','; // time stamp to logged data
  dataString += readSensors();  //add sensor logged data
  printData(); //print data to serial screen
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    return 0; } else { return 99; }
}

int readRwSensor() {     //read red weevill detector, time this to last 1 second since this is replacing the loop 1 sec delay
  for (int i=1;i <=1000; i++) {
    curRwSensorValue = digitalRead(rwSensorPin);
    if ((curRwSensorValue == 1) & (oldRwSensorValue == 0)) {
      rwCount++;
    }
    delay(1);
    oldRwSensorValue = curRwSensorValue;
  }
  return rwCount;
}

int writeDayToFullLog() {
  File dataFile = SD.open(fullLogName, FILE_WRITE);  //open log file 
  String dataString=String(t.year,DEC)+String(t.mon,DEC)+String(t.mday,DEC); //i need to write new day to full log
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    return 0; } else { return 99; }
}

 
void getTime(){
  DS3231_get(&t);
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,t.mon, t.mday, t.hour, t.min, t.sec);             
}

void printData(){
  Serial.print(" hour = ");           Serial.print(t.hour);           Serial.print(" min = ");            Serial.print(t.min);          Serial.print(" sec = ");            Serial.print(t.sec);
  Serial.print(" bmp280 temp = ");    Serial.print(bmp280_temp);      Serial.print(" bmp280 press = ");   Serial.print(bmp280_press);   Serial.print(" bmp280 alt = ");     Serial.print(bmp280_alt);
  Serial.print(" dht22 humidity = "); Serial.print(dht22_humidity);   Serial.print(" bh1750 light = ");   Serial.print(bh_lux);         Serial.print(" rain = ");           Serial.print(rain);
  Serial.print(" RW count= ");        Serial.print(curRwCount);       Serial.println(" ");
}


String readSensors() {

  bmp280_temp=String(float(bmp.readTemperature()));     // bmp280 temperature in Celsius 
  bmp280_press=String(long(bmp.readPressure()));        // bmp280 pressure in pascal.i need a long here, else truncated because int is 16 bits
  bmp280_alt=String(int(bmp.readAltitude(1013.25)));    // remove decimals altitude in meter

  sensors_event_t event;  
  //this measurement is not good
  /*
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature from DHT22");
  }
  else {
    dht22_temp=String(int(event.temperature)*100);      // dht22 temperature in Celsius multiply by 100 to have decimals
  }
  */
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity from DHT22");
  }
  else {
    dht22_humidity=String(float(event.relative_humidity));  // dht22 rel humidity 
  }

  lux = lightMeter.readLightLevel();
  bh_lux = String(lux);

  rain = analogRead(A7);
   
  String dataString = bmp280_temp + ',' + bmp280_press + ',' + bmp280_alt + ',' + dht22_humidity + ',' + bh_lux + ',' + rain + ',' + curRwCount ;
 
  return dataString;
}


