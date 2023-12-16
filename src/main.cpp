/* ********************************************************
 *  TeaBot Firmware 2.0
 *  by hpop 
 *  
 * *********************************************************
 *  This code is based on the TeaBot V1.0 by SnakeP 
 *  https://www.thingiverse.com/thing:5250153)
 *  
 *  which is based on the code from Carlo Stramaglia 
 *  https://www.youtube.com/c/CarloStramaglia
 * 
 *  ********************************************************
 * 
 *  License: CC BY 4.0 Deed 
 *  https://creativecommons.org/licenses/by/4.0/
 * 
 *  ********************************************************
 *  Single click - set time in minutes
 *  Idle for 3 seconds - start tea making
 *  Long click - reset / stop tea making
 */


#include <Arduino.h>
#include <Servo.h>
#include <TM1637Display.h>
#include "OneButton.h"

// ======= pins =========
#define PIN_BUTTON_INPUT A2
#define PIN_DISPLAY_CLK 2
#define PIN_DISPLAY_DIO 3
#define PIN_SERVO 9

// ====== options =======

//Delay between servo steps 
//(the higher the value, the slower the servo moves)
unsigned int DELAY = 10; 

//Servo position value definition
int POS_UP = 110;        //Up-position
int POS_MIDDLE = 85;     //Middle-position
int POS_DOWN = 35;       //Down-position

//Dip function every 2 minutes
bool DIP = true; 


// ====== variables =======
Servo myservo;

unsigned int teaMinutes = 0;
unsigned long lastButtonPressedTime = 0;
unsigned int i = 0;

bool timerRunning = false;
unsigned long timerStart = 0;

unsigned int currentMinute = 0;
unsigned int currentSecond = 0;

unsigned int lastDipMinute = 0;

TM1637Display display(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);
OneButton button(PIN_BUTTON_INPUT, true);


// =======================================
// ====== display helper functions =======
// =======================================
void showNumber(unsigned int number) {
  if (number < 10) {
    display.showNumberDec(number, true, 2, 2);
  } else {
    display.showNumberDec(number);
  }
}

void displayTime(unsigned int minutes, unsigned int seconds) {

  char buffer[50];
  sprintf(buffer, "%d:%d", minutes, seconds);
  Serial.println(buffer);

  if (minutes < 10 && minutes > 0) {
    display.showNumberDecEx(seconds + minutes * 100, 0x40);
  } else if (minutes == 0 && seconds >= 10) {
    display.showNumberDecEx(seconds + minutes * 100, 0x80, true, 3, 1);
  } else if (minutes == 0 && seconds < 10) {
    display.showNumberDecEx(seconds + minutes * 100, 0x80, true, 3, 1);
  } else {
    display.showNumberDecEx(seconds + minutes * 100, 0x40); 
  }               
}

void showEmptyState() {
  uint8_t emptyState[4] = { 0x40, 0xc0, 0x40, 0x40 };
  display.setSegments(emptyState);
}


// =======================================
// ====== servo helper functions =========
// =======================================
void moveTo(int pos, int delaytime) {
  int currentpos = myservo.read();
  if (currentpos == pos) {
    return;
  }
  
  if (currentpos < pos) {
    for (; currentpos < pos; currentpos++) {
      myservo.write(currentpos);
      delay(delaytime);
    }
  }
  else {
    for (; currentpos > pos; currentpos--) {
      myservo.write(currentpos);
      delay (delaytime);
    }
  }
}

void downAndDip() {
  moveTo(POS_DOWN, DELAY);
  delay(300);
  moveTo(POS_MIDDLE, DELAY);
  moveTo(POS_DOWN, DELAY * 2.5);
}

void down() {
  moveTo(POS_DOWN, DELAY);
}

void dip() {
  Serial.println("Dip");
  lastDipMinute = currentMinute;
  moveTo(POS_MIDDLE, DELAY);
  moveTo(POS_DOWN, DELAY);
  delay(300);
  moveTo(POS_MIDDLE, DELAY);
  moveTo(POS_DOWN, DELAY);
}

void up() {
  moveTo(POS_UP + 10, DELAY);
  delay(500);
  moveTo(POS_UP, DELAY);
}


// =======================================
// ========== main functions =============
// =======================================
void incrementMinute() {
  teaMinutes++;
  
  char buffer[50];
  sprintf(buffer, "Set timer to %d", teaMinutes);
  Serial.println(buffer);

  displayTime(teaMinutes, 0);
  lastButtonPressedTime = millis();

} // incrementMinute

void updateTimeLeft() {
  int timeLeft = teaMinutes * 60 - (millis() - timerStart) / 1000;
  unsigned int minutes = timeLeft / 60;
  unsigned int seconds = timeLeft % 60;

  if (minutes == currentMinute && seconds == currentSecond) {
    return;
  }
  
  displayTime(minutes, seconds);
  currentMinute = minutes;
  currentSecond = seconds;
}
  

void startTeaMaking() {
  Serial.println("Start Tea Making");
  if (teaMinutes == 0) {
    return;
  }
  
  lastButtonPressedTime = 0;
  down(); 
  timerStart = millis();
  timerRunning = true;
}

void reset() {
  Serial.println("Reset");
  showEmptyState();

  i = 0;
  teaMinutes = 0;
  timerRunning = false;
  timerStart = 0;
  lastButtonPressedTime = 0;
  lastDipMinute = 0;
  currentMinute = 0;
  currentSecond = 0;
  up();
}

void stopTeaMaking() {
  Serial.println("Stop Tea Making");
  reset();
}

boolean timerHasFinished() {
  return millis() - timerStart > teaMinutes * 60000;
}

boolean shouldDip() {
  return DIP 
    && currentMinute > 0 
    && currentSecond == 0
    && currentMinute % 2 == 0
    && currentMinute != lastDipMinute 
    && currentMinute != teaMinutes;
}

void showAnimation(const uint8_t (*frames)[4], int numFrames, unsigned int delaytime) {
  for (int i = 0; i < numFrames; i++) {
    display.setSegments(frames[i]);
    delay(delaytime);
  }
}

// Animation on startup
void fancyStartupDisplayAnimation() {
  display.setBrightness(1);
  uint8_t frames[][4] = {
    { 0x20, 0x00, 0x00, 0x00 },  // Frame 0
    { 0x30, 0x00, 0x00, 0x00 },  // Frame 1
    { 0x18, 0x00, 0x00, 0x00 },  // Frame 2
    { 0x0c, 0x00, 0x00, 0x00 },  // Frame 3
    { 0x06, 0x00, 0x00, 0x00 },  // Frame 4
    { 0x02, 0x01, 0x00, 0x00 },  // Frame 5
    { 0x00, 0x03, 0x00, 0x00 },  // Frame 6
    { 0x00, 0x06, 0x00, 0x00 },  // Frame 7
    { 0x00, 0x04, 0x08, 0x00 },  // Frame 8
    { 0x00, 0x00, 0x0c, 0x00 },  // Frame 9
    { 0x00, 0x00, 0x06, 0x00 },  // Frame 10
    { 0x00, 0x00, 0x02, 0x01 },  // Frame 11
    { 0x00, 0x00, 0x00, 0x03 },  // Frame 12
    { 0x00, 0x00, 0x00, 0x06 },  // Frame 13
    { 0x00, 0x00, 0x00, 0x04 },  // Frame 14
    { 0x00, 0x00, 0x00, 0x00 },  // Frame 15
  };
  showAnimation(frames, 16, 75);
}


void setup() {
  Serial.begin(57600);  
  myservo.attach(PIN_SERVO);
  fancyStartupDisplayAnimation();
  
  up();
  
  button.attachClick(incrementMinute);
  // button.attachDoubleClick(startTeaMaking);
  button.attachLongPressStart(reset);

  showEmptyState();
}

void loop() {
  button.tick();

  // start the tea making after 3 second idle
  if (lastButtonPressedTime != 0 && millis() - lastButtonPressedTime > 3000) {
    
    char buffer[50];
    sprintf(buffer, "Start tea making after %lu seconds idle", (millis() - lastButtonPressedTime) / 1000);
    Serial.println(buffer);

    startTeaMaking();
  }

  if (timerRunning) {
    updateTimeLeft();

    if (shouldDip()) {
      dip();
    }

    if (timerHasFinished()) {
      stopTeaMaking();
    }
  }

  delay(10);
}