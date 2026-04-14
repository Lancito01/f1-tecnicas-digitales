// ============================================================
// F1 Multiplayer Launch Game (clean implementation)
// ============================================================
// Edit this pin map section to match your board wiring.
// Current values are placeholders.

const uint8_t MAX_PLAYERS = 4;
const uint8_t TRACK_LENGTH = 10;

const uint8_t BUTTON_PINS[MAX_PLAYERS] = {42, 43, 44, 45};
const uint8_t TRACK_PINS[MAX_PLAYERS][TRACK_LENGTH] = {
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {12, 13, 14, 15, 16, 17, 18, 19, 20, 21},
    {22, 23, 24, 25, 26, 27, 28, 29, 30, 31},
    {32, 33, 34, 35, 36, 37, 38, 39, 40, 41},
};

const uint8_t PROMPT_LED_PIN = 46;
const uint8_t START_LIGHT_PINS[] = {47, 48, 49, 50, 51};  // Optional
const uint8_t START_LIGHT_COUNT = sizeof(START_LIGHT_PINS) / sizeof(START_LIGHT_PINS[0]);

// ============================================================
// Game timing configuration

const unsigned long BASE_STEP_MS = 1000;
const unsigned long MAX_REACTION_MS = 750;
const unsigned long LOBBY_IDLE_TIMEOUT_MS = 3000;
const unsigned long BUTTON_DEBOUNCE_MS = 20;
const unsigned long START_LIGHT_STEP_MS = 300;
const unsigned long PRE_PROMPT_MIN_MS = 900;
const unsigned long PRE_PROMPT_MAX_MS = 2200;

const uint8_t MIN_EXTRA_PROMPTS = 3;
const uint8_t MAX_EXTRA_PROMPTS = 4;

// ============================================================

enum GameState {
  STATE_LOBBY,
  STATE_START_SEQUENCE,
  STATE_RACING,
  STATE_FINISHED,
};

struct PlayerState {
  bool active;
  bool finished;
  bool falseStartForPrompt;
  uint8_t position;
  unsigned long stepIntervalMs;
  unsigned long nextMoveAtMs;
};

GameState gameState = STATE_LOBBY;
PlayerState players[MAX_PLAYERS];

bool buttonRawPressed[MAX_PLAYERS];
bool buttonStablePressed[MAX_PLAYERS];
bool buttonPressEdge[MAX_PLAYERS];
unsigned long buttonLastChangeMs[MAX_PLAYERS];

uint8_t activePlayerCount = 0;
int winnerIndex = -1;

uint8_t promptPositions[MAX_EXTRA_PROMPTS];
uint8_t promptCount = 0;
uint8_t nextPromptIndex = 0;

// ============================================================
// Helpers

void clearTrackRow(uint8_t playerIndex) {
  for (uint8_t i = 0; i < TRACK_LENGTH; ++i) {
    digitalWrite(TRACK_PINS[playerIndex][i], LOW);
  }
}

void clearAllTracks() {
  for (uint8_t player = 0; player < MAX_PLAYERS; ++player) {
    clearTrackRow(player);
  }
}

void setPlayerLedPosition(uint8_t playerIndex, uint8_t position) {
  clearTrackRow(playerIndex);
  digitalWrite(TRACK_PINS[playerIndex][position], HIGH);
}

void turnOffStartLights() {
  for (uint8_t i = 0; i < START_LIGHT_COUNT; ++i) {
    digitalWrite(START_LIGHT_PINS[i], LOW);
  }
}

void resetRoundState() {
  activePlayerCount = 0;
  winnerIndex = -1;
  promptCount = 0;
  nextPromptIndex = 0;

  clearAllTracks();
  digitalWrite(PROMPT_LED_PIN, LOW);
  turnOffStartLights();

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    players[i].active = false;
    players[i].finished = false;
    players[i].falseStartForPrompt = false;
    players[i].position = 0;
    players[i].stepIntervalMs = BASE_STEP_MS + MAX_REACTION_MS;
    players[i].nextMoveAtMs = 0;
  }
}

void resetButtonEdges() {
  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    buttonPressEdge[i] = false;
  }
}

void pollButtons() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    bool rawPressed = (digitalRead(BUTTON_PINS[i]) == LOW);

    if (rawPressed != buttonRawPressed[i]) {
      buttonRawPressed[i] = rawPressed;
      buttonLastChangeMs[i] = now;
    }

    if ((now - buttonLastChangeMs[i]) >= BUTTON_DEBOUNCE_MS && rawPressed != buttonStablePressed[i]) {
      buttonStablePressed[i] = rawPressed;
      if (buttonStablePressed[i]) {
        buttonPressEdge[i] = true;
      }
    }
  }
}

bool consumePressEdge(uint8_t playerIndex) {
  if (!buttonPressEdge[playerIndex]) {
    return false;
  }
  buttonPressEdge[playerIndex] = false;
  return true;
}

bool allActiveButtonsReleased() {
  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    if (players[i].active && buttonStablePressed[i]) {
      return false;
    }
  }
  return true;
}

void movePlayerForward(uint8_t playerIndex) {
  if (players[playerIndex].position >= (TRACK_LENGTH - 1)) {
    return;
  }

  digitalWrite(TRACK_PINS[playerIndex][players[playerIndex].position], LOW);
  players[playerIndex].position++;
  digitalWrite(TRACK_PINS[playerIndex][players[playerIndex].position], HIGH);
}

uint8_t getLeaderPosition() {
  uint8_t leader = 0;
  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    if (players[i].active && !players[i].finished && players[i].position > leader) {
      leader = players[i].position;
    }
  }
  return leader;
}

void printPromptSchedule() {
  Serial.print(F("Extra prompts at positions: "));
  for (uint8_t i = 0; i < promptCount; ++i) {
    Serial.print(promptPositions[i]);
    if (i + 1 < promptCount) {
      Serial.print(F(", "));
    }
  }
  Serial.println();
}

void preparePromptSchedule() {
  promptCount = random(MIN_EXTRA_PROMPTS, MAX_EXTRA_PROMPTS + 1);  // 3 or 4
  nextPromptIndex = 0;

  const uint8_t candidateCount = TRACK_LENGTH - 2;  // 1..8 for TRACK_LENGTH=10
  uint8_t candidates[TRACK_LENGTH - 2];
  for (uint8_t i = 0; i < candidateCount; ++i) {
    candidates[i] = i + 1;
  }

  // Fisher-Yates shuffle
  for (int i = candidateCount - 1; i > 0; --i) {
    int j = random(0, i + 1);
    uint8_t tmp = candidates[i];
    candidates[i] = candidates[j];
    candidates[j] = tmp;
  }

  for (uint8_t i = 0; i < promptCount; ++i) {
    promptPositions[i] = candidates[i];
  }

  // Sort ascending
  for (uint8_t i = 0; i < promptCount; ++i) {
    for (uint8_t j = i + 1; j < promptCount; ++j) {
      if (promptPositions[j] < promptPositions[i]) {
        uint8_t tmp = promptPositions[i];
        promptPositions[i] = promptPositions[j];
        promptPositions[j] = tmp;
      }
    }
  }
}

void applySharedPrompt(const char* label) {
  Serial.print(F("Prompt: "));
  Serial.println(label);

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    players[i].falseStartForPrompt = false;
  }
  resetButtonEdges();

  // Pre-prompt interval. Pressing here counts as false start and gives max penalty.
  unsigned long anticipationMs = random(PRE_PROMPT_MIN_MS, PRE_PROMPT_MAX_MS + 1);
  unsigned long anticipationStart = millis();

  while ((millis() - anticipationStart) < anticipationMs) {
    pollButtons();
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
      if (players[i].active && !players[i].finished && consumePressEdge(i)) {
        players[i].falseStartForPrompt = true;
        Serial.print(F("Player "));
        Serial.print(i + 1);
        Serial.println(F(" false start -> 750 ms penalty"));
      }
    }
    delay(1);
  }

  unsigned long reactionMs[MAX_PLAYERS] = {0, 0, 0, 0};
  bool captured[MAX_PLAYERS] = {false, false, false, false};

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    if (!players[i].active || players[i].finished) {
      captured[i] = true;
      continue;
    }

    if (players[i].falseStartForPrompt) {
      captured[i] = true;
      reactionMs[i] = MAX_REACTION_MS;
    }
  }

  digitalWrite(PROMPT_LED_PIN, HIGH);  // Press when this turns ON
  unsigned long promptStart = millis();

  while ((millis() - promptStart) < MAX_REACTION_MS) {
    pollButtons();
    unsigned long elapsed = millis() - promptStart;

    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
      if (players[i].active && !players[i].finished && !captured[i] && consumePressEdge(i)) {
        captured[i] = true;
        reactionMs[i] = elapsed;
      }
    }
    delay(1);
  }

  digitalWrite(PROMPT_LED_PIN, LOW);

  unsigned long now = millis();
  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    if (!players[i].active || players[i].finished) {
      continue;
    }

    if (!captured[i]) {
      reactionMs[i] = MAX_REACTION_MS;
    }

    players[i].stepIntervalMs = BASE_STEP_MS + reactionMs[i];
    players[i].nextMoveAtMs = now + players[i].stepIntervalMs;

    Serial.print(F("Player "));
    Serial.print(i + 1);
    Serial.print(F(" reaction: "));
    Serial.print(reactionMs[i]);
    Serial.print(F(" ms, step interval: "));
    Serial.print(players[i].stepIntervalMs);
    Serial.println(F(" ms"));
  }

  resetButtonEdges();
}

// ============================================================
// States

void runLobbyState() {
  resetRoundState();
  Serial.println();
  Serial.println(F("=== Lobby ==="));
  Serial.println(F("Press your button to join (1-4 players)."));
  Serial.println(F("Race starts after 3 seconds with no new joins."));

  unsigned long lastActivity = millis();

  while (true) {
    pollButtons();

    bool joinHappened = false;
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
      if (consumePressEdge(i) && !players[i].active) {
        players[i].active = true;
        players[i].position = 0;
        setPlayerLedPosition(i, 0);
        activePlayerCount++;
        joinHappened = true;

        Serial.print(F("Player "));
        Serial.print(i + 1);
        Serial.println(F(" joined."));
      }
    }

    if (joinHappened) {
      lastActivity = millis();
    }

    if (activePlayerCount > 0 && (millis() - lastActivity) >= LOBBY_IDLE_TIMEOUT_MS) {
      Serial.print(F("Starting race with "));
      Serial.print(activePlayerCount);
      Serial.println(F(" player(s)."));
      gameState = STATE_START_SEQUENCE;
      return;
    }

    delay(1);
  }
}

void runStartSequenceState() {
  Serial.println();
  Serial.println(F("=== Start Sequence ==="));

  turnOffStartLights();
  for (uint8_t i = 0; i < START_LIGHT_COUNT; ++i) {
    digitalWrite(START_LIGHT_PINS[i], HIGH);
    delay(START_LIGHT_STEP_MS);
  }
  delay(500);
  turnOffStartLights();

  applySharedPrompt("LAUNCH");
  preparePromptSchedule();
  printPromptSchedule();

  gameState = STATE_RACING;
}

void runRaceState() {
  while (gameState == STATE_RACING) {
    pollButtons();
    unsigned long now = millis();

    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
      if (!players[i].active || players[i].finished) {
        continue;
      }

      while (now >= players[i].nextMoveAtMs && !players[i].finished) {
        movePlayerForward(i);

        if (players[i].position >= (TRACK_LENGTH - 1)) {
          players[i].finished = true;
          if (winnerIndex < 0) {
            winnerIndex = i;
          }
        } else {
          players[i].nextMoveAtMs += players[i].stepIntervalMs;
        }
      }
    }

    if (winnerIndex >= 0) {
      gameState = STATE_FINISHED;
      return;
    }

    uint8_t leaderPosition = getLeaderPosition();
    if (nextPromptIndex < promptCount && leaderPosition >= promptPositions[nextPromptIndex]) {
      Serial.print(F("Gear prompt "));
      Serial.print(nextPromptIndex + 1);
      Serial.println(F(" triggered."));
      applySharedPrompt("GEAR");
      nextPromptIndex++;
    }

    delay(1);
  }
}

void runFinishedState() {
  Serial.println();
  Serial.println(F("=== Race Finished ==="));
  if (winnerIndex >= 0) {
    Serial.print(F("Winner: Player "));
    Serial.println(winnerIndex + 1);
  } else {
    Serial.println(F("No winner (unexpected state)."));
  }

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    if (!players[i].active) {
      continue;
    }
    Serial.print(F("Player "));
    Serial.print(i + 1);
    Serial.print(F(" final position: "));
    Serial.println(players[i].position);
  }

  Serial.println(F("Press any active player's button for a new race."));

  resetButtonEdges();
  while (!allActiveButtonsReleased()) {
    pollButtons();
    delay(1);
  }

  while (true) {
    pollButtons();
    for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
      if (players[i].active && consumePressEdge(i)) {
        gameState = STATE_LOBBY;
        return;
      }
    }
    delay(1);
  }
}

// ============================================================

void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  for (uint8_t player = 0; player < MAX_PLAYERS; ++player) {
    for (uint8_t i = 0; i < TRACK_LENGTH; ++i) {
      pinMode(TRACK_PINS[player][i], OUTPUT);
      digitalWrite(TRACK_PINS[player][i], LOW);
    }
  }

  pinMode(PROMPT_LED_PIN, OUTPUT);
  digitalWrite(PROMPT_LED_PIN, LOW);

  for (uint8_t i = 0; i < START_LIGHT_COUNT; ++i) {
    pinMode(START_LIGHT_PINS[i], OUTPUT);
    digitalWrite(START_LIGHT_PINS[i], LOW);
  }

  for (uint8_t i = 0; i < MAX_PLAYERS; ++i) {
    bool pressed = (digitalRead(BUTTON_PINS[i]) == LOW);
    buttonRawPressed[i] = pressed;
    buttonStablePressed[i] = pressed;
    buttonPressEdge[i] = false;
    buttonLastChangeMs[i] = millis();
  }

  resetRoundState();
}

void loop() {
  switch (gameState) {
    case STATE_LOBBY:
      runLobbyState();
      break;
    case STATE_START_SEQUENCE:
      runStartSequenceState();
      break;
    case STATE_RACING:
      runRaceState();
      break;
    case STATE_FINISHED:
      runFinishedState();
      break;
  }
}
