

/*
  SD card datalogger started using
  https://www.arduino.cc/en/Tutorial/Datalogger

 */


 /* 
   
 ***kernel connections
 
 SD card reader
 mega               SD card reader
 5V                 5V
 Gnd                Gnd
 53                 SDSC
 51                 MOSI
 52                 SCK
 50                 MISO

                    TOD DS3231
 5V                 5V
 Gnd                Gnd
 21                 SCL
 20                 SDA

                    Bluetooth
 5V                 5V
 Gnd                Gnd                   
 18(TX1)            RxD through divider 1.2K/2.2K for 3.3V level
 19(RX1)            TxD                

                    LCD removed
                    LCD display 5110
 3.3V               3.3V
 Gnd                Gnd
 Gnd                BL
 all signals through 5V/3.3V adapters    
 52(SCK)            SCLK
 51(MOSI)           DIN   
 49                 DC
 48                 CE/CS
 47                 RST   


 ***sensors connections
 
                    BMP280 (temperature/pressure/altitude)
 SCL(level shifter) SCL 
 SDA(level shifter) SDA
 GND                GND
 3.3V               Vcc                    
    

                    DHT22 (temperature/humidity)
                    pinout seen from top(grid side), left to right 1 to 4
 
5V                  1=Vcc
49                  2=data  connect 10K 1 to 2 
                    3=NC
GND                 4=GND        


                    BH1750 (light)

 5V                 Vcc
 GND                GND
 SCL                SCL
 SDA                SDA
 GND                ADDR
  
  */


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


#define BUFF_MAX 128
#define DHTPIN            48         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)




BH1750 lightMeter;
Adafruit_BMP280 bmp; 
DHT_Unified dht(DHTPIN, DHTTYPE);


//look at this for data logger
//https://edwardmallon.wordpress.com/2015/12/22/arduino-uno-based-data-logger-with-no-soldering/

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) -
// MOSI is LCD DIN - 
// pin 49 - Data/Command select (D/C)
// pin 48 - LCD chip select (CS)
// pin 47 - LCD reset (RST)
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!

// conflict 5110 and SD documented here: https://forum.arduino.cc/index.php?topic=53843.0
//Adafruit_PCD8544 display = Adafruit_PCD8544(49, 48, 47);





char filename[] = "00000000.xls";
const int chipSelect = 53;
char recv[BUFF_MAX];
uint8_t time[8];
struct ts t;
char buff[BUFF_MAX];
String logname,readstring,sensorData;


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
float loop_dly;


void setup() {
  int rc;
   
  // Open serial communications and wait for port to open:
  Serial.begin(9600); //console
  while (!Serial) { ; } //wait for usb ready
   
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card access failed");
    return;
  } else {
    Serial.println("card initialized.");
  }

  
  Wire.begin();
  DS3231_init(DS3231_INTCN);
  memset(recv, 0, BUFF_MAX);
  getTime();
  printTime();

  logname=String(t.year,DEC)+String(t.mon,DEC)+String(t.mday,DEC)+".xls"; //WARNING:name of file must be 12 car max
  
  Serial.print("logname : ");
  Serial.println(logname);
  
  Serial.println("Removing existing log file...");  // delete existing log with same name
  SD.remove(logname);
  
  Serial1.begin(9600); //serial interface to bluetooth

  //warning: to get proper address of bmp280, need to edit the adafruit lib header file
  if (!bmp.begin()) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
  while (1);
  }

  dht.begin();
  lightMeter.begin();
   
}

void loop() {
  int rc;
  getTime();
  String dataString = String(t.hour,DEC) + '\t' + String(t.min,DEC) + '\t' + String(t.sec,DEC) + '\t'; // time stamp to logged data
  sensorData=readSensors();
  dataString += sensorData;  //add sensor logged data
  
  printData(); //print data to serial screen
  
  logname=String(t.year,DEC)+String(t.mon,DEC)+String(t.mday,DEC)+".XLS"; //WARNING:name of file must be 12 car max //regenerate the file name in case i'm on next day
  
  File dataFile = SD.open(logname, FILE_WRITE);  //open log file

  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    Serial.println("error opening log file in write");
  }

  readConsole();
  executeCommand();

  int dly_sec=60;
  int dly_min=10;
  for (int j=0;j<= dly_min;j++) {      // dly_min
     for (int i=0;i<= dly_sec;i++) {   //1 min
       delay(1000);                    //1 sec
     }
  }  
  
 //while(1);
}



void getTime(){
  DS3231_get(&t);
  snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,
             t.mon, t.mday, t.hour, t.min, t.sec);

}

void printTime(){
  Serial.print("year = ");
  Serial.print(t.year);
  Serial.print(" month = ");
  Serial.print(t.mon);
  Serial.print(" day = ");
  Serial.println(t.mday);

  Serial.print(" day of year = ");
  Serial.println(t.yday);

  Serial.print("hour = ");
  Serial.print(t.hour);
  Serial.print(" minute = ");
  Serial.print(t.min);
  Serial.print(" second = ");
  Serial.println(t.sec);
}

void printData(){
  Serial.print(" hour = ");
  Serial.print(t.hour);
  Serial.print(" min = ");
  Serial.print(t.min);
  Serial.print(" sec = ");
  Serial.print(t.sec);
  Serial.print(" bmp280 temp x 100 = ");
  Serial.print(bmp280_temp);
  Serial.print(" bmp280 press = ");
  Serial.print(bmp280_press);
  Serial.print(" bmp280 alt = ");
  Serial.print(bmp280_alt);
  Serial.print(" dht22 temp x 100 = ");
  Serial.print(dht22_temp);
  Serial.print(" dht22 humidity = ");
  Serial.print(dht22_humidity);
  Serial.print(" bh1750 light = ");
  Serial.print(bh_lux);
  Serial.println(" ");
}

void readConsole(){
  while (Serial.available()) {
    delay(3);
    char c = Serial.read();
    readstring += c;
  }
} 

void executeCommand(){ 
  //dump SD to serial screen
  if (readstring == "dump") {       
    readstring=""; //i need to clear readstring, else i keep executing command
    File dataFile1 = SD.open(logname);
    if (dataFile1) { 
      while (dataFile1.available()) {
        Serial.println(dataFile1.read());
      }
      dataFile1.close();
    } else {
      Serial.println("error opening log file in read");
    }
  }

  // remove current log file from SD card   
  if (readstring == "clear") {
    readstring="";
    Serial.println("Removing existing log file...");   // delete existing log
    SD.remove(logname);
  }

  //dump data to bluetooth
  if (readstring == "blue") {
    readstring="";
    File dataFile1 = SD.open(logname);
    if (dataFile1) {
      Serial.println("Bluetooth xfer started");
      while (dataFile1.available()) {
        Serial1.println(dataFile1.read());   // dump data using bluetooth
        delay(100);  //i need a delay here, else bluetooth connection drops
      }
      dataFile1.close();
      Serial.println("Bluetooth xfer ended");
    } else {
      Serial.println("error opening log file in read");
    }
    delay(20);
  }

  //set time to hardcoded values
  if (readstring == "settime") {
     readstring="";
     // TssmmhhWDDMMYYYY 
     //$cmd=005922126122017;
     t.sec = 00;
     t.min = 32;
     t.hour = 16;
     //t.wday = 3;  //week day 0=sunday 
     //t.yday=61;
     t.mday = 27;
     t.mon = 12;
     t.year = 2017;
/*
     t.sec = inp2toi(cmd, 1);
     t.min = inp2toi(cmd, 3);
     t.hour = inp2toi(cmd, 5);
     t.wday = cmd[7] - 48;
     t.mday = inp2toi(cmd, 8);
     t.mon = inp2toi(cmd, 10);
     t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
*/     
     DS3231_set(t);
  }   
}

String readSensors() {

 

  bmp280_temp=String(int(bmp.readTemperature()*100));   // bmp280 temperature in Celsius multiply by 100 to have decimals
  bmp280_press=String(long(bmp.readPressure()));        // bmp280 pressure in pascal.i need a long here, else truncated because int is 16 bits
  bmp280_alt=String(int(bmp.readAltitude(1013.25)));    // remove decimals altitude in meter

  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature from DHT22");
  }
  else {
    dht22_temp=String(int(event.temperature)*100);      // dht22 temperature in Celsius multiply by 100 to have decimals
  }
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println("Error reading humidity from DHT22");
  }
  else {
    dht22_humidity=String(int(event.relative_humidity)*100);  // dht22 rel humidity multiply by 100 to have decimals
  }

  lux = lightMeter.readLightLevel();
  bh_lux = String(lux);
   
  String dataString = bmp280_temp + '\t' + bmp280_press + '\t' + bmp280_alt + '\t' + dht22_temp + '\t' + dht22_humidity + '\t' + bh_lux ;
 
 
  return dataString;
}

