// <>
#include <esp_system.h>

// ===== MAIN BUTTON & LED PINS =====
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

// ===== MATRIX PINS (MAX7219) =====
const int dataPin = 4;       // data pin
const int chipSelectPin = 2; // CS / LOAD
const int clockPin = 15;     // CLK

// ===== GAME CONSTANTS =====
enum State { WAITING_FOR_GAME,
             PLAYING,
             GAME_END };
const int playerCount = 4;
const int totalGears = 6;
const int penaltyMs = 1000;
const int reactionWindowMs = 800;
const int jumpStartThresholdMs = 111;
const int debounceGracePeriodMs = 250;

// ===== MATRIX CONSTANTS =====
const int modules = 4;           // 4 x 8x8 modules
const int chipsPerModule = 1;    // 1 MAX7219 per module
const int numDevices = modules * chipsPerModule;
const int matrixRows = 8;
const int colsPerChip = 8;
const int matrixCols = numDevices * colsPerChip; // 32 total columns
const int racetrackFinish = matrixCols - 1;     // column 31 is finish line

// ===== GAME VARIABLES =====
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

// ===== RACETRACK VARIABLES =====
int playerColumn[playerCount];                    // current column position (0-31)
int earlyPressCount[playerCount];                 // how many early presses in current gear
unsigned long raceStartTimeMs;
unsigned long accumulatedStallMs[playerCount];
unsigned long stallStartTimeMs[playerCount];
unsigned long stallReleaseTimeMs[playerCount];
bool playerStalled[playerCount];
bool lastGameplayBtn[playerCount];
float millisecondsPerColumn = 1.0f;
const int powerOfTwo[matrixRows] = {1, 2, 4, 8, 16, 32, 64, 128};
int pixels[matrixRows][matrixCols];               // pixel framebuffer

// ===== WINNER DISPLAY STATE =====
unsigned long winnerBlinkStartTime;
int winnerPlayerIdx = -1;
const int blinkIntervalMs = 500;

bool readButton(int buttonPin) {
  return (digitalRead(buttonPin) == LOW);  //* Button pressed: current LOW
}

void writeLed(int ledPin, bool value) {
  digitalWrite(ledPin, value ? HIGH : LOW);
}

// ===== MATRIX FUNCTIONS =====
void setAllPixelsOff() {
  for (int r = 0; r < matrixRows; r++) {
    for (int c = 0; c < matrixCols; c++) {
      pixels[r][c] = 0;
    }
  }
}

void setPixel(int row, int col, int state) {
  if (row < 0 || row >= matrixRows || col < 0 || col >= matrixCols) return;
  pixels[row][col] = state;
}

void sendByte(int value) {
  int bitMask = 128;
  for (int bit = 0; bit < 8; bit++) {
    digitalWrite(clockPin, LOW);
    if (value & bitMask) digitalWrite(dataPin, HIGH);
    else digitalWrite(dataPin, LOW);
    digitalWrite(clockPin, HIGH);
    bitMask >>= 1;
  }
}

void writeRegisterAllSame(int registerNumber, int value) {
  digitalWrite(chipSelectPin, LOW);
  for (int d = numDevices - 1; d >= 0; d--) {
    sendByte(registerNumber);
    sendByte(value);
  }
  digitalWrite(chipSelectPin, HIGH);
}

void writeRegisterAll(int registerNumber, const int values[]) {
  digitalWrite(chipSelectPin, LOW);
  for (int d = numDevices - 1; d >= 0; d--) {
    sendByte(registerNumber);
    sendByte(values[d]);
  }
  digitalWrite(chipSelectPin, HIGH);
}

int sliceToByte(int row, int deviceIndex) {
  int startCol = deviceIndex * colsPerChip;
  int value = 0;
  for (int bit = 0; bit < colsPerChip; bit++) {
    if (pixels[row][startCol + bit] == 1) value |= powerOfTwo[bit];
  }
  return value;
}

void renderMatrix() {
  int deviceValues[numDevices];
  for (int r = 0; r < matrixRows; r++) {
    for (int d = 0; d < numDevices; d++) {
      deviceValues[d] = sliceToByte(r, d);
    }
    writeRegisterAll(r + 1, deviceValues);
  }
}

void initializeMatrix() {
  writeRegisterAllSame(0x0F, 0x00); // display test register
  writeRegisterAllSame(0x09, 0x00); // decode mode
  writeRegisterAllSame(0x0B, 0x07); // scan limit
  writeRegisterAllSame(0x0A, 0x03); // intensity
  writeRegisterAllSame(0x0C, 0x01); // shutdown register
  delay(50);
  for (int r = 1; r <= matrixRows; r++) {
    writeRegisterAllSame(r, 0x00);
  }
  setAllPixelsOff();
  renderMatrix();
}

// ===== LANE ALLOCATION =====
int countActivePlayers() {
  int activeCount = 0;
  for (int i = 0; i < playerCount; i++) {
    if (activePlayers[i]) activeCount++;
  }
  return activeCount;
}

int getPlayerLaneIndex(int playerIndex) {
  if (!activePlayers[playerIndex]) return -1;

  int laneIndex = 0;
  for (int i = 0; i < playerIndex; i++) {
    if (activePlayers[i]) laneIndex++;
  }
  return laneIndex;
}

bool getPlayerRowRange(int laneIndex, int activeCount, int& startRow, int& endRow) {
  int logicalStartRow = -1;
  int logicalEndRow = -1;

  if (activeCount == 1) {
    if (laneIndex == 0) { logicalStartRow = 3; logicalEndRow = 4; }
  } else if (activeCount == 2) {
    if (laneIndex == 0) { logicalStartRow = 1; logicalEndRow = 2; }
    else if (laneIndex == 1) { logicalStartRow = 5; logicalEndRow = 6; }
  } else if (activeCount == 3) {
    if (laneIndex == 0) { logicalStartRow = 0; logicalEndRow = 1; }
    else if (laneIndex == 1) { logicalStartRow = 3; logicalEndRow = 4; }
    else if (laneIndex == 2) { logicalStartRow = 6; logicalEndRow = 7; }
  } else if (activeCount == 4) {
    logicalStartRow = laneIndex * 2;
    logicalEndRow = logicalStartRow + 1;
  }

  if (logicalStartRow < 0 || logicalEndRow < 0) {
    startRow = -1;
    endRow = -1;
    return false;
  }

  // The installed matrix is mirrored relative to the logical left-to-right lane order.
  startRow = matrixRows - 1 - logicalEndRow;
  endRow = matrixRows - 1 - logicalStartRow;
  return true;
}

void drawLaneSegment(int startRow, int endRow, int col) {
  for (int row = startRow; row <= endRow; row++) {
    setPixel(row, col, 1);
  }
}

void drawPlayerAtColumn(int playerIndex, int activeCount, int col) {
  int laneIndex = getPlayerLaneIndex(playerIndex);
  int startRow, endRow;
  if (laneIndex < 0 || !getPlayerRowRange(laneIndex, activeCount, startRow, endRow)) return;
  drawLaneSegment(startRow, endRow, col);
}

void renderWaitingPreview() {
  int activeCount = countActivePlayers();

  setAllPixelsOff();
  for (int player = 0; player < playerCount; player++) {
    if (!activePlayers[player]) continue;
    drawPlayerAtColumn(player, activeCount, 0);
  }
  renderMatrix();
}

void renderRaceState() {
  int activeCount = countActivePlayers();

  setAllPixelsOff();
  for (int player = 0; player < playerCount; player++) {
    if (!activePlayers[player]) continue;
    drawPlayerAtColumn(player, activeCount, playerColumn[player]);
  }
  renderMatrix();
}

void renderGameEndState() {
  int activeCount = countActivePlayers();
  unsigned long elapsed = millis() - winnerBlinkStartTime;
  bool blink = (elapsed / blinkIntervalMs) % 2 == 0;

  setAllPixelsOff();
  for (int player = 0; player < playerCount; player++) {
    if (!activePlayers[player]) continue;
    if (player == winnerPlayerIdx && !blink) continue;
    drawPlayerAtColumn(player, activeCount, playerColumn[player]);
  }
  renderMatrix();
}

void resetRoundState() {
  writeLed(mainLedPin, false);
  powerMainLed = false;
  shouldPress = false;
  currentGearRunning = 0;
  currentGearRunningTime = 0;
  playerReactionTime = 0;
  raceStartTimeMs = 0;
  winnerPlayerIdx = -1;
  lastWinner = -1;

  for (int player = 0; player < playerCount; player++) {
    playerColumn[player] = 0;
    earlyPressCount[player] = 0;
    accumulatedStallMs[player] = 0;
    stallStartTimeMs[player] = 0;
    stallReleaseTimeMs[player] = 0;
    playerStalled[player] = false;
    lastGameplayBtn[player] = false;
    for (int gear = 0; gear < totalGears; gear++) {
      playerTimes[player][gear] = activePlayers[player] ? -1 : -2;
    }
  }
}

void enterWaitingState() {
  currentState = WAITING_FOR_GAME;
  lastPressed = false;
  resetRoundState();
}

void finalizePlayerStall(int player, unsigned long releaseTimeMs) {
  if (!playerStalled[player]) return;

  if (releaseTimeMs < stallStartTimeMs[player]) {
    releaseTimeMs = stallStartTimeMs[player];
  }

  accumulatedStallMs[player] += releaseTimeMs - stallStartTimeMs[player];
  stallStartTimeMs[player] = 0;
  stallReleaseTimeMs[player] = 0;
  playerStalled[player] = false;
}

void updateExpiredStalls(unsigned long nowMs) {
  for (int player = 0; player < playerCount; player++) {
    if (!playerStalled[player]) continue;
    if (stallReleaseTimeMs[player] == 0) continue;
    if (nowMs >= stallReleaseTimeMs[player]) {
      finalizePlayerStall(player, stallReleaseTimeMs[player]);
    }
  }
}

void refreshPlayerColumns(unsigned long nowMs) {
  if (raceStartTimeMs == 0) return;

  updateExpiredStalls(nowMs);

  for (int player = 0; player < playerCount; player++) {
    if (!activePlayers[player]) {
      playerColumn[player] = 0;
      continue;
    }

    unsigned long totalStall = accumulatedStallMs[player];
    if (playerStalled[player]) {
      if (stallReleaseTimeMs[player] == 0 || nowMs < stallReleaseTimeMs[player]) {
        totalStall += nowMs - stallStartTimeMs[player];
      } else {
        totalStall += stallReleaseTimeMs[player] - stallStartTimeMs[player];
      }
    }

    long movingTimeMs = (long)(nowMs - raceStartTimeMs) - (long)totalStall;
    if (movingTimeMs < 0) movingTimeMs = 0;

    int column = (int)(movingTimeMs / millisecondsPerColumn);
    if (column > racetrackFinish) column = racetrackFinish;
    playerColumn[player] = column;
  }
}

void startGame() {
  resetRoundState();
  currentState = PLAYING;
  lastPressed = false;

  unsigned long totalGearTimeMs = 0;
  for (int gear = 0; gear < totalGears; gear++) {
    gearTimings[gear] = random(1000, 3001);
    totalGearTimeMs += gearTimings[gear];
  }

  millisecondsPerColumn = totalGearTimeMs / (float)racetrackFinish;
  if (millisecondsPerColumn < 1.0f) millisecondsPerColumn = 1.0f;

  unsigned long nowMs = millis();
  raceStartTimeMs = nowMs;
  currentGearRunningTime = nowMs;
  playerReactionTime = nowMs;
  lastBlinkTimeMs = nowMs;

  for (int player = 0; player < playerCount; player++) {
    lastGameplayBtn[player] = activePlayers[player] ? readButton(playerBtnPins[player]) : false;
  }

  writeLed(mainLedPin, true);
  refreshPlayerColumns(nowMs);
}

void enterGameEndState(int winner) {
  currentState = GAME_END;
  winnerPlayerIdx = winner;
  lastWinner = winner + 1;
  winnerBlinkStartTime = millis();
  lastPressed = false;
  writeLed(mainLedPin, false);
}


void setup() {
  // Initialize button and LED pins
  pinMode(mainBtnPin, INPUT_PULLUP);
  pinMode(mainLedPin, OUTPUT);
  for (int i = 0; i < playerCount; i++) {
    pinMode(playerBtnPins[i], INPUT_PULLUP);
    pinMode(playerLedPins[i], OUTPUT);
  }

  // Initialize matrix pins
  pinMode(dataPin, OUTPUT);
  pinMode(chipSelectPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  digitalWrite(chipSelectPin, HIGH);
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);

  randomSeed((uint32_t)esp_random());  // makes timings variable each reset
  Serial.begin(9600);
  Serial.println("> Waiting for user to start game...");
  Serial.print("Matrix initialized: ");
  Serial.print(matrixRows);
  Serial.print("x");
  Serial.println(matrixCols);

  currentState = WAITING_FOR_GAME;
  lastPressed = false;

  // Initialize player activation state (all OFF by default)
  for (int i = 0; i < playerCount; i++) {
    activePlayers[i] = false;
    lastPlayerRaw[i] = false;
    stablePlayerBtn[i] = false;
    lastPlayerDebounceTime[i] = 0;
    writeLed(playerLedPins[i], false);
  }

  // Initialize matrix
  initializeMatrix();
  enterWaitingState();
  renderWaitingPreview();
}

void loop() {
  bool currPressed = readButton(mainBtnPin);
  unsigned long nowMs = millis();

  switch (currentState) {
    case WAITING_FOR_GAME:
      for (int i = 0; i < playerCount; i++) {
        writeLed(playerLedPins[i], activePlayers[i]);
      }

      for (int p = 0; p < playerCount; p++) {
        bool raw = readButton(playerBtnPins[p]);
        if (raw != lastPlayerRaw[p]) {
          lastPlayerDebounceTime[p] = nowMs;
        }
        if (nowMs - lastPlayerDebounceTime[p] > playerDebounceMs) {
          if (raw != stablePlayerBtn[p]) {
            stablePlayerBtn[p] = raw;
            if (stablePlayerBtn[p]) {
              activePlayers[p] = !activePlayers[p];
              writeLed(playerLedPins[p], activePlayers[p]);
            }
          }
        }
        lastPlayerRaw[p] = raw;
      }

      renderWaitingPreview();

      if (currPressed && !lastPressed) {
        lastPressed = true;
        writeLed(mainLedPin, false);
      } else if (!currPressed && lastPressed) {
        if (countActivePlayers() == 0) {
          Serial.println("No players activated. Toggle player buttons to join.");
          lastPressed = false;
          break;
        }

        startGame();
      }
      break;

    case PLAYING:
      {
        bool playerPressed[playerCount];
        bool playerJustPressed[playerCount];

        for (int player = 0; player < playerCount; player++) {
          if (!activePlayers[player]) {
            playerPressed[player] = false;
            playerJustPressed[player] = false;
            lastGameplayBtn[player] = false;
          } else {
            playerPressed[player] = readButton(playerBtnPins[player]);
            playerJustPressed[player] = playerPressed[player] && !lastGameplayBtn[player];
            lastGameplayBtn[player] = playerPressed[player];
          }
        }

        if (currentGearRunning < totalGears
            && !shouldPress
            && nowMs - currentGearRunningTime >= (unsigned long)gearTimings[currentGearRunning]) {
          shouldPress = true;
          playerReactionTime = nowMs;

          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) continue;
            if (!playerStalled[player]) {
              playerStalled[player] = true;
              stallStartTimeMs[player] = playerReactionTime;
              stallReleaseTimeMs[player] = 0;
            }
          }
        }

        for (int player = 0; player < playerCount; player++) {
          if (!activePlayers[player]) continue;
          if (!shouldPress && playerJustPressed[player] && nowMs - currentGearRunningTime > debounceGracePeriodMs) {
            earlyPressCount[player]++;
          }
        }

        if (shouldPress) {
          if (nowMs - lastBlinkTimeMs > 30) {
            lastBlinkTimeMs = nowMs;
            powerMainLed = !powerMainLed;
            writeLed(mainLedPin, powerMainLed);
          }

          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) continue;
            if (playerJustPressed[player] && nowMs - playerReactionTime < jumpStartThresholdMs) {
              earlyPressCount[player]++;
            }
          }

          for (int player = 0; player < playerCount; player++) {
            if (!activePlayers[player]) continue;
            if (playerJustPressed[player] && playerTimes[player][currentGearRunning] == -1) {
              unsigned long reactionTimeMs = nowMs - playerReactionTime;
              unsigned long releaseTimeMs = playerReactionTime + reactionTimeMs + (earlyPressCount[player] * penaltyMs);
              playerTimes[player][currentGearRunning] = reactionTimeMs;
              if (stallReleaseTimeMs[player] == 0 || releaseTimeMs > stallReleaseTimeMs[player]) {
                stallReleaseTimeMs[player] = releaseTimeMs;
              }
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

          if (nowMs - playerReactionTime >= reactionWindowMs || allPlayersDone) {
            for (int player = 0; player < playerCount; player++) {
              if (!activePlayers[player]) continue;
              if (playerTimes[player][currentGearRunning] == -1) {
                playerTimes[player][currentGearRunning] = penaltyMs;
              }

              if (playerStalled[player] && stallReleaseTimeMs[player] == 0) {
                stallReleaseTimeMs[player] = playerReactionTime
                                           + playerTimes[player][currentGearRunning]
                                           + (earlyPressCount[player] * penaltyMs);
              } else if (playerStalled[player]) {
                unsigned long releaseTimeMs = playerReactionTime
                                            + playerTimes[player][currentGearRunning]
                                            + (earlyPressCount[player] * penaltyMs);
                if (releaseTimeMs > stallReleaseTimeMs[player]) {
                  stallReleaseTimeMs[player] = releaseTimeMs;
                }
              }

              if (stallReleaseTimeMs[player] != 0 && stallReleaseTimeMs[player] <= nowMs) {
                finalizePlayerStall(player, stallReleaseTimeMs[player]);
              }

              earlyPressCount[player] = 0;
            }

            writeLed(mainLedPin, true);
            powerMainLed = false;
            currentGearRunning++;
            currentGearRunningTime = nowMs;
            shouldPress = false;
          }
        }

        refreshPlayerColumns(nowMs);

        bool gameFinished = false;
        int winner = -1;
        for (int player = 0; player < playerCount; player++) {
          if (!activePlayers[player]) continue;
          if (playerColumn[player] >= racetrackFinish) {
            gameFinished = true;
            winner = player;
            break;
          }
        }

        if (gameFinished) {
          enterGameEndState(winner);
          Serial.print("Race finished! Winner: ");
          Serial.println(playerNames[winner]);
          for (int player = 0; player < playerCount; player++) {
            if (activePlayers[player]) {
              Serial.print(playerNames[player]);
              Serial.print(" - Column: ");
              Serial.println(playerColumn[player]);
            }
          }
        }

        renderRaceState();
        break;
      }

    case GAME_END:
      for (int i = 0; i < playerCount; i++) {
        writeLed(playerLedPins[i], false);
      }

      if (winnerPlayerIdx >= 0 && winnerPlayerIdx < playerCount && activePlayers[winnerPlayerIdx]) {
        writeLed(playerLedPins[winnerPlayerIdx], true);
      }

      renderGameEndState();

      if (currPressed && !lastPressed) {
        lastPressed = true;
      } else if (!currPressed && lastPressed) {
        enterWaitingState();
      }
      break;
  }
}
