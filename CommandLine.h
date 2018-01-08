

//found here:https://create.arduino.cc/projecthub/mikefarr/simple-command-line-interface-4f0a3f



/*****************************************************************************

  How to Use CommandLine:
    Create a sketch.  Look below for a sample setup and main loop code and copy and paste it in into the new sketch.

   Create a new tab.  (Use the drop down menu (little triangle) on the far right of the Arduino Editor.
   Name the tab CommandLine.h
   Paste this file into it.

  Test:
     Download the sketch you just created to your Arduino as usual and open the Serial Window.  Typey these commands followed by return:
      add 5, 10
      subtract 10, 5

    Look at the add and subtract commands included and then write your own!


*****************************************************************************
  Here's what's going on under the covers
*****************************************************************************
  Simple and Clear Command Line Interpreter

     This file will allow you to type commands into the Serial Window like,
        add 23,599
        blink 5
        playSong Yesterday

     to your sketch running on the Arduino and execute them.

     Implementation note:  This will use C strings as opposed to String Objects based on the assumption that if you need a commandLine interpreter,
     you are probably short on space too and the String object tends to be space inefficient.

   1)  Simple Protocol
         Commands are words and numbers either space or comma spearated
         The first word is the command, each additional word is an argument
         "\n" terminates each command

   2)  Using the C library routine strtok:
       A command is a word separated by spaces or commas.  A word separated by certain characters (like space or comma) is called a token.
       To get tokens one by one, I use the C lib routing strtok (part of C stdlib.h see below how to include it).
           It's part of C language library <string.h> which you can look up online.  Basically you:
              1) pass it a string (and the delimeters you use, i.e. space and comman) and it will return the first token from the string
              2) on subsequent calls, pass it NULL (instead of the string ptr) and it will continue where it left off with the initial string.
        I've written a couple of basic helper routines:
            readNumber: uses strtok and atoi (atoi: ascii to int, again part of C stdlib.h) to return an integer.
              Note that atoi returns an int and if you are using 1 byte ints like uint8_t you'll have to get the lowByte().
            readWord: returns a ptr to a text word

   4)  DoMyCommand: A list of if-then-elses for each command.  You could make this a case statement if all commands were a single char.
      Using a word is more readable.
          For the purposes of this example we have:
              Add
              Subtract
              nullCommand
*/
/******************sample main loop code ************************************

  #include "CommandLine.h"

  void
  setup() {
  Serial.begin(115200);
  }

  void
  loop() {
  bool received = getCommandLineFromSerialPort(CommandLine);      //global CommandLine is defined in CommandLine.h
  if (received) DoMyCommand(CommandLine);
  }

**********************************************************************************/



//Name this tab: CommandLine.h

#include <string.h>
#include <stdlib.h>

//this following macro is good for debugging, e.g.  print2("myVar= ", myVar);
#define print2(x,y) (Serial.print(x), Serial.println(y))


#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

#define COMMAND_BUFFER_LENGTH        25                        //length of serial buffer for incoming commands
char   CommandLine[COMMAND_BUFFER_LENGTH + 1];                 //Read commands into this buffer from Serial.  +1 in length for a termination char

const char *delimiters            = ", \n";                    //commands can be separated by return, space or comma


//***********   add the new commands names here

const char *addCommandToken                 = "add";                    
const char *subtractCommandToken            = "sub";                     
const char *listLogCommandToken             = "listLog";                  // listLog                  (lists all SD card files and subdirs)
const char *deleteLogCommandToken           = "deleteLog";                // deleteLog                (deletes all files in the root of SD card)
const char *startLogCommandToken            = "startLog";                 // startLog                 (start loging)
const char *stopLogCommandToken             = "stopLog";                  // stopLog                  (stops logging°
const char *dumpLogSerialCommandToken       = "dumpLogSerial";            // dumpLogSerial <curDayLogName.csv>    (dumps log file to serial screen)
const char *dumpLogBTCommandToken           = "dumpLogBT";                // dumpLogSerial <curDayLogName.csv>    (dumps log file to bluetooth)
const char *dumpFullLogSerialCommandToken   = "dumpFullLogSerial";        // dumpFullLogSerial        (dumps log file to serial screen)
const char *dumpFullLogBTCommandToken       = "dumpFullLogBT";            // dumpFullLogSerial        (dumps log file to bluetooth)
const char *setLoopDlyCommandToken          = "setLoopDly";               // setLoopDly <dly in mn>   (set delay between samples)
const char *setTimeCommandToken             = "setTime";                  // setTime T355720619112011  T<sec(2)><mn(2)><hour(2)><dayofweek(1)><day(2)><month(2)><year(4)>     (set logger clock)
const char *dispTimeCommandToken            = "dispTime";                 // dispTime                 (display logger clock)



void printTime(){
  Serial.print("year = ");            Serial.print(t.year);           Serial.print(" month = ");          Serial.print(t.mon);          Serial.print(" day = ");            Serial.print(t.mday);
  Serial.print(" day of week = ");    Serial.println(t.wday);         Serial.print("hour = ");            Serial.print(t.hour);         Serial.print(" minute = ");         Serial.print(t.min);      
  Serial.print(" second = ");         Serial.println(t.sec);
}



/*************************************************************************************************************
    getCommandLineFromSerialPort()
      Return the string of the next command. Commands are delimited by return"
      Handle BackSpace character
      Make all chars lowercase
*************************************************************************************************************/

//commands from serial monitor
bool
getCommandLineFromSerialPort(char * commandLine)
{
  static uint8_t charsRead = 0;                      //note: COMAND_BUFFER_LENGTH must be less than 255 chars long
  //read asynchronously until full command input
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case CR:      //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        commandLine[charsRead] = NULLCHAR;       //null terminate our command char array
        if (charsRead > 0)  {
          charsRead = 0;                           //charsRead is static, so have to reset
          Serial.println(commandLine);
          return true;
        }
        break;
      case BS:                                    // handle backspace in input: put a space in last char
        if (charsRead > 0) {                        //and adjust commandLine and charsRead
          commandLine[--charsRead] = NULLCHAR;
          Serial << byte(BS) << byte(SPACE) << byte(BS);  //no idea how this works, found it on the Internet
        }
        break;
      default:
        // c = tolower(c);
        if (charsRead < COMMAND_BUFFER_LENGTH) {
          commandLine[charsRead++] = c;
        }
        commandLine[charsRead] = NULLCHAR;     //just in case
        break;
    }
  }
  return false;
}

//commands from bluetooth (BT)
bool
getCommandLineFromBT(char * commandLine)
{
  static uint8_t charsRead = 0;                      //note: COMAND_BUFFER_LENGTH must be less than 255 chars long
  //read asynchronously until full command input
  while (Serial1.available()) {
    char c = Serial1.read();
    switch (c) {
      case CR:      //likely have full command in buffer now, commands are terminated by CR and/or LS
      case LF:
        commandLine[charsRead] = NULLCHAR;       //null terminate our command char array
        if (charsRead > 0)  {
          charsRead = 0;                           //charsRead is static, so have to reset
          Serial.println(commandLine);             //echo command to serial port, not BT
          return true;
        }
        break;
      case BS:                                    // handle backspace in input: put a space in last char
        if (charsRead > 0) {                        //and adjust commandLine and charsRead
          commandLine[--charsRead] = NULLCHAR;
          Serial1 << byte(BS) << byte(SPACE) << byte(BS);  //no idea how this works, found it on the Internet
        }
        break;
      default:
        // c = tolower(c);
        if (charsRead < COMMAND_BUFFER_LENGTH) {
          commandLine[charsRead++] = c;
        }
        commandLine[charsRead] = NULLCHAR;     //just in case
        break;
    }
  }
  return false;
}


/* ****************************
   readNumber: return a 16bit (for Arduino Uno) signed integer from the command line
   readWord: get a text word from the command line

*/
int
readNumber () {
  char * numTextPtr = strtok(NULL, delimiters);         
  return atoi(numTextPtr);                            
}

char * readWord() {
  char * word = strtok(NULL, delimiters);              
  return word;
}

void
nullCommand(char * ptrToCommandName) {
  print2("Command not found: ", ptrToCommandName);      //see above for macro print2
}




//***********   add the new commands parms here

int addCommand() {                                     
  int firstOperand = readNumber();
  int secondOperand = readNumber();
  return firstOperand + secondOperand;
}

int subtractCommand() {                               
  int firstOperand = readNumber();
  int secondOperand = readNumber();
  return firstOperand - secondOperand;
}

void listLogCommand() {                               
  return;
}

void deleteLogCommand() {                             
  return;
}

bool startLogCommand() {                             
  return true;
}

bool stopLogCommand() {                             
  return false;
}

char * dumpLogSerialCommand() {    
  char * dumpcurDayLogName = readWord();                         
  return dumpcurDayLogName;
}

char * dumpLogBTCommand() {    
  char * dumpcurDayLogName = readWord();                         
  return dumpcurDayLogName;
}

void dumpFullLogSerialCommand() {                           
  return ;
}

void dumpFullLogBTCommand() {                         
  return ;
}

int setLoopDlyCommand() {                               
  int loopDlyLocal = readNumber();
  return loopDlyLocal;
}

char * setTimeCommand() {    
  char * setTimeValue = readWord();                         
  return setTimeValue;
}

void dispTimeCommand() {                             
  return;
}

/****************************************************
   DoMyCommand
*/
bool
DoMyCommand(char * commandLine) {
  //  print2("\nCommand: ", commandLine);
  int result;

  char * ptrToCommandName = strtok(commandLine, delimiters);
  bool validCmd = false;
  //  print2("commandName= ", ptrToCommandName);



  //***********   add the new commands here

    if (strcmp(ptrToCommandName, addCommandToken) == 0) {                  
      result = addCommand();
      print2(">    The sum is = ", result);
      validCmd = true;
    } 
  
    if (strcmp(ptrToCommandName, subtractCommandToken) == 0) {           
      result = subtractCommand();                                        
      print2(">    The difference is = ", result);
      validCmd = true;
    }


    if (strcmp(ptrToCommandName, listLogCommandToken) == 0) {          
      //warning:this command list all SD, including sub dirs. file names format used by SD lib is 8.3 (like toto.txt, name like this _is_long_file_name.txt look stranger)
      if (!volume.init(card)) {
        Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
      }
      Serial.println("\nFiles found on the card (name, date and size in bytes): ");
      root.openRoot(volume);
      root.ls(LS_R | LS_DATE | LS_SIZE);
      validCmd = true;
    }  


    if (strcmp(ptrToCommandName, deleteLogCommandToken) == 0) {     
      //warning: this command intentionnaly remove the files from root, but skips the sub directories
      //SD.begin(chipSelect);   //53 
      File root;
      root = SD.open("/");
      while (true) {
        File entry =  root.openNextFile();
        Serial.print(entry.name());    
        if (SD.remove(entry.name())) {
          Serial.println(" deleted"); 
        } else {
          Serial.println(" failed deletion: this command leaves subdirs unaffected");      
        }
        if (! entry) { break; }     //no more files
        entry.close();
      }    
      validCmd = true;
    }  

    if (strcmp(ptrToCommandName, startLogCommandToken) == 0) {           
      runLogger = startLogCommand();                                        
      validCmd = true;
    }

    if (strcmp(ptrToCommandName, stopLogCommandToken) == 0) {           
      runLogger = stopLogCommand();                                        
      validCmd = true;
    }

    if (strcmp(ptrToCommandName, dumpLogSerialCommandToken) == 0) { 
      validCmd = true;
      char c ; 
      int i;
      char * dumpcurDayLogName = dumpLogSerialCommand();   
      File dataFile = SD.open(dumpcurDayLogName);
      if (dataFile) { 
        while (dataFile.available()) {
          c=dataFile.read();
          if ( c == '\0' ) {
            Serial.println(" ");
          } else {
            Serial.print(c);
          }  
        }
        dataFile.close();
      } else {
        Serial.println("error opening log file in read");
      }      
   }


   if (strcmp(ptrToCommandName, dumpLogBTCommandToken) == 0) { 
      validCmd = true;
      char c ; 
      char read_char;
      int i;
      char * dumpcurDayLogName = dumpLogBTCommand();  
      File dataFile = SD.open(dumpcurDayLogName);
      if (dataFile) {
        Serial.println("Bluetooth xfer started");
        while (dataFile.available()) {
          read_char=char(dataFile.read());
          Serial1.print(read_char);   // serial1 is interface to bluetooth xmt/rcv
          //delay(5);  //i need a delay here, else bluetooth connection drops ?
        }
        dataFile.close();
        Serial.println("Bluetooth xfer ended");
      } else {
        Serial.println("error opening log file in read");
      }
      delay(20);
  }


   if (strcmp(ptrToCommandName, dumpFullLogSerialCommandToken) == 0) { 
      validCmd = true;
      char c ; 
      File dataFile = SD.open(fullLogName);
      if (dataFile) { 
        while (dataFile.available()) {
          c=dataFile.read();
          if ( c == '\0' ) {
            Serial.println(" ");
          } else {
            Serial.print(c);
          }  
        }
        dataFile.close();
      } else {
        Serial.println("error opening full log file in read");
      }      
   }


   if (strcmp(ptrToCommandName, dumpFullLogBTCommandToken) == 0) { 
      validCmd = true;
      char read_char;  
      File dataFile = SD.open(fullLogName);
      if (dataFile) {
        Serial.println("Bluetooth xfer started");
        while (dataFile.available()) {
          read_char=char(dataFile.read());
          Serial1.print(read_char);   // serial1 is interface to bluetooth xmt/rcv
          //delay(5);  //i need a delay here, else bluetooth connection drops ?
        }
        dataFile.close();
        Serial.println("Bluetooth xfer ended");
      } else {
        Serial.println("error opening log file in read");
      }
      delay(20);
  }




  if (strcmp(ptrToCommandName, setLoopDlyCommandToken) == 0) { 
     validCmd = true;
     loopDly = setLoopDlyCommand();
  }   


  if (strcmp(ptrToCommandName, setTimeCommandToken) == 0) { 
      validCmd = true;

      // TssmmhhWDDMMYYYY 
      //timeValue=059221206122017
      char * timeValue = setTimeCommand();
      //char * timeValue = "T005911107012017";
      t.sec = inp2toi(timeValue, 1);
      t.min = inp2toi(timeValue, 3);
      t.hour = inp2toi(timeValue, 5);
      t.wday = timeValue[7] - 48;
      t.mday = inp2toi(timeValue, 8);
      t.mon = inp2toi(timeValue, 10);
      t.year = inp2toi(timeValue, 12) * 100 + inp2toi(timeValue, 14);
      DS3231_set(t);
      printTime();   
  }

  if (strcmp(ptrToCommandName, dispTimeCommandToken) == 0) {  
      validCmd = true;
      printTime();                             
  }

  if (validCmd == false) { nullCommand(ptrToCommandName);};
  
}



