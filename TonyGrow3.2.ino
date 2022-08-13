/*
Tony's Grow

    2022-02-27:
        v3.0  - Moved to tony2.0test, iterating with hardware, copying from v1 and v2 as needed
    2022-03-13:
        v3.1 - Added RTC and SD Card functionality
    2022-08-13
        v3.2 - Moved to GitHub 
*/
// ==== DEFINES AND INCLUDES ===================================================================================
#include <TaskScheduler.h>

//Soil Moisture Sensors
const int AirValue = 520;   //you need to replace this value with Value_1
const int WaterValue = 260;  //you need to replace this value with Value_2
int intervals = (AirValue - WaterValue)/3;
int soilMoistureValue = 0;
#define soilMoist1 A1
#define soilMoist2 A2
#define soilMoist3 A3


//Humidity and Temperature Sensor Init
#include <DHT.h>
#include <DHT_U.h>
#define DHTpinAmbient 7 //Ambient Data is connected to Pin 7
float AmbientHum;
float AmbientTempC;
float AmbientTempF;
#define DHTpinLights 37 //Lights Height Hum Temp sensor
float lightsHum;
float lightsTempF;
float lightsTempC;
#define DHTpinMid 35 //Mid height Hum Temp sensor
float midHum;
float midTempF;
float midTempC;
#define DHTpinSoil 33 //Soil level Hum Temp sensor
float soilHum;
float soilTempF;
float soilTempC;
float averageHum;
float averageTempF;
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
/*  DHT dht[] = {
    {DHTpinAmbient, DHTTYPE},
    {DHTpinLights, DHTTYPE},
    {DHTpinMid, DHTTYPE},
    {DHTpinSoil, DHTTYPE},
  };
*/
    DHT dhtAmbient(DHTpinAmbient, DHTTYPE);
    DHT dhtLights(DHTpinLights, DHTTYPE);
    DHT dhtMid(DHTpinMid, DHTTYPE);
    DHT dhtSoil(DHTpinSoil, DHTTYPE);

//Relay Box 1
#define lights      22
#define foliageFans 24
#define exhaustFans 26
#define RB1OUT4     28

//Relay Box 2
#define heater      23
#define co2Dispense 25
#define waterPump   27
#define RB2OUT4     29

//Liquid Crystal Display
#include <Wire.h> //I2C communication with the LCD Display
#include <LiquidCrystal_I2C.h>  //SDA = 20, SCL =21 on Arduino Mega
LiquidCrystal_I2C lcd(0x27, 20, 3); //Starts at 0, which means the 3 siginfies a 4 row display

//RotaryEncoder
#include <Arduino.h>
#include <Encoder.h>
const uint8_t pinA = 8;
const uint8_t pinB = 3;
#define RotarySwitch 36 //Rotary switch is attached to pin 36 
Encoder myEnc(pinA, pinB);

//Menu
int menuCategory = 0;
int menuOption = 0;

//Environmental Thresholds
float tempUpperLimit = 84;
float tempLowerLimit = 64;
float humUpperLimit = 92;
float humLowerLimit = 20;

//Real Time Clock (RTC)
#include <ThreeWire.h>  
#include <RtcDS1302.h>
ThreeWire myWire(6,5,2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
String timeNow;
#define countof(abc) (sizeof(abc) / sizeof(abc[0]))

//Micro SD Card
#include <SPI.h>
#include <SD.h>
const int chipSelect = 4;

// ==== Debug and Test options ===========================================
#define _DEBUG_
//#define _TEST_


//===== Debugging macros =================================================

#ifdef _DEBUG_
#define SerialD Serial
#define _PM(a) SerialD.print(millis()); SerialD.print(": "); SerialD.println(a)
#define _PP(a) SerialD.print(a)
#define _PL(a) SerialD.println(a)
#define _PX(a) SerialD.println(a, HEX)
#else
#define _PM(a)
#define _PP(a)
#define _PL(a)
#define _PX(a)
#endif


// ==== Uncomment desired compile options =================================
 #define _TASK_SLEEP_ON_IDLE_RUN    // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
 #define _TASK_TIMECRITICAL         // Enable monitoring scheduling overruns
// #define _TASK_STATUS_REQUEST     // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
// #define _TASK_WDT_IDS            // Compile with support for wdt control points and task ids
// #define _TASK_LTS_POINTER        // Compile with support for local task storage pointer
// #define _TASK_PRIORITY           // Support for layered scheduling priority
// #define _TASK_MICRO_RES          // Support for microsecond resolution
// #define _TASK_STD_FUNCTION       // Support for std::function (ESP8266 and ESP32 ONLY)
// #define _TASK_DEBUG              // Make all methods and variables public for debug purposes
// #define _TASK_INLINE             // Make all methods "inline" - needed to support some multi-tab, multi-file implementations
// #define _TASK_TIMEOUT            // Support for overall task timeout
// #define _TASK_OO_CALLBACKS       // Support for dynamic callback method binding
// #define _TASK_DEFINE_MILLIS      // Force forward declaration of millis() and micros() "C" style
// #define _TASK_EXPOSE_CHAIN       // Methods to access tasks in the task chain
// #define _TASK_SCHEDULING_OPTIONS // Support for multiple scheduling options


// ==== Scheduling defines (cheat sheet) =====================
/*
  TASK_MILLISECOND
  TASK_SECOND
  TASK_MINUTE
  TASK_HOUR
  TASK_IMMEDIATE
  TASK_FOREVER
  TASK_ONCE
  TASK_NOTIMEOUT
  TASK_SCHEDULE     - schedule is a priority, with "catch up" (default)
  TASK_SCHEDULE_NC  - schedule is a priority, without "catch up"
  TASK_INTERVAL     - interval is a priority, without "catch up"
*/

// ==== Scheduler ==============================
Scheduler ts;

//Task periods
int savePeriod =      20;    //in seconds
int lightsPeriodOn  =   22;    //in hours
int lightsPeriodOff =    2;    //in hours
int foliagePeriodOn =    3;    //in hours
int foliagePeriodOff =   1;    //in hours
int exhaustPeriodOn =    5;    //in minutes
int exhaustPeriodOff =  20;    //in minutes
int waterPeriodOn =     10;    //in seconds
int waterPeriodOff =   100;    //in seconds 
int resevoirPeriod =     1;    //in hours
//datetime? vegToBloom (either period or date...need to compare to RTC 
int cO2Period =          5;    //in minutes
int vegDays =            42;   //in days
int flowerDays =         56;   //in days

//Forward declarations of functions called by tasks
void lcdTest();
bool LEDToggle();
bool lightsToggle();
bool foliageToggle();
bool exhaustToggle();
bool readSensors();
bool plantWaterToggle();
bool resevoirFill();
//bool vegToBloom();
bool cO2Inject();
bool saveToSD();
void readDateTime();

// ==== Task definitions ========================
//Task LEDToggler     (1 * TASK_SECOND, TASK_FOREVER, &LEDToggle, &ts, true);
Task saveToSDTask     (savePeriod * TASK_SECOND, TASK_FOREVER, &readSensors, &ts, true);
Task lightsToggler    (lightsPeriodOn * TASK_HOUR, TASK_FOREVER, &lightsToggle, &ts, true); //will adjust parameters within lightsToggle by lightsPeriodOn and lightsPeriodOff
Task foliageToggler   (foliagePeriodOn * TASK_HOUR, TASK_FOREVER, &foliageToggle, &ts, true);
Task exhaustToggler   (exhaustPeriodOn * TASK_MINUTE, TASK_FOREVER, &exhaustToggle, &ts, true);
Task plantWaterToggler(waterPeriodOn * TASK_SECOND, TASK_FOREVER, &plantWaterToggle, &ts, true);
Task resevoirFiller   (resevoirPeriod * TASK_HOUR, TASK_FOREVER, &resevoirFill, &ts, true);
//Task vegToBloomCheck(                                                                  );
Task cO2Toggler       (cO2Period * TASK_MINUTE, TASK_FOREVER, &cO2Inject, &ts, true);


// ==== CODE ======================================================================================
void setup() {

 //DEBUGGING TEST BS
#if defined(_DEBUG_) || defined(_TEST_)
  Serial.begin(9600);
  pinMode(13,OUTPUT); //onboard LED to blink for testing
  delay(1000);
  _PL("Scheduler Template: setup()");
#endif


  //Set all Relay Box outputs to OFF
  pinMode(heater, OUTPUT);
  pinMode(co2Dispense, OUTPUT);
  pinMode(waterPump, OUTPUT);
  pinMode(RB2OUT4, OUTPUT);
  pinMode(lights, OUTPUT);
  pinMode(foliageFans, OUTPUT);
  pinMode(exhaustFans, OUTPUT);
  pinMode(RB1OUT4, OUTPUT);
  digitalWrite(lights, HIGH);
  digitalWrite(foliageFans, HIGH);
  digitalWrite(exhaustFans, HIGH);
  digitalWrite(heater, HIGH);
  digitalWrite(co2Dispense, HIGH);
  digitalWrite(waterPump, HIGH);
  digitalWrite(RB1OUT4, HIGH);
  digitalWrite(RB2OUT4, HIGH);

  //Start Hum Temp Sensor service
  dhtAmbient.begin();
  dhtLights.begin();
  dhtMid.begin();
  dhtSoil.begin();

  //LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcdTest();

  //Rotary Encoder
  pinMode(RotarySwitch, INPUT_PULLUP);

  //Real Time Clock (RTC)
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }  
    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }
    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

//Micro SD Card

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  
//Setup Wizard asks for User input 
//  setupWizard();

}


void loop() {
  ts.execute();
}
 


bool LEDToggle(){
  digitalWrite(13, !digitalRead(13));
  return true;
}


bool lightsToggle(){
  if(digitalRead(lights) == HIGH){//HIGH is off in this case, a LOW signal powers the outlet wirh 120V
    lightsToggler.setInterval(lightsPeriodOn * TASK_HOUR); //Lights time on
  }
  else {
    lightsToggler.setInterval(lightsPeriodOff * TASK_HOUR); //Lights time off
  }
    digitalWrite(lights, !digitalRead(lights));
    return true;
}

bool foliageToggle(){
   if(digitalRead(foliageFans) == HIGH){//HIGH is off in this case, a LOW signal powers the outlet wirh 120V
    foliageToggler.setInterval(foliagePeriodOn * TASK_SECOND); //foliage fans time on
  }
  else {
    foliageToggler.setInterval(foliagePeriodOff * TASK_SECOND); //foliage fans time off
  }
    digitalWrite(foliageFans, !digitalRead(foliageFans));
    return true;
}


bool exhaustToggle(){
  readSensors();
  if(averageHum > humUpperLimit){
    if(digitalRead(exhaustFans) == HIGH){//HIGH is off in this case, a LOW signal powers the outlet wirh 120V
    exhaustToggler.setInterval(exhaustPeriodOn * TASK_SECOND); //exhaust fans time on
  }
    else {
    exhaustToggler.setInterval(exhaustPeriodOff * TASK_SECOND); //exhaust fans time off
  }
    digitalWrite(exhaustFans, !digitalRead(exhaustFans));
  }
    return true;
}


bool readSensors(){
  RtcDateTime now = Rtc.GetDateTime();
  
  AmbientHum = dhtAmbient.readHumidity();
  AmbientTempF = dhtAmbient.readTemperature(true);
  lightsHum = dhtLights.readHumidity();
  lightsTempF = dhtLights.readTemperature(true);
  midHum = dhtMid.readHumidity();
  midTempF = dhtMid.readTemperature(true);
  soilHum = dhtSoil.readHumidity();
  soilTempF = dhtSoil.readTemperature();
  averageHum = (soilHum + midHum + lightsHum) / 3;
  averageTempF = (soilTempF + midTempF + lightsTempF) / 3;
  soilMoistureValue = (analogRead(soilMoist1) + analogRead(soilMoist2) + analogRead(soilMoist3)) / 3;  
  readDateTime(now);
}

bool plantWaterToggle(){
  //check sensors, water plants if appropriate
  return true;
}

bool resevoirFill(){
  //check float sensors, opening mainline solenoid to refill. store interval value, set interval to 5 seconds until full, resetting interval to previous value
  return true;
}

bool vegToBloom(){
  //compare RTC date to whenever we're supposed to flip the lights schedule
  return true;
}

bool cO2Inject(){

  return true;
}

void lcdTest() {
  lcd.setCursor(0, 0);
  lcd.print("Line 1");
  lcd.setCursor(10, 0);
  lcd.print(millis());
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print("Line 2");
  lcd.setCursor(10, 1);
  lcd.print(millis());
  delay(500);
  lcd.setCursor(0, 2);
  lcd.print("Line 3");
  lcd.setCursor(10, 2);
  lcd.print(millis());
  delay(500);
  lcd.setCursor(0, 3);
  lcd.print("Line 4");
  lcd.setCursor(10, 3);
  lcd.print(millis());
  delay(1000);
  lcd.clear();
  delay(1000);
}

bool setupWizard(){
  lcd.clear();
  lcd.println("setup WIZZZZ");
  delay(1000);
  menuPicker();
  return true; //when done? not quite sure what to do with this return value...
}

int menuPicker(){

}

int userInput(){
  int value = 0;
  while(digitalRead(RotarySwitch) == HIGH){
    value = rotaryPosition(value);
    lcd.setCursor(18,3);
    lcd.println(value);
    delay(25);
    lcd.setCursor(15,3);
    lcd.print("     ");
  }
  return value;
}

int rotaryPosition(int value){
  long newPosition = myEnc.read();
  long oldPosition;
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    return newPosition;
  }
}


bool saveToSD(){
  String dataString = "";
  //string variables and datetime together
  dataString += String(readDateTime()) += ",";
  dataString += String(AmbientHum) += ",";
  dataString += String(AmbientTempF) += ",";
  dataString += String(lightsHum) += ",";
  dataString += String(lightsTempF) += ",";
  dataString += String(midHum) += ",";
  dataString += String(midTempF) += ",";
  dataString += String(soilHum) += ",";
  dataString += String(soilTempF) += ",";
  dataString += String(averageHum) += ",";
  dataString += String(averageTempF) += ",";
  dataString += String(soilHum) += ",";
  dataString += String(timeNow) += ",";
  dataString += String(digitalRead(lights)) += ",";
  dataString += String(digitalRead(foliageFans)) += ",";
  dataString += String(digitalRead(exhaustFans)) += ",";
  dataString += String(digitalRead(RB1OUT4)) += ",";
  dataString += String(digitalRead(heater)) += ",";
  dataString += String(digitalRead(co2Dispense)) += ",";
  dataString += String(digitalRead(waterPump)) += ",";
  dataString += String(digitalRead(RB2OUT4));
    
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}


void readDateTime(const RtcDateTime& dt){
    char datestring[20]; 

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
//    Serial.print(datestring);
//    Serial.println();
    timeNow = String(datestring);

}
