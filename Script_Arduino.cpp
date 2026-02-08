/*
 * PROIECT: Consola de Jocuri Retro pe Arduino
 * DESCRIERE: Sistem multi-game (Curse Auto & Brick Breaker) folosind ecran OLED I2C.
 * HARDWARE: Arduino Uno/Nano, Display OLED SSD1306, Modul Joystick.
 * AUTOR: Budoi Mihai-Alexandru
 * DATA: 2025
 */

#include <U8g2lib.h>
#include <Wire.h>
#include <avr/pgmspace.h> 

// ========================================================================
// 1. CONFIGURARE HARDWARE
// ========================================================================

// Mapare Pini Joystick si Butoane
# define JOY_X A0           // Axa X (Stanga-Dreapta)
# define JOY_Y A1           // Axa Y (Sus-Jos - folosit in meniu)
# define JOY_BUTTON_B 3     // Buton Confirmare / Start
# define JOY_BUTTON_D 4     // Buton Functii Speciale (Cheat / God Mode)
# define E 6                // Buton Iesire / Meniu Principal

// Initializare Driver Ecran (Rezolutie 128x32 pixeli)
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ========================================================================
// 2. GESTIUNEA STARILOR (STATE MACHINE)
// ========================================================================

const int STATE_WELCOME = -1;    // Ecran de bun venit
const int STATE_MAIN_MENU = 0;   // Meniul de selectie joc
const int STATE_CAR_RACE = 1;    // Joc 1: Curse
const int STATE_BRICK_BREAKER = 2; // Joc 2: Sparge Caramizi

int globalState = STATE_WELCOME; 
int subState = 0; // 0=Menu Start Joc, 1=In Joc, 2=Game Over, 3=Victorie
int menuSelection = 0; // Pozitia cursorului in meniu

// ========================================================================
// 3. VARIABILE SI CONSTANTE JOCURI
// ========================================================================

const int DISPLAY_WIDTH = 128; 
const int DISPLAY_HEIGHT = 32; 

// --- Configurare Joc 1: CAR RACE ---
const int NUMBER_OF_LANES = 3; 
const int ROAD_WIDTH = 90; 
const int LANE_WIDTH = ROAD_WIDTH / NUMBER_OF_LANES; 
// Centrare drum pe ecran
const int ROAD_START_X = (DISPLAY_WIDTH - ROAD_WIDTH) / 2; 
const int ROAD_LEFT_MARGIN = ROAD_START_X; 
const int ROAD_RIGHT_MARGIN = ROAD_START_X + ROAD_WIDTH; 
const int START_LANE_2 = ROAD_LEFT_MARGIN + LANE_WIDTH; 
const int LANE_DESIGN_WIDTH = 2;

// Identificatori Tipuri Vehicule (Actualizat: CAR -> VEHICLE)
const int TYPE_PLAYER = 1; 
const int TYPE_VEHICLE_1  = 2; 
const int TYPE_VEHICLE_2  = 3; 
const int TYPE_VEHICLE_3  = 4; 
const int TYPE_VEHICLE_4  = 5; 
const int TYPE_VEHICLE_5  = 6; // Motocicleta
const int TYPE_VEHICLE_6  = 7; 

// Proprietati Vehicul Jucator (Actualizat)
const int PLAYER_VEHICLE_WIDTH = 6; 
const int PLAYER_VEHICLE_HEIGHT = 8; 
const int PLAYER_VEHICLE_Y = DISPLAY_HEIGHT - PLAYER_VEHICLE_HEIGHT - 1; 
int vehicleX; // Pozitia X a jucatorului (fostul carX)

// Sistem Obstacole (NPCs)
const int MAX_OBSTACLES = 5; 
struct Obstacle { 
  int x;
  int y;
  bool active; 
  int lane; 
  int vehicleType; 
};
Obstacle obstacles[MAX_OBSTACLES]; 

// Parametri Coliziune si Viteza
const int MAX_COLLISION_HEIGHT = 8; 
const int MAX_COLLISION_WIDTH = 8; 
const int LANE_LENGTH = 4; // Lungimea marcajului discontinuu
int laneOffset = 0;        // Variabila pentru animatia drumului
int carRaceScore = 0; 
float gameSpeed = 2.0; 
long lastObstacleTime = 0; 
const long OBSTACLE_INTERVAL = 700; // Milisecunde intre obstacole
bool godModeActive = false; 

// --- Configurare Joc 2: BRICK BREAKER ---
const int PADDLE_WIDTH = 15; 
const int PADDLE_HEIGHT = 2; 
const int BALL_SPEED_INIT = 1; 
const int BALL_SIZE = 2; 

int paddleX; 
int ballX, ballY; 
int ballSpeedX, ballSpeedY; 
int brickBreakerScore = 0; 

// Generare Grid Caramizi
const int BRICK_WIDTH = 10; 
const int BRICK_HEIGHT = 4; 
const int BRICK_COLUMNS = 10; 
const int BRICK_ROWS = 2; 
const int BRICK_GAP = 2; 
const int TOTAL_BLOCK_WIDTH = (BRICK_COLUMNS * BRICK_WIDTH) + ((BRICK_COLUMNS - 1) * BRICK_GAP);
const int BRICK_START_X = (DISPLAY_WIDTH - TOTAL_BLOCK_WIDTH) / 2;

const int TOTAL_BRICKS = BRICK_COLUMNS * BRICK_ROWS; 
bool bricks[TOTAL_BRICKS]; 
int brickCount; 

// Variabile Debounce Butoane
bool buttonReleased = false; 
bool buttonEReleased = false;
bool buttonDReleased = false; 


// ========================================================================
// 4. GRAFICA (SPRITES) - STOCATE IN MEMORIA FLASH (PROGMEM)
// ========================================================================

// (Actualizat: car_matrix -> vehicle_matrix)

const byte player_vehicle_matrix[8][6] PROGMEM = {
    {0, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 0, 0},
    {0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 0, 0},
    {1, 1, 1, 1, 1, 1},
};

const int VEHICLE1_W = 6; const int VEHICLE1_H = 8;
const byte vehicle1_matrix[8][6] PROGMEM = {
    {0, 1, 1, 1, 1, 0}, 
    {1, 1, 0, 0, 1, 1}, 
    {0, 1, 1, 1, 1, 0}, 
    {0, 1, 0, 0, 1, 0}, 
    {0, 1, 1, 1, 1, 0}, 
    {0, 1, 0, 0, 1, 0}, 
    {0, 1, 1, 1, 1, 0}, 
    {1, 1, 1, 1, 1, 1}  
};

const int VEHICLE2_W = 6; const int VEHICLE2_H = 8;
const byte vehicle2_matrix[8][6] PROGMEM = {
    {1, 1, 0, 0, 1, 1}, 
    {1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 0}, 
    {0, 1, 0, 0, 1, 0}, 
    {0, 1, 1, 1, 1, 0}, 
    {0, 1, 0, 0, 1, 0}, 
    {0, 1, 1, 1, 1, 0}, 
    {0, 0, 1, 1, 0, 0}  
};

const int VEHICLE3_W = 7; const int VEHICLE3_H = 8;
const byte vehicle3_matrix[8][7] PROGMEM = {
    {1, 0, 1, 1, 1, 0, 1}, 
    {1, 1, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 1, 1, 1, 1}, 
    {0, 1, 1, 1, 1, 1, 0}, 
    {0, 1, 0, 0, 0, 1, 0}, 
    {0, 1, 1, 1, 1, 1, 0}, 
    {1, 1, 1, 1, 1, 1, 1}, 
    {1, 0, 1, 1, 1, 0, 1}  
};

const int VEHICLE4_W = 6; const int VEHICLE4_H = 8;
const byte vehicle4_matrix[8][6] PROGMEM = {
    {1, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 1, 1, 1}, 
    {1, 0, 0, 0, 0, 1}, 
    {1, 1, 1, 1, 1, 1}  
};

const int VEHICLE5_W = 5; const int VEHICLE5_H = 8;
const byte vehicle5_matrix[8][5] PROGMEM = {
    {0, 1, 1, 1, 0}, 
    {0, 1, 1, 1, 0}, 
    {1, 1, 1, 1, 1}, 
    {0, 1, 1, 1, 0}, 
    {0, 1, 1, 1, 0}, 
    {0, 0, 1, 0, 0}, 
    {0, 1, 1, 1, 0}, 
    {0, 1, 1, 1, 0}  
};

const int VEHICLE6_W = 7; const int VEHICLE6_H = 8;
const byte vehicle6_matrix[8][7] PROGMEM = {
    {0, 1, 1, 1, 1, 1, 0}, 
    {1, 1, 0, 0, 0, 1, 1}, 
    {0, 1, 1, 1, 1, 1, 0}, 
    {0, 0, 1, 1, 1, 0, 0}, 
    {0, 0, 1, 0, 1, 0, 0}, 
    {0, 1, 1, 1, 1, 1, 0}, 
    {1, 1, 0, 0, 0, 1, 1}, 
    {0, 1, 1, 1, 1, 1, 0}  
};

// ========================================================================
// 5. MOTOR GRAFIC (Randare)
// ========================================================================

// Functii ajutatoare pentru desenarea fiecarui tip de vehicul (Actualizat: drawVehicle)
void drawPlayerVehicle(int x, int y) { 
    for (int r = 0; r < PLAYER_VEHICLE_HEIGHT; r++) {
        for (int c = 0; c < PLAYER_VEHICLE_WIDTH; c++) {
            if (pgm_read_byte(&player_vehicle_matrix[r][c]) == 1) u8g2.drawPixel(x + c, y + r);
        }
    }
}
void drawVehicle1(int x, int y) { for(int r=0;r<VEHICLE1_H;r++) for(int c=0;c<VEHICLE1_W;c++) if(pgm_read_byte(&vehicle1_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }
void drawVehicle2(int x, int y) { for(int r=0;r<VEHICLE2_H;r++) for(int c=0;c<VEHICLE2_W;c++) if(pgm_read_byte(&vehicle2_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }
void drawVehicle3(int x, int y) { for(int r=0;r<VEHICLE3_H;r++) for(int c=0;c<VEHICLE3_W;c++) if(pgm_read_byte(&vehicle3_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }
void drawVehicle4(int x, int y) { for(int r=0;r<VEHICLE4_H;r++) for(int c=0;c<VEHICLE4_W;c++) if(pgm_read_byte(&vehicle4_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }
void drawVehicle5(int x, int y) { for(int r=0;r<VEHICLE5_H;r++) for(int c=0;c<VEHICLE5_W;c++) if(pgm_read_byte(&vehicle5_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }
void drawVehicle6(int x, int y) { for(int r=0;r<VEHICLE6_H;r++) for(int c=0;c<VEHICLE6_W;c++) if(pgm_read_byte(&vehicle6_matrix[r][c])) u8g2.drawPixel(x+c, y+r); }

// Selector general pentru desenare vehicule
void drawVehicle(int x, int y, int type) { 
    switch (type) {
        case TYPE_PLAYER:    drawPlayerVehicle(x, y); break; 
        case TYPE_VEHICLE_1: drawVehicle1(x, y); break; 
        case TYPE_VEHICLE_2: drawVehicle2(x, y); break;
        case TYPE_VEHICLE_3: drawVehicle3(x, y); break;
        case TYPE_VEHICLE_4: drawVehicle4(x, y); break;
        case TYPE_VEHICLE_5: drawVehicle5(x, y); break; // Deseneaza motocicleta
        case TYPE_VEHICLE_6: drawVehicle6(x, y); break;
        default: break;
    }
}

// --- ECRANE INTERFATA (UI) ---

void drawWelcomeScreen() { 
    char *title = "ARCADE GAME";
    char *msg = "PRESS B"; 
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf); 
        int title_width = u8g2.getStrWidth(title);
        u8g2.drawStr((DISPLAY_WIDTH - title_width) / 2, 10, title);
        int msg_width = u8g2.getStrWidth(msg);
        u8g2.drawStr((DISPLAY_WIDTH - msg_width) / 2, 25, msg);
    } while (u8g2.nextPage());
}

void drawGameStartMenu() { 
    char *title = (globalState == STATE_CAR_RACE) ? "RACE START" : "BRICK START";
    char *start_msg = "PRESS B TO START";
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf); 
        int title_width = u8g2.getStrWidth(title);
        u8g2.drawStr((DISPLAY_WIDTH - title_width) / 2, 10, title);
        int start_width = u8g2.getStrWidth(start_msg);
        u8g2.drawStr((DISPLAY_WIDTH - start_width) / 2, 25, start_msg);
    } while (u8g2.nextPage());
}

void drawGameOver() { 
    char *title = "GAME OVER";
    char *reset_msg = "RESET (B)";
    int final_score = (globalState == STATE_CAR_RACE) ? carRaceScore : brickBreakerScore;
    char scoreBuffer[15];
    sprintf(scoreBuffer, "SCORE: %d", final_score);
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf);
        int title_width = u8g2.getStrWidth(title);
        u8g2.drawStr((DISPLAY_WIDTH - title_width) / 2, 10, title);
        u8g2.drawStr(5, 25, scoreBuffer); 
        u8g2.drawStr(DISPLAY_WIDTH - u8g2.getStrWidth(reset_msg) - 3, 25, reset_msg); 
    } while (u8g2.nextPage());
}

void drawVictoryScreen() { 
    char *title = "CONGRATS!";
    char *msg1 = "YOU WON!";
    char *reset_msg = "RESET (B)";
    char scoreBuffer[15];
    sprintf(scoreBuffer, "SCORE: %d", brickBreakerScore);
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.drawStr((DISPLAY_WIDTH - u8g2.getStrWidth(title)) / 2, 8, title);
        u8g2.drawStr((DISPLAY_WIDTH - u8g2.getStrWidth(msg1)) / 2, 18, msg1); 
        u8g2.drawStr(5, 30, scoreBuffer); 
        u8g2.drawStr(DISPLAY_WIDTH - u8g2.getStrWidth(reset_msg) - 3, 30, reset_msg); 
    } while (u8g2.nextPage());
}

// --- MENIU PRINCIPAL ---

void navigateMainMenu() { 
    int valJoyY = analogRead(JOY_Y);
    // Navigare simpla sus-jos
    if (valJoyY < 400 && menuSelection < 1) { menuSelection++; delay(200); } 
    else if (valJoyY > 600 && menuSelection > 0) { menuSelection--; delay(200); }
}

void drawMainMenu() { 
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(5, 10, "CHOOSE GAME:"); 
        u8g2.drawStr(15, 20, "1. CAR RACE");
        u8g2.drawStr(15, 30, "2. BRICK BREAKER");
        // Cursorul este un patrat mic in dreptul selectiei
        u8g2.drawBox(5, 19 + menuSelection * 10, 5, 2);
    } while (u8g2.nextPage());
}

// Functie centrala pentru citirea butoanelor si schimbarea starilor
void checkButtonSelection() { 
    int readingB = digitalRead(JOY_BUTTON_B);
    if (readingB == HIGH) { buttonReleased = true; }
    
    // Logica Buton B (Selectie / Reset)
    if (buttonReleased && readingB == LOW) { 
        if (globalState == STATE_WELCOME) {
            globalState = STATE_MAIN_MENU;
        }
        else if (globalState == STATE_MAIN_MENU) {
            globalState = (menuSelection == 0) ? STATE_CAR_RACE : STATE_BRICK_BREAKER;
            subState = 0; 
        } 
        else if (subState == 0 || subState == 2 || subState == 3) {
            if (globalState == STATE_CAR_RACE) resetCarRace();
            else if (globalState == STATE_BRICK_BREAKER) resetBrickBreaker();
        }
        buttonReleased = false;
    }
    
    // Logica Buton E (Iesire in Meniu)
    int readingE = digitalRead(E);
    if (readingE == HIGH) { buttonEReleased = true; }

    if (buttonEReleased && readingE == LOW && globalState != STATE_MAIN_MENU) {
        globalState = STATE_MAIN_MENU;
        menuSelection = 0; 
        subState = 0;
        buttonEReleased = false;
    }

    // Logica Buton D (Cheat-uri)
    int readingD = digitalRead(JOY_BUTTON_D);
    if (readingD == HIGH) { buttonDReleased = true; }

    if (buttonDReleased && readingD == LOW) {
        if (globalState == STATE_BRICK_BREAKER && subState == 1) activateCheat();
        else if (globalState == STATE_CAR_RACE && subState == 1) godModeActive = !godModeActive; 
        buttonDReleased = false;
    }
}

// ========================================================================
// 6. LOGICA JOC: CAR RACE
// ========================================================================

void resetCarRace() { 
    carRaceScore = 0;
    // Pozitionare initiala pe banda din mijloc (Actualizat vehicleX)
    vehicleX = (START_LANE_2 + LANE_WIDTH / 2) - PLAYER_VEHICLE_WIDTH / 2;
    for (int i = 0; i < MAX_OBSTACLES; i++) { obstacles[i].active = false; }
    lastObstacleTime = millis();
    gameSpeed = 2.0;
    godModeActive = false; 
    subState = 1; // Start Joc
}

void readInput() { 
    int valJoyX = analogRead(JOY_X);
    // Mapare valoare joystick (0-1023) la latimea drumului
    int xMin = ROAD_LEFT_MARGIN;
    int xMax = ROAD_RIGHT_MARGIN - PLAYER_VEHICLE_WIDTH; 
    vehicleX = map(valJoyX, 0, 1023, xMin, xMax);
}

void drawCarRace() { 
    u8g2.firstPage();
    do {
        // 1. Desenare Drum si Marcaje (cu efect de miscare)
        for (int i = 0; i <= DISPLAY_HEIGHT / (LANE_LENGTH * 2); i++) {
            int yPos = (i * LANE_LENGTH * 2) + (int)laneOffset - LANE_LENGTH * 2;
            u8g2.drawVLine(ROAD_LEFT_MARGIN, 0, DISPLAY_HEIGHT);
            u8g2.drawVLine(ROAD_RIGHT_MARGIN, 0, DISPLAY_HEIGHT);
            u8g2.drawBox(ROAD_LEFT_MARGIN + LANE_WIDTH, yPos, LANE_DESIGN_WIDTH, LANE_LENGTH);
            u8g2.drawBox(ROAD_LEFT_MARGIN + LANE_WIDTH * 2, yPos, LANE_DESIGN_WIDTH, LANE_LENGTH);
        }
        // 2. Desenare Vehicul Jucator (Actualizat)
        drawVehicle(vehicleX, PLAYER_VEHICLE_Y, TYPE_PLAYER);
        if (godModeActive) { u8g2.setFont(u8g2_font_5x7_tf); u8g2.drawStr(3, 10, "GOD"); }
        
        // 3. Desenare Obstacole Active
        for (int i = 0; i < MAX_OBSTACLES; i++) {
            if (obstacles[i].active) {
                drawVehicle(obstacles[i].x, obstacles[i].y, obstacles[i].vehicleType);
            }
        }
        // 4. Desenare Scor
        char scoreBuffer[10]; sprintf(scoreBuffer, "S:%d", carRaceScore); 
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(DISPLAY_WIDTH - u8g2.getStrWidth(scoreBuffer) - 3, 10, scoreBuffer); 
    } while (u8g2.nextPage());
}

void updateCarRace() { 
  // Animatie drum
  laneOffset += (int)gameSpeed;
  if (laneOffset >= LANE_LENGTH * 2) { laneOffset = 0; }

  // Generare obstacole noi
  if (millis() - lastObstacleTime > OBSTACLE_INTERVAL) {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].active) {
          obstacles[i].active = true;
          obstacles[i].y = -10; 
          obstacles[i].lane = random(1, NUMBER_OF_LANES + 1); 
          int laneStart = ROAD_LEFT_MARGIN + (obstacles[i].lane - 1) * LANE_WIDTH;
          // Randomizare vehicul (Actualizat intervalul pentru TYPE_VEHICLE)
          obstacles[i].vehicleType = random(TYPE_VEHICLE_1, TYPE_VEHICLE_6 + 1); 
          obstacles[i].x = random(laneStart, laneStart + LANE_WIDTH - MAX_COLLISION_WIDTH + 4); 
          lastObstacleTime = millis();
          break;
        }
    }
  }

  // Actualizare pozitii si Verificare Coliziuni
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].active) {
      obstacles[i].y += (int)gameSpeed;
      
      // Algoritm de coliziune (Bounding Box)
      // Actualizat cu variabilele vehicleX si PLAYER_VEHICLE
      if (vehicleX < obstacles[i].x + MAX_COLLISION_WIDTH &&
          vehicleX + PLAYER_VEHICLE_WIDTH > obstacles[i].x &&
          PLAYER_VEHICLE_Y < obstacles[i].y + MAX_COLLISION_HEIGHT &&
          PLAYER_VEHICLE_Y + PLAYER_VEHICLE_HEIGHT > obstacles[i].y) {
        
        if (!godModeActive) { subState = 2; return; } // Game Over
      }
      
      // Depasire obstacol si crestere scor
      if (obstacles[i].y >= DISPLAY_HEIGHT) {
        obstacles[i].active = false;
        carRaceScore++;
        if (carRaceScore % 10 == 0 && carRaceScore > 0) gameSpeed += 0.5; // Crestere dificultate
      }
    }
  }
}

// ========================================================================
// 7. LOGICA JOC: BRICK BREAKER
// ========================================================================

void activateCheat() { 
    // Distruge automat toate caramizile (pentru demonstratie)
    for (int i = 0; i < TOTAL_BRICKS; i++) {
        if (bricks[i]) {
            bricks[i] = false;
            brickBreakerScore += 10;
            brickCount--;
        }
    }
    subState = 3; // Victorie instantanee
}

void resetBrickBreaker() { 
    brickBreakerScore = 0;
    paddleX = (DISPLAY_WIDTH - PADDLE_WIDTH) / 2;
    ballX = DISPLAY_WIDTH / 2;
    ballY = DISPLAY_HEIGHT - PADDLE_HEIGHT - BALL_SIZE - 1;
    ballSpeedX = BALL_SPEED_INIT;
    ballSpeedY = -BALL_SPEED_INIT;
    // Resetare Grid Caramizi
    for (int i = 0; i < TOTAL_BRICKS; i++) bricks[i] = true;
    brickCount = TOTAL_BRICKS;
    subState = 1; 
}

void readBrickBreakerInput() { 
    int valJoyX = analogRead(JOY_X);
    paddleX = map(valJoyX, 0, 1023, 0, DISPLAY_WIDTH - PADDLE_WIDTH);
}

void updateBrickBreaker() { 
    if (subState != 1) return;
    ballX += ballSpeedX;
    ballY += ballSpeedY;

    // Coliziune pereti laterali
    if (ballX <= 0 || ballX >= DISPLAY_WIDTH - BALL_SIZE) ballSpeedX = -ballSpeedX;
    // Coliziune tavan
    if (ballY <= 0) ballSpeedY = -ballSpeedY;

    // Coliziune Paleta
    if (ballY + BALL_SIZE >= DISPLAY_HEIGHT - PADDLE_HEIGHT &&
        ballX + BALL_SIZE > paddleX &&
        ballX < paddleX + PADDLE_WIDTH &&
        ballSpeedY > 0) { 
        
        ballSpeedY = -abs(ballSpeedY);
        // Efect: schimba unghiul in functie de unde loveste paleta
        int paddleCenter = paddleX + PADDLE_WIDTH / 2;
        int deltaX = ballX + BALL_SIZE / 2 - paddleCenter;
        ballSpeedX = constrain(deltaX / 3, -2, 2);
    }

    // Minge cazuta jos (Game Over)
    if (ballY >= DISPLAY_HEIGHT) subState = 2;
    
    // Coliziune Caramizi
    for (int i = 0; i < TOTAL_BRICKS; i++) {
        if (bricks[i]) {
            int r = i / BRICK_COLUMNS;
            int c = i % BRICK_COLUMNS; 
            int brickX = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP);
            int brickY = 3 + r * (BRICK_HEIGHT + BRICK_GAP);
            
            // Verificare suprapunere minge-caramida
            if (ballX < brickX + BRICK_WIDTH && ballX + BALL_SIZE > brickX &&
                ballY < brickY + BRICK_HEIGHT && ballY + BALL_SIZE > brickY) {
                
                bricks[i] = false;
                brickBreakerScore += 10;
                brickCount--;

                // Logica simplificata de ricoseu
                if (ballX + BALL_SIZE / 2 < brickX || ballX + BALL_SIZE / 2 > brickX + BRICK_WIDTH) {
                    ballSpeedX = -ballSpeedX;
                } else {
                    ballSpeedY = -ballSpeedY;
                }
                
                if (brickCount <= 0) subState = 3; // Victorie
                return;
            }
        }
    }
}

void drawBrickBreaker() { 
    u8g2.firstPage();
    do {
        u8g2.drawBox(paddleX, DISPLAY_HEIGHT - PADDLE_HEIGHT, PADDLE_WIDTH, PADDLE_HEIGHT);
        u8g2.drawBox(ballX, ballY, BALL_SIZE, BALL_SIZE);
        // Desenare grid
        for (int i = 0; i < TOTAL_BRICKS; i++) {
            if (bricks[i]) {
                int r = i / BRICK_COLUMNS;
                int c = i % BRICK_COLUMNS;
                int brickX = BRICK_START_X + c * (BRICK_WIDTH + BRICK_GAP);
                int brickY = 3 + r * (BRICK_HEIGHT + BRICK_GAP); 
                u8g2.drawBox(brickX, brickY, BRICK_WIDTH, BRICK_HEIGHT);
            }
        }
        char scoreBuffer[10]; sprintf(scoreBuffer, "S:%d", brickBreakerScore);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(DISPLAY_WIDTH - u8g2.getStrWidth(scoreBuffer) - 3, 30, scoreBuffer);
    } while (u8g2.nextPage());
}

// ========================================================================
// 8. SETUP & MAIN LOOP
// ========================================================================

void setup() {
    u8g2.begin(); // Initializare ecran
    Serial.begin(9600);
    // Initializare generator numere aleatoare (seed pe pin flotant)
    randomSeed(analogRead(A2)); 
    pinMode(JOY_BUTTON_B, INPUT_PULLUP); 
    pinMode(E, INPUT_PULLUP); 
    pinMode(JOY_BUTTON_D, INPUT_PULLUP); 
    
    paddleX = (DISPLAY_WIDTH - PADDLE_WIDTH) / 2;
}

void loop() {
    // Verificare input utilizator
    checkButtonSelection();
    
    // Selectare logica in functie de Starea Globala
    if (globalState == STATE_WELCOME) {
        drawWelcomeScreen();
    }
    else if (globalState == STATE_MAIN_MENU) {
        navigateMainMenu();
        drawMainMenu();
        delay(100);
    } 
    else if (globalState == STATE_CAR_RACE) {
        if (subState == 0) drawGameStartMenu();
        else if (subState == 1) { readInput(); updateCarRace(); drawCarRace(); delay(50); }
        else if (subState == 2) drawGameOver();
        else delay(100);
    }
    else if (globalState == STATE_BRICK_BREAKER) {
        if (subState == 0) drawGameStartMenu();
        else if (subState == 1) { readBrickBreakerInput(); updateBrickBreaker(); drawBrickBreaker(); delay(50); }
        else if (subState == 2) drawGameOver();
        else if (subState == 3) drawVictoryScreen();
        else delay(100); 
    }
}