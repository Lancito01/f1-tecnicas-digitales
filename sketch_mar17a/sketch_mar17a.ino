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
bool activePlayers[playerCount];
// per-player non-blocking debounce
const unsigned long playerDebounceMs = 50;
unsigned long lastPlayerDebounceTime[playerCount];
bool lastPlayerRaw[playerCount];
bool stablePlayerBtn[playerCount];
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
  // initialize player activation state (all OFF by default)
  for (int i = 0; i < playerCount; i++) {
    activePlayers[i] = false;
    lastPlayerRaw[i] = false;
    stablePlayerBtn[i] = false;
    lastPlayerDebounceTime[i] = 0;
    writeLed(playerLedPins[i], false);
  }
}

void loop() {
  bool currPressed = readButton(mainBtnPin);

  switch (currentState) {
    case WAITING_FOR_GAME:
      // show which players are activated
      for (int i = 0; i < playerCount; i++) {
        writeLed(playerLedPins[i], activePlayers[i]);
      }

      // allow players to toggle their activation by pressing their button (non-blocking debounce)
      for (int p = 0; p < playerCount; p++) {
        bool raw = readButton(playerBtnPins[p]); // true == pressed
        if (raw != lastPlayerRaw[p]) {
          lastPlayerDebounceTime[p] = millis();
        }
        if (millis() - lastPlayerDebounceTime[p] > playerDebounceMs) {
          if (raw != stablePlayerBtn[p]) {
            stablePlayerBtn[p] = raw;
            if (stablePlayerBtn[p]) { // stable press detected
              activePlayers[p] = !activePlayers[p];
              writeLed(playerLedPins[p], activePlayers[p]);
            }
          }
        }
        lastPlayerRaw[p] = raw;
      }

      if (currPressed && !lastPressed) {  //? user just pressed the main button
        lastPressed = currPressed;
        writeLed(mainLedPin, false);

      } else if (!currPressed && lastPressed) {  //? user just let go of the main button
        // count activated players
        int activeCount = 0;
        for (int i = 0; i < playerCount; i++) if (activePlayers[i]) activeCount++;
        if (activeCount == 0) {
          Serial.println("No players activated. Toggle player buttons to join.");
          lastPressed = currPressed;
          break;
        }

        // start game for activated players
        currentState = PLAYING;
        lastPressed = currPressed;
        for (int i = 0; i < totalGears; i++) {
          gearTimings[i] = random(1000, 3001);
          for (int player = 0; player < playerCount; player++) {
            if (activePlayers[player]) playerTimes[player][i] = -1; // waiting to press
            else playerTimes[player][i] = -2; // sentinel for inactive player
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
          // only consider presses from activated players
          if (!activePlayers[player]) {
            playerPressed[player] = false;
          } else {
            playerPressed[player] = readButton(playerBtnPins[player]);
          }
        }

        if (currentGearRunning >= totalGears) {
          currentState = GAME_END;
          // compute results only for activated players
          const int NOT_PLAYING_SCORE = 1000000000;
          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) {
              playerResults[player] = NOT_PLAYING_SCORE;
              continue;
            }
            playerResults[player] = 0;
            for (int gear = 0; gear < totalGears; gear++) {
              if (playerTimes[player][gear] >= 0) playerResults[player] += playerTimes[player][gear];
              else if (playerTimes[player][gear] == penaltyMs) playerResults[player] += penaltyMs;
              // ignore -2 sentinel for inactive players (shouldn't happen for active)
            }
          }

          // find best among activated players
          int bestPlayerIdx = -1;
          int bestResult = NOT_PLAYING_SCORE;
          bool hasTie = false;
          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) continue;
            if (bestPlayerIdx == -1) {
              bestPlayerIdx = player;
              bestResult = playerResults[player];
              hasTie = false;
            } else if (playerResults[player] < bestResult) {
              bestResult = playerResults[player];
              bestPlayerIdx = player;
              hasTie = false;
            } else if (playerResults[player] == bestResult) {
              hasTie = true;
            }
          }
          lastWinner = hasTie ? 0 : (bestPlayerIdx + 1);

          for (int player = 0; player < playerCount; player++) {
            Serial.print(playerNames[player]);
            if (!activePlayers[player]) Serial.println(" (not playing)");
            else Serial.println(" player times: ");
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
          if (!activePlayers[player]) continue;
          if (playerTimes[player][currentGearRunning] == -1) {
            allPlayersDone = false;
            break;
          }
        }

        if (millis() - playerReactionTime >= reactionWindowMs
            || allPlayersDone) {  // current gear ends
          // checks for "didn't press" (only for active players)
          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) continue;
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
      if (lastWinner == 0) {  // tie, turn on all *activated* players
        for (int i = 0; i < playerCount; i++) {
          if (activePlayers[i]) writeLed(playerLedPins[i], true);
        }
      } else {
        // only light winner if they were activated
        int idx = lastWinner - 1;
        if (idx >= 0 && idx < playerCount && activePlayers[idx]) writeLed(playerLedPins[idx], true);
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
