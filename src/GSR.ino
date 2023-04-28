#include "Watchy_GSR.h"
#include "DSEG7_Classic_Mini_Regular_60.h"
#include "meter_bitmaps.h"

// Place all of your data and variables here.

enum meterSettings {
  FIRST_MODE,
  DATE_MODE,
  TIME_MODE,
  OFF_MODE,
  STOPWATCH_MODE,
  LAST_MODE
};

const unsigned char *meterBitmaps[LAST_MODE + 1] = {
  nullptr, 
  meter_bitmap_meter_amps,
  meter_bitmap_meter_volts,
  meter_bitmap_meter_off,
  meter_bitmap_meter_diode,
  nullptr
};
  
RTC_DATA_ATTR uint8_t multimeterStyle;
RTC_DATA_ATTR uint8_t meterSetting = TIME_MODE;

class OverrideGSR : public WatchyGSR {
/*
 * Keep your functions inside the class, but at the bottom to avoid confusion.
 * Be sure to visit https://github.com/GuruSR/Watchy_GSR/blob/main/Override%20Information.md for full information on how to override
 * including functions that are available to your override to enhance functionality.
*/
  public:
    OverrideGSR() : WatchyGSR() {}


/*
    void InsertPost(){
    };
*/

/*
    String InsertNTPServer() { return "<your favorite ntp server address>"; }
*/

/*
    void InsertDefaults(){
    };
*/

/*
    bool OverrideBitmap(){
      return false;
    };
*/

/*
    void InsertOnMinute(){
    };
*/

/*
    void InsertWiFi(){
    };
*/

/*
    void InsertWiFiEnding(){
    };
*/

    void InsertAddWatchStyles(){
      multimeterStyle = AddWatchStyle("Multi");
    };


    void InsertInitWatchStyle(uint8_t StyleID){
      if (StyleID == multimeterStyle){
          Design.Menu.Top = 72;
          Design.Menu.Header = 25;
          Design.Menu.Data = 66;
          Design.Menu.Gutter = 3;
          Design.Menu.Font = &aAntiCorona12pt7b;
          Design.Menu.FontSmall = &aAntiCorona11pt7b;
          Design.Menu.FontSmaller = &aAntiCorona10pt7b;
          Design.Face.Bitmap = meterBitmaps[meterSetting];
          Design.Face.SleepBitmap = meterBitmaps[OFF_MODE];
          Design.Face.Gutter = 4;
          Design.Face.Time = 56;
          Design.Face.TimeHeight = 45;
          Design.Face.TimeColor = GxEPD_BLACK;
          Design.Face.TimeFont = &aAntiCorona36pt7b;
          Design.Face.TimeLeft = 0;
          Design.Face.TimeStyle = WatchyGSR::dCENTER;
          Design.Face.Day = 101;
          Design.Face.DayGutter = 4;
          Design.Face.DayColor = GxEPD_BLACK;
          Design.Face.DayFont = &aAntiCorona16pt7b;
          Design.Face.DayFontSmall = &aAntiCorona15pt7b;
          Design.Face.DayFontSmaller = &aAntiCorona14pt7b;
          Design.Face.DayLeft = 0;
          Design.Face.DayStyle = WatchyGSR::dCENTER;
          Design.Face.Date = 143;
          Design.Face.DateGutter = 4;
          Design.Face.DateColor = GxEPD_BLACK;
          Design.Face.DateFont = &aAntiCorona15pt7b;
          Design.Face.DateFontSmall = &aAntiCorona14pt7b;
          Design.Face.DateFontSmaller = &aAntiCorona13pt7b;
          Design.Face.DateLeft = 0;
          Design.Face.DateStyle = WatchyGSR::dCENTER;
          Design.Face.Year = 186;
          Design.Face.YearLeft = 99;
          Design.Face.YearColor = GxEPD_BLACK;
          Design.Face.YearFont = &aAntiCorona16pt7b;
          Design.Face.YearLeft = 0;
          Design.Face.YearStyle = WatchyGSR::dCENTER;
          Design.Status.WIFIx = 5;
          Design.Status.WIFIy = 5;
          Design.Status.BATTx = 155;
          Design.Status.BATTy = 5;
          OverrideDefaultMenu(true);
      }
    };



    void InsertDrawWatchStyle(uint8_t StyleID){
      if (StyleID == multimeterStyle){
          OverrideDefaultMenu(true);
            if (SafeToDraw()){
                if (meterSetting == TIME_MODE) drawTime();
                if (meterSetting == DATE_MODE) drawDate();
                //if (meterSetting == STOPWATCH_MODE) drawStopwatch();
            }
      }
    };
    
    void drawNumber(int value1, int value2) {
      display.setTextColor(GxEPD_BLACK);
      display.setFont(&DSEG7_Classic_Mini_Regular_60);
      display.setCursor(0, 90);
      display.printf("%02d.", value1);
      display.setCursor(100, 90);
      display.printf("%02d", value2);
    }

    void drawTime() {
      drawNumber(WatchTime.Local.Hour, WatchTime.Local.Minute);
      return;
    }
    
    void drawDate() {
      drawNumber(WatchTime.Local.Day, WatchTime.Local.Month + 1);
      return;
    }
    
    bool InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh) {
      switch (SwitchNumber){
        case 1: // Menu
          if (meterSetting < LAST_MODE - 1) {
            meterSetting++;
            Design.Face.Bitmap = meterBitmaps[meterSetting];
          }
          Haptic = true;
          Refresh = true;
          return true;
        case 2: //Back
          Refresh = true;
          return true;
        case 3: //Up
          ShowDefaultMenu();
          return true;
        case 4: //Down
          if (meterSetting > FIRST_MODE + 1) {
            meterSetting--;
            Design.Face.Bitmap = meterBitmaps[meterSetting];
          }
          Haptic = true;
          Refresh = true;
          return true;
      }
      return false;
  };
};

// Do not edit anything below this, leave all of your code above.
OverrideGSR watchy;

void setup(){
  watchy.init();
}

void loop(){}
