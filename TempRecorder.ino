#include <CurieSerialFlash.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#include <SerialCommand.h>
#include <SHT3x.h>
#include <CurieTime.h>
bool isRecording;
const char* FileName = "temp.txt";
const int FileSize = 256;
const int arduinoLED = 13;   // Arduino LED on board
SerialCommand sCmd;     // The demo SerialCommand object
SHT3x Sensor;
Thread myThread = Thread();
SerialFlashFile file;

// callback for myThread
void niceCallback(){
  if (isRecording){
    char buf[32];
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d %+03.1f %03.1f\r\n", year(),month(),day(),hour(),minute(),second(),Sensor.GetTemperature(),Sensor.GetRelHumidity());  
    file = SerialFlash.open(FileName);
    if(file){
      file.write(buf, 32);
      file.close();
      Serial.print(buf);
    }
    else{
      Serial.print("Write file");
      Serial.print("Failed.");
    }
  }
}
void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off
  isRecording = false;
  Serial.begin(19200);
  while(!Serial);
  // Setup callbacks for SerialCommand commands
  Sensor.Begin();
  Serial.println("1.Sensor Ready.");
  // Init. SPI Flash chip
  if (!SerialFlash.begin(ONBOARD_FLASH_SPI_PORT, ONBOARD_FLASH_CS_PIN)) {
    Serial.println("Unable to access SPI Flash chip.");
  }
  else{
    Serial.println("2.Flash Chip Ready.");
  }
  sCmd.addCommand("ON",    LED_on);          // Turns LED on
  sCmd.addCommand("OFF",   LED_off);         // Turns LED off
  sCmd.addCommand("TEMP", Temp);        // Echos the string argument back
  sCmd.addCommand("CLOCK", setClock);  // Converts two arguments to integers and echos them back.
  sCmd.addCommand("HELP", help);
  sCmd.addCommand("PREPAIR_FILE", prepairFile);
  sCmd.addCommand("REMOVE_FILE", removeFile);
  sCmd.addCommand("READ", readData);
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  myThread.onRun(niceCallback);
  myThread.setInterval(2000);
  Serial.println("3.Commands Handler Ready.");
  Serial.println("1-3 has been Checked, Type HELP to learn more.");
}
void loop() {
  Sensor.UpdateData();
  sCmd.readSerial();     // We don't do much, just process serial commands
  if(myThread.shouldRun())
    myThread.run();
}
void LED_on() {
  Serial.println("LED on, Start logging...");
  digitalWrite(arduinoLED, HIGH);
  isRecording = true;
}
void LED_off() {
  Serial.println("LED off, Stop logging.");
  digitalWrite(arduinoLED, LOW);
  isRecording = false;
}
void Temp() {
  char *arg;
  arg = sCmd.next();    // Get the next argument from the SerialCommand object buffer
  if (arg != NULL) {    // As long as it existed, take it
    Serial.print("Not Ready for Calibrate Temperature and Humidity: ");
    Serial.println(arg);
  }
  else {
    Serial.print("Temperature and Humidity: ");
    Serial.print(Sensor.GetTemperature());
    Serial.print("oC");
    Serial.print(Sensor.GetRelHumidity());
    Serial.println("%");
  }
}
void setClock() {
  int aNumber;
  int bNumber;
  char *arg;
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    Serial.print("First argument was: ");
    Serial.println(aNumber);
    arg = sCmd.next();
    if (arg != NULL) {
      bNumber = atol(arg);
      Serial.print("Second argument was: ");
      Serial.println(bNumber);
      switch(aNumber){
      case 1:
        setTime(hour(),minute(),second(),day(),month(),bNumber);
        break;
      case 2:
        setTime(hour(),minute(),second(),day(),bNumber,year());
        break;
      case 3:
        setTime(hour(),minute(),second(),bNumber,month(),year());  
        break;
      case 4:
        setTime(bNumber,minute(),second(),day(),month(),year());
        break;  
      case 5:
        setTime(hour(),bNumber,second(),day(),month(),year());
        break;
      case 6:
        setTime(hour(),minute(),bNumber,day(),month(),year());            
        break;      
      default:
        Serial.println("set time command OK");
      }
    }
    else {
      Serial.println("Set Time Command has not second argument");
    }   
  }
  else {    
    char buf[20];
    sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d", year(),month(),day(),hour(),minute(),second());  
    Serial.println(buf);
  }
}

void readData(){
  int aNumber;
  char *arg;
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    Serial.print("First argument was: ");
    Serial.println(aNumber);
  }
  else{
 
    Serial.println("No argument.");
  }
}

void prepairFile(){
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
  Serial.print("temp.txt file size is ");
    Serial.println(file.size());   
  file.close();
}

void removeFile(){
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
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("What?");
}

void help()
{
  Serial.println("Commands List: ON start logging. OFF stop logging. ");
  Serial.println("TEMP Corrent temperature and moisture. ");
  Serial.println("CLOCK 1/2/3/4/5/6 year/month/day/hour/minute/second to set time");
  Serial.println("PREPAIR_FILE create, check, and prepare a 'temp.txt' file.");
  Serial.println("REMOVE_FILE detect, erase and remove/delete the file");
}



