/*
-----TAPTASTIC-----
Reaction Game with OLED Display and NeoPixel Arcade Button
Games:
2. Colors - press the button when the given color is shown
3. Countdown - press the button when the timer hits 0
4. Count Up - press the button when the timer hits the specified number
5. Pinpoint - press the button the specified number of times as fast as you can
6. Classic - press the button when it lights up
7. Quick fire - press the button as many times as you can in the alloted time
*/


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <ESP_EEPROM.h>
#include <WiFiManager.h>
WiFiManager wm;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C

#define LEDPIN 2 //pin for the NeoPixel
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel btnLED(1, LEDPIN, NEO_GRB + NEO_KHZ800);
int hue = 0;

const int COLOR_COUNT = 7;
enum colors { 
  RED = 0, 
  GREEN, 
  BLUE,
  YELLOW,
  PURPLE,
  TEAL,
  ORANGE
};

const char* colorNames [COLOR_COUNT] = {
  "RED", 
  "GREEN", 
  "BLUE",
  "YELLOW",
  "PURPLE",
  "TEAL",
  "ORANGE"
};

uint32_t getColor(int selection)
{
  switch(selection)
  {
    case RED:     { return btnLED.Color(255, 0, 0);    }
    case GREEN:   { return btnLED.Color(0, 255, 0);    }
    case BLUE:    { return btnLED.Color(0, 0, 255);    }
    case YELLOW:  { return btnLED.Color(255, 255, 0);  }
    case PURPLE:  { return btnLED.Color(255, 0, 255);  }
    case TEAL:    { return btnLED.Color(0, 255, 255);  }
    case ORANGE:  { return btnLED.Color(255, 69, 0);  }
    
    default:{ return btnLED.Color(255, 255, 255); }
  }
}

unsigned long mytime; //current time
unsigned long gameTime; //elapsed or total game time
unsigned long gameStart; //when the game started
unsigned long gameEnd; //when the game ended
unsigned long gameUpdate; //when the game was last updated (button or screen count)
unsigned long gameLength; //how long the game is
unsigned long gameScore;
int gameSpeed = 1000; //default game speed
int gameState = 0; //0 = reset, 1 = programmed, 2 = opened, 3 = started, 4 = lost, 5  = finished, 6 = high score
int gameMode = 0; //0 = boot screen, 1 = game title, 2 = game intro, 3 = game play, 4 = game over
int gameButton; 
int currentGame;
int lastGameButton;
int buttonResult;
int gameColor; 
String gameColorName;
int click = 50; 
int hold = 1000;

//create an array to store the high score names
char gameHSname [8][4] = {"AAA", "BBB", "CCC", "DDD", "EEE", "FFF", "GGG", "HHH"};

struct highScores {
  int game1HS;
  int game2HS;
  int game3HS;
  int game4HS;
  int game5HS;
  int game6HS;
  int game7HS;
  char game1HSname[4];
  char game2HSname[4];
  char game3HSname[4];
  char game4HSname[4];
  char game5HSname[4];
  char game6HSname[4];
  char game7HSname[4];
} highScore, highScoreName;

//set up the EEPROM addresses for the high scores
int gameHSaddress [8] = {0, 3, 6, 9, 12, 15, 18, 21};
int gameHSnameAddress [8] = {21, 25, 29, 33, 37, 41, 45, 49};

//create an array to store the high scores
int gameHS [8] = {1000, 10000, 10000, 10000, 10000, 10000, 10000, 10};

//button variables
int buttonState;
int lastButtonState = HIGH;
unsigned long pressedTime;
unsigned long releasedTime;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 15;
const int buttonPin = 14;
int buttonPressed;

int screen = 0; //tracks which screen is displayed
int randomColor;
int fade = 0;
int direction = 1;
unsigned long rainbowUpdate;
int selectedColor;

void setup() {
  //wm.resetSettings(); // reset WiFi settings
  Serial.begin(115200);
  Serial.println("Booting");

  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(60);
    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("itMightBeMagic.com","TAPTASTIC")){
        Serial.println("connected...yeey :)");
    }
    else {
        Serial.println("Configportal running");
    }
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);


  EEPROM.begin(sizeof(highScores));
  EEPROM.get(gameHSaddress[1], gameHS[1]);
  EEPROM.get(gameHSaddress[2], gameHS[2]);
  EEPROM.get(gameHSaddress[3], gameHS[3]);
  EEPROM.get(gameHSaddress[4], gameHS[4]);
  EEPROM.get(gameHSaddress[5], gameHS[5]);
  EEPROM.get(gameHSaddress[6], gameHS[6]);
  EEPROM.get(gameHSaddress[7], gameHS[7]);

  EEPROM.get(gameHSnameAddress[1], gameHSname[1]);
  EEPROM.get(gameHSnameAddress[2], gameHSname[2]);
  EEPROM.get(gameHSnameAddress[3], gameHSname[3]);
  EEPROM.get(gameHSnameAddress[4], gameHSname[4]);
  EEPROM.get(gameHSnameAddress[5], gameHSname[5]);
  EEPROM.get(gameHSnameAddress[6], gameHSname[6]);
  EEPROM.get(gameHSnameAddress[7], gameHSname[7]);

  btnLED.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  screen = 1;
  gameMode = 1; //boot screen
  btnLED.clear();
  btnLED.show();

}

void loop() {
  mytime = millis();
  wm.process();

  if ((fade < 250) && (direction == 1) && (fade > 0)) {
    fade = fade + 20;
  } else if ((fade < 250) && (direction == 0) && (fade > 0)) {
    fade = fade - 20;
  } else if (fade == 0) {
    direction = 1;
    fade = 10;
  } else if (fade == 250) {
    direction = 0;
    fade = 240;
  }

  buttonResult = checkButton(); // check the button state
  if (buttonResult == 1) { // button is clicked, go to next screen
    
    if (gameMode == 1) { // if button is pressed while on the game title screen
      if (screen < 7) {
        screen++; // go to the next game title screen
      } else {
        screen = 2; // loop to the 1st screen
      }
    } else if (gameMode == 2) { // if button is pressed while on the game intro screen
      // do nothing
    } else if (gameMode == 3) { // if button is pressed while on the game play screen
      //do nothing
    } else { // if button is pressed while on the game over screen
      //do nothing
    }

  } else if (buttonResult == 2) { // button is held, go to next screen
    btnLED.setBrightness(255); //set the brightness to max
    // button is held, select current screen
    if (gameMode == 1) { // if button is held while on the game title screen
      gameMode = 2; // set game mode to intro
      switch (screen) {
        case 2: game2intro(); break;
        case 3: game3intro(); break;
        case 4: game4intro(); break;
        case 5: game5intro(); break;
        case 6: game6intro(); break;
        case 7: game7intro(); break;
        default: screen = 1; break;
      }
    } else if (gameMode == 2) { // if button is pressed while on the game intro screen
      // do nothing
    } else if (gameMode == 3) { // if button is pressed while on the game play screen
      //do nothing
    } else { // if button is pressed while on the game over screen
      //do nothing
    }
  } else {
    //do nothing
  }

  if (gameMode == 1) {
    switch (screen) {
      case 1: titleScreen(); break;
      case 2: gameTitle2(); break;
      case 3: gameTitle3(); break;
      case 4: gameTitle4(); break;
      case 5: gameTitle5(); break;
      case 6: gameTitle6(); break;
      case 7: gameTitle7(); break;
      default:
        //do nothing
        break;
    }
  }
}

void rainbow() {
  btnLED.clear();
  hue = (hue + 500) % 65536;  // hue cycles from 0 to 65535
  uint32_t rgbcolor = btnLED.ColorHSV(hue);
  btnLED.fill(rgbcolor);
  btnLED.show();
  if (hue > 65535) {
  hue = 0;
  }
}

int checkButton() { //return 0 if button is not pressed, 1 if button is clicked, 2 if button is held longer than hold
  int reading = digitalRead(buttonPin);
  if (reading != buttonState) {
    buttonState = reading;
    if (buttonState == LOW && lastButtonState == HIGH) { //button is being held down
      pressedTime = millis();
      lastButtonState = buttonState;
      return 0;
    } else if (buttonState == HIGH && lastButtonState == LOW) { //button was released
      releasedTime = millis();
      if (releasedTime - pressedTime < hold) {
        return 1;
      } else {
        //do nothing
      }
    } else {
      return 0;
    }
  } else if (buttonState == LOW && lastButtonState == LOW) { //button is still being held down
    if (millis() - pressedTime > hold) {
      return 2;
    } else {
      return 0;
    }
  }
  
  lastButtonState = reading; // update lastButtonState
  return 0; // return 0 if button is not pressed
}

void titleScreen() { // main screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10,0); 
  display.println(F("TAPTASTIC"));
  display.setTextSize(3);
  display.println(F("Ready?"));
  display.setTextSize(1);
  display.println("Press button to");
  display.println("select game");
  display.display();
  rainbow();
}

void gameTitle2() { // colors game
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(" Colors ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(2),0,1);
  btnLED.show();
}

void gameTitle3() { // countdown game
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(" Countdown");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(3),0,1);
  btnLED.show();
}

void gameTitle4() { //count up game
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(" Count UP ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(4),0,1);  
  btnLED.show();
}

void gameTitle5() { //pinpoint game
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(" Pinpoint ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(5),0,1);
  btnLED.show();
}

void gameTitle6() { //classic
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(" Classic ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(1),0,1);
  btnLED.show();
}

void gameTitle7() { //quick fire
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println("Quick Fire");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.println("Hold to");
  display.println(" START");
  display.display();
  btnLED.clear();
  btnLED.setBrightness(fade);
  btnLED.fill(getColor(0),0,1);
  btnLED.show();
}

void gameOverScreen(int HS, int score, int gameNumber) { //game over screen
  display.clearDisplay();
  display.setTextSize(1.5);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.print(" HIGH SCORE: ");
  display.print(HS);
  display.print(" ");
  display.print(gameHSname[gameNumber][0]);
  display.print(gameHSname[gameNumber][1]);
  display.print(gameHSname[gameNumber][2]);
  display.println(" ");
  display.setCursor(4,17);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.println("YOUR SCORE");
  display.setCursor(40,32);
  display.println(score);
  display.setTextSize(1);
  display.println("  CLICK TO TRY AGAIN");
  display.println("    HOLD TO EXIT");
  display.display();
  btnLED.clear();
  btnLED.show();
  while (digitalRead(buttonPin) == LOW) //wait for button to be released
  {
    delay(200);
  }
  int buttonResult = checkButton();
  while (buttonResult == 0) { //wait for button press or hold
    delay(100);
    buttonResult = checkButton();
  }
  if (buttonResult == 1) { //button pressed
    gameState = 0; //reset the state
    gameMode = 2; //set the game mode to intro
    if (gameNumber == 2) {
      game2intro();
    } else if (gameNumber == 3) {
      game3intro();
    } else if (gameNumber == 4) {
      game4intro();
    } else if (gameNumber == 5) {
      game5intro(); 
    } else if (gameNumber == 6) {
      game6intro();
    } else if (gameNumber == 7) {
      game7intro();
    } else {
      titleScreen();
    }
  } else if (buttonResult == 2) { //button held
    screen = 2; //set the screen to the first game
    gameState = 0; //reset the state
    gameMode = 1; //set the game mode to title
    while (checkButton() != 0) { //fake the intro screen until the button is released
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(15, 0);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.println(" Colors ");
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(3);
      display.println("Hold to");
      display.println(" START");
      display.display();
      btnLED.clear();
      btnLED.setBrightness(fade);
      btnLED.fill(getColor(2),0,1);
      btnLED.show();
    }
    gameTitle2(); //display the first game title screen
  }
}

void sorry(int failNumber, int gameNumber) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("  SORRY! ");
  display.setTextColor(SSD1306_WHITE); // Draw 'inverse' text
  display.setTextSize(1);
  display.println("You pressed the");
  display.print("button ");
  display.print(failNumber);
  display.println(" times");
  display.setCursor(0, 40);
  display.println("Press button to try  again or hold to     change game");
  display.display();
  btnLED.clear();
  btnLED.fill(getColor(0),0,1);
  btnLED.show();
  while (digitalRead(buttonPin) == LOW) //wait for button to be released
  {
    delay(200);
  }
  int buttonResult = checkButton();
  while (buttonResult == 0) { //wait for button press or hold
    delay(100);
    buttonResult = checkButton();
  }
  if (buttonResult == 1) { //button pressed
    gameState = 0; //reset the state
    gameMode = 2; //set the game mode to intro
    if (gameNumber == 2) {
      game2intro();
    } else if (gameNumber == 3) {
      game3intro();
    } else if (gameNumber == 4) {
      game4intro();
    } else if (gameNumber == 5) {
      game5intro();
    } else if (gameNumber == 6) {
      game6intro();
    } else if (gameNumber == 7) {
      game7intro();
    } else {
      titleScreen();
    }
  } else if (buttonResult == 2) { //button held
    screen = 2; //set the screen to the first game
    gameState = 0; //reset the state
    gameMode = 1; //set the game mode to title
    while (checkButton() != 0) { //fake the intro screen until the button is released
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(15, 0);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.println(" Colors ");
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(3);
      display.println("Hold to");
      display.println(" START");
      display.display();
      btnLED.clear();
      btnLED.setBrightness(fade);
      btnLED.fill(getColor(2),0,1);
      btnLED.show();
    }
    gameTitle2(); //display the first game title screen
  }
}

void tooSoon(int failNumber, int gameNumber) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println(" TOO SOON!");
  display.setTextColor(SSD1306_WHITE); // Draw 'inverse' text
  display.setTextSize(1);
  display.print("You clicked ");
  display.print(failNumber);
  display.println("ms");
  display.println("too soon!");
  display.println("Press button to try  again or hold to");
  display.println("change game");
  display.display();
  btnLED.clear();
  btnLED.fill(getColor(0),0,1);
  btnLED.show();
  while (digitalRead(buttonPin) == LOW) //wait for button to be released
  {
    delay(200);
  }
  int buttonResult = checkButton();
  while (buttonResult == 0) { //wait for button press or hold
    delay(100);
    buttonResult = checkButton();
  }
  if (buttonResult == 1) { //button pressed
    gameState = 0; //reset the state
    gameMode = 2; //set the game mode to intro
    if (gameNumber == 2) {
      game2intro();
    } else if (gameNumber == 3) {
      game3intro();
    } else if (gameNumber == 4) {
      game4intro();
    } else if (gameNumber == 5) {
      game5intro();
    } else if (gameNumber == 6) {
      game6intro();
    } else if (gameNumber == 7) {
      game7intro();
    } else {
      titleScreen();
    }
  } else if (buttonResult == 2) { //button held
    screen = 2; //set the screen to the first game
    gameState = 0; //reset the state
    gameMode = 1; //set the game mode to title
    while (checkButton() != 0) { //fake the intro screen until the button is released
      delay(10);
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(15, 0);
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.println(" Colors ");
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(3);
      display.println("Hold to");
      display.println(" START");
      display.display();
      btnLED.clear();
      btnLED.setBrightness(fade);
      btnLED.fill(getColor(2),0,1);
      btnLED.show();
    }
    gameTitle2(); //display the first game title screen
  }
}

void newHS(int newHighScore, int gameNumber) { //new high score screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println(" NEW HIGH!");
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.setTextSize(4);
  display.print(" ");
  display.print(newHighScore);
  display.println(" ");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.println(" CLICK TO ENTER NAME");
  display.display();
  btnLED.clear();
  btnLED.fill(getColor(6),0,1);
  btnLED.show();
  while (digitalRead(buttonPin) == LOW) //wait for button to be released
  {
    delay(200);
  }
  int buttonResult = checkButton();
  while (buttonResult == 0) { //wait for button press or hold
    delay(100);
    buttonResult = checkButton();
  }
  if (buttonResult == 1) { //button pressed
    enterName(newHighScore, gameNumber);
  } else if (buttonResult == 2) { //button held
    //do nothing
  }
}

void enterName(int newHighScore, int gameNumber) {
  int blinkSpeed = 250;
  int cursor = 0;
  int blinkState = 0;
  while (gameState == 6) {
    int buttonResult = checkButton();
    while (buttonResult == 0) { //wait for button press or hold
      if (millis() > (gameUpdate + blinkSpeed)) {
      blinkState = !blinkState;
      gameUpdate = millis();
      }
      delay(100);
      buttonResult = checkButton();
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.setTextColor(SSD1306_WHITE);
      display.println("ENTER NAME");
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
      display.setTextSize(4);
      display.print(" ");
      //only blink the current cursor position
      if (blinkState == 0) {
        display.print(gameHSname[gameNumber][0]);
        display.print(gameHSname[gameNumber][1]);
        display.print(gameHSname[gameNumber][2]);
      } else {
        if (cursor == 0) {
          display.print(" ");
          display.print(gameHSname[gameNumber][1]);
          display.print(gameHSname[gameNumber][2]);
        } else if (cursor == 1) {
          display.print(gameHSname[gameNumber][0]);
          display.print(" ");
          display.print(gameHSname[gameNumber][2]);
        } else if (cursor == 2) {
          display.print(gameHSname[gameNumber][0]);
          display.print(gameHSname[gameNumber][1]);
          display.print(" ");
        }
      }
      display.println(" ");
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.println("Click for next, hold to select");
      display.display();
      btnLED.clear();
      btnLED.show();
    }
    if (buttonResult == 1) { //button pressed
      if (gameHSname[gameNumber][cursor] < 'Z') {
      gameHSname[gameNumber][cursor]++;
      } else {
        gameHSname[gameNumber][cursor] = 'A';
      }
    } else if (buttonResult == 2) { //button held
      if (cursor < 2) {
        while (checkButton() == 2) //if the button is still being held from the game screen,wait for it to be released
        {
          delay(100);
        }
        cursor++;
      } else if (cursor = 2) {
        //save the high score
        EEPROM.put(gameHSnameAddress[gameNumber], gameHSname[gameNumber]);
        EEPROM.put(gameHSaddress[gameNumber], newHighScore);
        EEPROM.commit();
        screen = 2; //set the screen to the first game
        gameState = 0; //reset the state
        gameMode = 1; //set the game mode to title
        while (checkButton() != 0) { //fake the intro screen until the button is released
          display.clearDisplay();
          display.setTextSize(2);
          display.setCursor(15, 0);
          display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
          display.println(" Colors ");
          display.setTextColor(SSD1306_WHITE);
          display.setTextSize(3);
          display.println("Hold to");
          display.println(" START");
          display.display();
          btnLED.clear();
          btnLED.setBrightness(fade);
          btnLED.fill(getColor(2),0,1);
          btnLED.show();
        }
        gameTitle2(); //display the first game title screen
       }
    }
  }
}

void countdown() { //counts down from 3 to 0 on the OLED screen
  int gameStartCountdown = 4000;
  unsigned long countdownStart = millis();
  while (mytime < (countdownStart + gameStartCountdown)) {
    mytime = millis();
    display.clearDisplay();
    display.setTextSize(5);
    int timeLeft = (gameStartCountdown - (mytime - countdownStart)) / 1000;
    if (timeLeft > 0 && timeLeft < 4) {
      display.setCursor(50, 16);
      display.println(timeLeft);
    }
    display.display();
  }
  display.clearDisplay();
  display.display();
}

void readySetGo() {//displays ready...set...go on the OLED screen
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 16);
  display.setTextColor(SSD1306_WHITE);
  display.print("READY");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(38, 16);
  display.print("SET");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(74, 16);
  display.print("GO!");
  display.display();
  delay(1000);
}

void game2intro() { //colors - press the button when the given color is shown
  if (gameState == 0) { //select a random color
      selectedColor = random(COLOR_COUNT);
      gameColorName = colorNames[selectedColor];
      btnLED.setBrightness(255);
      gameState = 1;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("COLORS");
  display.setTextSize(1);
  display.println("Press the button whenit turns");
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.print(" ");
  display.print(gameColorName);
  display.println(" ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.fill(getColor(selectedColor),0,1);
  btnLED.show();
  delay(250);
  while (checkButton() != 0) //wait for button to be released
  {
    delay(250);
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  display.clearDisplay(); //blank display before going to countdown
  display.display();
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  countdown(); //display the countdown
  display.clearDisplay(); //clear screen after countdown
  display.display();
  game2(selectedColor); //start the game
}



void game2(int selectedColor) { //colors
  currentGame = 2;
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameTime = random(3000, 10000); //randomly pick game duration
    gameEnd = gameStart + gameTime; //calculate when the game should end
    gameSpeed = 250; //set the game speed (between color changes)
    gameUpdate = 0; //reset the game update time
    display.clearDisplay(); //turn off the display
    display.display();
    gameState = 3; //set the game state to running
  } 
  
  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {

    mytime = millis();
    while (digitalRead(buttonPin) == HIGH) { //monitor button without delay
      delay(10);
      mytime = millis(); //update the time

      if ((millis()- gameUpdate) > gameSpeed) { //change the button to a random color every 250ms
        gameUpdate = millis(); //record when the game was updated
        gameColor = random(COLOR_COUNT); //pick a random color
        while (gameColor == selectedColor) { //make sure the color is not the same as the target color
          gameColor = random(COLOR_COUNT); //if it's the same as the target color, pick a new color
        }
        btnLED.clear();
        btnLED.fill(getColor(gameColor),0,1); //set the pixel to the new color
        btnLED.show();
        } else if (millis() > gameEnd) { //when the timer hits 0 start monitoring for reaction time
        gameState = 5; //set the game state finished
        innerLoopBroken = true;
        break;
        }
      }

    if (millis() < gameEnd) {//if the game exited early, display the too soon screen
      gameState = 4; //set the game state to lost
      gameMode = 4; //set the game mode to over
      gameScore = gameEnd - millis(); //calculate the reaction time
      tooSoon(gameScore, currentGame);
    } else { //game is over
      // do nothing
    }
  }
  

  while (gameState == 5) { //game is finished
    gameMode = 4; //set the game mode to over
    while (digitalRead(buttonPin) == HIGH) {
      btnLED.clear();
      btnLED.fill(getColor(selectedColor),0,1);
      btnLED.show();
      delay(10);
      gameEnd = millis(); //keep updating the game end time until the button is pressed
    }
    gameScore = gameEnd - (gameStart + gameTime) ; //calculate the reaction time
    if (gameScore < gameHS[currentGame]) { //if the score is a new high score
      gameHS[currentGame] = gameScore; //set the new high score
      gameState = 6; //set the game state to new high score
      newHS(gameHS[currentGame], currentGame); //display the new high score screen
    } else { //if the score is not a new high score
      gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
    }
  }
}

void game3intro() { //countdown - press the button when the timer hits 0
  gameState = 1; //nothing to configure
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println(" COUNTDOWN ");
  display.setTextSize(1);
  display.println("Press the button when the timer hits 0");
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.show();
  delay(1000); //let button be released
  while (checkButton() != 0) //if the button is still being held from the intro screen,wait for it to be released
  {
    //do nothing
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  display.clearDisplay(); //blank display before going to countdown
  display.display();
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  readySetGo(); //display the countdown
  display.clearDisplay(); //clear screen after countdown
  display.display();
  game3(); 
}

void game3() { //countdown
  currentGame = 3;
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameTime = random(10, 15) * 1000; //randomly pick game duration, ensuring it is evenly divisible by 1000
    gameEnd = gameStart + gameTime; //calculate when the game should end
    gameSpeed = 250; //set the game speed (between color changes)
    display.clearDisplay(); //turn off the display
    delay(100);
    gameState = 3; //set the game state to running
  } 
  
  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {
    mytime = millis();
    while (digitalRead(buttonPin) == HIGH) { //monitor button without delay
      mytime = millis(); //update the time
      unsigned long timeLeft = (gameEnd - mytime); //calculate time remaining until gameEnd
      if ((timeLeft > (gameTime / 3)) && (millis() < gameEnd)) { //only show countdown if timeLeft is > 1/3rd of gameTime
        display.clearDisplay();
        display.setTextSize(5);
        display.setCursor(50, 16);
        display.println(timeLeft / 1000);
        display.display();
      } else if (millis() > gameEnd) { //when the timer hits 0 start monitoring for reaction time
      gameState = 5; //set the game state finished
      innerLoopBroken = true;
      delay(10);
      break;
      } else if ((timeLeft < (gameTime / 3)) && (timeLeft >= 0)) { //blank the screen when less than 1/3rd of gameTime is left
        display.clearDisplay();
        display.setCursor(0,16);
        display.setTextSize(4);
        display.println("? ? ?");
        display.display();
      }
    }

    if (millis() < gameEnd) {//if the game exited early, display the too soon screen
    gameState = 4; //set the game state to lost
    gameMode = 4; //set the game mode to over
    gameScore = gameEnd - millis(); //calculate the reaction time
    delay(10);
    tooSoon(gameScore, currentGame);
    } else { //game is over
    // do nothing
    }
  }

  if (millis() < gameEnd) {//if the game exited early, display the too soon screen
    gameState = 4; //set the game state to lost
    gameMode = 4; //set the game mode to over
    gameScore = gameEnd - millis(); //calculate the reaction time
    delay(250);
    tooSoon(gameScore, currentGame);
    } else { //game is over
    // do nothing
  }

  while (gameState == 5) { //game is finished
    gameMode = 4; //set the game mode to over
    while (digitalRead(buttonPin) == HIGH) {
      delay(10);
      gameEnd = millis(); //keep updating the game end time until the button is pressed
    }
    gameScore = gameEnd - (gameStart + gameTime) ; //calculate the reaction time
    if (gameScore < gameHS[currentGame]) { //if the score is a new high score
      gameHS[currentGame] = gameScore; //set the new high score
      gameState = 6; //set the game state to new high score
      newHS(gameHS[currentGame], currentGame); //display the new high score screen
    } else { //if the score is not a new high score
      gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
    }
  }

}

void game4intro() { //count up - press the button when the timer hits the specified number
  if (gameState == 0) { //select a random color
      gameTime = random(3, 9); //randomly pick game duration
      gameState = 1;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println(" COUNT UP");
  display.setTextSize(1);
  display.print("Press the button when the counter hits ");
  display.println(gameTime * 100);
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.show();
  while (checkButton() != 0) //if the button is still being held from the intro screen,wait for it to be released
  {
    //do nothing
    delay(250);
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  display.clearDisplay(); //blank display before going to countdown
  display.display();
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  countdown(); //display the countdown
  display.clearDisplay(); //clear screen after countdown
  display.display();
  game4(gameTime); //start the game
}

void game4(int gameTime) { //count up
  currentGame = 4;
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameEnd = gameStart + (gameTime * 1000); //calculate when the game should end
    display.clearDisplay(); //turn off the display
    delay(250);
    gameState = 3; //set the game state to running
  } 
  
  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {
    mytime = millis();
    //delay(1000);
    while (digitalRead(buttonPin) == HIGH) { //monitor button without delay
      mytime = millis(); //update the time
      int timeLeft = (gameEnd - mytime); //calculate time remaining until gameEnd
      if (timeLeft > gameTime) { 
        display.clearDisplay();
        display.setTextSize(4);
        display.setCursor(24, 16);
        display.println((millis() - gameStart) / 10);
        display.display();
      } else if (millis() > gameEnd) { //when the timer hits 0 start monitoring for reaction time
      gameState = 5; //set the game state finished
      innerLoopBroken = true;
      delay(10);
      break;
      }
    }

    if (millis() < gameEnd) {//if the game exited early, display the too soon screen
    gameState = 4; //set the game state to lost
    gameMode = 4; //set the game mode to over
    gameScore = gameEnd - millis(); //calculate the reaction time
    delay(10);
    tooSoon(gameScore, currentGame);
    } else { //game is over
    // do nothing
    }
  }

  if (millis() < gameEnd) {//if the game exited early, display the too soon screen
    gameState = 4; //set the game state to lost
    gameMode = 4; //set the game mode to over
    gameScore = gameEnd - millis(); //calculate the reaction time
    delay(250);
    tooSoon(gameScore, currentGame);
    } else { //game is over
    // do nothing
  }

  while (gameState == 5) { //game is finished
    gameMode = 4; //set the game mode to over
    while (digitalRead(buttonPin) == HIGH) {
      gameEnd = millis(); //keep updating the game end time until the button is pressed
      display.clearDisplay();
      display.setTextSize(4);
      display.setCursor(24, 16);
      display.println((millis() - gameStart) / 10);
      display.display();
    }
    gameScore = gameEnd - (gameStart + (gameTime * 1000)); //calculate the reaction time
    if (gameScore < gameHS[currentGame]) { //if the score is a new high score
      gameHS[currentGame] = gameScore; //set the new high score
      gameState = 6; //set the game state to new high score
      newHS(gameHS[currentGame], currentGame); //display the new high score screen
    } else { //if the score is not a new high score
      gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
    }
  }
}

void game5intro() { //pinpoint - press the button the specified number of times as fast as you can
  if (gameState == 0) { //configure the game
      gameSpeed = random(40, 100);
      btnLED.setBrightness(255);
      gameState = 1;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println(" PINPOINT ");
  display.setTextSize(1);
  display.print("Press the button");
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setCursor(40, 32);
  display.print(" ");
  display.print(gameSpeed);
  display.print(" ");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.println(" times");
  display.setCursor(0, 48);
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.show();
  delay(250);
  while (checkButton() != 0) //if the button is still being held from the intro screen,wait for it to be released
  {
    delay(250);
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(0, 16);
  display.setTextColor(SSD1306_WHITE);
  display.print("READY");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(38, 16);
  display.print("SET");
  display.display();
  delay(1000);
  display.clearDisplay();
  display.setCursor(74, 16);
  display.print("GO!");
  display.display();
  game5(gameSpeed); //start the game
}

void game5(int gameSpeed) { //pinpoint
  currentGame = 5;
  int allowance = 350; //allowance for button press
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameTime = 3000; //start with 3 seconds on the clock
    gameEnd = gameStart + gameTime; //calculate when the game should end
    gameState = 3; //set the game state to running
    gameColor = 0; //set the game color to 0
  } 

  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {
    gameButton = digitalRead(buttonPin); //read the button state
    delay(10);
    if ((gameButton == LOW) && (gameButton != lastGameButton)) { //if the button switched states
      gameScore++; //add 1 to the score
      gameTime = gameTime + allowance; //add 250ms to the game time
      gameEnd = gameStart + gameTime; //calculate when the game should end
      gameUpdate = millis(); //record the last time the button was pressed
      display.clearDisplay();
      display.setTextSize(4);
      display.setCursor(32, 16);
      display.print(random(1,100)); //display a random number to make it harder
      display.display();
      if (gameColor < COLOR_COUNT) {
        gameColor++; 
        btnLED.clear();
        btnLED.fill(getColor(gameColor),0,1); //set the pixel to the new color
        btnLED.show();
      } else {
        gameColor = 0;
        btnLED.clear();
        btnLED.fill(getColor(gameColor),0,1); //set the pixel to the new color
        btnLED.show();
      }
      lastGameButton = gameButton; //set the last button state to the current button state
    } else if ((gameButton == HIGH) && (gameButton != lastGameButton)) {//if the button switched states
      lastGameButton = gameButton; //set the last button state to the current button state
    } else if (millis() > gameEnd) { //when the timer hits stop counting
      gameState = 5; //set the game state finished
      innerLoopBroken = true;
      break;
    }
  }
  gameMode = 4; //set the game mode to over
  if (gameScore == gameSpeed) {
    gameLength = gameSpeed * allowance;
    //last button press - game start - (gameSpeed * allowance)
    gameScore = gameUpdate - gameStart - gameLength;
  } else if (gameScore < gameSpeed) {
    gameState = 4; //set the game state to lost
    sorry(gameScore, currentGame);
    return;
  } else if (gameScore > gameSpeed) {
    gameState = 4; //set the game state to lost
    sorry(gameScore, currentGame);
    return;
  }

  if (gameScore < gameHS[currentGame]) { //if the score is a new high score
    gameHS[currentGame] = gameScore; //set the new high score
    gameState = 6; //set the game state to new high score
    newHS(gameHS[currentGame], currentGame); //display the new high score screen
  } else { //if the score is not a new high score
    gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
  }
}
  

void game6intro() { //classic - press the button when it lights up
  if (gameState == 0) { //configure the game
      btnLED.setBrightness(255);
      gameState = 1;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("  CLASSIC");
  display.setTextSize(1);
  display.println("Press the button whenit lights up");
  display.setCursor(0, 48);
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.show();
  delay(250);
  while (checkButton() != 0) //if the button is still being held from the intro screen,wait for it to be released
  {
    //do nothing
    delay(250);
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  display.clearDisplay(); //blank display before going to countdown
  display.display();
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  countdown(); //display the countdown
  display.clearDisplay(); //clear screen after countdown
  display.display();
  game6(); //start the game
}

void game6() { //classic
  currentGame = 6;
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameTime = random(3000, 10000); //randomly pick game duration
    gameEnd = gameStart + gameTime; //calculate when the game should end
    display.clearDisplay(); //turn off the display
    display.display();
    gameState = 3; //set the game state to running
  } 
  
  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {

    mytime = millis();
    while (digitalRead(buttonPin) == HIGH) { //monitor button without delay
      delay(10);
      mytime = millis(); //update the time

      if (millis() > gameEnd) { //when the timer hits 0 start monitoring for reaction time
        gameState = 5; //set the game state finished
        innerLoopBroken = true;
        break;
        }
      }

    if (millis() < gameEnd) {//if the game exited early, display the too soon screen
      gameState = 4; //set the game state to lost
      gameMode = 4; //set the game mode to over
      gameScore = gameEnd - millis(); //calculate the reaction time
      tooSoon(gameScore, currentGame);
    } else { //game is over
      // do nothing
    }
  }
  

  while (gameState == 5) { //game is finished
    gameMode = 4; //set the game mode to over
    while (digitalRead(buttonPin) == HIGH) {
      btnLED.clear();
      btnLED.fill(getColor(0),0,1);
      btnLED.show();
      delay(10);
      gameEnd = millis(); //keep updating the game end time until the button is pressed
    }
    gameScore = gameEnd - (gameStart + gameTime) ; //calculate the reaction time
    if (gameScore < gameHS[currentGame]) { //if the score is a new high score
      gameHS[currentGame] = gameScore; //set the new high score
      gameState = 6; //set the game state to new high score
      newHS(gameHS[currentGame], currentGame); //display the new high score screen
    } else { //if the score is not a new high score
      gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
    }
  }
}

void game7intro() { //quick fire - press the button as many times as you can in the alloted time
  if (gameState == 0) {
      btnLED.setBrightness(255);
      gameState = 1;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("QUICK FIRE");
  display.setTextSize(1);
  display.println("Press the button as  many times as you canin 30 seconds");
  display.println("");
  display.println("Press button to start");
  display.display();
  btnLED.clear();
  btnLED.show();
  delay(250);
  while (checkButton() != 0) //if the button is still being held from the intro screen,wait for it to be released
  {
    //do nothing
    delay(250);
  }
  while ((gameMode == 2) && (checkButton() == 0)) { // && (checkButton == 0) while the game is in intro mode and the button is not pressed
    //wait for the button to be pressed
    delay(10);
    }
  gameMode = 3; // set the game mode to play
  gameState = 2; // set the game state to open
  display.clearDisplay(); //blank display before going to countdown
  display.display();
  btnLED.clear(); //blank button before going to countdown
  btnLED.show();
  countdown(); //display the countdown
  display.clearDisplay(); //clear screen after countdown
  display.display();
  game7(); //start the game
}

void game7() { //quick fire
  currentGame = 7;
  if (gameState == 2) { // coming from intro screen, configure the game
    gameScore = 0; //reset the score
    gameStart = millis(); //record when the game started
    gameTime = 30000; //set the game duration to 30 seconds
    gameEnd = gameStart + gameTime; //calculate when the game should end
    display.clearDisplay(); //turn off the display
    display.display();
    gameState = 3; //set the game state to running
    gameButton = HIGH; //set the button state to high
  } 

  bool innerLoopBroken = false;
  while (gameState == 3 && !innerLoopBroken) {
    mytime = millis();
    display.clearDisplay();
    display.setTextSize(4);
    display.setCursor(24, 16);
    display.println(gameScore);
    display.display();
    gameButton = digitalRead(buttonPin); //read the button state
    if ((gameButton == LOW) && (gameButton != lastGameButton)) { //if the button switched states
      gameScore++; //add 1 to the score
      if (gameColor < COLOR_COUNT) {
        gameColor++; 
        btnLED.clear();
        btnLED.fill(getColor(gameColor),0,1); //set the pixel to the new color
        btnLED.show();
      } else {
        gameColor = 0;
        btnLED.clear();
        btnLED.fill(getColor(gameColor),0,1); //set the pixel to the new color
        btnLED.show();
      }
      
      lastGameButton = gameButton; //set the last button state to the current button state
    } else if ((gameButton == HIGH) && (gameButton != lastGameButton)) {//if the button switched states
      lastGameButton = gameButton; //set the last button state to the current button state
    } else if (millis() > gameEnd) { //when the timer hits stop counting
      gameState = 5; //set the game state finished
      innerLoopBroken = true;
      break;
    }
  }
  

  while (gameState == 5) { //game is finished
    delay(3000);
    gameMode = 4; //set the game mode to over
    if (gameScore > gameHS[currentGame]) { //if the score is a new high score
      gameHS[currentGame] = gameScore; //set the new high score
      gameState = 6; //set the game state to new high score
      newHS(gameHS[currentGame], currentGame); //display the new high score screen
    } else { //if the score is not a new high score
      gameState = 6; //set the game state to new high score
      gameOverScreen(gameHS[currentGame], gameScore, currentGame); //pass the high score, the score, and the game number to the game over screen
    }
  }
}