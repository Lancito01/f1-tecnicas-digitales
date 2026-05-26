// Apr 28 sketch: beginner-friendly test for an 8x8 MAX7219 matrix.
//
// Wiring:
// VCC -> VIN
// GND -> GND
// DIN -> D27
// CS  -> D14
// CLK -> D12

const int dataPin = 27;
const int chipSelectPin = 14;
const int clockPin = 12;

const int matrixSize = 8;
const unsigned long stepDelayMs = 120;

// This table lets us build each row without using bit-shift syntax.
const int powerOfTwo[matrixSize] = {1, 2, 4, 8, 16, 32, 64, 128};

int pixels[matrixSize][matrixSize];

void setAllPixelsOff() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      pixels[row][col] = 0;
    }
  }
}

void setPixel(int row, int col, int state) {
  pixels[row][col] = state;
}

void sendByte(int value) {
  // Send one 8-bit value to the MAX7219, one bit at a time.
  int bitMask = 128;

  for (int bit = 0; bit < 8; bit++) {
    digitalWrite(clockPin, LOW);

    if (value >= bitMask) {
      digitalWrite(dataPin, HIGH);
      value = value - bitMask;
    } else {
      digitalWrite(dataPin, LOW);
    }

    digitalWrite(clockPin, HIGH);
    bitMask = bitMask / 2;
  }
}

void writeRegister(int registerNumber, int value) {
  digitalWrite(chipSelectPin, LOW);
  sendByte(registerNumber);
  sendByte(value);
  digitalWrite(chipSelectPin, HIGH);
}

/*
This function translates binary numbers to int as follows:
Each row has 8 lights, in binary, for example:
[1,0,0,1,0,1,1,0]
This function translates this number in binary 10010110 to int
so it can then be written into each row using renderMatrix()
*/
int rowToByte(int row) {
  int rowValue = 0;

  for (int col = 0; col < matrixSize; col++) {
    if (pixels[row][col] == 1) {
      rowValue = rowValue + powerOfTwo[col];
    }
  }

  return rowValue;
}

void renderMatrix() { // matrix's rows are not 0-based index
  for (int row = 0; row < matrixSize; row++) {
    writeRegister(row + 1, rowToByte(row)); // function to translate binary to int
  }
}

void showOnePixel(int row, int col) {
  setAllPixelsOff();
  setPixel(row, col, 1);
  renderMatrix();
}

void initializeMatrix() {
  // MAX7219 setup: no test mode, no digit decoding, use all 8 rows, normal operation.
  writeRegister(0x0F, 0x00);
  writeRegister(0x09, 0x00);
  writeRegister(0x0B, 0x07);
  writeRegister(0x0A, 0x08);
  writeRegister(0x0C, 0x01);

  setAllPixelsOff();
  renderMatrix();
}

void sweepColumnUp(int col) {
  for (int row = matrixSize - 1; row >= 0; row--) {
    showOnePixel(row, col);
    delay(stepDelayMs);
  }
}

void sweepColumnDown(int col) {
  for (int row = 0; row < matrixSize; row++) {
    showOnePixel(row, col);
    delay(stepDelayMs);
  }
}

void setup() {
  pinMode(dataPin, OUTPUT);
  pinMode(chipSelectPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  digitalWrite(chipSelectPin, HIGH);
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);

  Serial.begin(115200);
  Serial.println("MAX7219 matrix test starting...");

  initializeMatrix();
}

void loop() {
  for (int col = 0; col < matrixSize; col++) {
    Serial.print("Column ");
    Serial.print(col + 1);

    if (col % 2 == 0) {
      Serial.println(" upward");
      sweepColumnUp(col);
    } else {
      Serial.println(" downward");
      sweepColumnDown(col);
    }
  }
}
