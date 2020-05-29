#include <inttypes.h >
#include <EEPROM.h>
#include <CurieSerialFlash.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#include <SerialCommand.h>
#include <SHT3x.h>
#include <CurieTime.h>
const char* FileName = "temp.txt";
const int FileSize = 65536;
const int arduinoLED = 13;   // Arduino LED on board
const int write_pointer_EEPROM_address_a = 128;
const int write_pointer_EEPROM_address_b = 129;
const int write_pointer_EEPROM_address_c = 130;
const int write_pointer_EEPROM_address_d = 131;
const int period_EEPROM_address = 132;
const int error_code_EEPROM_address = 133; 
//error_code File_Save_Error_in_recording_thread 1
//prepare_file error 2
SerialCommand sCmd;     // The demo SerialCommand object
SHT3x Sensor;
Thread myThread = Thread();
SerialFlashFile file;
union {
  struct {
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
  };
  unsigned long ul;
} write_pointer; 
unsigned long read_sector;
int period;
bool isRecording;
short int error_code;

// callback for myThread
void niceCallback(){
  if (isRecording && (second()%period == 0)){
    char buf[32];
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d %+03.1f %03.1f\r\n", year(),month(),day(),hour(),minute(),second(),Sensor.GetTemperature(),Sensor.GetRelHumidity());
    digitalWrite(arduinoLED, HIGH);
    file = SerialFlash.open(FileName);
    if(file){      
      file.seek(write_pointer.ul*32);       
      file.write(buf, 32);
      file.close();
      write_pointer.ul++;
      Serial.print("write_point->");
      Serial.print(write_pointer.ul);
      Serial.print("|current_positon->");
      Serial.print(file.position());
      Serial.print(":");
      Serial.print(buf);
    }
    else{
      Serial.println("Write file temp.txt Failed. Stop Recording...");
      isRecording = false;
      error_code = 1;
      EEPROM.write(error_code_EEPROM_address, error_code); //error_code File_Save_Error_in_recording_thread 1
      EEPROM.write(write_pointer_EEPROM_address_a, write_pointer.a);
      EEPROM.write(write_pointer_EEPROM_address_b, write_pointer.b);
      EEPROM.write(write_pointer_EEPROM_address_c, write_pointer.c);
      EEPROM.write(write_pointer_EEPROM_address_d, write_pointer.d);
      Serial.println("error_code and write_point has been saved.");
    }
    digitalWrite(arduinoLED, LOW);
  }
}
void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off
  isRecording = false;
  write_pointer.a = EEPROM.read(write_pointer_EEPROM_address_a);
  write_pointer.b = EEPROM.read(write_pointer_EEPROM_address_b);
  write_pointer.c = EEPROM.read(write_pointer_EEPROM_address_c);
  write_pointer.d = EEPROM.read(write_pointer_EEPROM_address_d);
  period = EEPROM.read(period_EEPROM_address);
  error_code = EEPROM.read(error_code_EEPROM_address);
  read_sector = 0;
  Serial.begin(19200);
  while(!Serial);
  Sensor.Begin();
  Serial.println("Sensor Ready.");
  Serial.print("write_pointer->:");
  Serial.println(write_pointer.ul); 
  Serial.print("error_code->:");
  Serial.println(error_code);
  Serial.print("period->:");
  Serial.println(period);  
  // Init. SPI Flash chip
  if (!SerialFlash.begin(ONBOARD_FLASH_SPI_PORT, ONBOARD_FLASH_CS_PIN)) {
    Serial.println("Unable to access SPI Flash chip.");
  }
  else{
    Serial.println("Flash Chip Ready.");
  }
  sCmd.addCommand("REC",    LOG_on);          // Turns LED on
  sCmd.addCommand("STOP",   LOG_off);         // Turns LED off
  sCmd.addCommand("TEMP", Temp);        // Echos the string argument back
  sCmd.addCommand("POINT", query_write_pointer); 
  sCmd.addCommand("CLOCK", setClock);  // Converts two arguments to integers and echos them back.
  sCmd.addCommand("HELP", help);
  sCmd.addCommand("NEW", prepairFile);
  sCmd.addCommand("DEL", removeFile);
  sCmd.addCommand("READ", readData);
  sCmd.addCommand("PERIOD", changePeriod);
  sCmd.addCommand("FORMAT", formatSD);
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  myThread.onRun(niceCallback);
  myThread.setInterval(1000); 
  Serial.println("Commands Handler Ready.");
  Serial.println("Type READ to check logged data.");
  Serial.println("Type DEL, NEW and REC consequently to begin a new log.");
  Serial.println("Type Type STOP to finish logging and save the write_point.");
  Serial.println("Type CLOCK to check time.");
  Serial.println("Type HELP to learn more.");
   
}
void loop() {
  Sensor.UpdateData();
  sCmd.readSerial();     // We don't do much, just process serial commands
  if(myThread.shouldRun())
    myThread.run();
}
void LOG_on() {
  isRecording = true;
  if(period == 0){
    Serial.println("The interval log time is 0, logging cycle is not actually started.");
    isRecording = false;
    Serial.println("try command INTERVAL 2 to set every logging cycle per 2 seconds. and try ON again.");    
  }
  else{
    Serial.println("Start logging...");
  }
}
void LOG_off() {
  Serial.println("The write_pointer is now saving at:");
  isRecording = false;
  digitalWrite(arduinoLED, HIGH);
  EEPROM.write(write_pointer_EEPROM_address_a, write_pointer.a);
  EEPROM.write(write_pointer_EEPROM_address_b, write_pointer.b);
  EEPROM.write(write_pointer_EEPROM_address_c, write_pointer.c);
  EEPROM.write(write_pointer_EEPROM_address_d, write_pointer.d);
  digitalWrite(arduinoLED, LOW);
  Serial.println(write_pointer.ul);
  Serial.println("saved write_pointer, Stopped logging.");
}
void Temp() {
  char *arg;
  arg = sCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it existed, take it
    Serial.print("Not Ready for Calibrate Temperature and Humidity: ");
    Serial.println(arg);
  }
  else {
    Serial.println("Temperature and Humidity: ");
    Serial.print(Sensor.GetTemperature());
    Serial.print(" ");
    Serial.println(Sensor.GetRelHumidity());
  }
}
void setClock() {
  int iArg[6] = {};
  int i = 0;
  char *arg;
  char buf[20];
  arg = sCmd.next();
  if(arg != NULL){
    do{
        if(arg != NULL) iArg[i] = atoi(arg);
        arg = sCmd.next();
        i++;
    }while(arg != NULL && i < 6);
    //setTime(hour,minute,second,day,month,year);
    setTime(iArg[3],iArg[4],iArg[5], iArg[2],iArg[1],iArg[0]); 
    Serial.print("Time is set to:");
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", year(),month(),day(),hour(),minute(),second());
    Serial.println(buf);
  }
  else {    
    Serial.print("Time now on RTC clock is:");
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", year(),month(),day(),hour(),minute(),second());
    Serial.println(buf);
  }
}

void readData(){
  int aNumber;
  char *arg;
  const int buffer_length = 256;
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    read_sector = aNumber; 
    Serial.print("The read sector has moved to-> ");
    Serial.println(aNumber);
    Serial.print("Its portion is at-> ");
    Serial.println(read_sector*buffer_length); 
  }
  else{
    char file_buffer[buffer_length];
    digitalWrite(arduinoLED, HIGH);
    if (!SerialFlash.exists(FileName)) {
      Serial.println("No file " + String(FileName));
    }
    else{
      Serial.println("File " + String(FileName) + " has already exists");
      file = SerialFlash.open(FileName);
      Serial.print("temp.txt file size is ");
      Serial.println(file.size()); 
      Serial.print("sector->");
      Serial.print(read_sector);
      Serial.print(" write_pointer->");
      Serial.print(read_sector*buffer_length/32);
      Serial.print(" current_position->");
      Serial.print(read_sector*buffer_length);
      Serial.println(":");
      file.seek(read_sector*buffer_length);
      file.read(file_buffer, buffer_length);  
      file.close();
      Serial.println(file_buffer);
      read_sector++;     
    }
    digitalWrite(arduinoLED, LOW);  
  }
}

void prepairFile(){
  digitalWrite(arduinoLED, HIGH);
  //Create 'temp.txt' if it is not exist.
  if (!SerialFlash.exists(FileName)) {
    Serial.println("Creating file " + String(FileName));
    if(SerialFlash.createErasable(FileName, FileSize)) 
      Serial.println("temp.txt is successfully created");
  }
  else{
    Serial.println("File " + String(FileName) + " has already exists");
  }
  file = SerialFlash.open(FileName);
  if(file){
    Serial.print("temp.txt file size is ");
    Serial.println(file.size());   
    file.close();
  }
  else{
    error_code = 2; //prepare_file error 2
    EEPROM.write(error_code_EEPROM_address, error_code);
  }
  write_pointer.ul = 0;
  read_sector = 0;
  EEPROM.write(write_pointer_EEPROM_address_a, write_pointer.a);
  EEPROM.write(write_pointer_EEPROM_address_b, write_pointer.b);
  EEPROM.write(write_pointer_EEPROM_address_c, write_pointer.c);
  EEPROM.write(write_pointer_EEPROM_address_d, write_pointer.d);
  digitalWrite(arduinoLED, LOW);
}

void removeFile(){
  digitalWrite(arduinoLED, HIGH);
  if (!SerialFlash.exists(FileName)) {
    Serial.println("temp.txt is not existed.");
  }
  else{
    file = SerialFlash.open(FileName);
    file.erase();
    file.close();
    SerialFlash.remove(FileName);
    Serial.println("temp.txt is deleted.");
  }
  write_pointer.ul = 0;
  read_sector = 0;
  EEPROM.write(write_pointer_EEPROM_address_a, write_pointer.a);
  EEPROM.write(write_pointer_EEPROM_address_b, write_pointer.b);
  EEPROM.write(write_pointer_EEPROM_address_c, write_pointer.c);
  EEPROM.write(write_pointer_EEPROM_address_d, write_pointer.d);
  error_code = 0; //repair error by delete file
  EEPROM.write(error_code_EEPROM_address, error_code);
  digitalWrite(arduinoLED, LOW);
}

void changePeriod(){
  int aNumber;
  char *arg;
  arg = sCmd.next();
  digitalWrite(arduinoLED, HIGH);
  if (arg != NULL) {    // As long as it existed, take it
    aNumber = atoi(arg);
    Serial.print("Set the new log interval time to (seconds): ");
    Serial.println(aNumber);
    period = aNumber;
    EEPROM.write(period_EEPROM_address, period);
    
  }
  else {
    Serial.print("Current log period time(seconds): ");
    period = EEPROM.read(period_EEPROM_address);
    Serial.println(period);
  }
  digitalWrite(arduinoLED, LOW);
}

void formatSD(){
  Serial.println("wait, 30 seconds to 2 minutes for most chips...");
  digitalWrite(arduinoLED, HIGH);
  SerialFlash.eraseAll();
  while (SerialFlash.ready() == false) {
   // wait, 30 seconds to 2 minutes for most chips
  }
  error_code = 0; //repair error by format
  EEPROM.write(error_code_EEPROM_address, error_code);
  digitalWrite(arduinoLED, LOW);
  Serial.println("ok.");

}

void query_write_pointer(){
  Serial.println("XXXX(Data Logged)XXXX.__(Empty Space)___");
  Serial.println("                     ^ ");
  Serial.print("The current write_pointer is:");
  Serial.println(write_pointer.ul);
}


// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("Unknown command. Type HELP to study right ones.");
}

void help()
{
  Serial.println("Commands List: REC start logging. STOP stop logging. ");
  Serial.println("TEMP Corrent temperature and moisture. ");
  Serial.println("POINT Query current write_pointer.");
  Serial.println("CLOCK 1/2/3/4/5/6 year/month/day/hour/minute/second to set time");
  Serial.println("NEW create a new 'temp.txt' file.");
  Serial.println("DEL erase current file 'temp.txt' ");
  Serial.println("FORMAT your sd_card if you failed logging data after you tried DEL,NEW,REC commands sequence."); 
  Serial.println("READ read a section of logged temp and moisture data, and move the portion to next section.");
  Serial.println("READ (parameter1) move the portion to the defined parameter1. Example READ 0, READ 1, READ 2."); 
  Serial.println("PERIOD (parameter1), change log interval time (second) every logging period."); 
}



