// ================= CONFIG =================

#define NUM_PLAYERS 4
#define TRACK_LENGTH 10   // LEDs por carril
#define GEARS 4

// Pines botones
int buttonPins[NUM_PLAYERS] = {32, 33, 25, 26};

// Pines LEDs de cada carril
int trackPins[NUM_PLAYERS][TRACK_LENGTH] = {
  {2,4,5,18,19,21,22,23,13,12},
  {14,27,26,25,33,32,15,16,17,18},
  {19,21,22,23,13,12,14,27,26,25},
  {4,5,18,19,21,22,23,13,12,14}
};

// LEDs de largada (5 luces)
int startLights[5] = {15, 16, 17, 18, 19};

// ================= VARIABLES =================

bool activePlayer[NUM_PLAYERS];
int position[NUM_PLAYERS];
int gear[NUM_PLAYERS];
unsigned long lastShiftTime[NUM_PLAYERS];

bool raceStarted = false;
bool falseStart[NUM_PLAYERS];

// Ventana ideal de cambio (ms)
const int PERFECT_WINDOW = 150;
const int PENALTY_TIME = 500;

// ================= SETUP =================

void setup() {
  Serial.begin(115200);

  for(int i=0;i<NUM_PLAYERS;i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  for(int p=0;p<NUM_PLAYERS;p++){
    for(int i=0;i<TRACK_LENGTH;i++){
      pinMode(trackPins[p][i], OUTPUT);
    }
  }

  for(int i=0;i<5;i++){
    pinMode(startLights[i], OUTPUT);
  }

  resetGame();
}

// ================= LOOP =================

void loop() {
  selectPlayers();
  startSequence();
  raceLoop();
}

// ================= FUNCIONES =================

// Reset general
void resetGame(){
  for(int i=0;i<NUM_PLAYERS;i++){
    activePlayer[i] = false;
    position[i] = 0;
    gear[i] = 1;
    falseStart[i] = false;
  }
}

// ================= SELECCIÓN =================

void selectPlayers(){
  Serial.println("Seleccionando jugadores...");

  while(true){
    for(int i=0;i<NUM_PLAYERS;i++){
      if(digitalRead(buttonPins[i]) == LOW){
        activePlayer[i] = true;
        digitalWrite(trackPins[i][0], HIGH);
      }
    }

    // Si al menos uno juega y pasan 3 segundos sin cambios → continuar
    static unsigned long lastPress = millis();

    if(millis() - lastPress > 3000){
      break;
    }
  }
}

// ================= LARGADA =================

void startSequence(){
  Serial.println("Secuencia de largada...");

  // Encender luces progresivamente
  for(int i=0;i<5;i++){
    digitalWrite(startLights[i], HIGH);
    delay(500);
  }

  // Tiempo aleatorio
  int delayTime = random(200, 5000);
  unsigned long startWait = millis();

  // Detectar falsa largada
  while(millis() - startWait < delayTime){
    for(int i=0;i<NUM_PLAYERS;i++){
      if(activePlayer[i] && digitalRead(buttonPins[i]) == LOW){
        falseStart[i] = true;
        Serial.print("Falsa largada jugador ");
        Serial.println(i);
      }
    }
  }

  // Apagar luces → GO
  for(int i=0;i<5;i++){
    digitalWrite(startLights[i], LOW);
  }

  raceStarted = true;

  for(int i=0;i<NUM_PLAYERS;i++){
    lastShiftTime[i] = millis();
  }
}

// ================= CARRERA =================

void raceLoop(){
  while(raceStarted){

    for(int i=0;i<NUM_PLAYERS;i++){

      if(!activePlayer[i]) continue;

      // Penalización por falsa largada
      if(falseStart[i]){
        delay(1000);
        falseStart[i] = false;
      }

      // Movimiento automático base
      moveForward(i);

      // Sistema de cambios
      if(digitalRead(buttonPins[i]) == LOW){
        handleShift(i);
      }

      // Ganador
      if(position[i] >= TRACK_LENGTH-1){
        Serial.print("GANO JUGADOR ");
        Serial.println(i);
        raceStarted = false;
      }
    }

    delay(200);
  }
}

// ================= MOVIMIENTO =================

void moveForward(int player){

  if(position[player] < TRACK_LENGTH-1){
    digitalWrite(trackPins[player][position[player]], LOW);
    position[player]++;
    digitalWrite(trackPins[player][position[player]], HIGH);
  }
}

// ================= CAMBIOS =================

void handleShift(int player){

  unsigned long now = millis();
  int diff = now - lastShiftTime[player];

  // Simula timing ideal
  if(diff < PERFECT_WINDOW){
    // Muy temprano → penaliza
    Serial.println("Muy temprano!");
    delay(PENALTY_TIME);
  }
  else if(diff > PERFECT_WINDOW * 3){
    // Muy tarde → penaliza
    Serial.println("Muy tarde!");
    delay(PENALTY_TIME);
  }
  else{
    // Perfecto → boost
    Serial.println("Perfect shift!");
    boostPlayer(player);
  }

  // Subir marcha
  if(gear[player] < GEARS){
    gear[player]++;
  }

  lastShiftTime[player] = now;
}

// ================= BOOST =================

void boostPlayer(int player){

  for(int i=0;i<2;i++){
    moveForward(player);
  }
}
