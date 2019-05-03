#include <Time.h>
#include <TimeLib.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/SystemFont5x7.h>
#include <fonts/Arial14.h>

#define DMD_PIN_CLK   13
#define DMD_PIN_R     11
#define DMD_PIN_NOE   9
#define DMD_PIN_SCK   8
#define DMD_PIN_B     7
#define DMD_PIN_A     6

const int ANALOG_PIN_BRIGHTNESS_BTN = A0;
const int minAnalogBtnSensorValue = 0;
const int maxAnalogBtnSensorValue = 1023;
const int minBrightness = 1;
const int maxBrightness = 255;
int brightnessValue = 255;
int lastBrightnessValueRead = 255;

SoftDMD dmd(1,1, DMD_PIN_NOE, DMD_PIN_A, DMD_PIN_B, DMD_PIN_SCK, DMD_PIN_CLK, DMD_PIN_R);
const uint8_t *FONT = Arial14;
const int LED_PANEL_WIDTH = 32;
const int LED_PANEL_HEIGHT = 16;

const int btnA = 2;
const int btnB = 3;

int btnACurrentState = 0;
int btnALastState = -1;
int btnBCurrentState = 0;
int btnBLastState = -1;

int scoreTeamA = 0;
int scoreTeamB = 0;

// State machine to change print from scoreboard to stopwatch or vice-versa
int timeToUpdateStopwatch = 1000;  // Do this every second or X milliseconds
int timeUntilShowStopwatch = 30000;
int timeUntilShowScoreboard = 6000;
long int goTimeToUpdateStopwatch, goTimeUntilShowStopwatch, goTimeUntilShowScoreboard;
int interrupted = 0;
long intervalBetweenInterrupts = 500; // Push buttons will only count after 10 milliseconds after a push
unsigned long lastInterrupt = 0;

void setup() {
    goTimeToUpdateStopwatch = millis();
    goTimeUntilShowScoreboard = millis() + goTimeUntilShowScoreboard;
    goTimeUntilShowStopwatch = millis() + timeUntilShowStopwatch;

    pinMode(btnA, INPUT_PULLUP);
    pinMode(btnB, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(btnA), btnAInterrupt, FALLING);
    attachInterrupt(digitalPinToInterrupt(btnB), btnBInterrupt, FALLING);

    dmd.setBrightness(brightnessValue);
    dmd.selectFont(FONT);
    dmd.begin();
    dmd.fillScreen(true);
    
    //showStartupAnimation();
    printScoreToDMD();
}

void btnAInterrupt(){
    interrupted = 1;
    if((millis() - lastInterrupt) > intervalBetweenInterrupts) {
        lastInterrupt = millis();
        scoreTeamA++;
    }
    printScoreToDMD();
}

void btnBInterrupt(){
    interrupted = 1;
    if((millis() - lastInterrupt) > intervalBetweenInterrupts) {
        lastInterrupt = millis();
        scoreTeamB++;
    }
    printScoreToDMD();
}

void loop() {
    if(millis() >= goTimeUntilShowScoreboard) {
        printScoreToDMD();
        interrupted = 0;
    }

    if(millis() >= goTimeUntilShowStopwatch) {
        printTimeToDMD();
    }

    checkAndAjustBrightness();
}

void checkAndAjustBrightness() {
    brightnessValue = analogRead(ANALOG_PIN_BRIGHTNESS_BTN);
    brightnessValue = map(brightnessValue, minAnalogBtnSensorValue, maxAnalogBtnSensorValue, minBrightness, maxBrightness);

    // in case the sensor value is outside the range seen during calibration
    brightnessValue = constrain(brightnessValue, minBrightness, maxBrightness);

    if(brightnessValue < (lastBrightnessValueRead - 16) || brightnessValue > (lastBrightnessValueRead + 16)) {
        dmd.setBrightness(brightnessValue);
        lastBrightnessValueRead = brightnessValue;
    }
}

void checkInput() {
    btnACurrentState = digitalRead(btnA);
    btnBCurrentState = digitalRead(btnB);

    if(btnACurrentState != btnALastState) {
        if(btnACurrentState == LOW){
            scoreTeamA++;
            printScoreToDMD();
        }
    }
    if(btnBCurrentState != btnBLastState) {
        if(btnBCurrentState == LOW){
            scoreTeamB++;
            printScoreToDMD();
        }
    }

    btnALastState = btnACurrentState;
    btnBLastState = btnBCurrentState;
}

void printTimeToDMD() {
    int counter = 0;
    while(counter < 5 && interrupted == 0) {
        if(millis() >= goTimeToUpdateStopwatch) {
            clearPanel();

            String minutesString = String(minute());
            if(minute() < 10) minutesString = "0" + String(minute());

            String secondsString = String(second());
            if(second() < 10) secondsString = "0" + String(second());

            String stopwatchString = minutesString + ":" + secondsString;

            unsigned int stopwatchStringWidth = dmd.stringWidth(stopwatchString);
            dmd.drawString(LED_PANEL_WIDTH / 2 - stopwatchStringWidth / 2, 2, stopwatchString, GRAPHICS_INVERSE);

            goTimeToUpdateStopwatch = millis() + timeToUpdateStopwatch;
            counter++;
        }
    }

    goTimeUntilShowStopwatch = millis() + timeUntilShowStopwatch;
}

void printScoreToDMD() {
    clearPanel();

    int scoreSeparatorX = LED_PANEL_WIDTH / 2 - 2;
    dmd.drawLine(LED_PANEL_WIDTH / 2 - 1, 9, LED_PANEL_WIDTH / 2 + 1, 12, GRAPHICS_OFF);
    dmd.drawLine(LED_PANEL_WIDTH / 2 - 1, 12, LED_PANEL_WIDTH / 2 + 1, 9, GRAPHICS_OFF);

    String scoreTeamAString = String(scoreTeamA);
    unsigned int scoreTeamAStringWidth = dmd.stringWidth(scoreTeamAString);
    dmd.drawString(scoreSeparatorX / 2 - scoreTeamAStringWidth / 2, 2, scoreTeamAString, GRAPHICS_INVERSE);

    String scoreTeamBString = String(scoreTeamB);
    unsigned int scoreTeamBStringWidth = dmd.stringWidth(scoreTeamBString);
    int widthAvailableForScoreTeamB = PANEL_WIDTH - (scoreSeparatorX + 5);
    int scoreTeamBStringX = (scoreSeparatorX + 5) + (widthAvailableForScoreTeamB / 2 - scoreTeamBStringWidth / 2);
    dmd.drawString(scoreTeamBStringX, 2, scoreTeamBString, GRAPHICS_INVERSE);

    goTimeUntilShowScoreboard = millis() + timeUntilShowScoreboard;
}

void clearPanel() {
    dmd.clearScreen();
    dmd.fillScreen(true);
}

void showStartupAnimation() {
    int lastPixelXSetted = 0;
    int lastPixelYSetted = 0;
    int aux = -1;
    for(int i = 0; i < 96; i++) {
        if(i < 32) {
            lastPixelXSetted = i;
            lastPixelYSetted = 0;
            dmd.setPixel(i, 0, GRAPHICS_OFF);
        } else if(i < 48) {
            aux++;
            lastPixelXSetted = 31;
            lastPixelYSetted = aux;
            dmd.setPixel(31, aux, GRAPHICS_OFF);
        } else if(i < 80) {
            lastPixelXSetted = 80-i;
            lastPixelYSetted = 15;
            dmd.setPixel(80-i, 15, GRAPHICS_OFF);
        } else {
            lastPixelXSetted = 0;
            lastPixelYSetted = 96-i;
            dmd.setPixel(0, 96-i, GRAPHICS_OFF);
        }
        delay(12);
        //dmd.setPixel(lastPixelXSetted, lastPixelYSetted, GRAPHICS_ON);
    }
}