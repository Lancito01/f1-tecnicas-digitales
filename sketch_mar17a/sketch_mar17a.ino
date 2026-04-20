// <>
const int mainBtnPin = 19;
const int blueBtnPin = 18;
const int redBtnPin = 5;
const int mainLedPin = 23;
const int blueLedPin = 25;
const int redLedPin = 26;

int buttonPins[] = {mainBtnPin, blueBtnPin, redBtnPin};
int ledPins[] = {mainLedPin, blueLedPin, redLedPin};

enum State { WAITING_FOR_GAME, PLAYING, GAME_END };
State currentState = WAITING_FOR_GAME;

void setup() {
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(ledPins[i], OUTPUT);
  }
  Serial.begin(9600);
  Serial.println("> Waiting for user to start game...");
}

bool readButton(int buttonPin) {
  return (digitalRead(buttonPin) == LOW); //* Button pressed: current LOW
}

void writeLed(int ledPin, bool value) {
  digitalWrite(ledPin, value ? HIGH : LOW);
}

void loop() {
  switch (currentState) {
  case WAITING_FOR_GAME:
    for (int i = 0; i < 3; i++) {
      writeLed(ledPins[i], readButton(buttonPins[i]));
    }
    break;
  case PLAYING:
    break;
  }
}
