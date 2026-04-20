// <>
const int mainBtnPin = 19;
const int blueBtnPin = 18;
const int redBtnPin = 5;
const int mainLedPin = 23;
const int blueLedPin = 25;
const int redLedPin = 26;

enum State { WAITING_FOR_GAME, PLAYING, GAME_END };
State currentState = WAITING_FOR_GAME;

int buttonPins[] = {mainBtnPin, blueBtnPin, redBtnPin};
int ledPins[] = {mainLedPin, blueLedPin, redLedPin};
int randomWaitTime = random(3000, 6001);
int msSinceStartOfGame = 0;
int lastWinner = -1; //* 0 for blue, 1 for red
bool isMainLedPowered = false;

bool readButton(int buttonPin) {
  return (digitalRead(buttonPin) == LOW); //* Button pressed: current LOW
}

void writeLed(int ledPin, bool value) {
  digitalWrite(ledPin, value ? HIGH : LOW);
}

bool lastPressed = readButton(mainBtnPin);

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }
  Serial.begin(9600);
  Serial.println("> Waiting for user to start game...");
}

void loop() {
  bool currPressed = readButton(mainBtnPin);
  Serial.println(currentState);

  switch (currentState) {
  case WAITING_FOR_GAME:
    if (currPressed && !lastPressed) { //? user just pressed the button
      lastPressed = currPressed;
      writeLed(mainLedPin, isMainLedPowered);
      writeLed(blueLedPin, false);
      writeLed(redLedPin, false);

    } else if (!currPressed && lastPressed) { //? user just let go of the button
      currentState = PLAYING;
      lastPressed = currPressed;
      writeLed(mainLedPin, true);
      delay(randomWaitTime);
      writeLed(mainLedPin, false);
      msSinceStartOfGame = millis();
    }
    break;
  case PLAYING: {
    bool bluePlayer = readButton(blueBtnPin);
    bool redPlayer = readButton(redBtnPin);

    //? blink main led after each iteration
    writeLed(mainLedPin, isMainLedPowered);
    isMainLedPowered = !isMainLedPowered;

    if (!bluePlayer && !redPlayer) //* early return
      break;

    if (bluePlayer) {
      lastWinner = 0;
    } else {
      lastWinner = 1;
    }
    currentState = GAME_END;
    break;
  }
  case GAME_END:
    isMainLedPowered = false;
    writeLed(mainLedPin, isMainLedPowered);
    writeLed(lastWinner ? redLedPin : blueLedPin, true);
    break;
    if (currPressed)
      currentState = WAITING_FOR_GAME;
  }
}
