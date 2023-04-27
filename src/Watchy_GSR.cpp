#include "Watchy_GSR.h"

static const char UserAgent[] PROGMEM = "Watchy";
//WiFi statics.
static const wifi_power_t RawWiFiTX[11]= {WIFI_POWER_19_5dBm,WIFI_POWER_19dBm,WIFI_POWER_18_5dBm,WIFI_POWER_17dBm,WIFI_POWER_15dBm,WIFI_POWER_13dBm,WIFI_POWER_11dBm,WIFI_POWER_8_5dBm,WIFI_POWER_7dBm,WIFI_POWER_5dBm,WIFI_POWER_2dBm};
static const char *IDWiFiTX[11] = {"19.5dBm [Max]","19dBm","18.5dBm","17dBm","15dBm [Laptop]","13dBm","11dBm","8.5dBm [Medium]","7dBm","5dBm [10 meters]","2dBm [Low]"};

char WiFiIDs[] PROGMEM = "ABCDEFGHIJ";

int AlarmVBs[] = {0x01FE, 0x00CC, 0x01B6, 0x014A};
const uint16_t Bits[10] = {1,2,4,8,16,32,64,128,256,512};
const float Reduce[5] = {1.0,0.8,0.6,0.4,0.2};

// Specific defines to this Watchy face.
#define GName "GSR"
#define GSettings "GSR-Options"
#define GTZ "GSR-TZ"

// Protected
RTC_DATA_ATTR struct GSRWireless final {
    bool Requested;          // Request WiFi.
    bool Working;            // Working on getting WiFi.
    bool Results;            // Results of WiFi, found an AP?
    uint8_t Index;           // 10 = built-in, roll backwards to 0.
    uint8_t Requests;        // WiFi Connect requests.
    struct APInfo {
        char APID[33];
        char PASS[64];
        uint8_t TPWRIndex;
    } AP[10];                // Using APID to avoid internal confusion with SSID.
    unsigned long Last;      // Used with millis() to maintain sanity.
    bool Tried;              // Tried to connect at least once.
    wifi_power_t TransmitPower;
    wifi_event_id_t WiFiEventID;
} GSRWiFi;
RTC_DATA_ATTR struct CPUWork final {
    uint32_t Freq;
    bool     Locked;
} CPUSet;
RTC_DATA_ATTR struct Stepping final {
    uint8_t Hour;
    uint8_t Minutes;
    bool Reset;
    uint32_t Yesterday;
} Steps;
RTC_DATA_ATTR struct Optional final {
    bool TwentyFour;                  // If the face shows 24 hour or Am/Pm.
    bool LightMode;                    // Light/Dark mode.
    bool Feedback;                    // Haptic Feedback on buttons.
    bool Border;                      // True to set the border to black/white.
    bool Lefty;                       // Swaps the buttons to the other side.
    bool Swapped;                     // Menu and Back buttons swap ends (vertically).
    bool Orientated;                  // Set to false to not bother which way the buttons are.
    uint8_t Turbo;                    // 0-10 seconds.
    uint8_t MasterRepeats;            // Done for ease, will be in the Alarms menu.
    int Drift;                        // Seconds drift in RTC.
    bool UsingDrift;                  // Use the above number to add to the RTC by dividing it by 1000.
    uint8_t SleepStyle;               // 0==Disabled, 1==Always, 2==Sleeping
    uint8_t SleepMode;                // Turns screen off (black, won't show any screen unless a button is pressed)
    uint8_t SleepStart;               // Hour when you go to bed.
    uint8_t SleepEnd;                 // Hour when you wake up.
    uint8_t Performance;              // Performance style, "Turbo", "Normal", "Battery Saving" 
    bool NeedsSaving;                 // NVS code to tell it things have been updated, so save to NVS.
    bool BedTimeOrientation;          // Make Buttons only work while Watch is in normal orientation.
    uint8_t WatchFaceStyle;           // Using the Style values from Defines_GSR.
    uint8_t LanguageID;               // The LanguageID.
} Options;
RTC_DATA_ATTR struct DesignStyles final {
    uint8_t Count;
    char Style[32 * GSR_MaxStyles];
} WatchStyles;

RTC_DATA_ATTR struct MenuUse final {
    int8_t Style;           // GSR_MENU_INNORMAL or GSR_MENU_INOPTIONS
    int8_t Item;            // What Menu Item is being viewed.
    int8_t SubItem;         // Used for menus that have sub items, like alarms and Sync Time.
    int8_t SubSubItem;      // Used mostly in the alarm to offset choice.
} Menu;

RTC_DATA_ATTR int GuiMode;
RTC_DATA_ATTR bool VibeMode;          // Vibe Motor is On=True/Off=False, used for the Haptic and Alarms.
RTC_DATA_ATTR String WatchyStatus;    // Used for the indicator in the bottom left, so when it changes, it asks for a screen refresh, if not, it doesn't.
RTC_DATA_ATTR int BasicWatchStyles;
RTC_DATA_ATTR bool DefaultWatchStyles;  // States that the original 2 Watch Styles are to be added.
RTC_DATA_ATTR uint8_t GSR_UP_PIN;       // Used to catch the different pin allocation for the up button.
RTC_DATA_ATTR uint64_t GSR_UP_MASK;
RTC_DATA_ATTR uint64_t BTN_MASK;
RTC_DATA_ATTR float HWVer;

RTC_DATA_ATTR struct Countdown final {
  bool Active;
  bool Repeats;
  bool Repeating;
  uint8_t Hours;  // Converted to work the same as the CountUp so seconds can be done.
  uint8_t Mins;
  uint8_t Secs;
  uint8_t MaxHours;
  uint8_t MaxMins;
  uint8_t MaxSecs;
  uint32_t MaxTime; // MaxHours*3600+MaxMins*60+MaxSecs.
  uint8_t Tone;       // 10 to start.
  uint8_t ToneLeft;   // 30 to start.
  uint8_t MaxTones;   // 0-4 (20-80%) tone duration reduction.
  time_t LastUTC;
  time_t StopAt;
} TimerDown;

RTC_DATA_ATTR struct CountUp final {
  bool Active;
  time_t SetAt;
  time_t StopAt;
} TimerUp;

RTC_DATA_ATTR struct BatteryUse final {
    float Last;             // Used to track battery changes, only updates past 0.01 in change.
    int8_t Direction;       // -1 for draining, 1 for charging.
    int8_t DarkDirection;   // Direction copy for Options.SleepMode.
    int8_t UpCount;         // Counts how many times the battery is in a direction to determine true charging.
    int8_t DownCount;
    int8_t State;           // 0=not visible, 1= showing chargeme, 2= showing reallychargeme, 3=showing charging.
    int8_t DarkState;       // Dark state of above.
    float MinLevel;         // Lowest level before the indicator comes on.
    float LowLevel;         // The battery is about to get too low for the RTC to function.
} Battery;

RTC_DATA_ATTR struct NTPUse final {
    uint8_t State;          // State = 0=Off, 1=Start WiFi, 2=Wait for WiFi, TZ, send NTP request, etc, Finish.  See function ProcessNTP();
    uint8_t Wait;           // Counts up to 3 minutes, then fails.
    uint8_t Pause;          // How many 50ms to pause for.
    time_t Last;            // Last time it worked.
    bool TimeZone;          // Update Timezone during ProcessNTP.
    bool UpdateUTC;         // Update UTC during ProcessNTP.
    bool TimeTest;          // Test the RTC against ESP32's MS count for 1 minute.
    uint8_t TestCount;      // Counts the duration of minutes, does 2.
    bool NTPDone;           // Sets it to Done when an NTP has happened in the past.
} NTPData;

RTC_DATA_ATTR struct GoneDark final {
    bool Went;
    unsigned long Last;
    bool Woke;
    unsigned long Tilt;     // Used to track Tilt, if upright count for 1 second.
} Darkness;                 // Whether or not the screen is darkened.

RTC_DATA_ATTR struct dispUpdate final {
    bool Full;
    bool Drawn;
    bool Init;
    bool Tapped;
    unsigned long LastSecond;
} Updates;

//WatchyRTC WatchyGSR::SRTC;
RTC_DATA_ATTR SmallRTC WatchyGSR::SRTC;
RTC_DATA_ATTR SmallNTP WatchyGSR::SNTP;
RTC_DATA_ATTR Designing Design;
RTC_DATA_ATTR TimeData WatchTime;
RTC_DATA_ATTR bool MenuOverride;     // True if override, but can be disabled *IF* held down for 10 seconds, it would not open the menu.
RTC_DATA_ATTR StableBMA SBMA;
RTC_DATA_ATTR LocaleGSR LGSR;
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> WatchyGSR::display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RESET, EPD_BUSY));

volatile uint8_t Button;

//RTC_DATA_ATTR alignas(8) struct AlarmID {
RTC_DATA_ATTR     uint8_t   Alarms_Hour[4];
RTC_DATA_ATTR     uint8_t   Alarms_Minutes[4];
RTC_DATA_ATTR     uint16_t  Alarms_Active[4];         // Bits 0-6 (days of week), 7 Repeat, 8 Active, 9 Tripped (meaning if it isn't repeating, don't fire).
RTC_DATA_ATTR     uint8_t   Alarms_Times[4];          // Set to 10 to start, ends when zero.
RTC_DATA_ATTR     uint8_t   Alarms_Playing[4];        // Means the alarm tripped and it is to be played (goes to 0 when it finishes).
RTC_DATA_ATTR     uint8_t   Alarms_Repeats[4];        // 0-4 (20-80%) reduction in repetitions.
//} Alarms[4];

WiFiClient WiFiC;             // Tz
HTTPClient HTTP;              // Tz
Olson2POSIX OP;               // Tz code.
WebServer server(80);
unsigned long OTATimer;
bool WatchyAPOn;   // States Watchy's AP is on for connection.  Puts in Active Mode until back happens.
bool ActiveMode;   // Moved so it can be checked.
bool Sensitive;    // Loop code is sensitive, like OTAUpdate, TimeTest
bool OTAUpdate;    // Internet based OTA Update.
bool AlarmReset;   // Moved out here to work with Active Mode.
bool OTAEnd;       // Means somewhere, it wants this to end, so end it.
int OTATry;        // Tries to connect to WiFi.
bool DoHaptic;     // Want it to happen after screen update.
bool UpdateDisp;   // Display needs to be updated.
bool IDidIt;       // Tells if the Drifting was done this minute.
bool AlarmsOn;     // Moved for CPU.
bool Rebooted;     // Used in DisplayInit to force a full initial on power up.
time_t TurboTime;  // Moved here for less work.
uint8_t Missed;    // Button not in menu, not used, so can be used by override.
bool AllowHaptic;  // For the Task to tell the main thread it can't use the Haptic feedback due to alarm/timer.
uint8_t HapticMS;  // Holds how long it runs for.
unsigned long LastButton, OTAFail, LastHelp;

WatchyGSR::WatchyGSR(){}  //constructor

// Init Defaults after a reboot, setup all the variables here for defaults to avoid randomness.
void WatchyGSR::setupDefaults(){
    Options.TwentyFour = false;
    Options.LightMode = true;
    Options.Feedback = true;
    Options.Border = false;
    Options.Lefty = false;
    Options.Swapped = false;
    Options.Turbo = 3;
    Options.SleepStyle = 0;
    Options.SleepMode = 3;
    Options.SleepStart = 23;
    Options.SleepEnd = 7;
    Options.MasterRepeats = 0;  // 100%.
    Options.BedTimeOrientation = false;
    Options.WatchFaceStyle = 0;
    Options.LanguageID = 0;
    GSRWiFi.TransmitPower = WiFi.getTxPower();
    Steps.Hour = 6;
    Steps.Minutes = 0;
    InsertDefaults();
}

void WatchyGSR::init(String datetime){
    uint64_t wakeupBit;
    bool DoOnce, B, Up;
    unsigned long Since, APLoop;
    uint8_t I;
    String S;
    BaseType_t SoundRet;
    TaskHandle_t SoundHandle = NULL;
    esp_sleep_wakeup_cause_t wakeup_reason;

    Wire.begin(SDA, SCL); //init i2c
    NVS.begin();

    pinMode(GSR_MENU_PIN, INPUT);   // Prep these for the loop below.
    pinMode(GSR_BACK_PIN, INPUT);
    pinMode(GSR_UP_PIN, INPUT);
    pinMode(GSR_DOWN_PIN, INPUT);

    wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason
    wakeupBit = esp_sleep_get_ext1_wakeup_status();
    DoOnce = true;
    IDidIt = false;
    Updates.Drawn = false;
    Updates.Init = true;
    Updates.Tapped = false;
    Updates.LastSecond = 0;  // Invalid second.
    LastButton = 0;
    Missed = 0;
    Darkness.Last = 0;
    Darkness.Tilt = 0;
    Darkness.Woke = false;
    TurboTime = 0;
    AllowHaptic = true;
    CPUSet.Freq=getCpuFrequencyMhz();

    switch (wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0: //RTC Alarm
            RefreshCPU(GSR_CPUDEF);
            WatchTime.Drifting += Options.Drift;
            IDidIt = true;
            UpdateUTC();
            WatchTime.NewMinute = true;
            UpdateClock();
            detectBattery();
            UpdateDisp=Showing();
            InsertOnMinute();
            break;
        case ESP_SLEEP_WAKEUP_EXT1: //button Press
            RefreshCPU(GSR_CPUDEF);
            UpdateUTC();
            Button = getButtonMaskToID(wakeupBit);
            if (Options.SleepStyle != 4) UpdateDisp = !Showing();
            if (Darkness.Went && UpRight()){
                if (Button == 9 && Options.SleepStyle > 1 && Options.SleepStyle != 4){  // Accelerometer caused this.
                    if (Options.SleepMode == 0) Options.SleepMode = 2;  // Do this to avoid someone accidentally not setting this before usage.
                    Darkness.Woke=true; Updates.Tapped=true; Darkness.Last=millis(); Darkness.Tilt = Darkness.Last; UpdateDisp = true; // Update Screen to new state.
                }else if (Button == 10 && !WatchTime.BedTime){  // Wrist.
                    Darkness.Woke=true; Darkness.Last=millis(); Darkness.Tilt = Darkness.Last; UpdateDisp = true; // Do this anyways, always.
                }else if (Button != 0) {
                    Darkness.Woke=true; Darkness.Last=millis(); Darkness.Tilt = Darkness.Last; UpdateDisp = true; // Update Screen to new state.
                }
            }
            if (Darkness.Woke || Button != 0) UpdateClock();  // Make sure this is done when buttons are pressed for a wakeup.
            break;
        default: //reset
            WatchStyles.Count = 0;
            BasicWatchStyles = -1;
            SRTC.init();
            Battery.MinLevel = SRTC.getRTCBattery();
            Battery.LowLevel = SRTC.getRTCBattery(true);
            GSR_UP_PIN = 32;
            GSR_UP_MASK = GPIO_SEL_32;
            //HWVer = SRTC.getWatchyHWVer();
            //if (SRTC.getType() == PCF8563){ if (HWVer == 1.5) { GSR_UP_PIN = 32; GSR_UP_MASK = GPIO_SEL_32; } else { GSR_UP_PIN = 35; GSR_UP_MASK = GPIO_SEL_35; } }
            HWVer = 1.0;  // 1.2
            if (SRTC.getType() == PCF8563){ if (SRTC.getADCPin() == 35) { HWVer =1.5; GSR_UP_PIN = 32; GSR_UP_MASK = GPIO_SEL_32; } else { HWVer = 2.0; GSR_UP_PIN = 35; GSR_UP_MASK = GPIO_SEL_35; } } // 1.2
            BTN_MASK = GSR_MENU_MASK|GSR_BACK_MASK|GSR_UP_MASK|GSR_DOWN_MASK;
            initZeros();
            setupDefaults();
            _bmaConfig();
            Rebooted=true;
            if (DefaultWatchStyles){
                I = AddWatchStyle("{%0%}"); // Classic GSR
                I = AddWatchStyle("Ballsy");
                I = AddWatchStyle("LCD");
                BasicWatchStyles = I;
            }
            InsertAddWatchStyles();
            if (WatchStyles.Count == 0){
                I = AddWatchStyle("{%0%}"); // Classic GSR
                I = AddWatchStyle("Ballsy");
                I = AddWatchStyle("LCD");
                BasicWatchStyles = I;
                DefaultWatchStyles = true;
            }
            UpdateUTC();
            if (OkNVS(GName)) B = NVS.getString(GTZ,S);
            OP.setCurrentPOSIX(S);
            RetrieveSettings();
            RefreshCPU(GSR_CPUDEF);
            initWatchFaceStyle();
            WatchTime.WatchyRTC = esp_timer_get_time() + ((60 - WatchTime.UTC.Second) * 1000000);
            WatchTime.EPSMS = (millis() + (1000 * (60 - WatchTime.UTC.Second)));
            UpdateClock();
            UpdateFonts();
            InsertPost();
            AlarmsOn=false;
            Updates.Full=true;
            UpdateDisp=true;
            Darkness.Went=true; // Fake this.
            Darkness.Woke=true;
            Darkness.Last=millis();
            Darkness.Tilt=Darkness.Last;
#ifndef GxEPD2DarkBorder
            Options.Border=false;
#endif
            break;
    }

    B = true;
    if (Darkness.Went)
    {
        if (Options.SleepStyle == 4) B = (Updates.Tapped || Battery.DarkState != Battery.State);
        else B = (Darkness.Woke || Button != 0 || Battery.DarkState != Battery.State);
    }

    if (B || Updates.Full || WatchTime.NewMinute){
        // Sometimes BMA crashes - simply try to reinitialize bma...

        if (SBMA.getErrorCode() != 0) {
            SBMA.shutDown();
            SBMA.wakeUp();
            SBMA.softReset();
            _bmaConfig();
        }

        //Init interrupts.
        attachInterrupt(digitalPinToInterrupt(GSR_MENU_PIN), std::bind(&WatchyGSR::handleInterrupt,this), HIGH);
        attachInterrupt(digitalPinToInterrupt(GSR_BACK_PIN), std::bind(&WatchyGSR::handleInterrupt,this), HIGH);
        attachInterrupt(digitalPinToInterrupt(GSR_UP_PIN), std::bind(&WatchyGSR::handleInterrupt,this), HIGH);
        attachInterrupt(digitalPinToInterrupt(GSR_DOWN_PIN), std::bind(&WatchyGSR::handleInterrupt,this), HIGH);
        SoundRet = xTaskCreate(SoundAlarms,"Alarming",20480,NULL,(configMAX_PRIORITIES -1),&SoundHandle);
        DisplayInit();

        if (Button > 0) { handleButtonPress(Button); Button = 0; }
        if (GuiMode == GSR_WATCHON && MenuOverride) {
            if (Missed == 0) Missed = getButtonPins();
            if ((Missed != 1 && Button > 0) || (Missed == 1 && LastHelp == 0)) { LastHelp = millis(); SetTurbo(); }
            else if (Missed == 1 && millis() - LastHelp > 9999) { ShowDefaultMenu(); Missed = 0; }
        }
        if (Missed > 0) { if (InsertHandlePressed(Missed, DoHaptic, UpdateDisp)) SetTurbo(); Missed = 0; Button = 0; }

        CalculateTones();
        AlarmsOn =(Alarms_Times[0] > 0 || Alarms_Times[1] > 0 || Alarms_Times[2] > 0 || Alarms_Times[3] > 0 || TimerDown.ToneLeft > 0);
        ActiveMode = (InTurbo() || DarkWait() || NTPData.State > 0 || AlarmsOn || WatchyAPOn || OTAUpdate || NTPData.TimeTest || WatchTime.DeadRTC || GSRWiFi.Requested || GSRWiFi.Requests > 0 || TimerAbuse());
        ActiveMode |= InsertNeedAwake(!ActiveMode);
        Sensitive = ((OTAUpdate && Menu.SubItem == 3) || (NTPData.TimeTest && Menu.SubItem == 2));

        RefreshCPU();

        while(DoOnce){
            DoOnce = false;  // Do this whole thing once, catch late button presses at the END of the loop just before Deep Sleep.
            ManageTime();   // Handle Time method.
//            AlarmReset = millis();
            if (UpdateDisp) showWatchFace(); //partial updates on tick
            if (ActiveMode == true){
                while (ActiveMode == true) { // Here, we hijack the init and LOOP until the NTP is done, watching for the proper time when we *SHOULD* update the screen to keep time with everything.
                    Since=millis();
                    ManageTime();   // Handle Time method.
                    Up=SBMA.IsUp();
                    // Wrist Tilt delay, keep screen on during this until you put your wrist down.
                    if ((Options.SleepStyle == 1 || (Options.SleepStyle > 2 && Options.SleepStyle != 4) || WatchTime.DeadRTC) && !WatchTime.BedTime){
                        if (Darkness.Went && Up && !Darkness.Woke){ // Do this when the wrist is UP.
                            if (Darkness.Tilt == 0) Darkness.Tilt = millis();
                            else if (millis() - Darkness.Tilt > 999) { Darkness.Last = millis(); Darkness.Woke = true; UpdateDisp=Showing(); }
                        }
                        if (!Up) Darkness.Tilt = 0;
                        else if (Darkness.Tilt != 0 && millis() - Darkness.Tilt > 999 && Darkness.Woke) { Darkness.Last = millis(); }
                    }
                    processWiFiRequest(); // Process any WiFi requests.
                    if (!Sensitive){
                        if (currentWiFi() == WL_CONNECTED && NTPData.State == 0 && !OTAUpdate && !WatchyAPOn && !NTPData.TimeTest) InsertWiFi();
                        if (NTPData.State > 0 && !WatchyAPOn && !OTAUpdate){
                            if (NTPData.Pause == 0) ProcessNTP(); else NTPData.Pause--;
                            if (WatchTime.NewMinute){
                                NTPData.Wait++;
                                UpdateDisp=Showing();
                            }
                        }

                        // Here, make sure the clock updates on-screen.
                        if (WatchTime.NewMinute){
                            detectBattery();
                            UpdateDisp |= Showing();  // **TEST
                        }

                        if (GSRWiFi.Requests == 0 && WatchyAPOn && !OTAUpdate){
                            switch (Menu.SubItem){
                                case 0: // Turn off AP.
                                    OTAEnd = true;
                                    break;
                                case 1: // Turn on AP.
                                    if (WiFi.getMode() != WIFI_AP || (millis() - OTATimer > 4000 && OTATry < 3)){
                                        OTATimer=millis();
                                        OTATry++;
                                        WiFi.setHostname(WiFi_AP_SSID);
                                        OTAEnd |= (!WiFi.softAP(WiFi_AP_SSID, WiFi_AP_PSWD, 1, WiFi_AP_HIDE, WiFi_AP_MAXC));
                                        if (!OTAEnd) UpdateWiFiPower();
                                    }else if (WiFi.getMode() == WIFI_AP){
                                        WatchyGSR::StartWeb();
                                        Menu.SubItem++;
                                        setStatus("WiFi-AP");
                                        UpdateDisp=Showing();
                                        APLoop=millis();
                                    }else Menu.SubItem = 0; // Fail, something is amiss.
                                    break;
                                default: // 2 to 5 is here.
                                    if (Menu.SubItem > 1){
                                        if(WiFi.getMode() == WIFI_STA){
                                            Menu.SubItem = 0;
                                            break;
                                        }
                                        server.handleClient();
                                        /*if (Server.handleRequests()){
                                            Menu.SubItem = 0;
                                            break;
                                        }*/
                                        if (millis() - APLoop > 8000){
                                            Menu.SubItem = roller(Menu.SubItem + 1, 2,4);
                                            UpdateDisp = Showing();
                                            APLoop=millis();
                                        }
                                    }
                            }
                        }
                    }
                    if (OTAUpdate){
                      switch (Menu.SubItem){
                          case 1: // Wait for WiFi to connect or fail.
                              if (WiFi.status() != WL_CONNECTED && currentWiFi() != WL_CONNECT_FAILED) OTATimer = millis();
                              else if (WiFi.status() == WL_CONNECTED){
                                  Menu.SubItem++;
                                  UpdateDisp = Showing();
                              }else OTAEnd=true;
                              break;
                          case 2: // Setup Arduino OTA and wait for it to either finish or fail by way of back button held for too long OR 2 minute with no upload.
                              if (Menu.Item == GSR_MENU_OTAU){
                              ArduinoOTA.setHostname(WiFi_AP_SSID);
                              ArduinoOTA
                                .onStart([]() {
                                  String Type;
                                  if (ArduinoOTA.getCommand() == U_FLASH)
                                    Type = "sketch";
                                  else // U_SPIFFS
                                    Type = "filesystem";
                                  // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                                })
                                .onEnd([]() {
                                  OTAEnd = true;
                                })
                                .onProgress([](unsigned int progress, unsigned int total) {
                                  OTATimer=millis();
                                })
                                .onError([](ota_error_t error) {
                                  OTAEnd=true;
                                });
                              RefreshCPU(GSR_CPUMAX);
                              ArduinoOTA.begin();
                              }else if (Menu.Item == GSR_MENU_OTAM) WatchyGSR::StartWeb();
                              Menu.SubItem++;
                              showWatchFace();
                              break;
                          case 3: // Monitor back button and turn WiFi off if it happens, or if duration is longer than 2 minutes.
                              if (WiFi.status() == WL_DISCONNECTED) OTAEnd = true;
                              else if (Menu.Item == GSR_MENU_OTAU)      ArduinoOTA.handle();
                              else if (Menu.Item == GSR_MENU_OTAM)      server.handleClient();
                              if (getButtonPins() != 2) OTATimer = millis(); // Not pressing "BACK".
                              if (millis() - OTATimer > 10000 || millis() - OTAFail > 600000) OTAEnd = true;  // Fail if holding back for 10 seconds OR 600 seconds has passed.
                        }
                    }

                    // OTAEnd code.
                    if (OTAEnd){
                        if (Menu.Item == GSR_MENU_OTAU)      ArduinoOTA.end();
                        else if (Menu.Item == GSR_MENU_OTAM) server.stop();
                        if (WatchyAPOn) server.stop();
                        VibeTo(false);
                        OTAEnd=false;
                        OTAUpdate=false;
                        WatchyAPOn = false;
                        endWiFi();
                        if (Menu.Item != GSR_MENU_TOFF){
                            Menu.SubItem=0;
                            UpdateUTC();
                            UpdateClock();
                        }
                        Battery.UpCount=0;  // Stop it from thinking the battery went wild.
                        UpdateDisp=Showing();
                    }

                    // Don't do anything time sensitive while in OTA Update.
                    if (!Sensitive){
                        // Here, check for button presses and respond, done here to avoid turbo button presses.

                        if (!UpdateDisp && !DarkWait()) GoDark();
                        handleInterrupt();
                        if (Button > 0) { handleButtonPress(Button); Button = 0; }
                        if (GuiMode == GSR_WATCHON && MenuOverride) {
                            if (Missed == 0) Missed = getButtonPins();
                            if ((Missed != 1 && Button > 0) || (Missed == 1 && LastHelp == 0)) { LastHelp = millis(); SetTurbo(); }
                            else if (Missed == 1 && millis() - LastHelp > 9999) { ShowDefaultMenu(); Missed = 0; }
                        }
                        if (Missed > 0) { if (InsertHandlePressed(Missed, DoHaptic, UpdateDisp)) SetTurbo(); Missed = 0; Button = 0; }
                        CalculateTones(); TimerAbuse();
                        if (UpdateDisp) showWatchFace(); //partial updates on tick
                        if (!Updates.Init) { if (!(InTurbo() || DarkWait())) DisplaySleep(); }

                        AlarmsOn =(Alarms_Times[0] > 0 || Alarms_Times[1] > 0 || Alarms_Times[2] > 0 || Alarms_Times[3] > 0 || TimerDown.ToneLeft > 0);
                        ActiveMode = (InTurbo() || DarkWait() || NTPData.State > 0 || AlarmsOn || WatchyAPOn || OTAUpdate || NTPData.TimeTest || WatchTime.DeadRTC || GSRWiFi.Requested || GSRWiFi.Requests > 0 || TimerAbuse());
                        ActiveMode |= InsertNeedAwake(!ActiveMode);

                        if (WatchTime.DeadRTC && Options.NeedsSaving) RecordSettings();
                        RefreshCPU(GSR_CPUDEF);
                        Since=50-(millis()-Since);
                        if (Since <= 50 && Since > 0) delay(Since);
                    }
                    WatchTime.NewMinute=false;
                    IDidIt=false;
                    Sensitive = ((OTAUpdate && Menu.SubItem == 3) || (NTPData.TimeTest && Menu.SubItem == 2));
                }
            }

            if (Button > 0) { handleButtonPress(Button); Button = 0; }
            if (GuiMode == GSR_WATCHON && MenuOverride) {
                if (Missed == 0) Missed = getButtonPins();
                if ((Missed != 1 && Button > 0) || (Missed == 1 && LastHelp == 0)) { LastHelp = millis(); SetTurbo(); }
                else if (Missed == 1 && millis() - LastHelp > 9999) { ShowDefaultMenu(); Missed = 0; }
            }
            if (Missed > 0) { if (InsertHandlePressed(Missed, DoHaptic, UpdateDisp)) SetTurbo(); Missed = 0; Button = 0; }
            processWiFiRequest(); // Process any WiFi requests.
            CalculateTones(); TimerAbuse();
            if (UpdateDisp) showWatchFace(); //partial updates on tick
            AlarmsOn =(Alarms_Times[0] > 0 || Alarms_Times[1] > 0 || Alarms_Times[2] > 0 || Alarms_Times[3] > 0 || TimerDown.ToneLeft > 0);
            ActiveMode = (InTurbo() || DarkWait() || NTPData.State > 0 || AlarmsOn || WatchyAPOn || OTAUpdate || NTPData.TimeTest || WatchTime.DeadRTC || GSRWiFi.Requested || GSRWiFi.Requests > 0 || TimerAbuse());
            ActiveMode |= InsertNeedAwake(!ActiveMode);
            if (ActiveMode) DoOnce = true;
        }
    }
    if (SoundRet == pdPASS) vTaskDelete(SoundHandle);
    deepSleep();
}

void WatchyGSR::StartWeb(){
    /*return index page which is stored in basicIndex */
    server.on("/", HTTP_GET, [=]() {
      server.sendHeader("Connection", "close");
      String S = basicIndex;
      S.replace("{%^%}",(OTA() ? basicOTA : ""));
      S = LGSR.LangString(S,true,Options.LanguageID,1,4);
      server.send(200, "text/html", S);
      OTATimer=millis();
    });
    server.on("/settings", HTTP_GET, [=]() {
      String S = LGSR.LangString(settingsIndex,true,Options.LanguageID,5,6);
      S.replace("{??}",GetSettings());
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", S);
      OTATimer=millis();
    });
    server.on("/wifi", HTTP_GET, [=]() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", buildWiFiAPPage());
      OTATimer=millis();
    });
    server.on("/update", HTTP_GET, [=]() {
      if (OTA()){
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", LGSR.LangString(updateIndex,true,Options.LanguageID,8,13));
        OTATimer=millis();
      }
    });
    server.on("/settings", HTTP_POST, [=](){
        if (server.argName(0) == "settings") { StoreSettings(server.arg(0)); RecordSettings(); }
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", LGSR.LangString(settingsDone,true,Options.LanguageID,5,7));
        OTATimer=millis();
    });
    server.on("/wifi", HTTP_POST, [=](){
        uint8_t I = 0;
        while (I < server.args()){
            parseWiFiPageArg(server.argName(I),server.arg(I)); I++;
        }
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", LGSR.LangString(wifiDone,true,Options.LanguageID,14,16));
        RecordSettings();
        OTAFail = millis() - 598000;
    });
    server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "Upload Failed." : "Watchy will reboot!");
      delay(2000);
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        OTATimer=millis();

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          OTAEnd=true;
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/

        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) OTAEnd=true;

      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          OTAEnd=true;
        }
      }
    });
    RefreshCPU(GSR_CPUMAX);
    server.begin();
}


void WatchyGSR::showWatchFace(){
  bool B = (Battery.Last > Battery.MinLevel);
  if (Options.Performance > 0 && B) RefreshCPU((Options.Performance == 1 ? GSR_CPUMID : GSR_CPUMAX));
  DisplayInit();
  display.setFullWindow();
  drawWatchFace();
  if (Options.Feedback && DoHaptic && B && AllowHaptic) HapticMS = 9;
//  {
//    VibeTo(true);
//    delay(40);
//    VibeTo(false);
//  } else delay(40);
  UpdateDisp=false;
  Darkness.Went=false;
  Darkness.Last = millis();
  RefreshCPU(GSR_CPULOW);
  DoHaptic=false;
  display.display(!Updates.Full); //partial refresh
  RefreshCPU(GSR_CPUDEF);
  if (!(InTurbo() || DarkWait())) DisplaySleep();
  Updates.Full=false;
  Updates.Drawn=true;
}

void WatchyGSR::drawWatchFace(){
    display.fillScreen(BackColor());
    display.setTextWrap(false);
    if (!OverrideBitmap()) if (Design.Face.Bitmap) display.drawBitmap(0, 0, Design.Face.Bitmap, 200, 200, ForeColor(), BackColor());
    display.setTextColor(ForeColor());

    drawWatchFaceStyle();

    drawChargeMe();
    // Show WiFi/AP/TZ/NTP if in progress.
    drawStatus();
    if (GuiMode == GSR_MENUON) drawMenu();
    UpdateDisp = false;
}

void WatchyGSR::drawTime(uint8_t Flags){
    String O;
    bool PM = false;
    O = MakeTime(WatchTime.Local.Hour | (Flags & 96), WatchTime.Local.Minute, PM);
    display.setFont(Design.Face.TimeFont);
    display.setTextColor(Design.Face.TimeColor);

    drawData(O,Design.Face.TimeLeft,Design.Face.Time,Design.Face.TimeStyle, Design.Face.Gutter, ((Flags & 32) ? false : true), PM);
}

void WatchyGSR::drawDay(){
    String O = LGSR.GetFormatID(Options.LanguageID,0);
    O.replace("{W}",LGSR.GetWeekday(Options.LanguageID, WatchTime.Local.Wday));
    setFontFor(O,Design.Face.DayFont,Design.Face.DayFontSmall,Design.Face.DayFontSmaller,Design.Face.DayGutter);
    display.setTextColor(Design.Face.DayColor);
    drawData(O, Design.Face.DayLeft, Design.Face.Day, Design.Face.DayStyle, Design.Face.DayGutter);
}

void WatchyGSR::drawDate(bool Short){
    String O = LGSR.GetFormatID(Options.LanguageID,1);
    O.replace("{M}",(Short ? LGSR.GetShortMonth(Options.LanguageID, WatchTime.Local.Month) : LGSR.GetMonth(Options.LanguageID, WatchTime.Local.Month)));
    O.replace("{D}",String(WatchTime.Local.Day));
    setFontFor(O,Design.Face.DateFont,Design.Face.DateFontSmall,Design.Face.DateFontSmaller,Design.Face.DateGutter);
    display.setTextColor(Design.Face.DateColor);
    drawData(O, Design.Face.DateLeft, Design.Face.Date, Design.Face.DateStyle, Design.Face.DateGutter);
}

void WatchyGSR::drawYear(){
    display.setFont(Design.Face.YearFont);
    display.setTextColor(Design.Face.YearColor);
    drawData(String(WatchTime.Local.Year + RTC_LOCALYEAR_OFFSET), Design.Face.YearLeft, Design.Face.Year, Design.Face.YearStyle, Design.Face.Gutter);  //1900
}

void WatchyGSR::drawMenu(){
    int16_t  x1, y1, z1;
    uint16_t w, h;
    String O, S ,T;
    unsigned long U;

    display.drawBitmap(0, Design.Menu.Top, (Menu.Style == GSR_MENU_INOPTIONS) ? OptionsMenuBackground : MenuBackground, GSR_MenuWidth, GSR_MenuHeight, ForeColor(), BackColor());
    display.setTextColor(Options.LightMode && Menu.Style != GSR_MENU_INNORMAL ? GxEPD_WHITE : GxEPD_BLACK);
    switch (Menu.Item){
        case GSR_MENU_STEPS:
            if (Menu.SubItem > 0 && Menu.SubItem < 4) O = LGSR.GetID(Options.LanguageID,2);
            else if (Menu.SubItem == 4) O = LGSR.GetID(Options.LanguageID,3);
            else O = LGSR.GetID(Options.LanguageID,4);
            break;
        case GSR_MENU_ALARMS:
            O = LGSR.GetID(Options.LanguageID,5);
            break;
        case GSR_MENU_TIMERS:
            O = LGSR.GetID(Options.LanguageID,6);
            break;
        case GSR_MENU_OPTIONS:
            O = LGSR.GetID(Options.LanguageID,7);
            break;
        case GSR_MENU_ALARM1:
            if (Menu.SubItem > 0 && Menu.SubItem < 4) O = LGSR.GetID(Options.LanguageID,8);
            else if (Menu.SubItem == 4) O = LGSR.GetID(Options.LanguageID,9);
            else if (Menu.SubItem > 4) O = LGSR.GetID(Options.LanguageID,10);
            else O = LGSR.GetID(Options.LanguageID,11);
            break;
        case GSR_MENU_ALARM2:
            if (Menu.SubItem > 0 && Menu.SubItem < 4) O = LGSR.GetID(Options.LanguageID,12);
            else if (Menu.SubItem == 4) O = LGSR.GetID(Options.LanguageID,13);
            else if (Menu.SubItem > 4) O = LGSR.GetID(Options.LanguageID,14);
            else O = LGSR.GetID(Options.LanguageID,15);
            break;
        case GSR_MENU_ALARM3:
            if (Menu.SubItem > 0 && Menu.SubItem < 4) O = LGSR.GetID(Options.LanguageID,16);
            else if (Menu.SubItem == 4) O = LGSR.GetID(Options.LanguageID,17);
            else if (Menu.SubItem > 4) O = LGSR.GetID(Options.LanguageID,18);
            else O = LGSR.GetID(Options.LanguageID,19);
            break;
        case GSR_MENU_ALARM4:
            if (Menu.SubItem > 0 && Menu.SubItem < 4) O = LGSR.GetID(Options.LanguageID,20);
            else if (Menu.SubItem == 4) O = LGSR.GetID(Options.LanguageID,21);
            else if (Menu.SubItem > 4) O = LGSR.GetID(Options.LanguageID,22);
            else O = LGSR.GetID(Options.LanguageID,23);
            break;
        case GSR_MENU_TONES:
            O = LGSR.GetID(Options.LanguageID,24);
            break;
        case GSR_MENU_TIMEDN:
            O = LGSR.GetID(Options.LanguageID,25);
            break;
        case GSR_MENU_TMEDCF:
            O = LGSR.GetID(Options.LanguageID,119);
            break;
        case GSR_MENU_TIMEUP:
            O = LGSR.GetID(Options.LanguageID,26);
            break;
        case GSR_MENU_STYL:
            O = LGSR.GetID(Options.LanguageID,27);
            break;
        case GSR_MENU_DISP:
            O = LGSR.GetID(Options.LanguageID,28);
            break;
        case GSR_MENU_LANG:
            O = LGSR.GetID(Options.LanguageID,29);
            break;
        case GSR_MENU_BRDR:
            O = LGSR.GetID(Options.LanguageID,30);
            break;
        case GSR_MENU_SIDE:
            O = LGSR.GetID(Options.LanguageID,31);
            break;
        case GSR_MENU_SWAP:
            O = LGSR.GetID(Options.LanguageID,32);
            break;
        case GSR_MENU_ORNT:
            O = LGSR.GetID(Options.LanguageID,33);
            break;
        case GSR_MENU_MODE:
            O = LGSR.GetID(Options.LanguageID,34);
            break;
        case GSR_MENU_FEED:
            O = LGSR.GetID(Options.LanguageID,35);
            break;
        case GSR_MENU_TRBO:
            O = LGSR.GetID(Options.LanguageID,36);
            break;
        case GSR_MENU_DARK:
            switch(Menu.SubItem){
                case 0:
                    O = LGSR.GetID(Options.LanguageID,37);
                    break;
                case 1:
                    O = LGSR.GetID(Options.LanguageID,38);
                    break;
                case 2:
                    O = LGSR.GetID(Options.LanguageID,39);
                    break;
                case 3:
                    O = LGSR.GetID(Options.LanguageID,40);
                    break;
                case 4:
                    O = LGSR.GetID(Options.LanguageID,41);
                    break;
                case 5:
                    O = LGSR.GetID(Options.LanguageID,42);
            }
            break;
        case GSR_MENU_SAVE:
            O = LGSR.GetID(Options.LanguageID,43);
            break;
        case GSR_MENU_TPWR:
            O = LGSR.GetID(Options.LanguageID,44);
            break;
        case GSR_MENU_INFO:
            O = LGSR.GetID(Options.LanguageID,45);
            break;
        case GSR_MENU_TRBL:
            O = LGSR.GetID(Options.LanguageID,46);
            break;
        case GSR_MENU_SYNC:
            O = LGSR.GetID(Options.LanguageID,47);
            break;
        case GSR_MENU_WIFI:
            O = LGSR.GetID(Options.LanguageID,48);
            break;
        case GSR_MENU_OTAU:
            if (Menu.SubItem == 2 || Menu.SubItem == 3) O = LGSR.GetID(Options.LanguageID,49); else O = LGSR.GetID(Options.LanguageID,50);
            break;
        case GSR_MENU_OTAM:
            if (Menu.SubItem == 2 || Menu.SubItem == 3) O = LGSR.GetID(Options.LanguageID,51); else O = LGSR.GetID(Options.LanguageID,52);
            break;
        case GSR_MENU_SCRN:
            O = LGSR.GetID(Options.LanguageID,53);
            break;
        case GSR_MENU_RSET:
            O = LGSR.GetID(Options.LanguageID,54);
            break;
        case GSR_MENU_TOFF:
            if (WatchTime.DeadRTC) O = LGSR.GetID(Options.LanguageID,55); else O = LGSR.GetID(Options.LanguageID,56);
            break;
        case GSR_MENU_UNVS:
            switch (Menu.SubItem){
                case 0:
                    O = LGSR.GetID(Options.LanguageID,57);
                    break;
                case 1:
                    O = LGSR.GetID(Options.LanguageID,58);
                    break;
                case 2:
                    O = LGSR.GetID(Options.LanguageID,59);
                    break;
            }
    }
    setFontFor(O, Design.Menu.Font, Design.Menu.FontSmall, Design.Menu.FontSmaller, Design.Menu.Gutter);
    display.getTextBounds(O, 0, Design.Menu.Header + Design.Menu.Top, &x1, &y1, &w, &h);
    w = (196 - w) /2;
    display.setCursor(w + 2, Design.Menu.Header + Design.Menu.Top);
    display.print(O);
    display.setTextColor(GxEPD_BLACK);  // Only show menu in Light mode
    if (Menu.Item == GSR_MENU_STEPS){  //Steps
        switch (Menu.SubItem){
            case 0: // Steps.
              S = ""; if (Steps.Yesterday > 0) S = " (" + MakeSteps(Steps.Yesterday) + ")";
              O = MakeSteps(SBMA.getCounter()) + S;
              break;
            case 1: // Hour.
              O="[" + MakeHour(Steps.Hour) + "]:" + MakeMinutes(Steps.Minutes) + MakeTOD(Steps.Hour, true);
              break;
            case 2: // x0 minutes.
              S=MakeMinutes(Steps.Minutes);
              O=MakeHour(Steps.Hour) + ":[" + S.charAt(0) + "]" + S.charAt(1) + MakeTOD(Steps.Hour, true);
              break;
            case 3: // 0x minutes.
              S=MakeMinutes(Steps.Minutes);
              O=MakeHour(Steps.Hour) + ":" + S.charAt(0) + "[" + S.charAt(1) + "]" + MakeTOD(Steps.Hour, true);
              break;
            case 4: // Sunday.
              O = LGSR.GetID(Options.LanguageID,60);
          }
    }else if (Menu.Item == GSR_MENU_ALARMS){
            O = LGSR.GetID(Options.LanguageID,61);
    }else if (Menu.Item >= GSR_MENU_ALARM1 && Menu.Item <= GSR_MENU_ALARM4){  // Alarms
        O = "";
        S=MakeMinutes(Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1]);
        switch (Menu.SubItem){
            case 0: // Menu to Edit.
              O = LGSR.GetID(Options.LanguageID,62);
              break;
            case 1: // Hour.
              O="[" + MakeHour(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]) + "]:" + S + MakeTOD(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1], false) + " " + getReduce(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1]);
              break;
            case 2: // x0 minutes.
              O=MakeHour(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]) + ":[" + S.charAt(0) + "]" + S.charAt(1) + MakeTOD(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1], false) + " " + getReduce(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1]);
              break;
            case 3: // 0x minutes.
              O=MakeHour(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]) + ":" + S.charAt(0) + "[" + S.charAt(1) + "]" + MakeTOD(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1], false) + " " + getReduce(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1]);
              break;
            case 4: // Repeats
              O=MakeHour(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]) + ":" + S.charAt(0) + S.charAt(1) + MakeTOD(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1], false) + " [" + getReduce(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1]) + "]";
              break;
            case 5: // Sunday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[0]) == Bits[0]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,0);
              break;
            case 6: // Monday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[1]) == Bits[1]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,1);
              break;
            case 7: // Tuesday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[2]) == Bits[2]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,2);
              break;
            case 8: // Wedmesday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[3]) == Bits[3]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,3);
              break;
            case 9: // Thursday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[4]) == Bits[4]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,4);
              break;
            case 10: // Friday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[5]) == Bits[5]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,5);
              break;
            case 11: // Saturday.
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & Bits[6]) == Bits[6]) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetWeekday(Options.LanguageID,6);
              break;
            case 12:  // Repeat
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & GSR_ALARM_REPEAT) == GSR_ALARM_REPEAT) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetID(Options.LanguageID,63);
              break;
            case 13:  // Active
              display.setTextColor(((Alarms_Active[Menu.Item - GSR_MENU_ALARM1] & GSR_ALARM_ACTIVE) == GSR_ALARM_ACTIVE) ? GxEPD_WHITE : GxEPD_BLACK);
              O=LGSR.GetID(Options.LanguageID,64);
        }
    }else if (Menu.Item == GSR_MENU_TONES){   // Repeats on Alarms.
        O = getReduce(Options.MasterRepeats) + " " + LGSR.GetID(Options.LanguageID,65);
    }else if (Menu.Item == GSR_MENU_TIMERS){  // Timers
        O = LGSR.GetID(Options.LanguageID,66);
    }else if (Menu.Item == GSR_MENU_TIMEDN){ // Countdown
        if (TimerDown.Active && !WatchTime.DeadRTC) UpdateUTC(true);
        S=MakeMinutes(TimerDown.Active ? TimerDown.Mins : TimerDown.MaxMins);
        T=MakeMinutes(TimerDown.Active ? TimerDown.Secs : TimerDown.MaxSecs);
        switch (Menu.SubItem){
            case 0:
                O = LGSR.GetID(Options.LanguageID,62);
                break;
            case 1: // Hours
                O="[" + String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + "]:" + S + ":" + T + " " + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69));
                break;
            case 2: // 1x minutes.
                O=String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + ":[" + S.charAt(0) + "]" + S.charAt(1) + ":" + T.charAt(0) + T.charAt(1) + " " + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69));
                break;
            case 3: // x1 minutes.
                O=String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + ":" + S.charAt(0) + "[" + S.charAt(1) + "]:" + T.charAt(0) + T.charAt(1) + " " + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69));
                break;
            case 4: // 1x seconds.
                O=String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + ":" + S.charAt(0) + S.charAt(1) + ":[" + T.charAt(0) + "]" + T.charAt(1) + " " + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69));
                break;
            case 5: // x1 seconds..
                O=String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + ":" + S.charAt(0) + S.charAt(1) + ":" + T.charAt(0) + "[" + T.charAt(1) + "] " + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69));
                break;
            case 6: // Button.
                O=String(TimerDown.Active ? TimerDown.Hours : TimerDown.MaxHours) + ":" + S + ":" + T + " [" + LGSR.GetID(Options.LanguageID, (TimerDown.Active ? 68 : 69)) + "]";
        }
    }else if (Menu.Item == GSR_MENU_TMEDCF){ // Countdown Settings
        switch (Menu.SubItem){
            case 0:
                O = LGSR.GetID(Options.LanguageID,62);
                break;
            case 1: // Repeatable.
               O="[" + LGSR.GetID(Options.LanguageID,(TimerDown.Repeats ? 118 : 117)) + "] " + getReduce(TimerDown.MaxTones);
               break;
            case 2: // %
               O=LGSR.GetID(Options.LanguageID,(TimerDown.Repeats ? 118 : 117)) + " [" + getReduce(TimerDown.MaxTones) + "]";
                break;
        }
    }else if (Menu.Item == GSR_MENU_TIMEUP){ // Elapsed
        switch (Menu.SubItem){
            case 0:
                O = LGSR.GetID(Options.LanguageID,62);
                break;
            case 1:
                if (TimerUp.Active) { if(!WatchTime.DeadRTC) UpdateUTC(true); U = (WatchTime.UTC_RAW - TimerUp.SetAt); } else U = (TimerUp.StopAt - TimerUp.SetAt);
                y1 = U / 3600; x1 = (U - (y1 * 3600)) / 60; z1 = U % 60;
                O = String(y1) + ":" + MakeMinutes(x1) + ":" + MakeMinutes(z1) + " [" + LGSR.GetID(Options.LanguageID, (TimerUp.Active ? 68 : 69)) + "]";
        }
    }else if (Menu.Item == GSR_MENU_OPTIONS){ // Options Menu
        O = LGSR.GetID(Options.LanguageID,70);
    }else if (Menu.Item == GSR_MENU_STYL){  // Switch Watch Style
        O = "";
        for (x1 = 0; x1 < 32; x1++) O = O + WatchStyles.Style[Options.WatchFaceStyle * 32 + x1];
        O = LGSR.LangString(O,false,Options.LanguageID,0,1); // Only translate the onboard ones.
    }else if (Menu.Item == GSR_MENU_LANG){  // Show current Language
          O = LGSR.GetLangName(Options.LanguageID);
    }else if (Menu.Item == GSR_MENU_DISP){  // Switch Mode
          O = LGSR.GetID(Options.LanguageID,(Options.LightMode ? 71 : 72));
    }else if (Menu.Item == GSR_MENU_SIDE){  // Dexterity Mode
          O = LGSR.GetID(Options.LanguageID,(Options.Lefty ? 73 : 74));
    }else if (Menu.Item == GSR_MENU_SWAP){  // Swap Menu/Back Buttons
          O = LGSR.GetID(Options.LanguageID,(Options.Swapped ? 75 : 76));
    }else if (Menu.Item == GSR_MENU_BRDR){  // Border Mode
          O = LGSR.GetID(Options.LanguageID,(Options.Border) ? 72 : 71);
    }else if (Menu.Item == GSR_MENU_ORNT){  // Watchy Orientation.
          O = LGSR.GetID(Options.LanguageID,(Options.Orientated ? 77 : 78));
    }else if (Menu.Item == GSR_MENU_MODE){  // 24hr Format Swap.
          O = LGSR.GetID(Options.LanguageID,(Options.TwentyFour ? 79 : 80));
    }else if (Menu.Item == GSR_MENU_FEED){  // Feedback
        if (Options.Feedback){
            O = LGSR.GetID(Options.LanguageID,81);
        }else {
            O = LGSR.GetID(Options.LanguageID,(WatchTime.DeadRTC ? 82 : 83));
        }
    }else if (Menu.Item == GSR_MENU_TRBO){  // Turbo!
        if (Options.Turbo > 0 && !WatchTime.DeadRTC) O=String(Options.Turbo) + " " + MakeSeconds(Options.Turbo); else O = LGSR.GetID(Options.LanguageID,68);
    }else if (Menu.Item == GSR_MENU_DARK){  // Dark Running.
            switch(Menu.SubItem){
                case 0:
                    O = LGSR.GetID(Options.LanguageID,67);
                    break;
                case 1:
                    switch (Options.SleepStyle){
                        case 0:
                            O = LGSR.GetID(Options.LanguageID,83);
                            break;
                        case 1:
                            O = LGSR.GetID(Options.LanguageID,84);
                            break;
                        case 2:
                            O = LGSR.GetID(Options.LanguageID,85);
                            break;
                        case 3:
                            O = LGSR.GetID(Options.LanguageID,86);
                            break;
                        case 4:
                            O = LGSR.GetID(Options.LanguageID,87);
                    }
                    break;
                case 2:
                    O = String(Options.SleepMode) + " " + MakeSeconds(Options.SleepMode);
                    break;
                case 3:
                    O = "[" + MakeHour(Options.SleepStart) + MakeTOD(Options.SleepStart, true) + "] " + LGSR.GetID(Options.LanguageID,88) + " " + MakeHour(Options.SleepEnd) + MakeTOD(Options.SleepEnd, true);
                    break;
                case 4:
                    O = MakeHour(Options.SleepStart) + MakeTOD(Options.SleepStart, true) + " " + LGSR.GetID(Options.LanguageID,88) + " [" + MakeHour(Options.SleepEnd) + MakeTOD(Options.SleepEnd, true) + "]";
                    break;
                case 5:
                    O = LGSR.GetID(Options.LanguageID,(Options.BedTimeOrientation ? 77 : 78));
            }
    }else if (Menu.Item == GSR_MENU_TPWR){  // WiFi Tx Power
        O = IDWiFiTX[getTXOffset(GSRWiFi.TransmitPower)];
    }else if (Menu.Item == GSR_MENU_INFO){  // Information
        switch (Menu.SubItem){
            case 0:
                O = LGSR.GetID(Options.LanguageID,89) + ": " + String(Build);
                break;
            case 1:
                O = LGSR.GetID(Options.LanguageID,90) + ": " + String(Battery.Last - (Battery.Last > GSR_MaxBattery ? 1.00 : 0.00)) + "V";
                break;
            case 2:
                if (HWVer > 1.0){ O = (HWVer == 2.0) ? "V2.0 PCF8563" : "V1.5 PCF8563"; } else O = "V1 DS3231M";
                break;
        }
    }else if (Menu.Item == GSR_MENU_SAVE){  // Performance
        if (Options.Performance == 2 || WatchTime.DeadRTC) O = LGSR.GetID(Options.LanguageID,91);
        else O = LGSR.GetID(Options.LanguageID,(Options.Performance == 1 ? 76 : 92));
    }else if (Menu.Item == GSR_MENU_TRBL){  // Troubleshooting.
        O = LGSR.GetID(Options.LanguageID,70);
    }else if (Menu.Item == GSR_MENU_SYNC){  // NTP
        if (Menu.SubItem == 0){
            O = LGSR.GetID(Options.LanguageID,93);
        }else if (Menu.SubItem == 1){
            O = LGSR.GetID(Options.LanguageID,94);
        }else if (Menu.SubItem == 2){
            O = LGSR.GetID(Options.LanguageID,95);
        }else if (Menu.SubItem == 3){
            O = LGSR.GetID(Options.LanguageID,96);
        }
    }else if (Menu.Item == GSR_MENU_WIFI){ // Reset Steps.
        if (Menu.SubItem == 0){
            O = LGSR.GetID(Options.LanguageID,97);
        }else if (Menu.SubItem == 1){
            O = LGSR.GetID(Options.LanguageID,98);
        }else if (Menu.SubItem == 2){
            O = WiFi_AP_SSID;
        }else if (Menu.SubItem == 3){
            O = WiFi.softAPIP().toString();
        }else if (Menu.SubItem == 4){
            O = LGSR.GetID(Options.LanguageID,99);
        }
    }else if (Menu.Item == GSR_MENU_OTAU || Menu.Item == GSR_MENU_OTAM){  // OTA Update.
        if (Menu.SubItem == 0){
            O = LGSR.GetID(Options.LanguageID,100);
        }else if (Menu.SubItem == 1){
            O = LGSR.GetID(Options.LanguageID,101);
        }else if (Menu.SubItem == 2 || Menu.SubItem == 3){
            O = WiFi.localIP().toString();
        }
    }else if (Menu.Item == GSR_MENU_SCRN){  // Reset Screen.
        O = LGSR.GetID(Options.LanguageID,60);
    }else if (Menu.Item == GSR_MENU_RSET){  // Reboot Watchy
        switch (Menu.SubItem){
            case 1:
                O = LGSR.GetID(Options.LanguageID,102);
                break;
            default:
                O = LGSR.GetID(Options.LanguageID,103);
        }
    }else if (Menu.Item == GSR_MENU_TOFF){  // Time Travel detect.
        switch (Menu.SubItem){
            case 0:
                if (WatchTime.DeadRTC) O = LGSR.GetID(Options.LanguageID,67); else O = LGSR.GetID(Options.LanguageID,93);
                break;
            case 1:
                O = LGSR.GetID(Options.LanguageID,104);
                break;
            case 2:
                O = LGSR.GetID(Options.LanguageID,105);
                break;
            case 3:
                if (WatchTime.DeadRTC) O = LGSR.GetID(Options.LanguageID,106); else { if (Options.UsingDrift){ if (Options.Drift< 0) O = "+" + String(0 - Options.Drift) + " " + MakeSeconds(0 - Options.Drift); else O = "-" + String(Options.Drift) + " " + MakeSeconds(Options.Drift); } else O = LGSR.GetID(Options.LanguageID,107); }
        }
    }else if (Menu.Item == GSR_MENU_UNVS){  // USE NVS
        switch (Menu.SubItem){
            case 0:
                O = LGSR.GetID(Options.LanguageID,(OkNVS(GName) ? 108 : 109));
                break;
            case 1:
                O = LGSR.GetID(Options.LanguageID,(Menu.SubSubItem == 0 ? 110 : 67));
                break;
            case 2:
                O = LGSR.GetID(Options.LanguageID,111);
                break;
        }
    }

    if (O > ""){
        setFontFor(O, Design.Menu.Font, Design.Menu.FontSmall, Design.Menu.FontSmaller, Design.Menu.Gutter);
        display.getTextBounds(O, 0, Design.Menu.Data + Design.Menu.Top, &x1, &y1, &w, &h);
        w = (196 - w) /2;
        display.setCursor(w + 2, Design.Menu.Data + Design.Menu.Top);
        display.print(O);
    }
}

void WatchyGSR::drawData(String dData, byte Left, byte Bottom, WatchyGSR::DesOps Style, byte Gutter, bool isTime, bool PM){
    uint16_t w, Width, Height, Ind;
    int16_t X, Y;

    display.getTextBounds(dData, Left, Bottom, &X, &Y, &Width, &Height);

    Bottom = constrain(Bottom, Gutter, 200 - Gutter);
    switch (Style){
        case WatchyGSR::dLEFT:
            Left = Gutter;
            break;
        case WatchyGSR::dRIGHT:
            Left = constrain(200 - (Gutter + Width), Gutter, 200 - Gutter);
            break;
        case WatchyGSR::dSTATIC:
            Left = constrain(Left, Gutter, 200 - Gutter);
            break;
        case WatchyGSR::dCENTER:
            Left = constrain(4 + ((196 - (Gutter + Width)) / 2), Gutter, 200 - Gutter);
            break;
    };
    display.setCursor(Left, Bottom);
    display.print(dData);

    if (isTime && PM){
        if (Style == WatchyGSR::dRIGHT) Left = constrain(Left - 12, Gutter, 200 - Gutter);
        else Left = constrain(Left + Width + 6, Gutter, 190);
        display.drawBitmap(Left, Bottom - Design.Face.TimeHeight, PMIndicator, 6, 6, ForeColor());
    }
}

void WatchyGSR::setFontFor(String O, const GFXfont *Normal, const GFXfont *Small, const GFXfont *Smaller, byte Gutter){
    int16_t  x1, y1;
    uint16_t w, h;
    display.setTextWrap(false);
    byte wi = (200 - (2 * Gutter));
    display.setFont(Normal); display.getTextBounds(O, 0, 0, &x1, &y1, &w, &h);
    if (w > wi) { display.setFont(Small); display.getTextBounds(O, 0, 0, &x1, &y1, &w, &h); }
    if (w > wi) { display.setFont(Smaller); display.getTextBounds(O, 0, 0, &x1, &y1, &w, &h); }
 }

void WatchyGSR::deepSleep(){
  uint8_t I, N, D, H; // 2.1
  bool BatOk, BT,B, DM;
  UpdateUTC(); UpdateClock(); // 2.1

  B = false; VibeTo(false);
  UpdateBMA(); GoDark();
  DM = (Darkness.Went && !TimerDown.Active && GuiMode != GSR_MENUON);

  if (DM){
    D = WatchTime.Local.Wday + 1; // 2.1
    H = WatchTime.Local.Hour;   // 2.1
    BatOk = (Battery.Last == 0 || Battery.Last > Battery.LowLevel);
    BT = (Options.SleepStyle == 2 && WatchTime.BedTime);
    B = (((Options.SleepStyle == 1 || (Options.SleepStyle > 2 && Options.SleepStyle != 4)) || BT) && BatOk);
    if (Battery.Direction == 1) N = (WatchTime.UTC.Minute - (WatchTime.UTC.Minute%5) + 5); else N = (WatchTime.UTC.Minute < 30 ? 30 : 60);
    if (WatchTime.NextAlarm != 99){ if (Alarms_Minutes[WatchTime.NextAlarm] >= WatchTime.Local.Minute && Alarms_Minutes[WatchTime.NextAlarm] < N) N = Alarms_Minutes[WatchTime.NextAlarm]; }
    if (N == 60){ // 2.1
        H = (H + 1) % 24;
        if (H == 0) D = ((D + 1) > 7 ? 1 : D + 1);
    } // 2.1
  }

  if (Options.NeedsSaving) RecordSettings();
  DisplaySleep();
  if (DM) SRTC.atMinuteWake(N % 60, H, D); else SRTC.nextMinuteWake();  // (2.1) Moved to fix PCF8563 issue with pinMode thanks to ZeroKelvinKeyboard for the find.
  //if (DM) SRTC.atMinuteWake(N); else SRTC.nextMinuteWake();
  //for(I = 0; I < 40; I++) { pinMode(I, INPUT); }
  esp_sleep_enable_ext0_wakeup((gpio_num_t)GSR_RTC_INT_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup((B ? SBMA.WakeMask() : 0) | BTN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press  ... |ACC_INT_MASK
  esp_deep_sleep_start();
}

void WatchyGSR::GoDark(){
  if ((Updates.Drawn || Battery.Direction != Battery.DarkDirection || Battery.State != Battery.DarkDirection || !Darkness.Went || Battery.Last < Battery.LowLevel) && !Showing())
  {
    Darkness.Went=true;
    Darkness.Woke=false;
    Darkness.Tilt=0;
    Updates.Init=Updates.Drawn;
    display.setFullWindow();
    DisplayInit(true);  // Force it here so it fixes the border.
    display.fillScreen(GxEPD_BLACK);
    if (!OverrideSleepBitmap()) if (Design.Face.SleepBitmap) display.drawBitmap(0, 0, Design.Face.SleepBitmap, 200, 200, ForeColor(), BackColor());
    drawChargeMe(true);
    Battery.DarkDirection = Battery.Direction;
    Battery.DarkState = Battery.State;
    display.display(true);
    Updates.Drawn=false;
    display.hibernate();
    LastHelp = 0;
  }
}

void WatchyGSR::detectBattery(){
    float CBAT, BATOff;
    CBAT = getBatteryVoltage(); // Check battery against previous versions to determine which direction the battery is going.
    BATOff = CBAT - Battery.Last;
     //Detect Power direction

    if (BATOff > 0.03){
      Battery.UpCount++;
      if (Battery.UpCount > 3){
            if (Battery.Direction == 1) Battery.Last = CBAT;
            Battery.Direction = 1; Battery.UpCount = 0; Battery.DownCount = 0;
            // Check if the NTP has been done.
            if (WatchTime.UTC_RAW - NTPData.Last > 14400 && NTPData.State == 0){
                NTPData.State = 1;
                NTPData.TimeZone = (OP.getCurrentPOSIX() == OP.TZMISSING);   // No Timezone, go get one!
                NTPData.UpdateUTC = true;
                AskForWiFi();
            }
        }
    }else{
        if (BATOff < 0.00) Battery.DownCount++;
        if (Battery.DownCount > 2)
        {
            if (Battery.Direction == -1) Battery.Last = CBAT;
            Battery.Direction = -1; Battery.UpCount = 0; Battery.DownCount = 0;
        }
    }
    // Do battery state here.
    if (Battery.Direction > 0) Battery.State = 3;
    else Battery.State = (Battery.Last > Battery.MinLevel ? (Battery.Last > Battery.LowLevel ? 0 : 1) : 2);
}

void WatchyGSR::ProcessNTP(){
  bool B;

  // Do ProgressNTP here.
  switch (NTPData.State){
    // Start WiFi and Connect.
    case 1:{
      HTTP.setUserAgent(UserAgent);
      if (WiFi.status() != WL_CONNECTED){
          if(currentWiFi() == WL_CONNECT_FAILED){
              NTPData.Pause = 0;
              NTPData.State = 99;
              break;
          }
      }
      NTPData.Wait = 0;
      NTPData.Pause = 80;
      NTPData.State++;
      break;
    }

    // Am I Connected?  If so, ask for NTP.
    case 2:{
      if (WiFi.status() != WL_CONNECTED){
          if(currentWiFi() == WL_CONNECT_FAILED){
              NTPData.Pause = 0;
              NTPData.State = 99;
              break;
          }
          NTPData.Pause = 80;
          if (NTPData.Wait > 2){
              NTPData.Pause = 0;
              NTPData.State = 99;
              break;
          }
          break;
      }
      if (NTPData.TimeZone == false){
          NTPData.State = 5;
          NTPData.Pause = 0;
          break;
      }

      NTPData.State++;
      setStatus("TZ");
      // Do the next part.
      OP.beginOlsonFromWeb();
      NTPData.Wait = 0;
      NTPData.Pause = 20;
      break;
  }

    case 3:{
      // Get GeoLocation
      if (WiFi.status() == WL_DISCONNECTED){
          NTPData.Pause = 0;
          NTPData.State = 99;
          break;
      }
      if (OP.gotOlsonFromWeb()) {
        WatchTime.TimeZone = OP.getCurrentOlson();
        OP.endOlsonFromWeb();
        NTPData.Wait = 0;
        NTPData.Pause = 0;
        NTPData.State++;   // Test
      }else if (NTPData.Wait > 0){
          NTPData.Pause = 0;
          NTPData.State = 99;
          OP.endOlsonFromWeb();
      }
      break;
    }

    case 4:{
      // Process Timezone POSIX.
      String NewTZ = OP.getPOSIX(WatchTime.TimeZone);
      if(NewTZ == OP.TZMISSING){
          NTPData.Pause = 0;
          NTPData.State = 99;
          break;
      }
      if (OkNVS(GName)) B = NVS.setString(GTZ,NewTZ);
      OP.setCurrentPOSIX(NewTZ);
      NTPData.Wait = 0;
      NTPData.Pause = 0;
      NTPData.State++;
      if (!NTPData.UpdateUTC) UpdateDisp=Showing();
      break;
    }

    case 5:{
    if (NTPData.UpdateUTC == false || WiFi.status() == WL_DISCONNECTED || NTPData.Wait > 0){
          NTPData.State = 99;
          NTPData.Pause = 0;
          break;
      }
      NTPData.NTPDone = false;
      setStatus("NTP");
      SNTP.Begin(InsertNTPServer()); //"132.246.11.237","132.246.11.227");
      NTPData.Wait = 0;
      NTPData.Pause = 20;
      NTPData.State++;
      break;
    }

    case 6:{
      //if (time(nullptr) < 1000000000l){
      if (!SNTP.Query()){
        if (NTPData.Wait > 0){
            NTPData.Pause = 0;
            NTPData.State = 99;
        }
        break;
      }
      WatchTime.UTC_RAW = SNTP.Results;
      WatchTime.UTC = SNTP.tmResults;
      SRTC.set(WatchTime.UTC);
      WatchTime.Drifting = 0;
      WatchTime.EPSMS = (millis() + (1000 * (60 - WatchTime.UTC.Second)));
      WatchTime.WatchyRTC = esp_timer_get_time() + ((60 - WatchTime.UTC.Second) * 1000000);
      NTPData.NTPDone = true;
      NTPData.Pause = 0;
      NTPData.State = 99;
      break;
    }

    case 99:{
      OTAEnd = true;
      SNTP.End();
      NTPData.Wait = 0;
      NTPData.Pause = 0;
      NTPData.State = 0;
      NTPData.Last = WatchTime.UTC_RAW; // Moved from section 6 to here, to limit the atttempts.
      NTPData.UpdateUTC = false;
      NTPData.TimeZone = false;
      if (NTPData.TimeTest && Menu.Item == GSR_MENU_TOFF) { Menu.SubItem = 2; NTPData.TestCount = 0; }
      if (NTPData.TimeTest) setStatus(""); // Clear status once it is done.
      Battery.UpCount=0;  // Stop it from thinking the battery went wild.
    }
  }
}

void WatchyGSR::drawChargeMe(bool Dark){
  // Shows Battery Direction indicators.
  if (Battery.Direction == 1){
      display.drawBitmap(Design.Status.BATTx, Design.Status.BATTy, Charging, 40, 17, (Dark ? GxEPD_WHITE : ForeColor()));   // Show Battery charging bitmap.
  }else if (Battery.Last < Battery.MinLevel){
      display.drawBitmap(Design.Status.BATTx, Design.Status.BATTy, (Battery.Last < Battery.LowLevel ? ChargeMeBad : ChargeMe), 40, 17, (Dark ? GxEPD_WHITE : ForeColor()));   // Show Battery needs charging bitmap.
  }
}

void WatchyGSR::drawStatus(){
  uint8_t X = Design.Status.WIFIx;
  bool Ok = true;
  if (WatchyStatus > ""){
      Ok = false;
      display.setFont(&Bronova_Regular13pt7b);
      if (WatchyStatus.startsWith("WiFi")){
          display.drawBitmap(X, Design.Status.WIFIy - 18, iWiFi, 19, 19, ForeColor());
          if (WatchyStatus.length() > 4){
              display.setCursor(X + 17, Design.Status.WIFIy);
              display.setTextColor(ForeColor());
              display.print(WatchyStatus.substring(4));
          }
      }
      else if (WatchyStatus == "TZ")  display.drawBitmap(X, Design.Status.WIFIy - 18, iTZ, 19, 19, ForeColor());
      else if (WatchyStatus == "NTP") display.drawBitmap(X, Design.Status.WIFIy - 18, iSync, 19, 19, ForeColor());
      else if (WatchyStatus == "ESP") display.drawBitmap(X, Design.Status.WIFIy - 18, iSync, 19, 19, ForeColor());
      else if (WatchyStatus == "NC")  display.drawBitmap(X, Design.Status.WIFIy - 18, iNoClock, 19, 19, ForeColor());
      else if (Alarms_Times[1] > 0 || Alarms_Times[2] > 0 || Alarms_Times[3] > 0 || Alarms_Times[4] > 0 || TimerDown.ToneLeft > 0) Ok = true;
      else{
          display.setTextColor(ForeColor());
          display.setCursor(X, Design.Status.WIFIy);
          display.print(WatchyStatus);
      }
  }
  if (Ok){
    if (TimerDown.ToneLeft > 0) {display.drawBitmap(X, Design.Status.WIFIy - 15, Tone_C, 12, 14, ForeColor()); X += 13; }
    if (Alarms_Times[0] > 0) {display.drawBitmap(X, Design.Status.WIFIy - 15, Tone_1, 12, 14, ForeColor()); X += 13; }
    if (Alarms_Times[1] > 0) {display.drawBitmap(X, Design.Status.WIFIy - 15, Tone_2, 12, 14, ForeColor()); X += 13; }
    if (Alarms_Times[2] > 0) {display.drawBitmap(X, Design.Status.WIFIy - 15, Tone_3, 12, 14, ForeColor()); X += 13; }
    if (Alarms_Times[3] > 0) {display.drawBitmap(X, Design.Status.WIFIy - 15, Tone_4, 12, 14, ForeColor()); }
  }
}

void WatchyGSR::setStatus(String Status){
    if (Status == "" && !SRTC.isOperating()) Status = "NC";
    if (WatchyStatus != Status){
      WatchyStatus = Status;
      UpdateDisp=Showing();
    }
}

void WatchyGSR::VibeTo(bool Mode){
    if (Mode != VibeMode){
        if (Mode){
//            SBMA.enableFeature(BMA423_WAKEUP, false);
            pinMode(GSR_VIB_MOTOR_PIN, OUTPUT);
            digitalWrite(GSR_VIB_MOTOR_PIN, true);
        }else{
            digitalWrite(GSR_VIB_MOTOR_PIN, false);
//            SBMA.enableFeature(BMA423_WAKEUP,(Options.SleepStyle == 2 && WatchTime.BedTime) || (Options.SleepStyle > 2 && Options.SleepStyle != 4));
        }
        VibeMode = Mode;
    }
}

void WatchyGSR::SoundAlarms(void * parameter){
    bool WaitForNext, Pulse, Used;
    unsigned long M;
    uint8_t AlarmIndex = 0;
    WaitForNext=false;
    Pulse=false;
    AlarmReset=0;

                          // Here, do the alarm buzzings by way of which one is running.
    while (true){
        M = millis();
        Used = false;
        if (!OTAUpdate){
            if (WaitForNext){
                // Wait for the next second to change between any alarms still to play.
                if (M - AlarmReset > 999){ // Time changed.
                    AlarmReset = M;
                    WaitForNext = false;
                    AlarmIndex = roller(AlarmIndex + 1, 0, 4);
                    if (getToneTimes(AlarmIndex) == 0){
                        AlarmIndex = roller(AlarmIndex + 1, 0, 4);
                        if (getToneTimes(AlarmIndex) == 0){
                            AlarmIndex = roller(AlarmIndex + 1, 0, 4);
                            if (getToneTimes(AlarmIndex) == 0){
                                AlarmIndex = roller(AlarmIndex + 1, 0, 4);
                                if (getToneTimes(AlarmIndex) == 0){
                                    AlarmIndex = roller(AlarmIndex + 1, 0, 4);
                                }
                            }
                        }
                    }
                }
            }
            if(AlarmIndex == 4){
                if (TimerDown.ToneLeft > 0){
                    if (TimerDown.Tone == 0){
                        WaitForNext=true;
                        TimerDown.ToneLeft--;
                        if (TimerDown.ToneLeft > 0){
                            Used = true;
                            TimerDown.Tone = 22;
                        }else UpdateDisp=true;
                    }else{
                        Pulse = (((TimerDown.Tone / 2) & 1) != 0);
                        TimerDown.Tone--;
                        if (TimerDown.Tone == 0 && TimerDown.ToneLeft == 0) UpdateDisp=true;
                        Used = true;
                        if (!Pulse && TimerDown.Tone > 0) TimerDown.Tone--;
                        VibeTo(Pulse);   // Turns Vibe on or off depending on bit state.
                        if (Pulse) { Darkness.Last=M; HapticMS = 0; }
                    }
                }else WaitForNext=true;
            }else if (Alarms_Times[AlarmIndex] > 0){
                if (Alarms_Playing[AlarmIndex] > 0){
                    Alarms_Playing[AlarmIndex]--;
                    if (Menu.SubItem > 0 && Menu.Item - GSR_MENU_ALARM1 == AlarmIndex){
                        VibeTo(false);
                        Alarms_Playing[AlarmIndex]=0;
                        Alarms_Times[AlarmIndex]=0;
                        UpdateDisp=true;
                    }else{
                        Used = true;
                        Pulse = ((AlarmVBs[AlarmIndex] & Bits[Alarms_Playing[AlarmIndex] / 3]) != 0);
                        VibeTo(Pulse);   // Turns Vibe on or off depending on bit state.
                        if (Pulse) { Darkness.Last=M; HapticMS = 0; }
                    }
                    if (Alarms_Playing[AlarmIndex] == 0 && Alarms_Times[AlarmIndex] > 0){
                        Alarms_Times[AlarmIndex]--;   // Decrease count, eventually this will all stop on it's own.
                        WaitForNext = true;
                        if (Alarms_Times[AlarmIndex] > 0) { Darkness.Last=M; Alarms_Playing[AlarmIndex] = 30; Used = true; } else UpdateDisp=true;
                    }
                }else WaitForNext = true;
            }else WaitForNext = true;
        }
        AllowHaptic = !Used;
        if (Used) HapticMS = 0;
        M = (50-(millis() - M));
        M += millis();
        while (M > millis()) {
            if (AllowHaptic && HapticMS > 0){
              HapticMS--; VibeTo((HapticMS > 0));
            }
            delay(5);
        }
    }
}

void WatchyGSR::handleButtonPress(uint8_t Pressed){
  uint8_t I;
  int ml, mh;

  if (Darkness.Went && Options.SleepStyle == 4 && !WatchTime.DeadRTC && !Updates.Tapped) return; // No buttons unless a tapped happened.
  if (!UpRight()) return; // Don't do buttons if not upright.
  if (Pressed < 5 && LastButton > 0 && (millis() - LastButton) < GSR_KEYPAUSE) return;
  if (Darkness.Went && !Darkness.Woke) { Darkness.Woke=true; Darkness.Last=millis(); Darkness.Tilt = Darkness.Last; UpdateUTC(); UpdateClock(); UpdateDisp=true; return; }  // Don't do the button, just exit.
  if ((NTPData.TimeTest || OTAUpdate) && (Pressed == 3 || Pressed == 4)) return;  // Up/Down don't work in these modes.

  switch (Pressed){
    case 1:
          if (GuiMode != GSR_MENUON && !MenuOverride ){ // If MenuOverride is on, it will not let the menu work, meaning, unless it is open already, it won't open.
            GuiMode = GSR_MENUON;
            DoHaptic = true;
            UpdateDisp = true;  // Quick Update.
            SetTurbo();
          }else if (GuiMode == GSR_MENUON){
              if (Menu.Item == GSR_MENU_OPTIONS && Menu.SubItem == 0){  // Options
                  Menu.Item = GSR_MENU_STYL;
                  Menu.Style = GSR_MENU_INOPTIONS;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_TRBL && Menu.SubItem == 0){  // Troubleshooting.
                  Menu.Style = GSR_MENU_INTROUBLE;
                  Menu.Item = GSR_MENU_SCRN;
                  DoHaptic = true;
                  UpdateDisp = true;
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_STEPS){ // Steps
                  if (Menu.SubItem == 4){
                      SBMA.resetStepCounter();
                      Menu.SubItem = 0;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else if (Menu.SubItem < 4){
                      Menu.SubItem++;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_ALARMS){  // Alarms menu.
                  Menu.Style = GSR_MENU_INALARMS;
                  Menu.Item = GSR_MENU_ALARM1;
                  DoHaptic = true;
                  UpdateDisp = true;
                  SetTurbo();
              }else if (Menu.Item >= GSR_MENU_ALARM1 && Menu.Item <= GSR_MENU_ALARM4){  // Alarms
                  if (Menu.SubItem < 5){
                      Menu.SubItem++;
                      if (Menu.SubItem == 5) Menu.SubItem += WatchTime.Local.Wday; // Jump ahead to the day.
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else if (Menu.SubItem > 4 && Menu.SubItem < 12){
                      Alarms_Active[Menu.Item - GSR_MENU_ALARM1] ^= Bits[Menu.SubItem - 5];  // Toggle day.
                      Options.NeedsSaving = true;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else if (Menu.SubItem == 12){
                      Alarms_Active[Menu.Item - GSR_MENU_ALARM1] ^= GSR_ALARM_REPEAT;  // Toggle repeat.
                      Options.NeedsSaving = true;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else if (Menu.SubItem == 13){
                      Alarms_Active[Menu.Item - GSR_MENU_ALARM1] ^= GSR_ALARM_ACTIVE;  // Toggle Active.
                      Options.NeedsSaving = true;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_TONES){   // Tones.
                      Options.MasterRepeats = roller(Options.MasterRepeats + 1, (WatchTime.DeadRTC ? 4 : 0), 4);
                      Alarms_Repeats[0] = Options.MasterRepeats;
                      Alarms_Repeats[1] = Options.MasterRepeats;
                      Alarms_Repeats[2] = Options.MasterRepeats;
                      Alarms_Repeats[3] = Options.MasterRepeats;
                      Options.NeedsSaving = true;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
              }else if (Menu.Item == GSR_MENU_TIMERS){  // Timers menu.
                  Menu.Style = GSR_MENU_INTIMERS;
                  Menu.Item = GSR_MENU_TIMEDN;
                  DoHaptic = true;
                  UpdateDisp = true;
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_TIMEDN){
                  if (Menu.SubItem == 6){
                      if (TimerDown.Active){
                          TimerDown.Active=false;
                          DoHaptic = true;
                          UpdateDisp = true;  // Quick Update.
                          SetTurbo();
                      }else if ((TimerDown.MaxSecs + TimerDown.MaxMins + TimerDown.MaxHours) > 0){
                          StartCD();
                          DoHaptic = true;
                          UpdateDisp = true;  // Quick Update.
                          SetTurbo();
                      }
                  }else{
                      Menu.SubItem++;
                      if (TimerDown.MaxSecs + TimerDown.MaxMins + TimerDown.MaxHours == 0 && Menu.SubItem == 6) Menu.SubItem = 4; //Stop it from being startable.
                      if (TimerDown.Active) Menu.SubItem = 6;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_TMEDCF){
                  if (Menu.SubItem < 2){
                      Menu.SubItem++;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_TIMEUP){
                  if (Menu.SubItem == 0){
                      Menu.SubItem = 1;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else{
                      if (TimerUp.Active){
                          UpdateUTC();
                          TimerUp.StopAt = WatchTime.UTC_RAW;
                          TimerUp.Active = false;
                      }else{
                          UpdateUTC();
                          TimerUp.SetAt = WatchTime.UTC_RAW;
                          TimerUp.StopAt = TimerUp.SetAt;
                          TimerUp.Active = true;
                      }
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_TPWR && !(GSRWiFi.Requests > 0 || WatchyAPOn)){
                  I = roller(getTXOffset(GSRWiFi.TransmitPower) + 1,0,10);
                  GSRWiFi.TransmitPower = RawWiFiTX[I];
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_INFO){ // Information
                  Menu.SubItem = roller(Menu.SubItem + 1, 0, 2);
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_SAVE && !WatchTime.DeadRTC){  // Battery Saver.
                  Options.Performance = roller(Options.Performance + 1,0,2);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_SYNC){  // Sync Time
                  if (Menu.SubItem == 0){
                      Menu.SubItem++;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }else{
                      NTPData.State = 1;
                      switch (Menu.SubItem){
                        case 1:
                          NTPData.UpdateUTC = true;
                          NTPData.TimeZone = (OP.getCurrentPOSIX() == OP.TZMISSING);   // No Timezone, go get one!
                          break;
                        case 2:
                          NTPData.UpdateUTC = false;
                          NTPData.TimeZone = true;
                          break;
                        case 3:
                          NTPData.TimeZone = true;
                          NTPData.UpdateUTC = true;
                      }
                      GuiMode = GSR_WATCHON;
                      Menu.Item = GSR_MENU_STYL;
                      Menu.SubItem = 0;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                      AskForWiFi();
                  }
              }else if (Menu.Item == GSR_MENU_STYL){  // Switch Watch Face
                  Options.WatchFaceStyle = roller(Options.WatchFaceStyle + 1,0,WatchStyles.Count - 1);
                  initWatchFaceStyle();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_LANG){  // Language switch.
                  I = roller(Options.LanguageID + 1,0,LGSR.MaxLangID());
                  if (I != Options.LanguageID){
                      Options.LanguageID = I;
                      WatchyGSR::initWatchFaceStyle();  // If fonts will need to be changed per language down the road, I don't know.
                      Options.NeedsSaving = true;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_DISP){  // Switch Mode
                  Options.LightMode = !Options.LightMode;
                  Options.NeedsSaving = true;
                  Updates.Full = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_SIDE){  // Dexterity Mode
                  Options.Lefty = !Options.Lefty;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_SWAP){  // Swap Menu/Back Buttons
                  Options.Swapped = !Options.Swapped;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_BRDR){  // Border Mode
#ifdef GxEPD2DarkBorder
                  Options.Border = !Options.Border;
                  Options.NeedsSaving = true;
                  Updates.Init = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
#endif
              }else if (Menu.Item == GSR_MENU_ORNT){  // Watchy Orientation
                  Options.Orientated = !Options.Orientated;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_MODE){  // Switch Time Mode
                  Options.TwentyFour = !Options.TwentyFour;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_FEED && !WatchTime.DeadRTC){  // Feedback.
                  Options.Feedback = !Options.Feedback;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_TRBO && !WatchTime.DeadRTC){  // Turbo
                  Options.Turbo = roller(Options.Turbo + 1, 0, 10);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_DARK){  // Sleep Mode.
                  if (Menu.SubItem < 5){
                      Menu.SubItem++;
                      DoHaptic = true;
                      UpdateDisp = true;  // Quick Update.
                      SetTurbo();
                  }
              }else if (Menu.Item == GSR_MENU_SCRN){  // Reset Screen
                  GuiMode = GSR_WATCHON;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  Updates.Full = true;
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_WIFI && !WatchyAPOn){  // Watchy Connect
                  Menu.SubItem++;
                  WatchyAPOn = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if ((Menu.Item == GSR_MENU_OTAU || Menu.Item == GSR_MENU_OTAM) && !(GSRWiFi.Requests > 0 || WatchyAPOn)){  // Watchy OTA
                  Menu.SubItem++;
                  OTAUpdate=true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  AskForWiFi();
              }else if (Menu.Item == GSR_MENU_RSET){  // Watchy Reboot
                  if (Menu.SubItem == 1) ESP.restart(); else Menu.SubItem++;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_TOFF && NTPData.State == 0 && Menu.SubItem == 0){  // Detect Drift
                  if (WatchTime.DeadRTC){
                      Options.NeedsSaving = true;
                      WatchTime.DeadRTC = false;
                  }else{
                      NTPData.TimeTest = true;
                      NTPData.State = 1;
                      Menu.SubItem = 1;
                      NTPData.UpdateUTC = true;
                      AskForWiFi();
                  }
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.Item == GSR_MENU_UNVS){  // USE NVS
                  if (Menu.SubItem == 0) Menu.SubItem++;
                  else if (Menu.SubItem == 1){
                      if (Menu.SubSubItem == 1){
                          if (OkNVS(GName)) Menu.SubItem == 2;  // delete, request
                          else { SetNVS(GName,true); Menu.SubItem = 0; Menu.SubSubItem = 0; }
                      }else Menu.SubItem = 0; // Keep, don't change.
                  }else if (Menu.SubItem == 2){
                      NVSEmpty();
                      ESP.restart();
                  }
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          } else Missed = 1;
          break;
    case 2:
      if (GuiMode == GSR_MENUON){   // Back Button [SW2]
          if (Menu.Item == GSR_MENU_STEPS && Menu.SubItem > 0) {  // Exit for Steps, back to Steps.
              if (Menu.SubItem == 4) Menu.SubItem = 2;  // Go back to the Hour, so it is the same as the alarms.
              Menu.SubItem--;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item >= GSR_MENU_ALARM1 && Menu.Item <= GSR_MENU_ALARM4 && Menu.SubItem > 0){
              if (Menu.SubItem < 5 && Menu.SubItem > 0){
                  Menu.SubItem--;
              }else if (Menu.SubItem > 4){
                  Menu.SubItem = 1;
              }
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_TIMEDN && Menu.SubItem > 0){
              Menu.SubItem--;
              if (TimerDown.Active) Menu.SubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_TMEDCF && Menu.SubItem > 0){
              Menu.SubItem--;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_TIMEUP && Menu.SubItem > 0){
              Menu.SubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_DARK && Menu.SubItem > 0){  // Sleep Mode.
              Menu.SubItem--;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_SYNC && Menu.SubItem > 0){
              Menu.SubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_WIFI && Menu.SubItem > 0){
              Menu.SubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if ((Menu.Item == GSR_MENU_OTAU || Menu.Item == GSR_MENU_OTAM) && Menu.SubItem > 0){
              break;    // DO NOTHING!
          }else if (Menu.Style == GSR_MENU_INALARMS){  // Alarms
              Menu.Style = GSR_MENU_INNORMAL;
              Menu.Item = GSR_MENU_ALARMS;
              DoHaptic = true;
              UpdateDisp = true;
              SetTurbo();
          }else if (Menu.Style == GSR_MENU_INTIMERS){  // Timers
              Menu.Style = GSR_MENU_INNORMAL;
              Menu.Item = GSR_MENU_TIMERS;
              DoHaptic = true;
              UpdateDisp = true;          
              SetTurbo();
          }else if (Menu.Style == GSR_MENU_INOPTIONS){  // Options
              Menu.SubItem = 0;
              Menu.SubSubItem = 0;
              Menu.Item=GSR_MENU_OPTIONS;
              Menu.Style=GSR_MENU_INNORMAL;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Style == GSR_MENU_INTROUBLE && Menu.SubItem == 0){  // Troubleshooting.
              Menu.SubItem = 0;
              Menu.SubSubItem = 0;
              Menu.Item=GSR_MENU_TRBL;
              Menu.Style=GSR_MENU_INOPTIONS;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_RSET && Menu.SubItem > 0){  // Watchy Reboot
              Menu.SubItem--;
          }else if (Menu.Item == GSR_MENU_TOFF && Menu.SubItem > 0){  // Drift Travel
              if (Menu.SubItem == 3){
                  Menu.SubItem = 0;
                  Menu.SubSubItem = 0;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item == GSR_MENU_UNVS){  // USE NVS
              Menu.SubItem = 0;
              Menu.SubSubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else{
              GuiMode = GSR_WATCHON;
              Menu.SubItem = 0;
              Menu.SubSubItem = 0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }
      } else Missed = 2;  // Missed a SW2.
      break;
    case 3:
      if (GuiMode == GSR_MENUON){     // Up Button [SW3]
          // Handle the sideways choices here.
          if (Menu.Item == GSR_MENU_STEPS && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1: // Hour
                  Steps.Hour=roller(Steps.Hour + 1, 0,23);
                  Options.NeedsSaving = true;
                  Steps.Reset = false;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2: //  x0 Minutes
                  mh = (Steps.Minutes / 10);
                  ml = Steps.Minutes - (mh * 10);
                  mh = roller(mh + 1, 0, 5);
                  Steps.Minutes = (mh * 10) + ml;
                  Steps.Reset = false;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 3: //  x0 Minutes
                  mh = (Steps.Minutes / 10);
                  ml = Steps.Minutes - (mh * 10);
                  ml = roller(ml + 1, 0, 9);
                  Steps.Minutes = (mh * 10) + ml;
                  Steps.Reset = false;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item >= GSR_MENU_ALARM1 && Menu.Item <= GSR_MENU_ALARM4 && Menu.SubItem > 0){
              if (Menu.SubItem == 1){ // Hour
                  Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]=roller(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1] + 1, 0,23);
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem == 2){ //  x0 Minutes
                  mh = (Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] / 10);
                  ml = Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] - (mh * 10);
                  mh = roller(mh + 1, 0, 5);
                  Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] = (mh * 10) + ml;
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem == 3){ //  x0 Minutes
                  mh = (Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] / 10);
                  ml = Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] - (mh * 10);
                  ml = roller(ml + 1, 0, 9);
                  Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] = (mh * 10) + ml;
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem == 4){ //  Repeats.
                  Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1] = roller(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1] - 1, (WatchTime.DeadRTC ? 4 : 0), 4);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem > 4){
                  Menu.SubItem = roller(Menu.SubItem + 1, 5, 13);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          } else if (Menu.Item == GSR_MENU_TIMEDN && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1: // Hour
                  TimerDown.MaxHours=roller(TimerDown.MaxHours + 1, 0,23);
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2: //  x0 Minutes
                  mh = (TimerDown.MaxMins / 10);
                  ml = TimerDown.MaxMins - (mh * 10);
                  mh = roller(mh + 1, 0, 5);
                  TimerDown.MaxMins = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 3: //  x0 Minutes
                  mh = (TimerDown.MaxMins / 10);
                  ml = TimerDown.MaxMins - (mh * 10);
                  ml = roller(ml + 1, 0, 9);
                  TimerDown.MaxMins = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 4: //  x0 Seconds
                  mh = (TimerDown.MaxSecs / 10);
                  ml = TimerDown.MaxSecs - (mh * 10);
                  mh = roller(mh + 1, 0, 5);
                  TimerDown.MaxSecs = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 5: //  x0 Seconds
                  mh = (TimerDown.MaxSecs / 10);
                  ml = TimerDown.MaxSecs - (mh * 10);
                  ml = roller(ml + 1, 0, 9);
                  TimerDown.MaxSecs = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          } else if (Menu.Item == GSR_MENU_TMEDCF && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1:   // Repeatable
                  TimerDown.Repeats = !TimerDown.Repeats;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2:   // Tone Repeats
                  TimerDown.MaxTones = roller(TimerDown.MaxTones - 1, (WatchTime.DeadRTC ? 4 : 0), 4);
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item == GSR_MENU_DARK && Menu.SubItem > 0){  // Sleep Mode.
              switch (Menu.SubItem){
                  case 1: // Style.
                      Options.SleepStyle = roller(Options.SleepStyle + 1, (WatchTime.DeadRTC ? 1 : 0), (WatchTime.DeadRTC ? 2 : 4));
                      Updates.Tapped = true;
                      Options.NeedsSaving = true;
                      break;
                  case 2: // SleepMode (0=off, 10 seconds)
                      Options.SleepMode = roller(Options.SleepMode + 1, 1, 10);
                      Options.NeedsSaving = true;
                      break;
                  case 3: // SleepStart (hour)
                      if (Options.SleepStart + 1 != Options.SleepEnd) { Options.SleepStart = roller(Options.SleepStart + 1, 0,23); Options.NeedsSaving = true; } else return;
                      break;
                  case 4: // SleepStart (hour)
                      if (Options.SleepEnd + 1 != Options.SleepStart) { Options.SleepEnd = roller(Options.SleepEnd + 1, 0,23); Options.NeedsSaving = true; } else return;
                      break;
                  case 5: // Orientation.
                      Options.BedTimeOrientation = !Options.BedTimeOrientation;
                      Options.NeedsSaving = true;
              }
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_SYNC && Menu.SubItem > 0){
              Menu.SubItem = roller(Menu.SubItem - 1, 1, 3);
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_WIFI && Menu.SubItem > 0){
              // Do nothing!
          }else if (Menu.Item == GSR_MENU_TOFF && Menu.SubItem > 0){
              // Do nothing!
          }else if (Menu.Item == GSR_MENU_UNVS && Menu.SubItem == 1){  // USE NVS
              if (Menu.SubSubItem == 1){
                  Menu.SubSubItem = 0;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
              return;
          }else{
              if (Menu.Style == GSR_MENU_INOPTIONS){
                  Menu.Item = roller(Menu.Item - 1, GSR_MENU_STYL, (NTPData.State > 0 || WatchyAPOn || OTAUpdate || Battery.Last < Battery.MinLevel) ? GSR_MENU_TRBL : GSR_MENU_OTAM);
              }else if (Menu.Style == GSR_MENU_INALARMS){
                  Menu.Item = roller(Menu.Item - 1, GSR_MENU_ALARM1, GSR_MENU_TONES);
              }else if (Menu.Style == GSR_MENU_INTIMERS){
                  Menu.Item = roller(Menu.Item - 1, GSR_MENU_TIMEDN, GSR_MENU_TIMEUP);
              }else if (Menu.Style == GSR_MENU_INTROUBLE){
                  Menu.Item = roller(Menu.Item - 1, GSR_MENU_SCRN, GSR_MENU_UNVS);
              }else{
                  Menu.Item = roller(Menu.Item - 1, GSR_MENU_STEPS, GSR_MENU_OPTIONS);
              }
              Menu.SubItem=0;
              Menu.SubSubItem=0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }
      } else Missed = 3;  // Missed a SW3.
      break;
    case 4:
      if (GuiMode == GSR_MENUON){   // Down Button [SW4]
          // Handle the sideways choices here.
          if (Menu.Item == GSR_MENU_STEPS && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1: // Hour
                  Steps.Hour=roller(Steps.Hour - 1, 0,23);
                  Options.NeedsSaving = true;
                  Steps.Reset = false;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2: //  x0 Minutes
                  mh = (Steps.Minutes / 10);
                  ml = Steps.Minutes - (mh * 10);
                  mh = roller(mh - 1, 0, 5);
                  Steps.Minutes = (mh * 10) + ml;
                  Steps.Reset = false;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 3: //  x0 Minutes
                  mh = (Steps.Minutes / 10);
                  ml = Steps.Minutes - (mh * 10);
                  ml = roller(ml - 1, 0, 9);
                  Steps.Minutes = (mh * 10) + ml;
                  Steps.Reset = false;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item >= GSR_MENU_ALARM1 && Menu.Item <= GSR_MENU_ALARM4 && Menu.SubItem > 0){
              if (Menu.SubItem == 1){ // Hour
                  Alarms_Hour[Menu.Item - GSR_MENU_ALARM1]=roller(Alarms_Hour[Menu.Item - GSR_MENU_ALARM1] - 1, 0,23);
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
             }else if (Menu.SubItem == 2){ //  x0 Minutes
                  mh = (Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] / 10);
                  ml = Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] - (mh * 10);
                  mh = roller(mh - 1, 0, 5);
                  Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] = (mh * 10) + ml;
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem == 3){ //  x0 Minutes
                  mh = (Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] / 10);
                  ml = Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] - (mh * 10);
                  ml = roller(ml - 1, 0, 9);
                  Alarms_Minutes[Menu.Item - GSR_MENU_ALARM1] = (mh * 10) + ml;
                  Alarms_Active[Menu.Item - GSR_MENU_ALARM1] &= GSR_ALARM_NOTRIGGER;
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem == 4){ //  Repeats.
                  Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1] = roller(Alarms_Repeats[Menu.Item - GSR_MENU_ALARM1] + 1, (WatchTime.DeadRTC ? 4 : 0), 4);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }else if (Menu.SubItem > 4){
                  Menu.SubItem = roller(Menu.SubItem - 1, 5, 13);
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          } else if (Menu.Item == GSR_MENU_TIMEDN && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1: // Hour
                  TimerDown.MaxHours=roller(TimerDown.MaxHours - 1, 0,23);
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2: //  x0 Minutes
                  mh = (TimerDown.MaxMins / 10);
                  ml = TimerDown.MaxMins - (mh * 10);
                  mh = roller(mh - 1, 0, 5);
                  TimerDown.MaxMins = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 3: //  x0 Minutes
                  mh = (TimerDown.MaxMins / 10);
                  ml = TimerDown.MaxMins - (mh * 10);
                  ml = roller(ml - 1, 0, 9);
                  TimerDown.MaxMins = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 4: //  x0 Seconds
                  mh = (TimerDown.MaxSecs / 10);
                  ml = TimerDown.MaxSecs - (mh * 10);
                  mh = roller(mh - 1, 0, 5);
                  TimerDown.MaxSecs = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 5: //  x0 Secondss
                  mh = (TimerDown.MaxSecs / 10);
                  ml = TimerDown.MaxSecs - (mh * 10);
                  ml = roller(ml - 1, 0, 9);
                  TimerDown.MaxSecs = (mh * 10) + ml;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item == GSR_MENU_TMEDCF && Menu.SubItem > 0){
              switch (Menu.SubItem){
              case 1:   // Repeatable
                  TimerDown.Repeats = !TimerDown.Repeats;
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
                  break;
              case 2:   // Tone Repeats
                  TimerDown.MaxTones = roller(TimerDown.MaxTones + 1, (WatchTime.DeadRTC ? 4 : 0), 4);
                  StopCD();
                  Options.NeedsSaving = true;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
          }else if (Menu.Item == GSR_MENU_DARK && Menu.SubItem > 0){  // Sleep Mode.
              switch (Menu.SubItem){
                  case 1: // Style.
                      Options.SleepStyle = roller(Options.SleepStyle - 1, (WatchTime.DeadRTC ? 1 : 0), (WatchTime.DeadRTC ? 2 : 4));
                      Updates.Tapped = true;
                      Options.NeedsSaving = true;
                      break;
                  case 2: // SleepMode (0=off, 10 seconds)
                      Options.SleepMode = roller(Options.SleepMode - 1, 1, 10);
                      Options.NeedsSaving = true;
                      break;
                  case 3: // SleepStart (hour)
                      if (Options.SleepStart - 1 != Options.SleepEnd) { Options.SleepStart = roller(Options.SleepStart - 1, 0,23); Options.NeedsSaving = true; } else return;
                      break;
                  case 4: // SleepStart (hour)
                      if (Options.SleepEnd - 1 != Options.SleepStart) { Options.SleepEnd = roller(Options.SleepEnd - 1, 0,23); Options.NeedsSaving = true; } else return;
                      break;
                  case 5: // Orientation.
                      Options.BedTimeOrientation = !Options.BedTimeOrientation;
                      Options.NeedsSaving = true;
              }
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_SYNC && Menu.SubItem > 0){
              Menu.SubItem = roller(Menu.SubItem + 1, 1, 3);
              Options.NeedsSaving = true;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }else if (Menu.Item == GSR_MENU_WIFI && Menu.SubItem > 0){
              // Do nothing!
          }else if (Menu.Item == GSR_MENU_TOFF && Menu.SubItem > 0){
              // Do nothing!
          }else if (Menu.Item == GSR_MENU_UNVS && Menu.SubItem == 1){  // USE NVS
              if (Menu.SubSubItem == 0){
                  Menu.SubSubItem = 1;
                  DoHaptic = true;
                  UpdateDisp = true;  // Quick Update.
                  SetTurbo();
              }
              return;
          }else{
              if (Menu.Style == GSR_MENU_INOPTIONS){
                  Menu.Item = roller(Menu.Item + 1, GSR_MENU_STYL, (NTPData.State > 0 || WatchyAPOn || OTAUpdate || Battery.Last < Battery.MinLevel) ? GSR_MENU_TRBL : GSR_MENU_OTAM);
              }else if (Menu.Style == GSR_MENU_INALARMS){
                  Menu.Item = roller(Menu.Item + 1, GSR_MENU_ALARM1, GSR_MENU_TONES);
              }else if (Menu.Style == GSR_MENU_INTIMERS){
                  Menu.Item = roller(Menu.Item + 1, GSR_MENU_TIMEDN, GSR_MENU_TIMEUP);
              }else if (Menu.Style == GSR_MENU_INTROUBLE){
                  Menu.Item = roller(Menu.Item + 1, GSR_MENU_SCRN, GSR_MENU_UNVS);
              }else{
                  Menu.Item = roller(Menu.Item + 1, GSR_MENU_STEPS, GSR_MENU_OPTIONS);
              }
              Menu.SubItem=0;
              Menu.SubSubItem=0;
              DoHaptic = true;
              UpdateDisp = true;  // Quick Update.
              SetTurbo();
          }
      } else Missed = 4;  // Missed a SW4.
      break;
    default:
      if (Pressed < 9) Missed = Pressed;  // Pass off other buttons not handled here.
  }
}

void WatchyGSR::UpdateTimerDown(){
    uint16_t T;
    if (TimerDown.Active){
        UpdateUTC(true); T = (TimerDown.StopAt - WatchTime.UTC_RAW);
        if (T > -1){
            TimerDown.Hours = (T / 3600);
            T = T - (TimerDown.Hours * 3600);
            TimerDown.Mins = (T / 60);
            TimerDown.Secs = (T % 60);
        }
        if (TimerDown.Hours == 0 && TimerDown.Mins == 0 && (WatchTime.DeadRTC || TimerDown.Secs == 0)){
            TimerDown.Tone = 22;
            TimerDown.ToneLeft = 255;
            TimerDown.Active = false;
            if (TimerDown.Repeats) StartCD();
            Darkness.Last=millis();
            UpdateDisp = true;  // Quick Update.
        }
    }
}

bool WatchyGSR::TimerAbuse(){
    bool A = ((TimerUp.Active && Menu.Item == GSR_MENU_TIMEUP) || (TimerDown.Active && Menu.Item == GSR_MENU_TIMEDN));
    unsigned long T = millis() / 1000;
    if (A){ if (Updates.LastSecond != T) { Updates.LastSecond = T; UpdateDisp=Showing(); } } // Make it update when a second happens.
    return (A || (TimerDown.Active && ((TimerDown.Hours * 3600) + (TimerDown.Mins * 60) + TimerDown.Secs) < 60));
}

void WatchyGSR::UpdateUTC(bool OnlyRead){
    if (!WatchTime.DeadRTC){
        SRTC.read(WatchTime.UTC);
        WatchTime.UTC_RAW = SRTC.MakeTime(WatchTime.UTC) + (NTPData.TimeTest ? 0 : WatchTime.Drifting);
        if (!OnlyRead) WatchTime.EPSMS = (millis() + (1000 * (60 - (WatchTime.UTC.Second + (NTPData.TimeTest ? 0 : WatchTime.Drifting)))));
    }
    SRTC.BreakTime(WatchTime.UTC_RAW,WatchTime.UTC);
}

void WatchyGSR::UpdateClock(){
    struct tm * TM;

    OP.setCurrentTimeZone();
    TM = localtime(&WatchTime.UTC_RAW);
    WatchTime.Local.Second = TM->tm_sec;
    WatchTime.Local.Minute = TM->tm_min;
    WatchTime.Local.Hour = TM->tm_hour;
    WatchTime.Local.Wday = TM->tm_wday;
    WatchTime.Local.Day = TM->tm_mday;
    WatchTime.Local.Month = TM->tm_mon;
    WatchTime.Local.Year = TM->tm_year;
    if (Options.SleepEnd > Options.SleepStart) WatchTime.BedTime = (WatchTime.Local.Hour >= Options.SleepStart && WatchTime.Local.Hour < Options.SleepEnd);
    else WatchTime.BedTime = (WatchTime.Local.Hour >= Options.SleepStart || WatchTime.Local.Hour < Options.SleepEnd);
    CalculateTones(); monitorSteps(); // Moved here for accuracy during Deep Sleep.
}

// Manage time will determine if the RTC is in use, will also set a flag to "New Minute" for the loop functions to see the minute change.
void WatchyGSR::ManageTime(){
    tmElements_t TM;  //    struct tm * tm;
    int I;
    bool B;

    if (WatchTime.DeadRTC) B = (WatchTime.WatchyRTC < esp_timer_get_time()); else B = (WatchTime.EPSMS < millis());
    if (B){
        // Deal with NTPData.TimeTest.
        if (NTPData.TimeTest){
            NTPData.TestCount++;
            UpdateUTC();
            if (NTPData.TestCount == 1) WatchTime.TravelTest = WatchTime.UTC_RAW + 60;      // Add the minute.
            if (NTPData.TestCount > 1){
                I = Options.Drift;
                if (NTPData.NTPDone){
                    if (WatchTime.TravelTest > WatchTime.UTC_RAW) Options.Drift = 0 - (WatchTime.TravelTest - WatchTime.UTC_RAW);
                    else if (WatchTime.UTC_RAW > WatchTime.TravelTest) Options.Drift = WatchTime.UTC_RAW - WatchTime.TravelTest;
                    else if (WatchTime.UTC_RAW == WatchTime.TravelTest) Options.Drift = 0;
                    if (Options.Drift < -29 || Options.Drift > 29){
                        WatchTime.DeadRTC = true; Options.NeedsSaving = true; Options.UsingDrift = false; Options.Feedback = false; Options.MasterRepeats = 4; Alarms_Repeats[0] = 4; Alarms_Repeats[1] = 4; Alarms_Repeats[2] = 4; Alarms_Repeats[3] = 4; TimerDown.MaxTones = 4;
                        NTPData.State = 1; NTPData.UpdateUTC = true; AskForWiFi();
                        if (Options.SleepStyle == 0) Options.SleepStyle = 1;
                    }else Options.UsingDrift = (Options.Drift != 0);
                    if (Menu.Item == GSR_MENU_TOFF) Menu.SubItem = 3;
                    NTPData.TimeTest = false;
                    if (Options.UsingDrift) WatchTime.Drifting = Options.Drift;
                }else{
                    NTPData.TimeTest = false;
                    if (Menu.Item == GSR_MENU_TOFF) Menu.SubItem = 0;
                }
                Options.NeedsSaving |= (I != Options.Drift);
                UpdateClock();
            }
        }
        if (WatchTime.DeadRTC) WatchTime.WatchyRTC += 60000000; else WatchTime.EPSMS += 60000;
        WatchTime.NewMinute=true;
    }

    if (WatchTime.NewMinute && !NTPData.TimeTest && !IDidIt){
        if (WatchTime.DeadRTC){
            WatchTime.UTC_RAW += 60;
            IDidIt = true;
            UpdateDisp=Showing();
            UpdateUTC();
            UpdateClock();
        }else{
            WatchTime.EPSMS += ((Options.UsingDrift ? Options.Drift : 0) * 1000);
            if (Options.UsingDrift){
                WatchTime.Drifting += Options.Drift;
                IDidIt = true;
            }
            UpdateDisp=Showing();
            UpdateUTC();
            UpdateClock();
        }
        InsertOnMinute();
    }
}

void WatchyGSR::_bmaConfig() {
uint8_t Type = SRTC.getType();
  if (SBMA.begin(_readRegister, _writeRegister, delay, Type) == false) {
    //fail to init BMA
    return;
  }

  if (!SBMA.defaultConfig()) return;  // Failed.
  // Enable BMA423 isStepCounter feature
  SBMA.enableFeature(BMA423_STEP_CNTR, true);
  // Enable isTilt feature
  //SBMA.enableTiltWake();
  // Enable isDoubleClick feature
//  SBMA.enableDoubleClickWake();

  // Reset steps
  //SBMA.resetStepCounter();

  // Turn on feature interrupt
  //SBMA.enableStepCountInterrupt();
}

void WatchyGSR::UpdateBMA(){
    bool BT = (Options.SleepStyle == 2 && WatchTime.BedTime);
    bool B = (Options.SleepStyle > 2 && Options.SleepStyle != 4);
    bool A = (Options.SleepStyle == 1);

    SBMA.enableDoubleClickWake(B | BT);
    SBMA.enableTiltWake((A | B) & !WatchTime.BedTime);
}

float WatchyGSR::getBatteryVoltage(){ return ((BatteryRead() - 0.0125) +  (BatteryRead() - 0.0125) + (BatteryRead() - 0.0125) + (BatteryRead() - 0.0125)) / 4; }
float WatchyGSR::BatteryRead(){ return analogReadMilliVolts(SRTC.getADCPin()) / 500.0f; } // Battery voltage goes through a 1/2 divider.

uint16_t WatchyGSR::_readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)address, (uint8_t)len);
  uint8_t i = 0;
  while (Wire.available()) {
    data[i++] = Wire.read();
  }
  return 0;
}

uint16_t WatchyGSR::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data, len);
  return (0 !=  Wire.endTransmission());
}

void WatchyGSR::UpdateFonts(){
    Design.Face.TimeColor = ForeColor();
    Design.Face.DayColor = ForeColor();
    Design.Face.DateColor = ForeColor();
    Design.Face.YearColor = ForeColor();
}

// Override functions.
uint16_t WatchyGSR::ForeColor(){ return (Options.LightMode ? GxEPD_BLACK : GxEPD_WHITE); }

uint16_t WatchyGSR::BackColor(){ return (Options.LightMode ? GxEPD_WHITE : GxEPD_BLACK); }

void WatchyGSR::InsertPost() {}
bool WatchyGSR::OverrideBitmap() { return false; }
bool WatchyGSR::OverrideSleepBitmap() { return false; }
void WatchyGSR::InsertDefaults() {}
void WatchyGSR::InsertOnMinute() {}
void WatchyGSR::InsertWiFi() {}
void WatchyGSR::InsertWiFiEnding() {}
void WatchyGSR::InsertAddWatchStyles() {}
void WatchyGSR::InsertInitWatchStyle(uint8_t StyleID) {}
void WatchyGSR::InsertDrawWatchStyle(uint8_t StyleID) {}
bool WatchyGSR::InsertNeedAwake(bool GoingAsleep) { return false; }
bool WatchyGSR::InsertHandlePressed(uint8_t SwitchNumber, bool &Haptic, bool &Refresh) { return false; }
void WatchyGSR::OverrideDefaultMenu(bool Override) { MenuOverride = Override; }
void WatchyGSR::ShowDefaultMenu() { if (MenuOverride && GuiMode == GSR_WATCHON) { GuiMode = GSR_MENUON; DoHaptic = true; UpdateDisp = true; SetTurbo(); } }
uint8_t WatchyGSR::AddWatchStyle(String StyleName){
    if (WatchStyles.Count >= GSR_MaxStyles || StyleName.length() > 30) return 255;  // Full / too long..
    for (int I = 0; I < WatchStyles.Count; I++)
        if (String(WatchStyles.Style[I * 32]) == StyleName) return 255;  // Error, alrady there.

    strcpy(&WatchStyles.Style[WatchStyles.Count * 32], StyleName.c_str());
    int O = WatchStyles.Count;
    WatchStyles.Count++;
    return O;
}
String WatchyGSR::InsertNTPServer() { return "pool.ntp.org"; }
void WatchyGSR::AllowDefaultWatchStyles(bool Allow) { DefaultWatchStyles = Allow; }

bool WatchyGSR::IsDark(){ return Darkness.Went; }
bool WatchyGSR::IsAM() { return (!Options.TwentyFour && WatchTime.Local.Hour < 12); }
bool WatchyGSR::IsPM() { return (!Options.TwentyFour && WatchTime.Local.Hour > 11); }
String WatchyGSR::GetLangWebID() { return LGSR.GetWebLang(Options.LanguageID); }

String WatchyGSR::MakeTime(int Hour, int Minutes, bool& Alarm){  // Use variable with Alarm, if set to False on the way in, returns PM indication.
    int H;
    String AP = "";
    H = (Hour & 31);
    if (!Options.TwentyFour){
        if (H > 11){
          AP = " " + LGSR.GetID(Options.LanguageID,114);
          if (!Alarm){
              Alarm = true;   // Tell the clock to use the PM indicator.
          }
      }else{
          AP = " " + LGSR.GetID(Options.LanguageID,115);
        }
        if (H > 12){
            H -= 12;
        }else if (H == 0){
            H = 12;
        }
    }
    return (((Hour & 64) && H < 10) ? " " : "") + String(H) + (Minutes < 10 ? ":0" : ":") + String(Minutes) + ((Hour & 32) ? "" : AP);
}

String WatchyGSR::MakeHour(uint8_t Hour){
    int H;
    H = Hour;
    if (!Options.TwentyFour){
        if (H > 12){
          H -= 12;
        }
        if (H == 0){
          H = 12;
        }
    }
    return String(H);
}

String WatchyGSR::MakeSeconds(uint8_t Seconds){ return LGSR.GetID(Options.LanguageID,(Seconds > 1 ? 112 : 113)) + "."; }

String WatchyGSR::MakeTOD(uint8_t Hour, bool AddZeros){
    if(Options.TwentyFour){
        if (AddZeros) return ":00";
        return "";
    }
    return " " + LGSR.GetID(Options.LanguageID,(Hour > 11 ? 114 : 115));
}

String WatchyGSR::MakeMinutes(uint8_t Minutes){
    return (Minutes < 10 ? "0" : "") + String(Minutes);
}

void WatchyGSR::ClockSeconds(){ UpdateUTC(true); UpdateClock(); }

String WatchyGSR::MakeSteps(uint32_t uSteps){
    String S, T, X;
    int I, C;

    S = String(uSteps);
    I = S.length();
    C = 0; T = "";

    while (I > 0){
        X = (I > 1 && C == 2) ? "," : "";
        T = X + S.charAt(I - 1) + T;
        C = roller(C + 1, 0, 2); I--;
    }
    return T;
}

void WatchyGSR::CheckAlarm(int I){
    uint16_t  B;
    bool bA;
    B = (GSR_ALARM_ACTIVE | Bits[WatchTime.Local.Wday]);
    bA = (Alarms_Hour[I] == WatchTime.Local.Hour && Alarms_Minutes[I] == WatchTime.Local.Minute);
    if (!bA && Alarms_Times[I] == 0 && (Alarms_Active[I] & GSR_ALARM_TRIGGERED) != 0){
        Alarms_Active[I] &= GSR_ALARM_NOTRIGGER;
    }else if ((Alarms_Active[I] & B) == B && (Alarms_Active[I] & GSR_ALARM_TRIGGERED) == 0){  // Active and Active Day.
        // Check alarm listed to see if it is earlier than the one slated.
        if (Alarms_Hour[I] == WatchTime.Local.Hour && Alarms_Minutes[I] > WatchTime.Local.Minute && !bA){
            if (WatchTime.NextAlarm == 99) WatchTime.NextAlarm = I; else if (Alarms_Minutes[I] < Alarms_Minutes[WatchTime.NextAlarm]) WatchTime.NextAlarm = I;
        }
        if (bA && Alarms_Times[I] == 0){
            Alarms_Times[I] = 255;
            Alarms_Playing[I] = 30;
            Darkness.Last=millis();
            UpdateDisp=true;  // Force it on, if it is in Dark Running.
            Alarms_Active[I] |= GSR_ALARM_TRIGGERED;
            if ((Alarms_Active[I] & GSR_ALARM_REPEAT) == 0){
                Alarms_Active[I] &= (GSR_ALARM_ALL - Bits[WatchTime.Local.Wday]);
                if ((Alarms_Active[I] & GSR_ALARM_DAYS) == 0){
                    Alarms_Active[I] ^= GSR_ALARM_ACTIVE; // Turn it off, not repeating.
                }
            }
        }
    }
}

// Counts the active (255) alarms/timers and after 3, sets them to lower values.
void WatchyGSR::CalculateTones(){
    uint8_t Count = 0;
    float Downgrade = 1.0;
    uint32_t TimeDown;
    WatchTime.NextAlarm = 99; CheckAlarm(0); CheckAlarm(1); CheckAlarm(2); CheckAlarm(3); UpdateTimerDown();
    TimeDown = TimerDown.Hours * 3600 + TimerDown.Mins * 60 + TimerDown.Secs;
    if (TimeDown < 11 && TimerDown.Repeats) Downgrade = gobig(0.2, TimeDown * 0.25);
    if (Alarms_Times[0] > 0) Count++;
    if (Alarms_Times[1] > 0) Count++;
    if (Alarms_Times[2] > 0) Count++;
    if (Alarms_Times[3] > 0) Count++;
    if (TimerDown.ToneLeft > 0) Count++;
    if (Count == 5){
        if (Alarms_Times[0] == 255) Alarms_Times[0] = 7 * Reduce[Alarms_Repeats[0]];
        if (Alarms_Times[1] == 255) Alarms_Times[1] = 7 * Reduce[Alarms_Repeats[1]];
        if (Alarms_Times[2] == 255) Alarms_Times[2] = 7 * Reduce[Alarms_Repeats[2]];
        if (Alarms_Times[3] == 255) Alarms_Times[3] = 7 * Reduce[Alarms_Repeats[3]];
        if (TimerDown.ToneLeft == 255) TimerDown.ToneLeft = (10 * Downgrade) * Reduce[TimerDown.MaxTones];
    }else if (Count == 4){
        if (Alarms_Times[0] == 255) Alarms_Times[0] = 8 * Reduce[Alarms_Repeats[0]];
        if (Alarms_Times[1] == 255) Alarms_Times[1] = 8 * Reduce[Alarms_Repeats[1]];
        if (Alarms_Times[2] == 255) Alarms_Times[2] = 8 * Reduce[Alarms_Repeats[2]];
        if (Alarms_Times[3] == 255) Alarms_Times[3] = 8 * Reduce[Alarms_Repeats[3]];
        if (TimerDown.ToneLeft == 255) TimerDown.ToneLeft = (12 * Downgrade) * Reduce[TimerDown.MaxTones];
    }else{
        if (Alarms_Times[0] == 255) Alarms_Times[0] = 10 * Reduce[Alarms_Repeats[0]];
        if (Alarms_Times[1] == 255) Alarms_Times[1] = 10 * Reduce[Alarms_Repeats[1]];
        if (Alarms_Times[2] == 255) Alarms_Times[2] = 10 * Reduce[Alarms_Repeats[2]];
        if (Alarms_Times[3] == 255) Alarms_Times[3] = 10 * Reduce[Alarms_Repeats[3]];
        if (TimerDown.ToneLeft == 255) TimerDown.ToneLeft = (15 * Downgrade) * Reduce[TimerDown.MaxTones];
    }
}

void WatchyGSR::StartCD(){
    TimerDown.Secs = TimerDown.MaxSecs;
    TimerDown.Mins = TimerDown.MaxMins;
    TimerDown.Hours = TimerDown.MaxHours;
    UpdateUTC();
    TimerDown.LastUTC = WatchTime.UTC_RAW;
    TimerDown.StopAt = TimerDown.LastUTC + (3600 * TimerDown.Hours) + (60 * TimerDown.Mins) + TimerDown.Secs;
    TimerDown.Repeating = TimerDown.Repeats;
    TimerDown.Active = true;
}

void WatchyGSR::StopCD(){
    if (TimerDown.ToneLeft > 0){
        TimerDown.ToneLeft = 1;
        TimerDown.Tone = 1;
    }
    TimerDown.Active = false;
    TimerDown.Repeating = false;
}

uint8_t WatchyGSR::getToneTimes(uint8_t ToneIndex){
    if (ToneIndex > 3) return TimerDown.ToneLeft;
    return Alarms_Times[ToneIndex];
}

String WatchyGSR::getReduce(uint8_t Amount){
    switch (Amount){
        case 0:
          return LGSR.GetID(Options.LanguageID,116);
        case 1:
          return "80%";
        case 2:
          return "60%";
        case 3:
          return "40%";
    }
    return "20%";
}

// Catches Steps and moves "Yesterday" into the other setting.
void WatchyGSR::monitorSteps(){
    if (Steps.Hour == WatchTime.Local.Hour && Steps.Minutes == WatchTime.Local.Minute){
        if (!Steps.Reset){
            Steps.Yesterday=SBMA.getCounter();
            SBMA.resetStepCounter();
            Steps.Reset=true;
        }
    }else if (Steps.Reset) Steps.Reset=false;
}

IRAM_ATTR void WatchyGSR::handleInterrupt(){
    uint8_t B = getButtonPins();
    if (Options.SleepStyle == 4 && !Updates.Tapped) return;   // Screen didn't permit buttons to work.
    if (B > 0 && (LastButton == 0 || (millis() - LastButton) > GSR_KEYPAUSE)) Button = B;
}

IRAM_ATTR uint8_t WatchyGSR::getButtonPins(){ return getSWValue((digitalRead(GSR_MENU_PIN) == 1), (digitalRead(GSR_BACK_PIN) == 1), (digitalRead(GSR_UP_PIN) == 1), (digitalRead(GSR_DOWN_PIN) == 1)); }

uint8_t WatchyGSR::getButtonMaskToID(uint64_t HW){
    uint8_t HB = getSWValue((HW & GSR_MENU_MASK), (HW & GSR_BACK_MASK), (HW & GSR_UP_MASK), (HW & GSR_DOWN_MASK));
    uint8_t LB = getButtonPins();
    if (SBMA.didBMAWakeUp(HW)) {           // Acccelerometer.
        if (SBMA.isDoubleClick()) return 9;  // Double Tap.
        else if (SBMA.isTilt()) return 10;  // Wrist Tilt.
    }
    if (LB > 0 && LB != HB) HB=LB;
    return HB;
}

IRAM_ATTR uint8_t WatchyGSR::getSWValue(bool SW1, bool SW2, bool SW3, bool SW4){
    bool T;
    if (Options.Swapped) { T = SW1; SW2=SW1; SW1=T; }
    if (Options.Lefty){
        T = SW1; SW1=SW4; SW4=T;
        T = SW3; SW3=SW2; SW2=T;
    }
    if (SW3 && SW1) return 5;
    if (SW3 && SW2) return 6;
    if (SW4 && SW1) return 7;
    if (SW4 && SW2) return 8;
    if (SW1) return 1;
    if (SW2) return 2;
    if (SW3) return 3;
    if (SW4) return 4;
    return 0;
}

void WatchyGSR::AskForWiFi(){ if (!GSRWiFi.Requested && !GSRWiFi.Working) GSRWiFi.Requested = true; }
void WatchyGSR::endWiFi(){
    if (GSRWiFi.Requests - 1 <= 0){
        GSRWiFi.Requests = 0;
        GSRWiFi.Requested = false;
        GSRWiFi.Working = false;
        GSRWiFi.Results = false;
        GSRWiFi.Index = 0;
        setStatus("");
        WiFi.disconnect();
        WiFi.removeEvent(GSRWiFi.WiFiEventID);
        GSRWiFi.WiFiEventID = 0;
        WiFi.mode(WIFI_OFF);
        InsertWiFiEnding();
    }else if (GSRWiFi.Requests > 0) GSRWiFi.Requests--;
}

void WatchyGSR::processWiFiRequest(){
    wl_status_t WiFiE = WL_CONNECT_FAILED;
    wl_status_t rWiFi = WiFi.status();
    wifi_config_t conf;
    String AP, PA, O;
    uint8_t I;

    if (GSRWiFi.Requested){
        GSRWiFi.Requested = false;
        if (GSRWiFi.Requests == 0){
            RefreshCPU(GSR_CPUMAX);
            OTATimer = millis();
            OTAFail = OTATimer;
            WiFi.disconnect();
            WiFi.setHostname(WiFi_AP_SSID);
            WiFi.mode(WIFI_STA);
#ifdef ARDUINO_ESP32_RELEASE_1_0_6
            if (GSRWiFi.WiFiEventID == 0) GSRWiFi.WiFiEventID = WiFi.onEvent(WatchyGSR::WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);  // ARDUINO_EVENT_WIFI_STA_DISCONNECTED
#else
            if (GSRWiFi.WiFiEventID == 0) GSRWiFi.WiFiEventID = WiFi.onEvent(WatchyGSR::WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);  // SYSTEM_EVENT_STA_DISCONNECTED
#endif
            GSRWiFi.Index = 0;
            GSRWiFi.Tried = false;
            GSRWiFi.Last = 0;
        }
        GSRWiFi.Working = true;
        GSRWiFi.Requests++;
    }

    if (GSRWiFi.Working) {
        if (getButtonPins() != 2) OTATimer = millis(); // Not pressing "BACK".
        if (millis() - OTATimer > 10000 || millis() - OTAFail > 600000) OTAEnd = true;  // Fail if holding back for 10 seconds OR 600 seconds has passed.
        if (rWiFi == WL_CONNECTED) { GSRWiFi.Working = false; GSRWiFi.Results = true; return; } // We got connected.
        if (millis() > GSRWiFi.Last){
            if (GSRWiFi.Tried){
                WiFi.disconnect();
                GSRWiFi.Index++;
                GSRWiFi.Tried = false;
            }
            if (GSRWiFi.Index < 11){
                if (GSRWiFi.Index == 0){
                    setStatus(WiFiIndicator(24));
                    GSRWiFi.Tried = true;
                    if (WiFi_DEF_SSID > ""){
                        WiFiE = WiFi.begin(WiFi_DEF_SSID,WiFi_DEF_PASS);
                        UpdateWiFiPower(WiFi_DEF_SSID,WiFi_DEF_PASS);
                    }else{
                        WiFiE = WiFi.begin();
                        esp_wifi_get_config((wifi_interface_t)WIFI_IF_STA, &conf);
                        UpdateWiFiPower(reinterpret_cast<const char*>(conf.sta.ssid),reinterpret_cast<const char*>(conf.sta.password));
                    }
                    GSRWiFi.Last = millis() + 9000;
                    //if (WiFiE == WL_CONNECT_FAILED || WiFiE == WL_NO_SSID_AVAIL) { GSRWiFi.Last = millis() + 53000; GSRWiFi.Index++; }   // Try the next one instantly.
                }
                if (GSRWiFi.Index > 0){
                    AP = APIDtoString(GSRWiFi.Index - 1); PA = PASStoString(GSRWiFi.Index - 1);
                    if (AP.length() > 0){
                        O = WiFiIndicator(GSRWiFi.Index);
                        setStatus(O);
                        GSRWiFi.Tried = true;
                        WiFiE = WiFi.begin(AP.c_str(),PA.c_str());
                        UpdateWiFiPower(GSRWiFi.AP[GSRWiFi.Index].TPWRIndex);
                        GSRWiFi.Last = millis() + 9000;
                    }else GSRWiFi.Index++;
                    //if (WiFiE == WL_CONNECT_FAILED || WiFiE == WL_NO_SSID_AVAIL || AP == "") { GSRWiFi.Last = millis() + 53000; GSRWiFi.Index++; }   // Try the next one instantly.
                }
            }else endWiFi();
        }
    }
}

String WatchyGSR::WiFiIndicator(uint8_t Index){
    unsigned char O[7];
    String S;

    O[0] = 87;
    O[1] = 105;
    O[2] = 70;
    O[3] = 105;
    O[4] = 45;
    O[5] = (64 + Index);
    O[6] = 0;

    S = reinterpret_cast<const char *>(O);
    return S;
}

void WatchyGSR::UpdateWiFiPower(String SSID, String PSK){
    uint8_t I;
    for (I = 0; I < 11; I++){ if (APIDtoString(I) == SSID && PASStoString(I) == PSK) { UpdateWiFiPower(GSRWiFi.AP[I].TPWRIndex); return; } }
    UpdateWiFiPower();
}

void WatchyGSR::UpdateWiFiPower(String SSID){
    uint8_t I;
    for (I = 0; I < 11; I++){ if (APIDtoString(I) == SSID) { UpdateWiFiPower(GSRWiFi.AP[I].TPWRIndex); return; } }
    UpdateWiFiPower();
}

void WatchyGSR::UpdateWiFiPower(uint8_t PWRIndex){
    wifi_power_t CW=WiFi.getTxPower();
    bool B;

    if (PWRIndex > 0) B = WiFi.setTxPower(RawWiFiTX[PWRIndex - 1]);
    else if (CW != GSRWiFi.TransmitPower && GSRWiFi.TransmitPower != 0) B = WiFi.setTxPower(GSRWiFi.TransmitPower);
}

wl_status_t WatchyGSR::currentWiFi(){
    if (WiFi.status() == WL_CONNECTED) return WL_CONNECTED;
    if (GSRWiFi.Working) return WL_IDLE_STATUS;  // Make like it is relaxing doing nothing.
    return WL_CONNECT_FAILED;
}

void WatchyGSR::WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    GSRWiFi.Last = millis() + 1000;
}

String WatchyGSR::buildWiFiAPPage(){
    String S = LGSR.LangString(wifiIndexA,true,Options.LanguageID,14,14);
    String T;
    uint8_t I, J;

    for (I = 0; I < 10; I++){
        T = wifiIndexB;
        T.replace("$",String(char(65 + I)));
        T.replace("#",String(char(48 + I)));
        T.replace("?",APIDtoString(I));
        S += T;

        T = LGSR.LangString(wifiIndexC,true,Options.LanguageID,15,16);
        T.replace("#",String(char(48 + I)));
        T.replace("$",PASStoString(I));
        S += T;

        for (J = 0; J < 12; J++){
            T = wifiIndexC1;
            T.replace("#",String(J));
            T.replace("%",(J == GSRWiFi.AP[I].TPWRIndex ? " selected" : ""));
            T.replace("$",(J > 0 ? IDWiFiTX[J - 1] : "* " + LGSR.GetWebID(Options.LanguageID,0) + " *"));
            S += T;
        }
        T = wifiIndexC2;
        S += (T + (I < 9 ? "</tr>" : ""));
    }
    return S + LGSR.LangString(wifiIndexD,true,Options.LanguageID,6,6);
}

void WatchyGSR::parseWiFiPageArg(String ARG, String DATA){
    uint8_t I = String(ARG.charAt(2)).toInt();
    String S = ARG.substring(0,2);

    if      (S == "AP") strcpy(GSRWiFi.AP[I].APID, DATA.c_str());
    else if (S == "PA") strcpy(GSRWiFi.AP[I].PASS, DATA.c_str());
    else if (S == "TX") GSRWiFi.AP[I].TPWRIndex = DATA.toInt();
}

String WatchyGSR::APIDtoString(uint8_t Index){
    String S = "";
    uint8_t I = 0;
    while (GSRWiFi.AP[Index].APID[I] != 0 && I < 32) { S += char(GSRWiFi.AP[Index].APID[I]); I++; }
    return S;
}

String WatchyGSR::PASStoString(uint8_t Index){
    String S = "";
    uint8_t I = 0;
    while (GSRWiFi.AP[Index].PASS[I] != 0 && I < 63) { S += char(GSRWiFi.AP[Index].PASS[I]); I++; }
    return S;
}

void WatchyGSR::initZeros(){
    String S = "";
    uint8_t I;
    GuiMode = GSR_WATCHON;
    VibeMode = false;
    WatchyStatus = "";
    WatchTime.TimeZone = "";
    WatchTime.Drifting = 0;
    OP.init();
    Menu.Style = GSR_MENU_INNORMAL;
    Menu.Item = 0;
    Menu.SubItem = 0;
    Menu.SubSubItem = 0;
    NTPData.Pause = 0;
    NTPData.Wait = 0;
    NTPData.NTPDone = false;
    Battery.Last = getBatteryVoltage() + 1;   // Done for power spike after reboot from making the watch think it's charging.
    Battery.UpCount = 0;
    Battery.DownCount = 0;
    ActiveMode = false;
    OTATry = 0;
    OTAEnd = false;
    OTAUpdate = false;
    OTATimer = millis();
    WatchyAPOn = false;
    DoHaptic = false;
    Steps.Reset=false;
    Alarms_Active[0]=0;
    Alarms_Active[1]=0;
    Alarms_Active[2]=0;
    Alarms_Active[3]=0;
    Alarms_Times[0]=0;
    Alarms_Times[1]=0;
    Alarms_Times[2]=0;
    Alarms_Times[3]=0;
    Alarms_Playing[0]=0;
    Alarms_Playing[1]=0;
    Alarms_Playing[2]=0;
    Alarms_Playing[3]=0;
    Alarms_Repeats[0]=0;
    Alarms_Repeats[1]=0;
    Alarms_Repeats[2]=0;
    Alarms_Repeats[3]=0;
    GSRWiFi.Requested=false;
    GSRWiFi.Working=false;
    GSRWiFi.Results=false;
    GSRWiFi.Index=0;
    Updates.Full=true;
    Updates.Drawn=true;
    strcpy(GSRWiFi.AP[0].APID,S.c_str());
    strcpy(GSRWiFi.AP[0].PASS,S.c_str());
    strcpy(GSRWiFi.AP[1].APID,S.c_str());
    strcpy(GSRWiFi.AP[1].PASS,S.c_str());
    strcpy(GSRWiFi.AP[2].APID,S.c_str());
    strcpy(GSRWiFi.AP[2].PASS,S.c_str());
    strcpy(GSRWiFi.AP[3].APID,S.c_str());
    strcpy(GSRWiFi.AP[3].PASS,S.c_str());
    strcpy(GSRWiFi.AP[4].APID,S.c_str());
    strcpy(GSRWiFi.AP[4].PASS,S.c_str());
    strcpy(GSRWiFi.AP[5].APID,S.c_str());
    strcpy(GSRWiFi.AP[5].PASS,S.c_str());
    strcpy(GSRWiFi.AP[6].APID,S.c_str());
    strcpy(GSRWiFi.AP[6].PASS,S.c_str());
    strcpy(GSRWiFi.AP[7].APID,S.c_str());
    strcpy(GSRWiFi.AP[7].PASS,S.c_str());
    strcpy(GSRWiFi.AP[8].APID,S.c_str());
    strcpy(GSRWiFi.AP[8].PASS,S.c_str());
    strcpy(GSRWiFi.AP[9].APID,S.c_str());
    strcpy(GSRWiFi.AP[9].PASS,S.c_str());
    GSRWiFi.AP[0].TPWRIndex=0;
    GSRWiFi.AP[1].TPWRIndex=0;
    GSRWiFi.AP[2].TPWRIndex=0;
    GSRWiFi.AP[3].TPWRIndex=0;
    GSRWiFi.AP[4].TPWRIndex=0;
    GSRWiFi.AP[5].TPWRIndex=0;
    GSRWiFi.AP[6].TPWRIndex=0;
    GSRWiFi.AP[7].TPWRIndex=0;
    GSRWiFi.AP[8].TPWRIndex=0;
    GSRWiFi.AP[9].TPWRIndex=0;
    NTPData.TimeZone=false;
    NTPData.UpdateUTC=true;
    NTPData.State=1;
    TimerUp.SetAt = WatchTime.UTC_RAW;
    TimerUp.StopAt = TimerUp.SetAt;
    TimerDown.MaxMins = 0;
    TimerDown.MaxHours = 0;
    TimerDown.Mins = 0;
    TimerDown.Hours = 0;
    TimerDown.MaxTones = 0;
    TimerDown.Active = false;
    DefaultWatchStyles = true;
    AskForWiFi();
}

// Settings Storage & Retrieval here.

String WatchyGSR::GetSettings(){
    unsigned char I[2048];
    unsigned char O[2048];
    int K = 0;
    int J = 0;
    uint8_t X, Y, W;
    uint16_t V;
    String S;
    size_t L;

       // Retrieve the settings from the current state into a base64 string.

    I[J] = 133; J++; // New Version.
    I[J] = (Steps.Hour); J++;
    I[J] = (Steps.Minutes); J++;
    K  = Options.TwentyFour ? 1 : 0;
    K |= Options.LightMode ? 2 : 0;
    K |= Options.Feedback ? 4 : 0;
    K |= Options.Border ? 8 : 0;
    K |= Options.Lefty ? 16 : 0;
    K |= Options.Swapped ? 32 : 0;
    K |= Options.Orientated ? 64 : 0;
    K |= Options.UsingDrift ? 128 : 0;
    I[J] = (K); J++;
    // Versuib 129. 
    I[J] = (Options.Drift & 255); J++;
    I[J] = ((Options.Drift >> 8) & 255); J++;
    W = ((Options.Performance & 15) << 4);
    I[J] = (Options.SleepStyle | W); J++;
    I[J] = (Options.SleepMode); J++;
    I[J] = (Options.SleepStart); J++;
    I[J] = (Options.SleepEnd); J++;
    // End Version 129.
    // Version 130.
    I[J] = (getTXOffset(GSRWiFi.TransmitPower)); J++;
    K  = Options.BedTimeOrientation ? 1 : 0;
    I[J] = (K); J++;
    // Emd Version 130.
    // Version 131.
    I[J] = Options.WatchFaceStyle; J++;
    I[J] = Options.LanguageID; J++;
    // End Version 131.

    V = (Options.MasterRepeats << 5); I[J] = (Options.Turbo | V); J++;

    V = (TimerDown.MaxTones << 5);
    I[J] = ((TimerDown.MaxHours) | V); J++;
    I[J] = (TimerDown.MaxMins); J++;
    I[J] = ((TimerDown.MaxSecs) | (TimerDown.Repeats ? 128 : 0)); J++;

    for (K = 0; K < 4; K++){
        V = (Alarms_Repeats[K] << 5);
        I[J] = (Alarms_Hour[K] | V); J++;
        I[J] = (Alarms_Minutes[K]); J++;
        V = (Alarms_Active[K] & GSR_ALARM_NOTRIGGER);
        I[J] = (V & 255); J++;
        I[J] = ((V >> 8) & 255); J++;
    }

    for (X = 0; X < 10; X++){
        S = APIDtoString(X);
        if (S > "") {
            I[J] = GSRWiFi.AP[X].TPWRIndex; J++; // Store the WiFi power per AP.
            W = S.length();
            I[J] = W; J++;
            for (Y = 0; Y < W; Y++){
                I[J] = S.charAt(Y); J++;
            }
            S = PASStoString(X);
            W = S.length();
            I[J] = W; J++;
            for (Y = 0; Y < W; Y++){
                I[J] = S.charAt(Y); J++;
            }
        }
    }
    I[J] = 0; J++;

    mbedtls_base64_encode(&O[0], 2047, &L, &I[0], J);

    O[L]=0;
    S = reinterpret_cast<const char *>(O);
    return S;
}

void WatchyGSR::StoreSettings(String FromUser){
    unsigned char O[2048], E[2048];
    int K = 0;
    int J = 0;
    uint16_t V;
    size_t L;
    bool Ok;
    uint8_t I, A, W, NewV;  // For WiFi 
    String S;

    J = FromUser.length(); if (J < 5) return;
    for (K = 0; K < J; K++) E[K] = FromUser.charAt(K); NewV = 0;

    mbedtls_base64_decode(&O[0], 2047, &L, &E[0], J); L--;  // Take dead zero off end.

    J = 0; if (L > J && O[J] > 128) {NewV = O[J]; J++; }  // Detect NewVersion and go past.
           if (L > J) Steps.Hour = constrain(O[J],0,23);
    J++;   if (L > J) Steps.Minutes = constrain(O[J],0,59);
    J++;
    if (L > J) {
        V = O[J];
        Options.TwentyFour = (V & 1) ? true : false;
        Options.LightMode = (V & 2) ? true : false;
        Options.Feedback = (V & 4) ? true : false;
#ifdef GxEPD2DarkBorder
        Options.Border = (V & 8) ? true : false;
#else
        Options.Border = false;
#endif
        Options.Lefty = (V & 16) ? true : false;
        Options.Swapped = (V & 32) ? true : false;
        Options.Orientated = (V & 64) ? true : false;
        Options.UsingDrift = (V & 128) ? true : false;
    }
    if (NewV > 128){
         J++; if (L > J + 1){
                  Options.Drift = (((O[J + 1] & 255) << 8) | O[J]); J++;
                  if (Options.Drift < -29 || Options.Drift > 29){
                      Options.UsingDrift = false;
                      WatchTime.DeadRTC = true;
                  }else WatchTime.DeadRTC = false;
              }
         J++; if (L > J){
            V = ((O[J] & 240) >> 4); Options.Performance = constrain(V,0,2);
            Options.SleepStyle = constrain((O[J] & 7),(WatchTime.DeadRTC ? 1 : 0),4);
         }
         J++; if (L > J) Options.SleepMode = constrain(O[J],1,10);
         J++; if (L > J) Options.SleepStart = constrain(O[J],0,23);
         J++; if (L > J) Options.SleepEnd = constrain(O[J],0,23);
         if (NewV > 129){
            J++; if (L > J){ V = constrain(O[J],0,10); GSRWiFi.TransmitPower = RawWiFiTX[V]; }
            J++; if (L > J){ V = O[J]; Options.BedTimeOrientation = (V & 1) ? true : false; }
         }
         if (NewV > 130){
            J++; if (L > J){ V = constrain(O[J],0,WatchStyles.Count - 1); Options.WatchFaceStyle = V; }
         }
         if (NewV > 131){
            J++; if (L > J) { V = constrain(O[J],0,LGSR.MaxLangID()); Options.LanguageID = V; }
         }
    }
    if (WatchTime.DeadRTC) Options.Feedback = false;
    J++; if (L > J){
        V = ((O[J] & 224) >> 5);
        Options.MasterRepeats = constrain(V,(WatchTime.DeadRTC ? 4 : 0),4);
        Options.Turbo = constrain((O[J] & 31),0,10);
    }
    J++; if (L > J){
        V = ((O[J] & 224) >> 5);
        TimerDown.MaxTones = constrain(V,(WatchTime.DeadRTC ? 4 : 0),4);
        TimerDown.MaxHours = constrain((O[J] & 31),0,23);
    }
    J++; if (L > J) TimerDown.MaxMins = constrain(O[J],0,59);
    if (NewV > 132){
        J++; if (L > J) { TimerDown.MaxSecs = constrain((O[J] & 127),0,59); TimerDown.Repeats = ((O[J] & 128) == 128); }
    }

    for (K = 0; K < 4; K++){
        J++; if (L > J){
            V = ((O[J] & 224) >> 5);
            Alarms_Repeats[K] = constrain(V,(WatchTime.DeadRTC ? 4 : 0),4);
            Alarms_Hour[K] = constrain((O[J] & 31),0,23);
        }
        J++; if (L > J) Alarms_Minutes[K] = constrain(O[J],0,59);
        J++; if (L > J + 1){
            V = ((O[J + 1] & 255) << 8);
            Alarms_Active[K] = ((O[J] | V) & GSR_ALARM_NOTRIGGER); J++;
        }
    }

    S = ""; for (I = 0; I < 10; I++) { strcpy(GSRWiFi.AP[I].APID,S.c_str()); strcpy(GSRWiFi.AP[I].PASS,S.c_str()); }

    J++; A = 0;
    while (L > J){
        Ok = false;
        if (L > J){ // get APx
            if (NewV > 129) { GSRWiFi.AP[A].TPWRIndex=constrain(O[J],0,10); J++; }
            W = O[J]; J++; S = "";
            if (L > J + (W - 1)) {  // Read APx.
                for (I = 0; L > J && I < W; I++){
                     S += String(char(O[J])); J++;
                }
                strcpy(GSRWiFi.AP[A].APID,S.c_str());
                Ok = true;
            }
            if (L > J){ // get APx
                W = O[J]; J++; S = "";
                if (L > J + (W - 1)) {  // Read PAx.
                    for (I = 0; L > J && I < W; I++){
                        S += String(char(O[J])); J++;
                    }
                    strcpy(GSRWiFi.AP[A].PASS,S.c_str());
                    Ok = true;
                }
            }
        }
        if (Ok) A++;
    }

    OTAFail = millis() - 598000;
}
// NVS code.
void WatchyGSR::RetrieveSettings(){
    if (OkNVS(GName)){
        String S = NVS.getString(GSettings);
        StoreSettings(S);
    }
    Options.NeedsSaving = false;
}

void WatchyGSR::RecordSettings(){
    bool B = true;
    if (OkNVS(GName)) B = NVS.setString(GSettings,GetSettings());
    Options.NeedsSaving = !B;
}

bool WatchyGSR::OkNVS(String FaceName){
    String S = NVS.getString("NoNVS");
    String R = ".#.";
    R.replace("#",FaceName);
    return (S.indexOf(R) < 0);
}

void WatchyGSR::SetNVS(String FaceName, bool Enabled){
    String S = NVS.getString("NoNVS");
    String R = ".#.";
    R.replace("#",FaceName);
    int I = S.indexOf(R);
    if (!Enabled) {
        if (I == 0) {
            S += R;
            bool B = NVS.setString("NoNVS",S);
        }else if (I > -1){
            S.replace(R,"");
            bool B = NVS.setString("NoNVS",S);
        }
    }
}

void WatchyGSR::NVSEmpty(){
    NVS.erase(GSettings);
    NVS.erase(GTZ);
}

// Turbo Mode!
void WatchyGSR::SetTurbo(){
    if (Battery.Last > Battery.MinLevel){
        TurboTime=millis();
        LastButton=TurboTime;  // Here for speed.
        //Darkness.Last=LastButton;     // Keeps track of SleepMode.
    }
}

bool WatchyGSR::InTurbo() { return (!WatchTime.DeadRTC && Options.Turbo > 0 && TurboTime != 0 && millis() - TurboTime < (Options.Turbo * 1000)); }

bool WatchyGSR::UpRight() {
    if (Options.Orientated || (WatchTime.BedTime && Options.BedTimeOrientation)) return SBMA.IsUp();
    return true;  // Fake it til you make it.
}

bool WatchyGSR::DarkWait(){
    bool B = ((Options.SleepStyle > 0 || WatchTime.DeadRTC) && Darkness.Last != 0 && (millis() - Darkness.Last) < (Options.SleepMode * 1000));
        if (Options.SleepStyle == 2){
            if (!WatchTime.BedTime) return false;
            return B;
        }else if ((GuiMode != GSR_MENUON && !WatchTime.DeadRTC && Options.SleepStyle > 0 && Options.SleepStyle != 4) || WatchTime.DeadRTC) return B;
    return false;
}

bool WatchyGSR::Showing() {
    bool B = Updates.Full;
    if (Options.SleepStyle > 0){
        B |= (Darkness.Last > 0 && (millis() - Darkness.Last) < (Options.SleepMode * 1000));
        if (Options.SleepStyle == 1){
            if (WatchTime.DeadRTC) return B;
            else return (B | (GuiMode != GSR_WATCHON)); // Hide because it isn't checking the rest.
        }
        if (Options.SleepStyle == 2){
            if (B) return B;
            if (Options.SleepEnd > Options.SleepStart) { if (WatchTime.Local.Hour >= Options.SleepStart && WatchTime.Local.Hour < Options.SleepEnd) return false; }
            else if (WatchTime.Local.Hour >= Options.SleepStart || WatchTime.Local.Hour < Options.SleepEnd) return false;
        }else if (Options.SleepStyle > 2 && Options.SleepStyle != 4) return B;
    }
    return true;
}

void WatchyGSR::RefreshCPU(){ RefreshCPU(0); }
void WatchyGSR::RefreshCPU(int Value){
    uint32_t C = 80;
    if (!WatchTime.DeadRTC && Battery.Last > Battery.MinLevel) {
        if (Value == GSR_CPUMAX) CPUSet.Locked = true;
        if (Value == GSR_CPUDEF) CPUSet.Locked = false;
        if (!CPUSet.Locked && Options.Performance != 2 && Value != GSR_CPULOW) C = (InTurbo() || Value == GSR_CPUMID) ? 160 : 80;
        if (WatchyAPOn || OTAUpdate || GSRWiFi.Requests > 0 || CPUSet.Locked || (Options.Performance == 0 && Value != GSR_CPULOW)) C = 240;
    }
    if (C != CPUSet.Freq) if (setCpuFrequencyMhz(C)); CPUSet.Freq = C;
}

bool WatchyGSR::OTA(){
    esp_partition_iterator_t IT = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (IT != NULL){
      const esp_partition_t *Part = esp_partition_get(IT);
      uint64_t Size = Part->size;
      esp_partition_iterator_release(IT);
      return (Size == 0x1E0000);
    }
    return false;
}

// Function to find the existing WiFi power in the static index.

uint8_t WatchyGSR::getTXOffset(wifi_power_t Current){
    uint8_t I;
    for (I = 0; I < 11; I++){ if (RawWiFiTX[I] == Current) return I; }
    return 0;
}

void WatchyGSR::DisplayInit(bool ForceDark){
#ifdef GxEPD2DarkBorder
  display.epd2.setDarkBorder(Options.Border | ForceDark);
#endif
  if (Updates.Init){
    display.init(0,Rebooted,10,true);  // Force it here so it fixes the border.
    Updates.Init=false;
    Rebooted=false;
  }
}

void WatchyGSR::DisplaySleep(){ if (!Updates.Init) { Updates.Init = true; display.hibernate(); } }

bool WatchyGSR::SafeToDraw() { return (!(OTAUpdate || WatchyAPOn || (Menu.Item == GSR_MENU_TOFF && Menu.SubItem == 2))); }
bool WatchyGSR::NoMenu() { return (GuiMode == GSR_WATCHON); };

void WatchyGSR::getAngle(uint16_t Angle, uint8_t Width, uint8_t Height, uint8_t &X, uint8_t &Y){
    float fX, fY, fA;
    if (Angle > 44 && Angle < 135){ // Right
        fA = Angle - 45; fA /= 90;
        fY = (fA * Height);
        fX = Width - 1;
    }else if (Angle > 134 && Angle < 225){ // Bottom
        fA = Angle - 135; fA /= 90;
        fX = Width - (fA * Width);
        fY = Height - 1;
    }else if (Angle > 224 && Angle < 315){ // Left.
        fA = Angle - 225; fA /= 90;
        fY = Height - (fA * Height);
        fX = 0;
    }else { // Top
        if (Angle > 314) Angle -= 315;
        else Angle += 45;
        fA = Angle; fA /= 90;
        fX = (fA * Width);
        fY = 0;
    }
    X = fX;
    Y = fY;
}

// Watch Face designs are here.

void WatchyGSR::initWatchFaceStyle(){
    uint8_t Style = Options.WatchFaceStyle;
    if (DefaultWatchStyles) { if (Style > BasicWatchStyles && Style != 255) { InsertInitWatchStyle(Style); return; } }
    else if (WatchStyles.Count > 0 && BasicWatchStyles == -1) { InsertInitWatchStyle(Style); return; }
    else Style = 0;
    switch (Style){
      case 1: // Ballsy
          Design.Menu.Top = 117;
          Design.Menu.Header = 25;
          Design.Menu.Data = 66;
          Design.Menu.Gutter = 3;
          Design.Menu.Font = &aAntiCorona12pt7b;
          Design.Menu.FontSmall = &aAntiCorona11pt7b;
          Design.Menu.FontSmaller = &aAntiCorona10pt7b;
          Design.Face.Bitmap = nullptr;
          Design.Face.SleepBitmap = nullptr;
          Design.Face.Gutter = 4;
          Design.Face.Time = 56;
          Design.Face.TimeHeight = 45;
          Design.Face.TimeColor = GxEPD_BLACK;
          Design.Face.TimeFont = &aAntiCorona36pt7b;
          Design.Face.TimeLeft = 0;
          Design.Face.TimeStyle = WatchyGSR::dCENTER;
          Design.Face.Day = 54;
          Design.Face.DayGutter = 4;
          Design.Face.DayColor = GxEPD_BLACK;
          Design.Face.DayFont = &aAntiCorona16pt7b;
          Design.Face.DayFontSmall = &aAntiCorona15pt7b;
          Design.Face.DayFontSmaller = &aAntiCorona14pt7b;
          Design.Face.DayLeft = 0;
          Design.Face.DayStyle = WatchyGSR::dCENTER;
          Design.Face.Date = 106;
          Design.Face.DateGutter = 4;
          Design.Face.DateColor = GxEPD_BLACK;
          Design.Face.DateFont = &aAntiCorona15pt7b;
          Design.Face.DateFontSmall = &aAntiCorona14pt7b;
          Design.Face.DateFontSmaller = &aAntiCorona13pt7b;
          Design.Face.DateLeft = 0;
          Design.Face.DateStyle = WatchyGSR::dCENTER;
          Design.Face.Year = 160;
          Design.Face.YearLeft = 99;
          Design.Face.YearColor = GxEPD_BLACK;
          Design.Face.YearFont = &aAntiCorona14pt7b;
          Design.Face.YearLeft = 0;
          Design.Face.YearStyle = WatchyGSR::dCENTER;
          Design.Status.WIFIx = 40;
          Design.Status.WIFIy = 82;
          Design.Status.BATTx = 120;
          Design.Status.BATTy = 66;
          break;
      case 2:
         Design.Menu.Top = 56;
          Design.Menu.Header = 25;
          Design.Menu.Data = 66;
          Design.Menu.Gutter = 3;
          Design.Menu.Font = &aAntiCorona12pt7b;
          Design.Menu.FontSmall = &aAntiCorona11pt7b;
          Design.Menu.FontSmaller = &aAntiCorona10pt7b;
          Design.Face.Bitmap = nullptr;
          Design.Face.SleepBitmap = nullptr;
          Design.Face.Gutter = 4;
          Design.Face.Time = 134;
          Design.Face.TimeHeight = 48;
          Design.Face.TimeColor = GxEPD_BLACK;
          Design.Face.TimeFont = &TRAN48pt7b;
          Design.Face.TimeLeft = 0;
          Design.Face.TimeStyle = WatchyGSR::dCENTER;
          Design.Face.Day = 44;
          Design.Face.DayGutter = 4;
          Design.Face.DayColor = GxEPD_BLACK;
          Design.Face.DayFont = &TRAN19pt7b;
          Design.Face.DayFontSmall = &TRAN19pt7b;
          Design.Face.DayFontSmaller = &TRAN19pt7b;
          Design.Face.DayLeft = 0;
          Design.Face.DayStyle = WatchyGSR::dCENTER;
          Design.Face.Date = 170;
          Design.Face.DateGutter = 4;
          Design.Face.DateColor = GxEPD_BLACK;
          Design.Face.DateFont = &TRAN19pt7b;
          Design.Face.DateFontSmall = &TRAN19pt7b;
          Design.Face.DateFontSmaller = &TRAN19pt7b;
          Design.Face.DateLeft = 0;
          Design.Face.DateStyle = WatchyGSR::dLEFT;
          Design.Face.Year = 170;
          Design.Face.YearLeft = 99;
          Design.Face.YearColor = GxEPD_BLACK;
          Design.Face.YearFont = &TRAN19pt7b;
          Design.Face.YearLeft = 0;
          Design.Face.YearStyle = WatchyGSR::dRIGHT;
          Design.Status.WIFIx = 5;
          Design.Status.WIFIy = 193;
          Design.Status.BATTx = 155;
          Design.Status.BATTy = 178;
          break;
       default:
          Design.Menu.Top = 72;
          Design.Menu.Header = 25;
          Design.Menu.Data = 66;
          Design.Menu.Gutter = 3;
          Design.Menu.Font = &aAntiCorona12pt7b;
          Design.Menu.FontSmall = &aAntiCorona11pt7b;
          Design.Menu.FontSmaller = &aAntiCorona10pt7b;
          Design.Face.Bitmap = nullptr;
          Design.Face.SleepBitmap = nullptr;
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
          Design.Status.WIFIy = 193;
          Design.Status.BATTx = 155;
          Design.Status.BATTy = 178;
          break;
   }
}

void WatchyGSR::drawWatchFaceStyle(){
    uint8_t X, Y;
    uint16_t A;
    uint8_t Style = Options.WatchFaceStyle;
    if (DefaultWatchStyles) { if (Style > BasicWatchStyles && Style != 255) { InsertDrawWatchStyle(Style); return; } }
    else if (WatchStyles.Count > 0 && BasicWatchStyles == -1) { InsertDrawWatchStyle(Style); return; }
    else Style = 0;
    switch (Style){
        case 1: // Ballsy
            drawDay();
            drawDate();
            if (SafeToDraw()){
                for (A = 0; A < 60; A++){
                    getAngle(A * 6, 190, 190, X, Y);
                    display.fillCircle(X + 5, Y + 5, (A == WatchTime.Local.Minute ? 5 : (A % 5 == 0 ? 3 : 1)), ForeColor());
                }
                X = WatchTime.Local.Hour;
                if (X > 11) X -= 12;
                A = (X * 30) + (WatchTime.Local.Minute / 2);
                getAngle(A, 158, 158, X, Y);
                display.fillCircle(X + 21, Y + 21, 9, ForeColor());
                if (WatchTime.Local.Hour < 12) display.fillCircle(X + 21, Y + 21, 3, BackColor());
            }
            if (NoMenu()) drawYear();
            break;
        case 2:
            if (SafeToDraw()){
                if (NoMenu()){
                    drawTime(96); // Pad + no AM.
                    if (IsAM()){
                        display.setFont(&TRAN6pt7b);
                        display.setTextColor(Design.Face.TimeColor);
                        display.setCursor(180, Design.Face.Time - (Design.Face.TimeHeight + 22));
                        display.print("AM");
                    }
                }
                drawDay();
                drawYear();
                drawDate(true); // Short month only.
/*                display.setTextColor(Design.Face.TimeColor);  Bad fonts.
                display.setFont(&TRAN15pt7b);
                uint16_t Width, Height;
                String S = String(SBMA.getCounter());
                int16_t X, Y;
                display.getTextBounds(S, 4, 196, &X, &Y, &Width, &Height);
                X = constrain(156 - Width, 44, 156);
                display.setCursor(X, 192);
                display.print(S);*/ 
            }
            break;
        default:
            if (SafeToDraw()){
                drawTime();
                drawDay();
                drawYear();
            }
            if (NoMenu()) drawDate();
            break;
    }
}
