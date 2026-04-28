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

void setup() {
  pinMode(mainBtnPin, INPUT_PULLUP);
  pinMode(redBtnPin, INPUT_PULLUP);
  pinMode(blueBtnPin, INPUT_PULLUP);
  pinMode(greenBtnPin, INPUT_PULLUP);
  pinMode(yellowBtnPin, INPUT_PULLUP);

  pinMode(mainLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
}

void loop() {
  digitalWrite(mainLedPin, digitalRead(mainBtnPin) == LOW ? HIGH : LOW);
  digitalWrite(redLedPin, digitalRead(redBtnPin) == LOW ? HIGH : LOW);
  digitalWrite(blueLedPin, digitalRead(blueBtnPin) == LOW ? HIGH : LOW);
  digitalWrite(greenLedPin, digitalRead(greenBtnPin) == LOW ? HIGH : LOW);
  digitalWrite(yellowLedPin, digitalRead(yellowBtnPin) == LOW ? HIGH : LOW);
}
