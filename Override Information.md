**PROPER OVERRIDE**

Don't edit Watchy_GSR.cpp or Watchy_GSR.h, instead, edit GSR.ino as all of the overriding can be done within that file, this way that one file can be used as your project file, allowing you to override the necessary parts of the Watchy_GSR without having to re-edit updates.  Just remember to backup your GSR.ino project file before updating (to ensure you don't lose it by accident).

**Overridding Watch_GSR functionality**

A structure allows you to reposition AND change font color along with font for each section:

| Design Element  | Default | Description |
| --------------- | ------- | ----------- |
| Design.Menu.Top | 72 | Top of where the Menu starts vertically on the screen. |
| Design.Menu.Header | **25** | Vertical baseline of Menu Header (section) **from Design.Menu.Top**. |
| Design.Menu.Data | **66** | Vertical baseline of Menu Data (value to set/change) **from Design.Menu.Top**. |
| Design.Menu.Gutter | **3** | Horizontal spacing from edge of display for Menu Header & Data items. (1.4.3C+) |
| Design.Menu.Font | &aAntiCorona12pt7b | Font used for Menu Header & Data display. |
| Design.Menu.FontSmall | &aAntiCorona11pt7b | Font used for Menu Header & Data display when above is too large. (1.4.3C+) |
| Design.Menu.FontSmaller | &aAntiCorona10pt7b | Font used for Menu Header & Data display when above is too large. (1.4.3C+) |
| Design.Face.Gutter | **4** | Horizontal spacing from edge of display for Face elements (Time, AM/PM indicator, Year, etc). (1.4.3C+) |
| Design.Face.Bitmap | {Unused} | A bitmap to place in the background of the Watch face during operation (overridable by OverrideBitmap below). |
| Design.Face.SleepBitmap | {Unused} | A bitmap use during Screen Blanking (overridable by OverrideSleepBitmap below). |
| Design.Face.Time | 56 | Vertical baseline of Time on screen. |
| Design.Face.TimeHeight | 45 | Height of font used for PM indicator so it will sit at the top of TimeFont. |
| Design.Face.TimeColor | GxEPD_BLACK | Color the Time is drawn in. |
| Design.Face.TimeFont | &aAntiCorona36pt7b | Font used for Time display. |
| Design.Face.TimeLeft | {Unused} | Left point to start drawing Time display. |
| Design.Face.TimeStyle | WatchyGSR::dCENTER | Alignment method for Time display.  If set for WatchyGSR:dRIGHT, AM/PM indicator will be on the left. |
| Design.Face.Day | 101 | Vertical baseline of Day of Week on screen. |
| Design.Face.DayGutter | **4** | Horizontal spacing from edge of display for Day of Week element. (1.4.3C+) |
| Design.Face.DayColor | GxEPD_BLACK; | Color the Day of Week is drawn in. |
| Design.Face.DayFont | &aAntiCorona16pt7b | Font used for Day of Week display. |
| Design.Face.DayFontSmall | &aAntiCorona15pt7b | Font used for Day of Week display when above is too large. (1.4.3C+) |
| Design.Face.DayFontSmaller | &aAntiCorona14pt7b | Font used for Day of Week display when above is too large. (1.4.3C+) |
| Design.Face.DayLeft | {Unused} | Left point to start drawing Day of Week display. |
| Design.Face.DayStyle | WatchyGSR::dCENTER | Alignment method for Day of Week display. |
| Design.Face.Date | 143 | Vertical baseline of Date on screen. |
| Design.Face.DateGutter | **4** | Horizontal spacing from edge of display for Date element. (1.4.3C+) |
| Design.Face.DateColor | GxEPD_BLACK | Color the Date is drawn in. |
| Design.Face.DateFont | &aAntiCorona15pt7b | Font used for Date display. |
| Design.Face.DateFontSmall | &aAntiCorona14pt7b | Font used for Date display when above is too large. (1.4.3C+) |
| Design.Face.DateFontSmaller | &aAntiCorona13pt7b | Font used for Date display when above is too large. (1.4.3C+) |
| Design.Face.DateLeft | {Unused} | Left point to start drawing Date display. |
| Design.Face.DateStyle | WatchyGSR::dCENTER | Alignment method for Date display. |
| Design.Face.Year | 186 | Vertical baseline of Year on screen. |
| Design.Face.YearColor | GxEPD_BLACK | Color the Year is drawn in. |
| Design.Face.YearFont | &aAntiCorona16pt7b | Font used for Year display. |
| Design.Face.YearLeft | {Unused} | Left point to start drawing Year display. |
| Design.Face.YearStyle | WatchyGSR::dCENTER | Alignment method for Year display. |
| Design.Status.WIFIx | 5 | Left edge of WIFI status on screen. |
| Design.Status.WIFIy | 193 | Vertical baseline of WIFI status on screen. |
| Design.Status.BATTx | 155 | Left edge of Battery state on screen. |
| Design.Status.BATTy | 178 | Vertical baseline of Battery state on screen. |

Functions available for overriding:

| Function Name | Usage |
| ------------- | --------------------------------- |
| showWatchFace() | Override to change the entire WatchFace drawing. |
| drawWatchFace() | Override to change the entire drawing of the WatchFace except screen refreshing. |
| drawTime() | Override to change the format of how the Time is drawn. |
| drawDay() | Override to change the format of how the Day of Week is drawn. |
| drawDate() | Override to change the format of how the Date is drawn. |
| drawYear() | Override to change the format of how the Year is drawn. |
| drawChargeMe() | Override to change the format of how the Battery Status is drawn. |
| drawStatus() | Override to change the format of how the current WiFi Status is drawn.  Also shows timer/alarms when running. |
 
Functions for inserting extra code in places.

| Function Name | Usage |
| ------------- | --------------------------------- |
| InsertPost() | This Function offers a post "boot" insert, so you can make changes after settings are loaded. |
| InsertNTPServer() | Use this to return "your favorite NTP Server". See **Version 1.4.3 Additions** below. |
| InsertDefaults() | This Function is done at the end of setupDefaults(), so you can add your own defaults. |
| OverrideBitmap() | Allows you to replace the drawing of this bitmap with either your own or nothing at all, sent `false` on return to tell it not to draw the Design.Face.Bitmap. |
| InsertOnMinute() | This Function is called once the Clock has been updated to the new minute but before the screen is drawn. |
| InsertWiFi() | This Function is called repeatedly in a loop only *IF* WiFi has been enabled and connected, only use this if you asked for it. |
| InsertWiFiEnding() | This Function is called when WiFi has been turned off. |
| InsertAddWatchStyles() | Use this function to add Watch Styles starting from Index 0 [`AllowDefaultWatchStyles(false)`] or from Index 2 if allowing default Watch Styles. |
| InsertNeedAwake() | Set in 3 sections of the Active Mode loop, gives the overriding the ability to keep the Watchy Active and run code in the loop. |
| InsertInitWatchStyle() | The init function as seen at the end of Watchy_GSR.cpp.  See **Version 1.4.2 Additions** below. |
| InsertDrawWatchStyle() | The Draw function as seen at the bottom of Watchy_GSR.cpp. See **Version 1.4.2 Additions** below. |
| InsertHandlePressed() | Allows you to intercept SW2 (SW1 requires `OverrideDefaultMenu(true)`) to SW4 when nothing else did anything with them, only when the menu isn't open. |
| OverrideSleepBitmap() | Allows you to replace the drawing of this bitmap with either your own or nothing at all, sent `false` on return to tell it not to draw the Design.Face.SleepBitmap. |
 
Functions available for communication:

| Function Name | Usage |
| ------------- | --------------------------------- |
| handleButtonPress(uint8_t Pressed) | Accepts Switch # from 1 to 4, can "fake" a button press. |
| getButtonPins() | Returns 0 (none) 1=MENU, 2=BACK, 3=UP, 4=DOWN for the switch pressed at the moment, see **Version 1.4.5 Additions** below for more combinations. |
| OverrideDefaultMenu(bool Override) | Set whether or not the default menu is overridden (true/**{false}**). |
| ShowDefaultMenu() | Will show the Default Menu when called only if the menu is being overridden and not already open. |
| float getBatteryVoltage() | Returns a cleaned battery voltage. |
| IsDark() | Is the screen currently black (Screen Off has triggered from settings). |
| VibeTo(bool Mode) | Set VibeTo to `true` to enable vibration motor, `false` to stop it. |
| MakeTime(int Hour, int Minutes, bool &**Alarm**) | Use a variable in **Alarm** parameter always.  When **Alarm** is set to `false` you'll get normal Hour & Minutes format based on Options, **Alarm** will be `true` for PM.  Setting **Alarm** to `true` for Alarm format.  Returns a String. |
| MakeHour(uint8_t Hour) | Return the hour formatted in a String using 12 or 24hr format. |
| MakeMinutes(uint8_t Minutes) | Returns a string of the Minutes. |
| ClockSeconds() | Updates the raw UTC and also converts it to the local time (WatchTime.Local) but doesn't interfere with minute updates. |
| ForeColor() | Returns the current Fore (font) color for usage with current style. |
| BackColor() | Returns the current Background color for usage with current style. |
| AskForWiFi() | Tells the Watchy_GSR that your code wants WiFi, when it connects, you will see InsertWiFi() called, make sure you keep track of this yourself. |
| currentWiFi() | Returns `WL_CONNECTED` when connected or `WL_CONNECT_FAILED` when not, InsertWiFi() is only called when `WL_CONNECTED` happens and no other process is using it. |
| endWiFi() | Tell Watchy_GSR that you're finished with the WiFi, only do this *IF* you asked for it. |
| AllowDefaultWatchStyles(bool Allow) | Will state if you want (**{true}**/false) the original Watch Styles (Index 0 (Classic GSR), 1 (Ballsy) and 2 (LCD) to be used first). |
| AddWatchStyle(String StyleName) | Will return the Index of the added Watch Style (255 = error), 30 character max limit on Watch Style name. |
| NoMenu() | This returns `true` if the Menu **isn't** open. |
| getAngle(uint16_t Angle, uint8_t Width, uint8_t Height, uint8_t &X, uint8_t &Y) | Give it an Angle, Width and Height, X and Y will have those values, useful for Analog displays |
| SBMA.getCounter() | Will return a uint32_t value of the current steps taken. |

**NOTES ON WiFi**

If you plan to use WiFi, remember, users will want to actually keep using the Watchy_GSR underneath while you're using WiFi, so while it is nice to pack everything in at once, the `InsertWiFi()` function is repeatedly called until you tell it you're done by saying `EndWiFi()`, you may see another `InsertWiFi()` after doing so, just be sure to ignore any `InsertWiFi()` calls you didn't ask for.

Breaking up your WiFi functions so that they're only done in parts is best.  Anything you have to wait for, make an int that tells you where you are in your work and when `InsertWiFi()` returns, continue where you left off.  When you're finished, make sure you `EndWiFi()` and make note you finished on your end by zeroing your int.

Recommend an int variable being set to 1 when you `AskForWiFi()`, then when `InsertWiFi()` is called, you do part of the work, ++ the int, when it returns again, repeat until you're finished.  When you finish, call `EndWiFi()` and set your int to 0.  When you receive an `InsertWiFiEnd()`, set your int to 0, this way, your code will always work properly.  You can even test in `InsertOnMinute()` if your int is 0 before commencing so you don't try twice.  Only `AskForWiFi()` once and only call `EndWiFi()` once or any other operation using WiFi may be cancelled.

| TIME Structure | Contents |
| -------------------- | ------------------ |
| WatchTime.Local.Second | Contains the current second(s) in Local time. |
| WatchTime.Local.Minute | Contains the current minute(s) in Local time. |
| WatchTime.Local.Hour | Contains the current Hour in Local time. |
| WatchTime.Local.Wday | Contains the Day of Week (Days since Sunday) in Local time. |
| WatchTime.Local.Day | Contains the Date (1 to 31) in Local time. |
| WatchTime.Local.Month | Contains the Month (0 to 11) in Local time. |
| WatchTime.Local.Year | Contains the Year (since 1900) in Local time. |
| | |
| WatchTime.UTC.Second | Contains the current second(s) in Coordinated Universal time. |
| WatchTime.UTC.Minute | Contains the current minute(s) in Coordinated Universal time. |
| WatchTime.UTC.Hour | Contains the current Hour in Coordinated Universal time. |
| WatchTime.UTC.Wday | Contains the Day of Week (Days since Sunday) in Coordinated Universal time. |
| WatchTime.UTC.Day | Contains the Date (1 to 31) in Coordinated Universal time. |
| WatchTime.UTC.Month | Contains the Month (0 to 11) in Coordinated Universal time. |
| WatchTime.UTC.Year | Contains the Year (since 1970) in Coordinated Universal time. |
| | |
| WatchTime.BedTime | This will be `true` if the time is within Screen Off's Sleeping range, even if it isn't in use. |

**Version 1.4.2 Additions**

The addition of the following functions will allow overriding both the original Watch Styles or adding to them, the author's choice.  This is all done without writing any changes to the Watchy_GSR base code, so as to avoid having to re-apply changes to that code after version changes.

Overriding `InsertDefaults()` function, you can call this function:

`AllowDefaultWatchStyles({true}/false);` // Will state if you want the original Watch Styles (Index 0 (Classic GSR) and 1 (Ballsy) to be used first).

Just after the default Watch Styles are added (if told to), another function will be called:

`InsertAddWatchStyles()` // Use this function to add Watch Styles starting from Index 0 [`AllowDefaultWatchStyles(false)`] or from Index 2 if allowing default Watch Styles.

`uint8_t AddWatchStyle(string StyleName)` // Will return the Index of the added Watch Style (255 = error), 30 character max limit on Watch Style name.

Override these two functions to add your Init and Draw for the Watch Styles:

`void InsertInitWatchStyle(uint8_t StyleID)`  // The init function as seen at the end of Watchy_GSR.cpp.

`void InsertDrawWatchStyle(uint8_t StyleID)`  // The Draw function as seen at the bottom of Watchy_GSR.cpp.

Overriding in this manner can be done using a switch and only checking for your Indexes, but you could also just as easily use an IF statement and record your Watch Styles in uint8_t variables for each during `InsertAddWatchStyles()`.  The best part about this is, all of this is done PRIOR to the settings reload from NVS (if not disabled), so your chosen Watch Style will remain after a reboot.

**Version 1.4.3 Additions**

`void InsertNTPServer()` { return "Your favorite NTP Server"; } // Will let you pick your favorite NTP server.

**Version 1.4.3E Changes**

Removed `void InsertBitmap()`

`void OverrideBitmap()` { return true/{false}; } // This function lets you stop the Design.Face.Bitmap from drawing if you return `true`.

`void OverrideSleepBitmap()` { return true/{false}; } // This function lets you stop the Design.Face.SleepBitmap from drawing if you return `true`.

**Version 1.4.4 Additions**

Locale and Menu Override are the main offerings available, instructions will be forthcoming for the Locale setup.  For Menu Overriding, users can tell Watchy GSR that they'll handle the Menu button, can also call `ShowDefaultMenu()` to bring the Watchy GSR Default Menu up for usage when wanted, if the author makes a mistake or there is a bug, the standard Watchy GSR Default Menu can be called up by holding the Menu button down for 10 seconds.

**Version 1.4.4 Additions**

getButtonPins() has 4 new values:  MENU + UP = 5, BACK + UP = 6, DOWN + MENU = 7 and DOWN + BACK = 8
This is currently in **BETA** but the functionality is 100% stable, just the button press duration may be too short to catch both buttons properly.

**Version 1.4.6 Additions**

`bool InsertNeedAwake(bool GoingAsleep)` { return true/{false}; } // This function runs 3 times per loop, lets you tell the Watchy to stay Awake (Active Mode), you can also run code in this as part of the main loop while in Active Mode.  Return value tells the Watchy to stay in Active Mode (Awake = `true`).

**How to Make Your Own Version**

First, use the Library Manager and give it a release zip, this will install Watchy_GSR as a library, go into the Watchy_GSR library and move the GSR.ino out of it and place it into where your project is.  Rename it to something like MyGSR.ino and edit it, you can do all of your overrides in that.  That ino file now becomes your main compile file.

If you're updating Watchy_GSR, be sure to remove the GSR.ino from it after the Library Manager has imported everything.
