// <>
#include <esp_system.h>
const int mainBtnPin = 22;
const int mainLedPin = 23;
const int redBtnPin = 32;
const int blueBtnPin = 33;
const int greenBtnPin = 25;
const int yellowBtnPin = 26;
const int redLedPin = 21;
const int blueLedPin = 19;
const int greenLedPin = 18;
const int yellowLedPin = 5;

enum State { WAITING_FOR_GAME,
             PLAYING,
             GAME_END };
const int playerCount = 4;
const int totalGears = 4;
const int penaltyMs = 1000;
const int reactionWindowMs = 800;
const int jumpStartThresholdMs = 111;
const int debounceGracePeriodMs = 250;

const int playerBtnPins[playerCount] = { redBtnPin, blueBtnPin, greenBtnPin, yellowBtnPin };
const int playerLedPins[playerCount] = { redLedPin, blueLedPin, greenLedPin, yellowLedPin };
const char* playerNames[playerCount] = { "Red", "Blue", "Green", "Yellow" };
int lastWinner = -1;  //* 0 for tie, 1 for red, 2 for blue, 3 for green, 4 for yellow
bool powerMainLed = false;
bool lastPressed;
unsigned long lastBlinkTimeMs;
int playerTimes[playerCount][totalGears];
int playerResults[playerCount];
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
  pinMode(mainBtnPin, INPUT_PULLUP);
  pinMode(mainLedPin, OUTPUT);
  for (int i = 0; i < playerCount; i++) {
    pinMode(playerBtnPins[i], INPUT_PULLUP);
    pinMode(playerLedPins[i], OUTPUT);
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
      for (int i = 0; i < playerCount; i++) {
        writeLed(playerLedPins[i], false);
      }
      if (currPressed && !lastPressed) {  //? user just pressed the button
        lastPressed = currPressed;
        writeLed(mainLedPin, false);

      } else if (!currPressed && lastPressed) {  //? user just let go of the button
        currentState = PLAYING;
        lastPressed = currPressed;
        gearTimings[0] = random(2000, 4001);
        for (int i = 0; i < totalGears; i++) {
          gearTimings[i] = random(1000, 3001);
          for (int player = 0; player < playerCount; player++) {
            playerTimes[player][i] = -1;
          }
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
        bool playerPressed[playerCount];
        for (int player = 0; player < playerCount; player++) {
          playerPressed[player] = readButton(playerBtnPins[player]);
        }

        if (currentGearRunning >= totalGears) {
          currentState = GAME_END;
          for (int player = 0; player < playerCount; player++) {
            playerResults[player] = 0;
            for (int gear = 0; gear < totalGears; gear++) {
              playerResults[player] += playerTimes[player][gear];
            }
          }

          int bestResult = playerResults[0];
          int bestPlayerIdx = 0;
          bool hasTie = false;
          for (int player = 1; player < playerCount; player++) {
            if (playerResults[player] < bestResult) {
              bestResult = playerResults[player];
              bestPlayerIdx = player;
              hasTie = false;
            } else if (playerResults[player] == bestResult) {
              hasTie = true;
            }
          }
          lastWinner = hasTie ? 0 : bestPlayerIdx + 1;

          for (int player = 0; player < playerCount; player++) {
            Serial.print(playerNames[player]);
            Serial.println(" player times: ");
            for (int gear = 0; gear < totalGears; gear++) {
              Serial.print(playerTimes[player][gear]);
              Serial.println(" ");
            }
            Serial.print("Result: ");
            Serial.println(playerResults[player]);
          }
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
        for (int player = 0; player < playerCount; player++) {
          if (!shouldPress && playerPressed[player] && millis() - currentGearRunningTime > debounceGracePeriodMs) {
            playerTimes[player][currentGearRunning] = penaltyMs;
          }
        }

        if (!shouldPress) break;  // early return case

        // check for jumpstart
        for (int player = 0; player < playerCount; player++) {
          if (playerPressed[player] && millis() - playerReactionTime < jumpStartThresholdMs) {
            playerTimes[player][currentGearRunning] = penaltyMs;
          }
        }

        // adds current time if they didn't press too early
        for (int player = 0; player < playerCount; player++) {
          if (playerPressed[player] && playerTimes[player][currentGearRunning] == -1) {
            playerTimes[player][currentGearRunning] = millis() - playerReactionTime;
          }
        }

        bool allPlayersDone = true;
        for (int player = 0; player < playerCount; player++) {
          if (playerTimes[player][currentGearRunning] == -1) {
            allPlayersDone = false;
            break;
          }
        }

        if (millis() - playerReactionTime >= reactionWindowMs
            || allPlayersDone) {  // current gear ends
          // checks for "didn't press"
          for (int player = 0; player < playerCount; player++) {
            if (playerTimes[player][currentGearRunning] == -1) {
              playerTimes[player][currentGearRunning] = penaltyMs;
            }
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
      for (int i = 0; i < playerCount; i++) {
        writeLed(playerLedPins[i], false);
      }
      if (lastWinner == 0) {  // tie, turn on all players
        for (int i = 0; i < playerCount; i++) {
          writeLed(playerLedPins[i], true);
        }
      } else {
        writeLed(playerLedPins[lastWinner - 1], true);
      }
      if (currPressed) {
        while (readButton(mainBtnPin)) {  // button is being held down (debounce)
          delay(1);
        }
        currentState = WAITING_FOR_GAME;
        lastPressed = false;
      }
      break;
  }
}
