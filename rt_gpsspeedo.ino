#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "TFT_eSPI.h"
#include "Button2.h"
#include "Timezone.h"
#include <Preferences.h>

//Serial Ports GPS is connected to.
#define RXD2 18
#define TXD2 17
//define backlight pins
#define PIN_POWER_ON 15 
#define PIN_LCD_BL 38
//Define GPS Baud
#define GPS_BAUD 9600
//Define button pins
#define BUTTON_A_PIN  0
#define BUTTON_B_PIN  14

//Initiate serial object for GPS. 2nd serial as 1st serial is for monitor
HardwareSerial gpsSerial(2);

// The TinyGPSPlus object
TinyGPSPlus gps;

// The TFT Diplay Object
TFT_eSPI tft = TFT_eSPI();

//Start two instances of the button
Button2 buttonA, buttonB;

//Define the time zone rules for BST ( London )
TimeChangeRule ukDST = {"BST", Last, Sun, Mar, 2, +60};  // Daylight time = UTC + 1 hours
TimeChangeRule ukGMT = {"GMT", Last, Sun, Oct, 2, +0};   // GMT = UTC + 0 hours
Timezone uk(ukDST, ukGMT);

//create a preferences instance called prefs
Preferences prefs;

//setup global variables for latter use
unsigned long milli_delay=500;
int spd=0,num_sats=0,brightness=250,TXT_Colour=TFT_WHITE,TXT_Back=TFT_BLACK;
String units="m",dir="c";
bool stateIsSaved = true;
long int lastButtonTime = millis();

// time variables
time_t t_prev_set;
int t_timesetinterval = 3600; //set microcontroller time every hour
int t_set=0;

void setup()
{
  Serial.begin(115200);

//Initiate the GPs comms
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started at 9600 baud rate");

//Power on and clear the display
  digitalWrite(PIN_POWER_ON, HIGH);
  digitalWrite(PIN_LCD_BL, HIGH);

//Init the TFT object and setup the display  
  tft.begin();
  tft.setRotation(1); //landscape with USB port on the right
  tft.fillScreen(TFT_BLACK);

//Sart each button and define what gets called for each type of button click
//only single and long clicks are being handled. Each button click type could have it's own function.
  buttonA.begin(BUTTON_A_PIN);
  buttonA.setClickHandler(click);
  buttonA.setLongClickHandler(click);
  buttonB.begin(BUTTON_B_PIN);
  buttonB.setClickHandler(click);
  buttonB.setLongClickHandler(click); 

  //create a preferences names space called gps-spdo
  prefs.begin("gps-spdo", false); 

  //check to see if we there are any saved vaules if there are read them in
  unsigned char flag = prefs.getUInt("mem_flag",0);
  if (flag == 1){
    units = prefs.getString("mem_units");
    dir=prefs.getString("mem_dir");
    TXT_Colour=prefs.getInt("mem_colour");
    brightness=prefs.getInt("mem_bright");
    //set the brightness here otherewise it won't be done 'till there is a GPS fix...
    analogWrite(PIN_LCD_BL,brightness);
  }
  else{
    saveCurrentState();
    prefs.putUInt("mem_flag",1);
  }
}

void loop()
{
  //'zero' the millis delay
  unsigned long milli_start=millis();

  while (gpsSerial.available() > 0){
  //send the serial buffer to TinyGPS until it's empty
    gps.encode(gpsSerial.read());
  }
  //While waiting for the GPS to update check to see if a button is pressed...  
  while(milli_start+milli_delay>millis()){
    buttonA.loop();
    buttonB.loop();
    }
  //check to see if a button was pressed >10 seconds ago AND we need to update the saved state.
  if (!stateIsSaved && (millis() - lastButtonTime > 10000)){ 
    saveCurrentState();
  } 
  //check to see if gps time is valid if it is then..
  if (gps.time.isValid())
  {
    //if time has been set AND the refresh interval has expired AND we can see some satellites to get the time then set it...
    if (t_set==1 && (now() - t_prev_set > t_timesetinterval) && (gps.satellites.value()>0) )
    {
    setthetime();
    t_prev_set = now();
    }
    //if time has not been set AND we can see some satellites to set it...
    else if(t_set==0 && (gps.satellites.value()>0))
    {
      setthetime();
    }
  }  
  // display GPS info...  
  displayInfo();
}

void displayInfo()
{
  //Display GPS on the screen
  //force the text 'anchor' to be bottom left so we can print exactly smae place as it changes from mph to kph
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TXT_Colour,TXT_Back);
  tft.setTextSize(2);
  
  //determine if speed is in Miles or K
  //show units, default is miles
  if(units=="m"){
    spd=gps.speed.mph();
    //4 is a 'normal' font that has all alphanumerc chars
    tft.drawString("mph  ",200,70,4);  
  }
  else if(units=="k"){
    spd=gps.speed.kmph();
    tft.drawString("kph  ",200,70,4);
  }

  //Setup the display for speed
  tft.setTextWrap(false, false);
  tft.setTextSize(2);

  //GPS is not very accurate below 5 so set display to -- if below 5 or there are no satellites...
  if (spd<5 || gps.satellites.value()<1 ){
    //drawRightString sets the bottom right corner of the text to the co-ords listed. Default would be to set the top left corner
    // want bottom right so it is against the speed units, if we used top left as the speed increased into 3 digits it would over write the units....
    //7 is the font, a 7 segment LCD font so can only show numbers and - and _ etc. not much else
    tft.drawRightString("--",200,10,7);
  } 
  //if speed if more than 5 and there is at least one satellite show the speed...
  else if(spd>=5 && gps.satellites.value()> 0){
    tft.drawRightString("   " + String(spd),200,10,7);
  }

  //set text anchor back to default top left corner of the text being written.
  //this is OK because the units are to the left of the changing numbers ( unlike speed ) so we want the left edge to be constant.
   tft.setTextDatum(TL_DATUM);
   tft.setTextSize(1);
   tft.setTextFont(4);

//if time has been set then display the time
  if (t_set==1){
  displaythetime();
  }
  else{
    tft.drawString("T: --:--",10,120);
  }

//Print Number of satellites to the display
  tft.drawString("S: " + String(gps.satellites.value())+"    ",10,150);

//If there are no satellites blank the vaules. Time stays as thats taken from the device time.
  if(gps.satellites.value()<1){
    tft.drawString("ALT: --     ",180,120);
      if(dir=="d"){
        tft.drawString("D: --       ",200,150);
    } 
    else if(dir=="c"){
      tft.drawString("C: --       ",200,150);
    } 
  }
  //if we can see some satellites then show the data...
  else{
  tft.drawString("ALT: " + String(int(gps.altitude.meters()))+"      ",180,120);
  if(dir=="d"){
    tft.drawString(" D: " + String(int(gps.course.deg()))+"         ",200,150);
    } 
    else if(dir=="c"){
      tft.drawString(" C: " + String(TinyGPSPlus::cardinal(gps.course.deg()))+"         ",200,150);
    } 
  }
}

//Handle the button clicks
//only short and long are enabled - no multi-press
void click(Button2& btn){
    //record the time the button was pressed and set the flag to state not saved...
    lastButtonTime = millis(); 
    stateIsSaved = false;
    switch (btn.getType()){
        case single_click:
          //cycle through the brightness
          if (btn == buttonA){
            if (brightness<250){
              brightness=brightness+50;
              analogWrite(PIN_LCD_BL,brightness);
            }
            else if(brightness>=250){
              brightness=50;
              analogWrite(PIN_LCD_BL,brightness);
            }
          } 
          else if (btn == buttonB) {
            //cycles through different text colours. Default is white. To change default change value of TXT_Colour at the start.
            switch(TXT_Colour){
              case TFT_WHITE:
                TXT_Colour=TFT_GREEN;
                displayInfo();
                break;
              case TFT_GREEN:
                TXT_Colour=TFT_ORANGE;
                displayInfo();
                break;
              case TFT_ORANGE:
                TXT_Colour=TFT_RED;
                displayInfo();
                break;
              case TFT_RED:
                TXT_Colour=TFT_WHITE;
                displayInfo();
                break;
              case empty:
                return;
            }
            }
            break;
      
        case double_click:
            //not used
            Serial.print("double ");
            break;
      
        case triple_click:
            //not used
            Serial.print("triple ");
            break;

        case long_click:
            //switch between mph and kph and update disaply
            if (btn == buttonB) {
              if (units=="k"){
                units="m";
                } else if (units=="m"){
                units="k";
              }
              displayInfo();
            }
            //switch between degrees and cardinal and update display
            if (btn == buttonA) {
              if (dir=="d"){
                dir="c";
                } else if (dir=="c"){
                dir="d";
                }
              displayInfo();
            }
            break;
      
          case empty:
            return;
    }
}

void setthetime(void)
{
  // Set Time from GPS date string
  int Year = gps.date.year();
  byte Month = gps.date.month();
  byte Day = gps.date.day();
  byte Hour = gps.time.hour();
  byte Minute = gps.time.minute();
  byte Second = gps.time.second();
  // set the time of the microcontroller to the UTC time from the GPS
  setTime(Hour, Minute, Second, Day, Month, Year);
  //set the flag to say time has been set...
  t_set=1;
}

void displaythetime(void)
{
  //Setup time vars. TinyGPS does not show leading 0 for time so we have to add them.
  time_t t_local, t_utc;
  String shour="",smin="";
  t_utc = now();  // read the time in the correct format to change via the TimeChangeRules
  //put the UTC time through the timezone DST rules
  t_local = uk.toLocal(t_utc);
  //Print local time to display. Not bothering with seconds or centiseconds
  //add leading zeros if single digit time.
  if (hour(t_local) < 10) {
    shour="0"+String(hour(t_local));
  } else{
    shour=String(hour(t_local));
  }
  if (minute(t_local) < 10) {
    smin="0"+String(minute(t_local));
  } else{
    smin=String(minute(t_local));
  }
  //print the time to the display
    tft.drawString("T: "+ shour + ":" + smin +"  ",10,120);
}

static void saveCurrentState(){
    prefs.putString("mem_units",units);
    prefs.putString("mem_dir",dir);
    prefs.putInt("mem_colour",TXT_Colour);
    prefs.putInt("mem_bright",brightness);
  stateIsSaved = true;
}
