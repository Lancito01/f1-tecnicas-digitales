// <>
const int mainBtnPin = 19;
const int blueBtnPin = 18;
const int redBtnPin = 5;
const int mainLedPin = 23;
const int blueLedPin = 25;
const int redLedPin = 26;

enum State { WAITING_FOR_GAME, PLAYING, GAME_END };

int buttonPins[] = {mainBtnPin, blueBtnPin, redBtnPin};
int ledPins[] = {mainLedPin, blueLedPin, redLedPin};
int msSinceStartOfGame = 0;
int lastWinner = -1; //* 0 for blue, 1 for red
bool powerMainLed = false;
bool lastPressed;
int lastBlinkTimeMs;
State currentState;

bool readButton(int buttonPin) {
  return (digitalRead(buttonPin) == LOW); //* Button pressed: current LOW
}

void writeLed(int ledPin, bool value) {
  digitalWrite(ledPin, value ? HIGH : LOW);
}

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }
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
    if (currPressed && !lastPressed) { //? user just pressed the button
      lastPressed = currPressed;
      writeLed(mainLedPin, false);

    } else if (!currPressed && lastPressed) { //? user just let go of the button
      currentState = PLAYING;
      lastPressed = currPressed;
      writeLed(mainLedPin, true);
      delay(random(1000, 4001));
      writeLed(mainLedPin, false);
      msSinceStartOfGame = millis();
      lastBlinkTimeMs = millis();
    }
    break;
  case PLAYING: {
    bool redPlayer = readButton(redBtnPin);
    bool bluePlayer = readButton(blueBtnPin);

    //? blink main led after each iteration
    if (millis() - lastBlinkTimeMs > 50) {
      lastBlinkTimeMs = millis();
      powerMainLed = !powerMainLed;
      writeLed(mainLedPin, powerMainLed);
    }

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
    writeLed(mainLedPin, false);
    writeLed(lastWinner ? redLedPin : blueLedPin, true);
    if (currPressed) {
      while (readButton(mainBtnPin))
        delay(1);
      currentState = WAITING_FOR_GAME;
      lastPressed = false;
    }
    break;
  }
}
