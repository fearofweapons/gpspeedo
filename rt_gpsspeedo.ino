#include <TinyGPSPlus.h>
#include "TFT_eSPI.h"
#include "Button2.h"

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

//setup variables for latter use
unsigned long milli_delay=500;
int spd=0,num_sats=0,brightness=250,TXT_Colour=TFT_WHITE,TXT_Back=TFT_BLACK;
String units="m",dir="c";

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
  tft.drawString("Hello",10,10);
  
//Sart each button and define what gets called for each type of button click
//only single and long clicks are being handled. Each click type or button could have it's own function.
  buttonA.begin(BUTTON_A_PIN);
  buttonA.setClickHandler(click);
  buttonA.setLongClickHandler(click);
  buttonB.begin(BUTTON_B_PIN);
  buttonB.setClickHandler(click);
  buttonB.setLongClickHandler(click); 
}

void loop()
{
  //'zero' the millis delay
  unsigned long milli_start=millis();

  //while (gpsSerial.available() > 0){
  //send the serial buffer to TinyGPS until it's empty
    //gps.encode(gpsSerial.read());
  //}
  //While waiting for the GPS to update check to see if a button is pressed...  
    //while(milli_start+milli_delay>millis()){
      //buttonA.loop();
      //buttonB.loop();
    //}
    
  // display GPS info on the LCD...  
    //displayInfo();
}

void displayInfo()
{
  //Display GPS on the screen

  //Setup time vars. TinyGPS does not show leading 0 for time so we have to add them.
  String shour="",smin="";

  //force the text 'anchor' to be bottom left so we can print exactly smae place as it changes from mph to kph
  tft.setTextDatum(BL_DATUM);
  tft.setTextSize(2);
  
  //determine if speed is in Miles or K
  //show units, default is miles
  if(units=="m"){
    spd=gps.speed.mph();
    //4 is a 'normal' font that has all alphanumerc chars
    tft.drawString("mph  ",200,70,4);  
  } else if(units=="k"){
    spd=gps.speed.kmph();
    tft.drawString("kph  ",200,70,4);
  }
  
  //Setup the display for speed
  tft.setTextWrap(false, false);
  tft.setTextSize(2);
  tft.setTextColor(TXT_Colour,TXT_Back);
  //GPS is not very accurate below 5 so set display to -- if below 5...
  if (spd<5){
    //drawRightString sets the bottom right corner of the text to the co-ords listed. Default would be to set the top left corner
    //no good for strings that change length where we need to know where they end
    //7 is the font, a 7 segment LCD font so can only show numbers and - and _ etc. not much else
    tft.drawRightString("--",200,10,7);
  } else if(spd>=5){
    tft.drawRightString("   " + String(spd),200,10,7);
  }

  //set text anchor back to default top left corner of the text being written.
   tft.setTextDatum(TL_DATUM);
   tft.setTextSize(1);
   tft.setTextFont(4);

//Print UTC time to display. Not bothering with seconds or centiseconds
//add leading zeros if single digit time.
if (gps.time.hour() < 10) {
  shour="0"+String(gps.time.hour());
} else{
  shour=String(gps.time.hour());
}
if (gps.time.minute() < 10) {
  smin="0"+String(gps.time.minute());
} else{
  smin=String(gps.time.minute());
}
  tft.drawString("T: "+ shour + ":" + smin +"  ",10,120);

  
//Print Number of satellites to the display
  tft.drawString("S: " + String(gps.satellites.value())+"    ",10,150);

//Print alititude to display
  tft.drawString("ALT: " + String(int(gps.altitude.meters()))+"    ",180,120);

  //Print heading to dispaly in degrees or Compass Points
  if(dir=="d"){
    tft.drawString(" D: " + String(int(gps.course.deg()))+"       ",200,150);
  } else if(dir=="c") {
    tft.drawString(" C: " + String(TinyGPSPlus::cardinal(gps.course.deg()))+"       ",200,150);
  }  
  
//determine if speed is in Miles or K
  if(units=="m"){
  spd=gps.speed.mph();
  } else if(units=="k"){
    spd=gps.speed.kmph();
  }

//send info to the serial monitor for debugging.
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.println(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    Serial.println("");
    Serial.print("Satellites = "); 
    Serial.println(gps.satellites.value()); 
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.println(gps.time.second());
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  Serial.println("Speed: "+String(spd));
  Serial.println("Card: " + String(TinyGPSPlus::cardinal(gps.course.deg())));
  Serial.println("-------------------------------");

  Serial.println();
}

//Handle the button clicks
//only short and long are enabled - no multi-press
void click(Button2& btn){
    switch (btn.getType()) {
        case single_click:
          //cycle through the brightness
          if (btn == buttonA) {
            if (brightness<250){
              brightness=brightness+50;
              analogWrite(PIN_LCD_BL,brightness);
              Serial.print("Brightness : ");
              Serial.println(String(brightness));
            }
            else if(brightness>=250){
              brightness=50;
              analogWrite(PIN_LCD_BL,brightness);
              Serial.print("Brightness : ");
              Serial.println(String(brightness));
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
            if (btn == buttonA) {
              if (units=="k"){
                units="m";
                } else if (units=="m"){
                units="k";
              }
              displayInfo();
            }
            //switch between degrees and cardinal and update display
            if (btn == buttonB) {
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
