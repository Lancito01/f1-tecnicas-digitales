// <>
#include <esp_system.h>
const int mainBtnPin = 19;
const int blueBtnPin = 18;
const int redBtnPin = 5;
const int mainLedPin = 23;
const int blueLedPin = 25;
const int redLedPin = 26;

enum State { WAITING_FOR_GAME,
             PLAYING,
             GAME_END };
const int totalGears = 4;
const int penaltyMs = 1000;
const int reactionWindowMs = 800;
const int jumpStartThresholdMs = 111;
const int debounceGracePeriodMs = 250;

int buttonPins[] = { mainBtnPin, blueBtnPin, redBtnPin };
int ledPins[] = { mainLedPin, blueLedPin, redLedPin };
int lastWinner = -1;  //* 0 for tie, 1 for blue, 2 for red
bool powerMainLed = false;
bool lastPressed;
unsigned long lastBlinkTimeMs;
int bluePlayerTimes[totalGears];
int redPlayerTimes[totalGears];
int gearTimings[totalGears];
int currentGearRunning;
unsigned long currentGearRunningTime;
unsigned long playerReactionTime;
bool shouldPress;
State currentState;

bool readButton(int buttonPin) {
  return (digitalRead(buttonPin) == LOW);  //* Button pressed: current LOW
}

void writeLed(int ledPin, bool value) {
  digitalWrite(ledPin, value ? HIGH : LOW);
}

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }
  randomSeed((uint32_t)esp_random());  // makes timings variable each reset
  Serial.begin(9600);
  Serial.println("> Waiting for user to start game...");
  currentState = WAITING_FOR_GAME;
  lastPressed = false;
}

void loop() {
  bool currPressed = readButton(mainBtnPin);

  switch (currentState) {
    case WAITING_FOR_GAME:
      writeLed(blueLedPin, false);
      writeLed(redLedPin, false);
      if (currPressed && !lastPressed) {  //? user just pressed the button
        lastPressed = currPressed;
        writeLed(mainLedPin, false);

      } else if (!currPressed && lastPressed) {  //? user just let go of the button
        currentState = PLAYING;
        lastPressed = currPressed;
        gearTimings[0] = random(2000, 4001);
        for (int i = 0; i < totalGears; i++) {
          gearTimings[i] = random(1000, 3001);
          bluePlayerTimes[i] = -1;
          redPlayerTimes[i] = -1;
        }

        lastBlinkTimeMs = millis();
        currentGearRunningTime = millis();
        currentGearRunning = 0;
        shouldPress = false;
        playerReactionTime = millis();
        writeLed(mainLedPin, true);
      }
      break;
    case PLAYING:
      {
        bool bluePlayer = readButton(blueBtnPin);
        bool redPlayer = readButton(redBtnPin);

        if (currentGearRunning >= totalGears) {
          currentState = GAME_END;
          int redResult = 0;
          for (int time : redPlayerTimes) {
            redResult += time;
          }

          int blueResult = 0;
          for (int time : bluePlayerTimes) {
            blueResult += time;
          }

          if (redResult < blueResult) {
            lastWinner = 2;
          } else if (redResult > blueResult) {
            lastWinner = 1;
          } else {
            lastWinner = 0;
          }

          Serial.println("Red player times: ");
          for (int score : redPlayerTimes) {
            Serial.print(score);
            Serial.println(" ");
          }
          Serial.print("Result: ");
            Serial.println(redResult);

          Serial.println("Blue player times: ");
          for (int score : bluePlayerTimes) {
            Serial.print(score);
            Serial.println(" ");
          }
          Serial.print("Result: ");
            Serial.println(blueResult);
          break;
        }

        //? blink main led when expecting a press
        if (millis() - lastBlinkTimeMs > 30 && shouldPress) {
          lastBlinkTimeMs = millis();
          powerMainLed = !powerMainLed;
          writeLed(mainLedPin, powerMainLed);
        }

        // waiting for next gear and activate it
        if (millis() - currentGearRunningTime > gearTimings[currentGearRunning]
            && !shouldPress) {
          shouldPress = true;
          playerReactionTime = millis();
        }

        // checks for "pressed too early"
        if (!shouldPress && redPlayer && millis() - currentGearRunningTime > debounceGracePeriodMs) {
          redPlayerTimes[currentGearRunning] = penaltyMs;
        }
        if (!shouldPress && bluePlayer && millis() - currentGearRunningTime > debounceGracePeriodMs) {
          bluePlayerTimes[currentGearRunning] = penaltyMs;
        }

        if (!shouldPress) break;  // early return case

        // check for jumpstart
        if (redPlayer && millis() - playerReactionTime < jumpStartThresholdMs) {
          redPlayerTimes[currentGearRunning] = penaltyMs;
        }
        if (bluePlayer && millis() - playerReactionTime < jumpStartThresholdMs) {
          bluePlayerTimes[currentGearRunning] = penaltyMs;
        }

        // adds current time if they didn't press too early
        if (redPlayer && redPlayerTimes[currentGearRunning] == -1) {
          redPlayerTimes[currentGearRunning] = millis() - playerReactionTime;
        }
        if (bluePlayer && bluePlayerTimes[currentGearRunning] == -1) {
          bluePlayerTimes[currentGearRunning] = millis() - playerReactionTime;
        }

        if (millis() - playerReactionTime >= reactionWindowMs
            || (redPlayerTimes[currentGearRunning] != -1 && bluePlayerTimes[currentGearRunning] != -1)) {  // current gear ends
          // checks for "didn't press"
          if (redPlayerTimes[currentGearRunning] == -1) {
            redPlayerTimes[currentGearRunning] = penaltyMs;
          }
          if (bluePlayerTimes[currentGearRunning] == -1) {
            bluePlayerTimes[currentGearRunning] = penaltyMs;
          }

          writeLed(mainLedPin, true);
          powerMainLed = false;
          currentGearRunning++;               // increase current gear
          currentGearRunningTime = millis();  // reset timer for gear switcher
          shouldPress = false;
        }
        break;
      }
    case GAME_END:
      writeLed(mainLedPin, false);
      if (lastWinner == 0) {  // tie, turn on both
        writeLed(redLedPin, true);
        writeLed(blueLedPin, true);
      } else {
        writeLed(lastWinner == 2 ? redLedPin : blueLedPin, true);
      }
      if (currPressed) {
        while (readButton(mainBtnPin)) {
          delay(1);
        }
        currentState = WAITING_FOR_GAME;
        lastPressed = false;
      }
      break;
  }
}
