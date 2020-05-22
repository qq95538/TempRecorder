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
// callback for myThread
void niceCallback(){
    Serial.print(year());
    Serial.print(second());
    Serial.print("Temperature and Humidity: ");
    Serial.print(Sensor.GetTemperature());
    Serial.print(" ");
    Serial.print(Sensor.GetRelHumidity());
    Serial.println("%");
}
void setup() {
  pinMode(arduinoLED, OUTPUT);      // Configure the onboard LED for output
  digitalWrite(arduinoLED, LOW);    // default to LED off
  Serial.begin(19200);
  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("ON",    LED_on);          // Turns LED on
  sCmd.addCommand("OFF",   LED_off);         // Turns LED off
  sCmd.addCommand("TEMP", Temp);        // Echos the string argument back
  sCmd.addCommand("P",     processCommand);  // Converts two arguments to integers and echos them back.
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
}
void LED_off() {
  Serial.println("LED off");
  digitalWrite(arduinoLED, LOW);
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
    Serial.print(" ");
    Serial.print(Sensor.GetRelHumidity());
    Serial.println("%");
  }
}
void processCommand() {
  int aNumber;
  char *arg;
  Serial.println("We're in processCommand");
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atoi(arg);    // Converts a char string to an integer
    Serial.print("First argument was: ");
    Serial.println(aNumber);
    setTime(0,0,0,1,1,aNumber);
  }
  else {
    Serial.println("No arguments");
  }
  arg = sCmd.next();
  if (arg != NULL) {
    aNumber = atol(arg);
    Serial.print("Second argument was: ");
    Serial.println(aNumber);
  }
  else {
    Serial.println("No second argument");
  }
}
// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  Serial.println("What?");
}
