#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#include <SerialCommand.h>
#include <SHT3x.h>
#include <CurieTime.h>
#define arduinoLED 13   // Arduino LED on board
SerialCommand sCmd;     // The demo SerialCommand object
SHT3x Sensor;
Thread myThread = Thread();
bool isRecording;
// callback for myThread
void niceCallback(){
  if (isRecording){
    Serial.print(year());
    Serial.print("/");
    Serial.print(month());
    Serial.print("/");
    Serial.print(day());
    Serial.print(" ");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.print(second());
    Serial.print(" ");
    Serial.print(Sensor.GetTemperature());
    Serial.print("oC");
    Serial.print("|");
    Serial.print(Sensor.GetRelHumidity());
    Serial.println("%");
  }
}
void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off
  isRecording = false;
  Serial.begin(19200);
  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("ON",    LED_on);          // Turns LED on
  sCmd.addCommand("OFF",   LED_off);         // Turns LED off
  sCmd.addCommand("TEMP", Temp);        // Echos the string argument back
  sCmd.addCommand("CLOCK", setClock);  // Converts two arguments to integers and echos them back.
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  Sensor.Begin();
  myThread.onRun(niceCallback);
  myThread.setInterval(1000);
  Serial.println("Ready");
}
void loop() {
  Sensor.UpdateData();
  sCmd.readSerial();     // We don't do much, just process serial commands
  if(myThread.shouldRun())
    myThread.run();
}
void LED_on() {
  Serial.println("LED on");
  digitalWrite(arduinoLED, HIGH);
  isRecording = true;
}
void LED_off() {
  Serial.println("LED off");
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
  Serial.println("We're setting onboard RTC clock");
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    Serial.print("First argument was: ");
    Serial.println(aNumber);
    
  }
  else {
    Serial.println("No arguments");
  }
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
      Serial.println("command");
      Serial.print(aNumber);
    }
  }
  else {
    Serial.println("No second argument");
  }
}
// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("What?");
}
