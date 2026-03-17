#include <Preferences.h>

Preferences preferences;

const int maxPlayers = 5;

int startLED[5] = {13, 12, 14, 27, 26};
int buttonPin = 32;
int buzzer = 5;

unsigned long playerTimes[maxPlayers];
int order[maxPlayers];

int numPlayers = 1;

unsigned long startTime;

// -------------------- SETUP --------------------

void setup()
{

    Serial.begin(115200);

    for (int i = 0; i < 5; i++)
    {
        pinMode(startLED[i], OUTPUT);
    }

    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(buzzer, OUTPUT);

    randomSeed(analogRead(0));
}

// -------------------- ESPERAR BOTON --------------------

void waitForButton()
{

    while (digitalRead(buttonPin) == HIGH)
        ;

    delay(300);
}

// -------------------- SELECT PLAYERS --------------------

int selectPlayers()
{

    int players = 1;

    unsigned long lastPress = millis();

    Serial.println("Seleccionar jugadores (1-5)");

    while (true)
    {

        if (digitalRead(buttonPin) == LOW)
        {

            delay(250);

            players++;

            if (players > 5)
                players = 1;

            Serial.print("Jugadores: ");
            Serial.println(players);

            lastPress = millis();
        }

        if (millis() - lastPress > 3000)
        {

            break;
        }
    }

    return players;
}

// -------------------- SECUENCIA DE LUCES --------------------

void startSequence()
{

    for (int i = 0; i < 5; i++)
    {

        digitalWrite(startLED[i], HIGH);
        tone(buzzer, 1000, 80);

        delay(600);
    }
}

// -------------------- ESPERA ALEATORIA --------------------

bool randomWait()
{

    long waitTime = random(500, 5000);

    unsigned long start = millis();

    while (millis() - start < waitTime)
    {

        if (digitalRead(buttonPin) == LOW)
        {

            return true;
        }
    }

    return false;
}

// -------------------- APAGADO DE LUCES --------------------

void lightsOut()
{

    for (int i = 0; i < 5; i++)
    {

        digitalWrite(startLED[i], LOW);
    }

    tone(buzzer, 2000, 150);

    startTime = micros();
}

// -------------------- TURNO DE JUGADOR --------------------

unsigned long playTurn()
{

    startSequence();

    bool falseStart = randomWait();

    if (falseStart)
    {

        Serial.println("FALSE START!");

        delay(1500);

        return 999999999;
    }

    lightsOut();

    while (digitalRead(buttonPin) == HIGH)
        ;

    unsigned long reaction = micros() - startTime;

    return reaction;
}

// -------------------- ORDENAR LEADERBOARD --------------------

void sortLeaderboard()
{

    for (int i = 0; i < numPlayers; i++)
    {
        order[i] = i;
    }

    for (int i = 0; i < numPlayers - 1; i++)
    {

        for (int j = i + 1; j < numPlayers; j++)
        {

            if (playerTimes[order[j]] < playerTimes[order[i]])
            {

                int temp = order[i];
                order[i] = order[j];
                order[j] = temp;
            }
        }
    }
}

// -------------------- MOSTRAR GANADOR --------------------

void showWinner(int winner)
{

    Serial.println("----- RESULTADOS -----");

    Serial.print("GANADOR: Jugador ");
    Serial.println(winner + 1);

    if (playerTimes[winner] != 999999999)
    {

        Serial.print("Tiempo: ");
        Serial.print(playerTimes[winner] / 1000.0);
        Serial.println(" ms");
    }
}

// -------------------- LEADERBOARD --------------------

void showLeaderboard()
{

    sortLeaderboard();

    Serial.println("===== LEADERBOARD =====");

    for (int i = 0; i < numPlayers; i++)
    {

        int p = order[i];

        Serial.print(i + 1);
        Serial.print(") Jugador ");
        Serial.print(p + 1);
        Serial.print("  ");

        if (playerTimes[p] == 999999999)
        {

            Serial.println("FALSE START");
        }
        else
        {

            Serial.print(playerTimes[p] / 1000.0);
            Serial.println(" ms");
        }

        delay(1200);
    }
}

// -------------------- CALCULAR GANADOR --------------------

int findWinner()
{

    unsigned long best = 999999999;
    int winner = -1;

    for (int i = 0; i < numPlayers; i++)
    {

        if (playerTimes[i] < best)
        {

            best = playerTimes[i];
            winner = i;
        }
    }

    return winner;
}

// -------------------- NEW GAME --------------------

void newGame()
{

    Serial.println("===== NEW GAME =====");

    numPlayers = selectPlayers();

    Serial.print("Jugadores confirmados: ");
    Serial.println(numPlayers);
}

// -------------------- LOOP PRINCIPAL --------------------

void loop()
{

    newGame();

    for (int i = 0; i < numPlayers; i++)
    {

        Serial.print("Turno Jugador ");
        Serial.println(i + 1);

        playerTimes[i] = playTurn();

        delay(2000);
    }

    int winner = findWinner();

    showWinner(winner);

    delay(2000);

    showLeaderboard();

    Serial.println("Presione boton para nuevo juego");

    waitForButton();
}
