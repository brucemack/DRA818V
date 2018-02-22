// Controller for a 2m radio based on the DRA818v
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

#define VFO_STEP_COUNT 5
const unsigned long vfoSteps[] = { 1000, 10000, 100000, 1000000, 10000000 };
unsigned long vfoStepIndex = 1;

#define MODE_COUNT 4
const char* modeLabels[] = { "VFO", "VOL", "SQL", "CTCSS" };
int mode = 0;

#define CTCSS_COUNT 38
int ctcssIndex = 0;
const String receiveCTCSS = "0000";
// CTCSS tones in HZ * 10
const int ctcssTones[] = {
  670,
  719,
  744,
  770,
  797,
  825,
  854,
  885,
  915,
  948,
  974,
  1000,
  1035,
  1072,
  1109,
  1148,
  1188,
  1230,
  1273,
  1318,
  1365,
  1413,
  1462,
  1514,
  1567,
  1622,
  1679,
  1738,
  1799,
  1862,
  1928,
  2035,
  2107,
  2181,
  2257,
  2336,
  2418,
  2503  
};

unsigned long vfoFreq = 162475000;
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
  // Here is what the command looks like:
  //AT+DMOSETGROUP=0,152.1250,152.1250,0012,4,0003<CR><LF>
  Serial1.print("AT+DMOSETGROUP=0,");
  Serial1.print(getMH(vfoFreq));
  Serial1.print(".");
  char buf[5];
  sprintf(buf,"%03lu",getKH(vfoFreq));
  Serial1.print(buf);
  Serial1.print("0");
  Serial1.print(",");
  Serial1.print(getMH(vfoFreq));
  Serial1.print(".");
  sprintf(buf,"%03lu",getKH(vfoFreq));
  Serial1.print(buf);
  Serial1.print("0");
  Serial1.print(",");
  sprintf(buf,"%04d",ctcssIndex + 1);
  Serial1.print(buf);
  Serial1.print(",");
  Serial1.print(squelch);
  Serial1.print(",");
  Serial1.print(receiveCTCSS);
  Serial1.write(13);
  Serial1.write(10);
}

void updateRadioVolume() {
  Serial1.print("AT+DMOSETVOLUME=");
  Serial1.print(volume);
  Serial1.write(13);
  Serial1.write(10);
}

void updateDisplay() {
  
  int rowHeight = 16;
  int startX = 10;
  int y = 17;

  // Top area
  display.setCursor(0,0);
  display.setTextSize(0);
  display.setTextColor(WHITE);
  display.print("KC1FSZ VHF");
  display.print("  ");
  display.print(modeLabels[mode]);
  display.drawLine(0,15,display.width(),15,WHITE);

  display.setTextColor(WHITE);

  // Display frequency setting
  if (mode == 0) {  
    // Frequency
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(getMH(vfoFreq));   
    display.setCursor(startX + 50,y);
    char buf[8];
    sprintf(buf,"%03lu",getKH(vfoFreq));
    display.print(buf);
    // Step
    y += 20;
    display.setCursor(0,y);  
    display.setTextSize(0);
    display.print(vfoSteps[vfoStepIndex]);
  }
  // Display volume setting
  else if (mode == 1) {
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(volume); 
  }
  // Display squelch setting
  else if (mode == 2) {
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(squelch);
  }
  // CTCSS 
  else if (mode == 3) {
    display.setCursor(startX,y);
    display.setTextSize(2);
    display.print(ctcssTones[ctcssIndex]); 
  }
}

void setup() {
  
  Serial1.begin(9600);
  delay(500);
  
  pinMode(PIN_D2,INPUT_PULLUP);
  pinMode(PIN_D3,INPUT_PULLUP);
  pinMode(PIN_D4,INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC,SSD1306_I2C_ADDRESS);
  
  // Initial display render
  updateDisplay();
  // Initial radio configuration
  Serial1.print("AT+DMOCONNECT");
  Serial1.write(13);
  Serial1.write(10);
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
    if (mode == MODE_COUNT) {
      mode = 0;
    }
    displayDirty = true;
  } 
  // Short click change the step size
  else if (clickDuration > 0) {
    if (mode == 0) {
      vfoStepIndex++;
      if (vfoStepIndex == VFO_STEP_COUNT) {
        vfoStepIndex = 0;    
      }
      displayDirty = true;
    }
  } 
  else if (mult != 0) {
    // Frequency
    if (mode == 0) {
      vfoFreq += (mult * vfoSteps[vfoStepIndex]);
      updateRadioGroup();
    }
    // Volume 
    else if (mode == 1) {
      if (mult > 0) {
        if (volume < 8) {
          volume++;
          updateRadioVolume();
        }
      } else if (mult < 0) {
        if (volume > 1) {
          volume--;
          updateRadioVolume();
        }
      }
    }
    // Squelch
    else if (mode == 2) {
      if (mult > 0) {
        if (squelch < 8) {
          squelch++;
          updateRadioGroup();
        }
      } else if (mult < 0) {
        if (squelch > 0) {
          squelch--;
          updateRadioGroup();
        }
      }
    }
    // CTCSS
    else if (mode == 3) {
      if (mult > 0) {
        if (ctcssIndex < CTCSS_COUNT - 1) {
          ctcssIndex++;
          updateRadioGroup();
        }
      } else if (mult < 0) {
        if (ctcssIndex > 0) {
          ctcssIndex--;
          updateRadioGroup();
        }
      }
    }
    
    displayDirty = true;
  }
   
  if (displayDirty) {
    display.clearDisplay();
    updateDisplay();
    display.display();
  } 
}

