#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PADDLE_WIDTH 16
#define PADDLE_HEIGHT 4
#define BALL_SIZE 2
#define BRICK_WIDTH 16
#define BRICK_HEIGHT 8
#define BRICKS_PER_ROW 8
#define NUM_ROWS 4
#define MAX_LEVEL 10

struct Brick {
  bool active = true;
};

struct PowerUp {
  int x, y;
  bool active = false;
};

Brick bricks[NUM_ROWS * BRICKS_PER_ROW];
int paddleX, paddleY = SCREEN_HEIGHT - 10;
int ballX, ballY;
int ballDirX = 2, ballDirY = -2;
int score = 0;
int health = 3;
bool gameActive = false;
const int ldrPin = A1;
PowerUp powerUp;
int level = 1;

int digits[10][7] = {
  {0, 0, 0, 0, 0, 0, 1}, // 0
  {1, 0, 0, 1, 1, 1, 1}, // 1
  {0, 0, 1, 0, 0, 1, 0}, // 2
  {0, 0, 0, 0, 1, 1, 0}, // 3
  {1, 0, 0, 1, 1, 0, 0}, // 4
  {0, 1, 0, 0, 1, 0, 0}, // 5
  {0, 1, 0, 0, 0, 0, 0}, // 6
  {0, 0, 0, 1, 1, 1, 1}, // 7
  {0, 0, 0, 0, 0, 0, 0}, // 8
  {0, 0, 0, 0, 1, 0, 0}  // 9
};

int segmentPins[7] = {2, 3, 4, 5, 6, 7, 8};

// Button pins
const int buttonUp = 10;
const int buttonDown = 11;
const int buttonSelect = 12;
const int potPin = A0;
int menuIndex = 0;

void setup() {
  Serial.begin(9600);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);
  pinMode(potPin, INPUT);

  pinMode(17, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);

  // OLED display initialization
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }

  for (int i = 0; i < 7; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }

  for (int pin = 2; pin <= 9; pin++) {
    pinMode(pin, OUTPUT);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.display();

  initGame();
}

void loop() {
  int sensorValue = analogRead(ldrPin);

  display.invertDisplay(sensorValue > 500);

  if (gameActive == false){
    if (health == 0) {
      endScreen();
    }
    handleMenu();
    health = 3;
  }
  else if (!digitalRead(buttonSelect)) {
    if (menuIndex == 0) {
      gameActive = true;
    } else {
      executeAction(menuIndex);
    }
    delay(200);
  }
  else {
    checkAllBricksGone();
    runGame();
    updateDisplay();
  }


}

void endScreen() {
  display.clearDisplay();

  String message = "Oyuncunun Skoru: " + String(score);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.display();

  delay(3000);

  gameActive = false;
  score = 0;
  level = 0;
  initGame();
}


void setupBricks() {
    randomSeed(analogRead(0));
    for (int i = 0; i < NUM_ROWS * BRICKS_PER_ROW; i++) {
        if (random(100) < 50) {
            bricks[i].active = true;
        } else {
            bricks[i].active = false;
        }
    }
}

void resetPaddleAndBall() {
  paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;

  ballX = paddleX + PADDLE_WIDTH / 2;
  ballY = paddleY - BALL_SIZE;
}


void handleMenu() {
  display.clearDisplay();
  display.setCursor(0,0);

  if (!digitalRead(buttonUp)) {
    menuIndex = 0;
    delay(200);
  }
  if (!digitalRead(buttonDown)) {
    menuIndex = 1;
    delay(200);
  }
  if (!digitalRead(buttonSelect)) {
    if (menuIndex == 0) {
      gameActive = true;
    } else {
      executeAction(menuIndex);
    }
    delay(200);
  }

  // Menu display
  if (menuIndex == 0) {
    display.println("> Baslat");
    display.println("  Cikis");
  } else {
    display.println("  Baslat");
    display.println("> Cikis");
  }

  display.display();
}

void executeAction(int index) {
  display.clearDisplay();
  display.setCursor(0, 0);
  if (index == 1) {
    display.println("Oyunumuza gosterdiginiz ilgi icin tesekkurler.");
    display.display();
    delay(2000);
    gameActive = false;
  }
}

void runGame() {
  display.clearDisplay();
  readInputs();
  moveBall();
  movePowerUp();
  checkCollisions();
  ledControl(health);
  updateDisplay();
  if (!digitalRead(buttonSelect)) {
    gameActive = false;
    initGame();
    //delay(200);
  }
}

void initGame() {
  resetPaddleAndBall();


  for (int i = 0; i < NUM_ROWS * BRICKS_PER_ROW; i++) {
    bricks[i].active = true;
  }

}


void readInputs() {
  int sensorValue = analogRead(potPin);
  paddleX = map(sensorValue, 0, 1023, 0, SCREEN_WIDTH - PADDLE_WIDTH);
}


void moveBall() {
  ballX += ballDirX;
  ballY += ballDirY;
  if (ballX <= 0 || ballX >= SCREEN_WIDTH - BALL_SIZE) ballDirX = -ballDirX;
  if (ballY <= 0) ballDirY = -ballDirY;
  if (ballY >= SCREEN_HEIGHT) {
    health--;
    if (health == 0){
      gameActive = false;
      initGame();
    } else {
      resetPaddleAndBall();
    }
  }
}

void displayDigit(int digit) {
  int number = score % 10;
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], (digits[number][i]));
  }
}

void checkCollisions() {
  if (ballY >= paddleY - BALL_SIZE && ballX >= paddleX && ballX <= paddleX + PADDLE_WIDTH) {
    ballDirY = -ballDirY;
  }

  for (int i = 0; i < NUM_ROWS * BRICKS_PER_ROW; i++) {
    if (bricks[i].active && ballX > (i % BRICKS_PER_ROW) * BRICK_WIDTH && ballX < (i % BRICKS_PER_ROW + 1) * BRICK_WIDTH && ballY > (i / BRICKS_PER_ROW) * BRICK_HEIGHT && ballY < ((i / BRICKS_PER_ROW + 1) * BRICK_HEIGHT)) {
      bricks[i].active = false;
      ballDirY = -ballDirY;
      score++;
      Serial.println(score);
      displayDigit(score % 9);

      if (random(100) < 10) {
        powerUp.x = (i % BRICKS_PER_ROW) * BRICK_WIDTH + BRICK_WIDTH / 2;
        powerUp.y = (i / BRICKS_PER_ROW) * BRICK_HEIGHT;
        powerUp.active = true;
      }
      break;
    }
  }
}

void updateDisplay() {
  display.clearDisplay();

  for (int row = 0; row < NUM_ROWS; row++) {
    for (int col = 0; col < BRICKS_PER_ROW; col++) {
      int brickX = col * BRICK_WIDTH;
      int brickY = row * BRICK_HEIGHT;
      if (bricks[row * BRICKS_PER_ROW + col].active) {
        display.fillRect(brickX, brickY, BRICK_WIDTH - 1, BRICK_HEIGHT - 1, WHITE);
      }
    }
  }


  display.fillRect(paddleX, paddleY, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);

  display.fillCircle(ballX, ballY, BALL_SIZE, WHITE);

  if (powerUp.active) {
    int baseHalf = 3;
    display.fillTriangle(powerUp.x, powerUp.y - 4, powerUp.x - baseHalf, powerUp.y + 2, powerUp.x + baseHalf, powerUp.y + 2, WHITE);
  }

  display.setCursor(0, SCREEN_HEIGHT - 10);
  display.print("Score: ");
  display.print(score);

  display.display();
}


void ledControl(int health){
  int pins[3] = {17, 18, 19};
  for (int i = 0; i < 3; i++) {
    digitalWrite(pins[i], LOW);
  }

  for (int i = 0; i < health; i++) {
    digitalWrite(pins[2 - i], HIGH);
  }

  for (int i = health; i < 3; i++) {
    digitalWrite(pins[2 - i], LOW);
  }
}


void movePowerUp() {
  if (powerUp.active) {
    powerUp.y += 2;
    if (powerUp.y >= SCREEN_HEIGHT) {
      powerUp.active = false;
    } else if (powerUp.y >= paddleY && powerUp.x >= paddleX && powerUp.x <= paddleX + PADDLE_WIDTH) { // Paddle ile çarpışma kontrolü
      health++;
      ledControl(health);
      powerUp.active = false;
    }
  }
}

void checkAllBricksGone() {
  bool allBricksGone = true;
  for (int i = 0; i < NUM_ROWS * BRICKS_PER_ROW; i++) {
    if (bricks[i].active) {
      allBricksGone = false;
      break;
    }
  }

  if (allBricksGone) {
    level++;
    if(level > MAX_LEVEL) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("You Win! Game Over");
      display.display();
      delay(5000);
      gameActive = false;
      level = 1;
      initGame();
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Level ");
      display.print(level - 1);
      display.println(" Complete! Next Level -> ");
      display.println(level);
      display.display();
      delay(3000);

      setupBricks();
      resetBall();
      ballDirX *= 1.2;
      ballDirY *= 1.2;
      gameActive = true;
    }
  }
}



void resetBall() {
    ballX = paddleX + PADDLE_WIDTH / 2;
    ballY = paddleY - BALL_SIZE;
    ballDirX = 2;
    ballDirY = -2;
}
