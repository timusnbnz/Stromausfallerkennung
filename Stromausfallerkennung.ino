#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"

const byte usbchar[] = {B01110, B01110, B01110, B00100, B00100, B00010, B00010, B00100};
const byte errorchar[] = {B01110, B01110, B01110, B00100, B00000, B00100, B01110, B00100};
const byte powerchar[] = {B01100, B00110, B00011, B11111, B11000, B01100, B00110, B00010};
const byte batterychar0[] = {B01110, B11011, B10001, B10001, B10001, B10001, B10001, B11111};
const byte batterychar1[] = {B01110, B11011, B10001, B10001, B11111, B11111, B11111, B11111};
const byte batterychar2[] = {B01110, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
const byte settingschar[] = {B01010, B10001, B10001, B11011, B01110, B01110, B01110, B01110};
const int pin_background = 9;
const int pin_contrast = 10;
const int pin_right = 11;
const int pin_middle = 12;
const int pin_left = 13;
const int pin_mains = A2;
const int pin_battery = A3;
const float analogmultiplier = 0.004887586;
const int buttonresponsivenes = 200;
const int rom_brightness = 125;
const int rom_contrast = 105;
const int rom_timeout = 20;
const int rom_dimming = 1;

unsigned long lastclick = 0;
unsigned long lastmillis = 0;
unsigned long outtagemillis = 0;
unsigned long outtageendmillis = 0;

DateTime outtageDateTime;
DateTime now;

int outtageBeginSeconds;
int outtageBeginMinutes;
int outtageBeginHours = -1;
int outtageBeginDay;
int outtageBeginMonth;
int outtageBeginYear;
int outtageTime;
int outtageSeconds;
int outtageMinutes;
int outtageHours;
int outtageLockVar;
int timeout;
int brightness;
int contrast;
int dimming;
int percent;
float vbat;
boolean mains;

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);
RTC_DS1307 rtc;

void setup() {
  Serial.begin(115200);

  pinMode(pin_background, OUTPUT);
  pinMode(pin_contrast, OUTPUT);
  pinMode(pin_left, INPUT);
  pinMode(pin_middle, INPUT);
  pinMode(pin_right, INPUT);

  lcd.begin(16, 2);
  lcd.createChar(2, usbchar);
  lcd.createChar(3, errorchar);
  lcd.createChar(4, powerchar);
  lcd.createChar(5, batterychar0);
  lcd.createChar(6, batterychar1);
  lcd.createChar(7, batterychar2);
  lcd.createChar(8, settingschar);
  lcd.home();

  eepromload();
  lcdsettings(contrast, brightness);
  systemcheck();
  startmenu();
}

void eepromload() {
  brightness = EEPROM.read(0);
  contrast = EEPROM.read(1);
  timeout = EEPROM.read(2);
  dimming = EEPROM.read(3);

}

void eepromupdate() {
  EEPROM.update(0, brightness);
  EEPROM.update(1, contrast);
  EEPROM.update(2, timeout);
  EEPROM.update(3, dimming);
}

void lcdsettings(int contrast, int brightness) {
  analogWrite(pin_background, brightness);
  analogWrite(pin_contrast, contrast);
}

void error(String error) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(3);
  lcd.setCursor(2, 0);
  lcd.print(error);
  lcd.setCursor(0, 1);
  lcd.print("Fix and reset");
  while (1);
}

void contrastmenu() {
  boolean breakup = true;
  delay(buttonresponsivenes);
  while (breakup) {
    backgroundservice();
    if (digitalRead(pin_left) == HIGH) {
      if (contrast != 0) {
        contrast -= 5;
        lcd.clear();
        delay(30);
      }
    }
    if (digitalRead(pin_right) == HIGH) {
      if (contrast != 255) {
        contrast += 5;
        delay(30);
      }
    }
    lcd.setCursor(1, 0);
    lcd.print("Kontrast");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(13, 1);
    lcd.print(">");
    lcd.setCursor(1, 1);
    int i = contrast / 20;
    while (i != 0) {
      i -= 1;
      lcd.print("=");
    }
    if (digitalRead(pin_middle) == HIGH) {
      lcd.clear();
      eepromupdate();
      breakup = false;
    }
  }
}

void brightnessmenu() {
  boolean breakup = true;
  delay(buttonresponsivenes);
  while (breakup) {
    backgroundservice();
    if (digitalRead(pin_left) == HIGH) {
      if (brightness != 0) {
        brightness -= 5;
        lcd.clear();
        delay(30);
      }
    }
    if (digitalRead(pin_right) == HIGH) {
      if (brightness != 255) {
        brightness += 5;
        delay(30);
      }
    }
    lcd.setCursor(1, 0);
    lcd.print("Helligkeit");
    lcd.setCursor(0, 1);
    lcd.print("<");
    lcd.setCursor(13, 1);
    lcd.print(">");
    lcd.setCursor(1, 1);
    int i = brightness / 20;
    while (i != 0) {
      i -= 1;
      lcd.print("=");
    }
    if (digitalRead(pin_middle) == HIGH) {
      lcd.clear();
      eepromupdate();
      breakup = false;
    }
  }
}

void timeoutmenu() {
  boolean breakup = true;
  delay(buttonresponsivenes);
  while (breakup) {
    backgroundservice();
    if (digitalRead(pin_left) == HIGH) {
      if (timeout > 10) {
        timeout -= 5;
      }
      else {
        timeout = 0;
      }
      lcd.clear();
      delay(buttonresponsivenes);
    }
    if (digitalRead(pin_right) == HIGH) {
      if (timeout != 120) {
        if (timeout == 0) {
          timeout += 5;
        }
        timeout += 5;
        lcd.clear();
        delay(buttonresponsivenes);
      }
    }
    lcd.setCursor(1, 0);
    lcd.print("Timeout");
    lcd.setCursor(0, 1);
    lcd.setCursor(1, 1);
    if (timeout == 0) {
      lcd.print("Deaktiviert");
    } else {
      lcd.print(timeout);
      lcd.print("s");
    }
    if (digitalRead(pin_middle) == HIGH) {
      lcd.clear();
      eepromupdate();
      breakup = false;
    }
  }
}

void dimmingmenu() {
  boolean breakup = true;
  delay(buttonresponsivenes);
  while (breakup) {
    backgroundservice();
    if ((digitalRead(pin_left) == HIGH) or (digitalRead(pin_right) == HIGH)) {
      delay(buttonresponsivenes);
      lcd.clear();
      if (dimming == 1) {
        dimming = 0;
      } else {
        dimming = 1;
      }
    }
    lcd.setCursor(1, 0);
    lcd.print("Dimming");
    lcd.setCursor(0, 1);
    lcd.setCursor(1, 1);
    if (dimming == 1) {
      lcd.print("Aktiviert");
    } else {
      lcd.print("Deaktiviert");
    }
    if (digitalRead(pin_middle) == HIGH) {
      lcd.clear();
      eepromupdate();
      breakup = false;
    }
  }
}

void settings() {
  int selector = 0;
  boolean working = true;
  delay(buttonresponsivenes);
  while (working) {
    backgroundservice();
    if (digitalRead(pin_left) == HIGH) {
      if (selector != -1) {
        selector -= 1;
        lcd.clear();
        delay(buttonresponsivenes);
      }
    }
    if (digitalRead(pin_right) == HIGH) {
      if (selector != 4) {
        selector += 1;
        lcd.clear();
        delay(buttonresponsivenes);
      }
    }
    
    if (selector == -1) {
      lcd.setCursor(0, 0);
      lcd.print("Verlassen      >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        working = false;
      }
    }
    
    if (selector == 0) {
      lcd.setCursor(0, 0);
      lcd.print("<   Kontrast   >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        contrastmenu();
        delay(buttonresponsivenes);
      }
    }
    
    if (selector == 1) {
      lcd.setCursor(0, 0);
      lcd.print("<  Helligkeit  >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        brightnessmenu();
        delay(buttonresponsivenes);
      }
    }
    
    if (selector == 2) {
      lcd.setCursor(0, 0);
      lcd.print("<   Timeout    >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        timeoutmenu();
        delay(buttonresponsivenes);
      }
    }
    
    if (selector == 3) {
      lcd.setCursor(0, 0);
      lcd.print("<   Dimming    >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        dimmingmenu();
        delay(buttonresponsivenes);
      }
    }
    
    if(selector == 4){
      lcd.setCursor(0,0);
      lcd.print("< Developed by: ");
      lcd.setCursor(0,1);
      lcd.print("Daniel & Tim ;D");
    }
  }
}

void startmenu() {
  int selector = 0;
  while (1) {
    backgroundservice();
    
    if (digitalRead(pin_left) == HIGH) {
      if (selector != -1) {
        selector -= 1;
        lcd.clear();
        delay(buttonresponsivenes);
      }
    }
    if (digitalRead(pin_right) == HIGH) {
      if (selector != 3) {
        selector += 1;
        lcd.clear();
        delay(buttonresponsivenes);
      }
    }
    
    if (selector == 3) {
      lcd.setCursor(0, 0);
      if (outtageLockVar == 1) {
        lcd.print("< Akt. Ausfall");
        lcd.setCursor(0, 1);
        if (outtageHours < 10) {
          lcd.print("0");
        }
        lcd.print(outtageHours);
        lcd.print(":");
        if (outtageMinutes < 10) {
          lcd.print("0");
        }
        lcd.print(outtageMinutes);
        lcd.print(":");
        if (outtageSeconds < 10) {
          lcd.print("0");
        }
        lcd.print(outtageSeconds);
      } else {
        lcd.print("< Verg. Ausfall");
        lcd.setCursor(0, 1);
        if (outtageBeginHours == -1) {
          lcd.print("Keine Daten");
        } else {
          if ((now.second() % 2) == 0) {
            lcd.setCursor(0, 1);
            if (outtageHours < 10) {
              lcd.print("0");
            }
            lcd.print(outtageHours);
            lcd.print(":");
            if (outtageMinutes < 10) {
              lcd.print("0");
            }
            lcd.print(outtageMinutes);
            lcd.print(":");
            if (outtageSeconds < 10) {
              lcd.print("0");
            }
            lcd.print(outtageSeconds);
            lcd.print("       ");
          } else {
            if (outtageBeginHours < 10) {
              lcd.print("0");
            }
            lcd.print(outtageBeginHours);
            lcd.print(":");
            if (outtageBeginMinutes < 10) {
              lcd.print("0");
            }
            lcd.print(outtageBeginMinutes);
            lcd.print(" ");
            lcd.print(outtageDateTime.toString("DD/MM/YY"));
          }
        }
      }

    }
    
    if (selector == 2) {
      lcd.setCursor(0, 0);
      lcd.print("<   Eingang    >");
      if ((millis() - lastmillis) > 1000) {
        lastmillis = millis();
        lcd.setCursor(0, 1);
        lcd.print(analogRead(pin_battery) * 2 * analogmultiplier);
        lcd.print("V");
      }
    }
    
    if (selector == 1) {
      if ((millis() - lastmillis) > 1000) {
        lcd.setCursor(0, 0);
        lcd.print("<   Batterie   >");
        lcd.setCursor(0, 1);
        lastmillis = millis();
        vbat = analogRead(pin_battery) * 2 * analogmultiplier;
        percent = (vbat - 5) * 22.25;
        lcd.print(vbat);
        lcd.print("V");
        lcd.setCursor(10, 1);
        if (percent > 100) {
          percent = 100;
        }
        if (percent < 0) {
          percent = 0;
        }
        if (percent < 99) {
          lcd.print(" ");
        }
        lcd.print(percent);
        lcd.print("%");
      }

    }
    if (selector == 0) {
      lcd.setCursor(0, 0);
      lcd.print("< Datum & Uhr  >");
      lcd.setCursor(0, 1);
      if (now.hour() < 10) {
        lcd.print("0");
      }
      lcd.print(now.hour());
      if ((now.second() % 2) == 0) {
        lcd.print(":");
      } else {
        lcd.print(" ");
      }
      if (now.minute() < 10) {
        lcd.print("0");
      }
      lcd.print(now.minute());
      lcd.print(" ");
      lcd.print(now.toString("DD/MM/YY"));
    }
    if (selector == -1) {
      backgroundservice();
      lcd.setCursor(0, 0);
      lcd.print("Einstellungen  >");
      lcd.setCursor(0, 1);
      lcd.print("-> Enter");
      if (digitalRead(pin_middle) == HIGH) {
        lcd.clear();
        settings();
        delay(buttonresponsivenes);

      }
    }
  }
}

void systemcheck() {
  lcd.clear();
  if (digitalRead(pin_left) == digitalRead(pin_middle) == digitalRead(pin_right) == HIGH) {
    lcd.setCursor(0, 0);
    lcd.print("EEPROM Reset");
    brightness = rom_brightness;
    contrast = rom_contrast;
    timeout = rom_timeout;
    dimming = rom_dimming;
    eepromupdate();
  }
  lcd.setCursor(0, 0);
  lcd.print("System-Selfcheck");
  while ((digitalRead(pin_left) == HIGH) or (digitalRead(pin_middle) == HIGH) or (digitalRead(pin_right) == HIGH)) {
    lcd.setCursor(0, 1);
    lcd.print("Button-Problem");
    delay(100);
  }
  lcd.clear();
  if (! rtc.begin())
  {
    error("RTC-Fehler");
  }
  if (! rtc.isrunning())
  {
    error("RTC stellen");
  }
}

void backgroundservice() {
  now = rtc.now();
  if (now.minute() == 165) {
    error("RTC-Versagen");
  }
  if (digitalRead(pin_left) == HIGH or digitalRead(pin_right) == HIGH or digitalRead(pin_middle) == HIGH) {
    lastclick = millis();
  }
  if (analogRead(pin_mains) < 512) {
    mains = false;
  } else {
    mains = true;
  }
  lcd.setCursor(15, 1);
  if (not mains) {
    if (outtageLockVar == 0) {//Stromausfallbeginn Handler
      lcd.clear();
      outtageDateTime = rtc.now();
      outtagemillis = millis();
      outtageBeginSeconds = outtageDateTime.second();
      outtageBeginMinutes = outtageDateTime.minute();
      outtageBeginHours = outtageDateTime.hour();
      outtageBeginDay = outtageDateTime.day();
      outtageBeginMonth = outtageDateTime.month();
      outtageBeginYear = (outtageDateTime.year()) - 2000;
      outtageLockVar = 1;
    } else if (outtageLockVar == 1) {//Stromausfall Handler
      outtageendmillis = millis();
      outtageTime = (outtageendmillis - outtagemillis) / 1000;
      outtageSeconds = outtageTime % 60;
      outtageTime = (outtageTime - outtageSeconds) / 60;
      outtageMinutes = outtageTime % 60;
      outtageTime = (outtageTime - outtageMinutes) / 60;
      outtageHours = outtageTime;
    }
    //Handler für Icon unten Rechts
    vbat = analogRead(pin_battery) * 2 * analogmultiplier;
    if (vbat < 5) {
      lcd.write(2);
    } else {
      percent = (vbat - 5) * 22.25;
      if (percent > 75) {
        lcd.write(7);
      } else if (percent < 25) {
        lcd.write(5);
      } else {
        lcd.write(6);
      }
      if (dimming == 1) {
        lcdsettings(contrast, brightness * 0.2);
      }
    }
  } else {
    outtageLockVar = 0;
    lcd.write(4);
  }
  if ((millis() - lastclick) > (timeout * 1000)) {//Timeout und Dimming Handler
    if (timeout != 0) {
      lcdsettings(contrast, 0);
    }
  } else {
    if (not mains) {
      if (dimming == 1) {
        lcdsettings(contrast, (brightness * 0.5));
      }
    } else {
      lcdsettings(contrast, brightness);
    }

  }
}

void loop() {

}
