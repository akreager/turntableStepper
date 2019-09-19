//  Stepper Motor Turntable
//  Allen Kreager
//  v0.01
//  2019-09-18
//  
//  Many thanks to the Arduino community,  especially the folks 
//  who maintain the libraries used here and the countless examples
//  provided on the Web.
//  
//  This is part of my DIY 3D scanner build. The turntable can
//  be set to either run one revolution a setable speed or to
//  stop a set number of times per one revolution.
//
//  Mode 1 is used to slowly rotate an object for a Microsoft
//  Kinect to scan the object's surface.
//  Mode 2 is to get pictures of an object from multiple
//  angles automatically.
//
//  A 5-way joystick breakout and small OLED screen are used to
//  set the options for each mode. A standard step-stick style
//  stepper driver drives the stepper motor. Joystick buttons
//  are debounced using timers and pin polling.


// Libraries used
#include <AccelStepper.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <Wire.h>

// Stepper driver definition and variables
#define DIR_PIN 4
#define STEP_PIN 5
#define EN_PIN A0

// OLED definition
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Define Joystick connection pins
#define UP     12
#define DOWN   11
#define LEFT   10
#define RIGHT   9
#define CLICK   8
#define SET     7
#define RESET   6

// Menu 3 variables
//const byte minCounter3 = 68;
//const byte maxCounter3 = 70;
//const byte counter3increment = 1;

// Global Variables
bool stepperDir = HIGH; //default direction is cw
byte runLevel = 0;  //current run mode
int menuLevel = 0;  //current menu level
const byte maxMenuLevel = 2;  //highest menu level
const int stepsPerRev = 3200; //steps for one full revolution of stepper
int stepsPerStop; //calculated in the code
const int maxStepperSpd = 1000; //global maximum speed
const byte minStops = 1;  //minimum mode 2 stops
byte stopsPerRevolution = minStops;
const byte debounceDelay = 20;

// Current button states
bool btnStateUP;
bool btnStateDOWN;
bool btnStateLEFT;
bool btnStateRIGHT;
bool btnStateCLICK;
bool btnStateSET;
bool btnStateRESET;

// Last button states
bool lastUPstate;
bool lastDOWNstate;
bool lastLEFTstate;
bool lastRIGHTstate;
bool lastCLICKstate;
bool lastSETstate;
bool lastRESETstate;

// Create stepper
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Create an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  // put your setup code here, to run once:
  // Initilaize stepper driver pins
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  // Set stepper parameters
  stepper.setPinsInverted(true, false, true);
  stepper.setMaxSpeed(maxStepperSpd);
  stepper.setAcceleration(500);

  // Initialize joystick pins with pullups
  pinMode(UP, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  pinMode(CLICK, INPUT_PULLUP);
  pinMode(SET, INPUT_PULLUP);
  pinMode(RESET, INPUT_PULLUP);

  // Initialize OLED and show splash screen
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  splashScreen0();
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long lastUPdebounce;
  unsigned long lastDOWNdebounce;

  // Read the state of the UP/DOWN buttons
  bool readingUP = digitalRead(UP);
  bool readingDOWN = digitalRead(DOWN);

  // Check for state change
  if (readingUP != lastUPstate) {
    lastUPdebounce = millis();
  }
  if (readingDOWN != lastDOWNstate) {
    lastDOWNdebounce = millis();
  }

  // Check if UP button state has stayed changed long enough
  if ((millis() - lastUPdebounce) > debounceDelay) {
    if (readingUP != btnStateUP) {
      btnStateUP = readingUP;
      if (btnStateUP == LOW) {
        menuLevel--;
        if (menuLevel < 0) {
          // loop the menu to the top
          menuLevel = maxMenuLevel;
        }
      }
    }
  }

  // Check if DOWN button state change has stayed changed long enough
  if ((millis() - lastDOWNdebounce) > debounceDelay) {
    if (readingDOWN != btnStateDOWN) {
      btnStateDOWN = readingDOWN;
      if (btnStateDOWN == LOW) {
        menuLevel++;
        if (menuLevel > maxMenuLevel) {
          // loop the menu to the end
          menuLevel = 0;
        }
      }
    }
  }

  switch (runLevel) {
    case 0:
      switch (menuLevel) {
        case 0:
          updateMenu0();
          break;
        case 1:
          updateMenu1();
          break;
        case 2:
          updateMenu2();
          break;
          //case 3:
          //updateMenu3();
          //break;
      }
      break;
    case 1:
      runOnce();
      break;
    case 2:
      stopsPerRev();
      break;
      //case 3:
      //stepperConstantSpeed();
      //break;
  }

  lastUPstate = readingUP;
  lastDOWNstate = readingDOWN;
}

void splashScreen0(void) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(F("Stepper"));
  display.println(F("Motor"));
  display.println(F("Turntable"));
  display.setTextSize(1);
  display.println(F("Allen"));
  display.print(F("Kreager"));
  display.setCursor(60, 48);
  display.print(F("v0.01"));
  display.setCursor(60, 56);
  display.print(F("2019-09-18"));
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
}

void stepperConstantSpeed(void) {
  // this code isn't actually used at this point
  unsigned long lastCLICKdebounce = 0;

  bool readingCLICK = digitalRead(CLICK);

  if (readingCLICK != lastCLICKstate) {
    lastCLICKdebounce = millis();
  }

  if ((millis() - lastCLICKdebounce) > debounceDelay) {
    if (readingCLICK != btnStateCLICK) {
      btnStateCLICK = readingCLICK;
      if (btnStateCLICK == LOW) {
        runLevel = 0;
        digitalWrite(EN_PIN, HIGH);
        display.invertDisplay(false);
      }
    }
  }

  stepper.runSpeed();

  lastCLICKstate = readingCLICK;
}

void runOnce(void) {
  stepper.run();

  if (stepper.distanceToGo() == 0) {
    runLevel = 0;
    digitalWrite(EN_PIN, HIGH);
    display.invertDisplay(false);
    stepper.setMaxSpeed(maxStepperSpd);
    stepper.setCurrentPosition(0);
  }
}

void stopsPerRev(void) {
  static byte stopsTaken = 1;
  stepper.run();

  if (stepper.distanceToGo() == 0) {
    stopsTaken++;
    if (stepperDir == LOW) {
      stepper.moveTo(-stepsPerStop * stopsTaken);
    }
    else {
      stepper.moveTo(stepsPerStop * stopsTaken);
    }
    takeApicture();
  }

  if (stopsTaken > stopsPerRevolution) {
    runLevel = 0;
    digitalWrite(EN_PIN, HIGH);
    display.invertDisplay(false);
    stepper.setCurrentPosition(0);
    stopsTaken = 1;
  }
}

void takeApicture(void) {
  // this is mostly a placeholder at this point
  // planning on adding an ir led to trigger a camera
  delay(3000);
}

void updateMenu0(void) {
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 17, WHITE);
  display.setTextColor(INVERSE);
  display.setTextSize(2);
  display.setCursor(1, 1);
  display.print(F("Mode:"));
  display.setTextColor(WHITE);
  display.setCursor(0, 19);
  display.setTextSize(1);
  display.println(F("1: Constant Speed"));
  display.println(F("2: Number of Stops"));
  display.println(F("   per Revolution"));
  display.display();
}

void updateMenu1(void) {
  // Static variables for this function only
  static int stepperSpd = 0;
  static byte speedIncrement = 25;
  unsigned long lastLEFTdebounce;
  unsigned long lastRIGHTdebounce;
  unsigned long lastCLICKdebounce;
  unsigned long lastSETdebounce;
  unsigned long lastRESETdebounce;

  //Read the state of the buttons
  bool readingLEFT = digitalRead(LEFT);
  bool readingRIGHT = digitalRead(RIGHT);
  bool readingCLICK = digitalRead(CLICK);
  bool readingSET = digitalRead(SET);
  bool readingRESET = digitalRead(RESET);

  // Check for state change
  if (readingLEFT != lastLEFTstate) {
    lastLEFTdebounce = millis();
  }
  if (readingRIGHT != lastRIGHTstate) {
    lastRIGHTdebounce = millis();
  }
  if (readingCLICK != lastCLICKstate) {
    lastCLICKdebounce = millis();
  }
  if (readingSET != lastSETstate) {
    lastSETdebounce = millis();
  }
  if (readingRESET != lastRESETstate) {
    lastRESETdebounce = millis();
  }

  // Check if LEFT button state change has stayed changed long enough
  if ((millis() - lastLEFTdebounce) > debounceDelay) {
    if (readingLEFT != btnStateLEFT) {
      btnStateLEFT = readingLEFT;
      if (btnStateLEFT == LOW && stepperSpd > 0) {
        stepperSpd -= speedIncrement;
      }
    }
  }

  // Check if RIGHT button state change has stayed changed long enough
  if ((millis() - lastRIGHTdebounce) > debounceDelay) {
    if (readingRIGHT != btnStateRIGHT) {
      btnStateRIGHT = readingRIGHT;
      if (btnStateRIGHT == LOW && stepperSpd < maxStepperSpd) {
        stepperSpd += speedIncrement;
      }
    }
  }

  // Check if CLICK button state change has stayed changed long enough
  if ((millis() - lastCLICKdebounce) > debounceDelay) {
    if (readingCLICK != btnStateCLICK) {
      btnStateCLICK = readingCLICK;
      if (btnStateCLICK == LOW) {
        if (stepperSpd != 0) {
          display.invertDisplay(true);
          digitalWrite(EN_PIN, LOW);
          stepper.setMaxSpeed(stepperSpd);
          if (stepperDir == LOW) {
            stepper.moveTo(-stepsPerRev);
          }
          else {
            stepper.moveTo(stepsPerRev);
          }
          runLevel = 1;
        }
        else {
          display.invertDisplay(true);
          delay(200);
          display.invertDisplay(false);
          delay(200);
          display.invertDisplay(true);
          delay(200);
          display.invertDisplay(false);
          delay(200);
          display.invertDisplay(true);
          delay(200);
          display.invertDisplay(false);
          delay(200);
        }
      }
    }
  }

  // Check if SET button state change has stayed changed long enough
  if ((millis() - lastSETdebounce) > debounceDelay) {
    if (readingSET != btnStateSET) {
      btnStateSET = readingSET;
      if (btnStateSET == LOW) {
        stepperDir = !stepperDir;
      }
    }
  }

  // Check if RESET button state change has stayed changed long enough
  if ((millis() - lastRESETdebounce) > debounceDelay) {
    if (readingRESET != btnStateRESET) {
      btnStateRESET = readingRESET;
      if (btnStateRESET == LOW) {
        stepperSpd = 0;
      }
    }
  }

  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 33, WHITE);
  display.setTextColor(INVERSE);
  display.setTextSize(3);
  display.setCursor(3, 5);
  display.println(F("1"));
  display.setTextSize(2);
  display.setCursor(22, 1);
  display.print(F("Constant"));
  display.setCursor(22, 16);
  display.print(F("Speed"));
  display.setTextColor(WHITE);
  display.setCursor(0, 36);
  if (stepperDir == LOW) {
    display.print(F("CCW "));
  }
  else if (stepperDir == HIGH) {
    display.print(F("CW "));
  }
  display.print(stepperSpd);
  display.display();

  // Set last readings to current
  lastLEFTstate = readingLEFT;
  lastRIGHTstate = readingRIGHT;
  lastCLICKstate = readingCLICK;
  lastSETstate = readingSET;
  lastRESETstate = readingRESET;
}

void updateMenu2(void) {
  const byte maxStops = 255;
  const byte stopsIncrement = 1;
  unsigned long lastLEFTdebounce;
  unsigned long lastRIGHTdebounce;
  unsigned long lastCLICKdebounce;
  unsigned long lastSETdebounce;
  unsigned long lastRESETdebounce;

  //Read the state of the buttons
  bool readingLEFT = digitalRead(LEFT);
  bool readingRIGHT = digitalRead(RIGHT);
  bool readingCLICK = digitalRead(CLICK);
  bool readingSET = digitalRead(SET);
  bool readingRESET = digitalRead(RESET);

  // Check for state change
  if (readingLEFT != lastLEFTstate) {
    lastLEFTdebounce = millis();
  }
  if (readingRIGHT != lastRIGHTstate) {
    lastRIGHTdebounce = millis();
  }
  if (readingCLICK != lastCLICKstate) {
    lastCLICKdebounce = millis();
  }
  if (readingSET != lastSETstate) {
    lastSETdebounce = millis();
  }
  if (readingRESET != lastRESETstate) {
    lastRESETdebounce = millis();
  }

  // Check if LEFT button state change has stayed changed long enough
  if ((millis() - lastLEFTdebounce) > debounceDelay) {
    if (readingLEFT != btnStateLEFT) {
      btnStateLEFT = readingLEFT;
      if (btnStateLEFT == LOW && stopsPerRevolution > minStops) {
        stopsPerRevolution -= stopsIncrement;
      }
    }
  }

  // Check if RIGHT button state change has stayed changed long enough
  if ((millis() - lastRIGHTdebounce) > debounceDelay) {
    if (readingRIGHT != btnStateRIGHT) {
      btnStateRIGHT = readingRIGHT;
      if (btnStateRIGHT == LOW && stopsPerRevolution < maxStops) {
        stopsPerRevolution += stopsIncrement;
      }
    }
  }

  // Check if CLICK button state change has stayed changed long enough
  if ((millis() - lastCLICKdebounce) > debounceDelay) {
    if (readingCLICK != btnStateCLICK) {
      btnStateCLICK = readingCLICK;
      if (btnStateCLICK == LOW) {
        display.invertDisplay(true);
        digitalWrite(EN_PIN, LOW);
        stepsPerStop = stepsPerRev / stopsPerRevolution;
        if (stepperDir == LOW) {
          stepper.moveTo(-stepsPerStop);
        }
        else {
          stepper.moveTo(stepsPerStop);
        }
        runLevel = 2;
      }
    }
  }

  // Check if SET button state change has stayed changed long enough
  if ((millis() - lastSETdebounce) > debounceDelay) {
    if (readingSET != btnStateSET) {
      btnStateSET = readingSET;
      if (btnStateSET == LOW) {
        stepperDir = !stepperDir;
      }
    }
  }

  // Check if RESET button state change has stayed changed long enough
  if ((millis() - lastRESETdebounce) > debounceDelay) {
    if (readingRESET != btnStateRESET) {
      btnStateRESET = readingRESET;
      if (btnStateRESET == LOW) {
        stopsPerRevolution = minStops;
      }
    }
  }

  display.clearDisplay();
  display.clearDisplay();
  display.fillRect(0, 0, SCREEN_WIDTH, 33, WHITE);
  display.setTextColor(INVERSE);
  display.setTextSize(3);
  display.setCursor(3, 5);
  display.println(F("2"));
  display.setTextSize(2);
  display.setCursor(22, 1);
  display.print(F("Stops"));
  display.setCursor(22, 16);
  display.print(F("per Rev."));
  display.setTextColor(WHITE);
  display.setCursor(0, 36);
  if (stepperDir == LOW) {
    display.print(F("CCW "));
  }
  else if (stepperDir == HIGH) {
    display.print(F("CW "));
  }
  display.print(stopsPerRevolution);
  display.display();

  // Set last readings to current
  lastLEFTstate = readingLEFT;
  lastRIGHTstate = readingRIGHT;
  lastCLICKstate = readingCLICK;
  lastSETstate = readingSET;
  lastRESETstate = readingRESET;
}

//void updateMenu3(void) {
//  static byte menuCounter3 = minCounter3;
//
//  //Read the state of the buttons
//  readingLEFT = digitalRead(LEFT);
//  readingRIGHT = digitalRead(RIGHT);
//  readingCLICK = digitalRead(CLICK);
//  readingSET = digitalRead(SET);
//  readingRESET = digitalRead(RESET);
//
//  // Check for state change
//  if (readingLEFT != lastLEFTstate) {
//    lastLEFTdebounce = millis();
//  }
//  if (readingRIGHT != lastRIGHTstate) {
//    lastRIGHTdebounce = millis();
//  }
//  if (readingCLICK != lastCLICKstate) {
//    lastCLICKdebounce = millis();
//  }
//  if (readingSET != lastSETstate) {
//    lastSETdebounce = millis();
//  }
//  if (readingRESET != lastRESETstate) {
//    lastRESETdebounce = millis();
//  }
//
//  // Check if LEFT button state change has stayed changed long enough
//  if ((millis() - lastLEFTdebounce) > debounceDelay) {
//    if (readingLEFT != btnStateLEFT) {
//      btnStateLEFT = readingLEFT;
//      if (btnStateLEFT == LOW && menuCounter3 > minCounter3) {
//        menuCounter3 -= counter3increment;
//      }
//    }
//  }
//
//  // Check if RIGHT button state change has stayed changed long enough
//  if ((millis() - lastRIGHTdebounce) > debounceDelay) {
//    if (readingRIGHT != btnStateRIGHT) {
//      btnStateRIGHT = readingRIGHT;
//      if (btnStateRIGHT == LOW && menuCounter3 < maxCounter3) {
//        menuCounter3 += counter3increment;
//      }
//    }
//  }
//
//  // Check if CLICK button state change has stayed changed long enough
//  if ((millis() - lastCLICKdebounce) > debounceDelay) {
//    if (readingCLICK != btnStateCLICK) {
//      btnStateCLICK = readingCLICK;
//      if (btnStateCLICK == LOW) {
//        display.clearDisplay();
//        display.setTextSize(3);
//        display.setTextColor(WHITE);
//        display.setCursor(15, 20);
//        if (menuCounter3 == 69) {
//          display.print(F("Nice."));
//        }
//        else {
//          display.print(F("CLICK!"));
//        }
//        display.display();
//        delay(500);
//        display.clearDisplay();
//        display.display();
//        //runLevel = 3;
//      }
//    }
//  }
//
//  //check if SET button state change has stayed changed long enough
//  if ((millis() - lastSETdebounce) > debounceDelay) {
//    if (readingSET != btnStateSET) {
//      btnStateSET = readingSET;
//      if (btnStateSET == LOW) {
//        stepperDir = !stepperDir;
//      }
//    }
//  }
//
//  //check if RESET button state change has stayed changed long enough
//  if ((millis() - lastRESETdebounce) > debounceDelay) {
//    if (readingRESET != btnStateRESET) {
//      btnStateRESET = readingRESET;
//      if (btnStateRESET == LOW) {
//        menuCounter3 = minCounter3;
//      }
//    }
//  }
//
//  display.clearDisplay();
//  display.fillRect(0, 0, SCREEN_WIDTH, 33, WHITE);
//  display.setTextColor(INVERSE);
//  display.setTextSize(3);
//  display.setCursor(3, 5);
//  display.println(F("3"));
//  display.setTextSize(2);
//  display.setCursor(22, 1);
//  display.print(F("Placehol"));
//  display.setCursor(22, 16);
//  display.print(F("der Text"));
//  display.setTextColor(WHITE);
//  display.setCursor(0, 36);
//  display.print(menuCounter3);
//  display.display();
//
//  // Set last readings to current
//  lastLEFTstate = readingLEFT;
//  lastRIGHTstate = readingRIGHT;
//  lastCLICKstate = readingCLICK;
//  lastSETstate = readingSET;
//  lastRESETstate = readingRESET;
//}
