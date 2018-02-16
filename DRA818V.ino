// DRA818v Controller
//
// 14-February-2018
// Bruce MacKinnon KC1FSZ

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DebouncedSwitch.h>
#include <RotaryEncoder.h>
#include <ClickDetector.h>
#include <Utils.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define PIN_D2 2
#define PIN_D3 3
#define PIN_D4 4

DebouncedSwitch db2(3L);
DebouncedSwitch db3(3L);
DebouncedSwitch db4(3L);
RotaryEncoder renc(&db2,&db3);
ClickDetector cd4(&db4);

const unsigned long vfoSteps[] = { 10000, 100000, 1000000, 10000000 };

int mode = 0;

unsigned long vfoFreq = 146640000;
unsigned long vfoStepIndex = 0;
String transmitCTCSS = "0018";
String receiveCTCSS = "0000";
int volume = 5;
int squelch = 5;

unsigned long getMH(unsigned long f) {
  return f / 1000000L;
}

unsigned long getKH(unsigned long f) {
  return (f / 1000L) % 1000L;
}

unsigned long getH(unsigned long f) {
  return f % 1000L;
}

void updateRadioGroup() {

  //AT+DMOSETGROUP=0,152.1250,152.1250,0012,4,0003<
  Serial.print("AT+DMOSETGROUP=0,");
  Serial.print(getMH(vfoFreq));
  Serial.print(".");
  char buf[5];
  sprintf(buf,"%03lu",getKH(vfoFreq));
  Serial.print(buf);
  Serial.print("0");
  Serial.print(",");
  Serial.print(getMH(vfoFreq));
  Serial.print(".");
  sprintf(buf,"%03lu",getKH(vfoFreq));
  Serial.print(buf);
  Serial.print("0");
  Serial.print(",");
  Serial.print(transmitCTCSS);
  Serial.print(",");
  Serial.print(squelch);
  Serial.print(",");
  Serial.print(receiveCTCSS);
  Serial.write(13);
  Serial.write(10);
}

void updateRadioVolume() {
  Serial.print("AT+DMOSETVOLUME=");
  Serial.print(volume);
  Serial.write(13);
  Serial.write(10);
}

void updateDisplay() {
  
  int rowHeight = 16;
  int startX = 10;
  int y = 17;

  // Clear area
  display.fillRect(0,0,display.width(),display.height(),0);

  // Top
  display.setCursor(0,0);
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.print("KC1FSZ VHF");
  display.print("   ");

  if (mode == 0) {
    display.print("VFO");
  } else if (mode == 1) {
    display.print("VOL");
  } else if (mode == 2) {
    display.print("SQL");
  }

  display.drawLine(0,15,display.width(),15,WHITE);
  
  display.setTextColor(WHITE);

  // Display frequency setting
  if (mode == 0) {  
    
    unsigned long f = 0;
    f = vfoFreq;
    char buf[8];

    // Number
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(getMH(f));   
    display.setCursor(startX + 50,y);
    sprintf(buf,"%03lu",getKH(f));
    display.print(buf);

    y += 20;
    display.setCursor(0,y);  
    display.setTextSize(0);
    display.print(vfoSteps[vfoStepIndex]);
  }
  else if (mode == 1) {
    // Display volume setting
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(volume); 
  }
  else if (mode == 2) {
    // Display squelch setting
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(squelch); 
  }
}

void setup() {
  
  Serial.begin(9600);
  delay(500);
  
  pinMode(PIN_D2,INPUT_PULLUP);
  pinMode(PIN_D3,INPUT_PULLUP);
  pinMode(PIN_D4,INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC,SSD1306_I2C_ADDRESS);

  display.clearDisplay();

  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.println("KC1FSZ VHF");
  display.drawLine(0,15,display.width(),15,WHITE);

  // Initial display render
  updateDisplay();
  // Initial radio configuration
  Serial.print("AT+DMOCONNECT");
  Serial.write(13);
  Serial.write(10);
  updateRadioVolume();
  updateRadioGroup();

  display.display();
}

void loop() {
  
  int c2 = digitalRead(PIN_D2);
  int c3 = digitalRead(PIN_D3);
  db2.loadSample(c2 == 0);
  db3.loadSample(c3 == 0);
  db4.loadSample(digitalRead(PIN_D4) == 0);
  
  long mult = renc.getIncrement();
  long clickDuration = cd4.getClickDuration();
  
  boolean displayDirty = false;

  // Long click changes the mode
  if (clickDuration > 500) {    
    mode++;
    if (mode > 2) {
      mode = 0;
    }
    displayDirty = true;
  } 
  // Short click change the step size
  else if (clickDuration > 0) {
    if (mode == 0) {
      vfoStepIndex++;
      if (vfoStepIndex > 3) {
        vfoStepIndex = 0;    
      }
      displayDirty = true;
    }
  } 
  else if (mult != 0) {
    // Frequency
    if (mode == 0) {
      long step = mult * vfoSteps[vfoStepIndex];
      vfoFreq += step;
      updateRadioGroup();
    }
    // Volume 
    else if (mode == 1) {
      if (mult > 0) {
        if (volume < 8) {
          volume++;
        }
      } else if (mult < 0) {
        if (volume > 1) {
          volume--;
        }
      }
      updateRadioVolume();
    }
    // Squelch
    else if (mode == 2) {
      if (mult > 0) {
        if (squelch < 8) {
          squelch++;
        }
      } else if (mult < 0) {
        if (squelch > 0) {
          squelch--;
        }
      }
      updateRadioGroup();
    }
    
    displayDirty = true;
  }
   
  if (displayDirty) {
    updateDisplay();
    display.display();
  } 
}

