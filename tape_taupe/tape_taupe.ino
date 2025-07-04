#include <stdio.h>

/* Pins de banchements :
* - Ruban leds : 3
* - Boutons : 4, 5, 6, 7, 8,    10, 11, 12
* - Ecran :
*  rétroéclairage 9
*  CLK A0
*  DIN A1
*  D/C A2
*  CE A3
*  RST 13
*/

/* ------------ Lvl 1 - couche matérielle ------------ */

// ----- 1.1 Leds

#include <FastLED.h>

// Constantes et variables liées au ruban de led
#define LED_PIN 3
#define NUM_LEDS 24
CRGB leds[NUM_LEDS];

void set_color(int btn_idx, CRGB color)
{
  leds[btn_idx * 3    ] = color;
  leds[btn_idx * 3 + 1] = color;
  leds[btn_idx * 3 + 2] = color;
  FastLED.show();
}

void set_color(int btn_idx, uint8_t r, uint8_t g, uint8_t b)
{
  set_color(btn_idx, CRGB(r, g, b));
}

void init_leds() {
  // Initiation du ruban de leds
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

// ----- 1.2 buttons

#include <PinChangeInterrupt.h>

// Constantes et variables liées aux boutons
#define NUM_BUTTONS 8
#define BTN_DELAY 300
uint8_t const buttonPins[NUM_BUTTONS] = {4, 5, 6, 7, 8,    10, 11, 12};
unsigned long last_push[NUM_BUTTONS] = {0};
bool btn_flags[NUM_BUTTONS] = {false};


/* Gestion des appuis boutons.
 * A chaque appui de bouton un flag est mis à true.
 * Un délai de rebons est ensuite appliqué pour éviter les appuismultiples.
 */
void buttonChange() {
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      unsigned long const current_time{millis()};
      if (current_time - last_push[i] > BTN_DELAY)
      {
        last_push[i] = current_time;
        btn_flags[i] = true;
      }
    }
  }
}

void init_buttons() {
  // Initialisation des callbacks de boutons
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(buttonPins[i]), buttonChange, FALLING);
  }
}

// ----- 1.3 screen

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define BACKLIGHT_PIN 9

// Brochage : CLK, DIN, D/C, CE, RST
Adafruit_PCD8544 display = Adafruit_PCD8544(A0, A1, A2, A3, 13);

#define CHAR_W 6
#define CHAR_H 8
#define SCRN_W 84
#define SCRN_H 48

void set_2digit_number(uint8_t const number)
{
  char buffer[3] = {
    '0' + ((number % 100) / 10),
    '0' + (number % 10),
    '\0'
  };

  display.clearDisplay();
  // Maximum de la taille écran
  display.setTextSize(6);
  // Centrage
  display.setCursor(8, 3);
  display.print(buffer);
  display.display();
}

void init_screen ()
{
  pinMode(BACKLIGHT_PIN, OUTPUT);
  analogWrite(BACKLIGHT_PIN, 64);

  display.begin();
  display.setContrast(10); // Ajuste selon ton écran
  display.clearDisplay();
  display.setTextSize(6);
  display.setTextColor(BLACK);
  display.display(); 
}


// ----- 1.4 Init printf (Bonus)

/* Redirection de printf vers serial */
// Fonction pour envoyer un caractère via Serial
int serial_putchar(char c, FILE* f) {
  Serial.write(c);
  return 0;
}
// Déclaration de la structure de flux
FILE serial_stdout;

void init_printf() {
  // Initialisation des prints
  Serial.begin(9600);

  // Redirection de printf sur Serial.
  fdev_setup_stream(&serial_stdout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &serial_stdout;

  delay(500);
}

// ----- 1 Init

void init_hardware() {
  init_printf();
  init_leds();
  init_buttons();
  init_screen();
  printf("Hardware initialisé\n");
}


/* ------------ Lvl 2 - couche jeu ------------ */

int score{0};
unsigned long started_at{0};

void init_game()
{
  printf("Init Game");
  // Compteur de points à 0
  score = 0;
  // Touches réinitialisées
  for (int btn{0} ; btn<NUM_BUTTONS ; btn++) {
    btn_flags[btn] = false;
    set_color(btn, CRGB::White);
  }

  // Attend uun premier appuis pour démarer le jeu
  bool no_push = true;
  do {
    delay(10);
    for (int btn{0} ; btn<NUM_BUTTONS ; btn++)
      no_push &= !btn_flags[btn];
  } while (no_push);
  // Enregistre le temps de départ pour laisser 1 minute de jeu
  started_at = millis();

  // Eteint tous les boutons
  for (int btn{0} ; btn<NUM_BUTTONS ; btn++) {
    btn_flags[btn] = false;
    set_color(btn, CRGB::Black);
  }

  // Initialise l'aléatoire en utilisant l'aléatoire du temps de démarrage
  randomSeed(started_at);
  init_random_btn();
}

int current_btn{NUM_BUTTONS/2};
void init_random_btn ()
{
  // Choisit aléatoirement un bouton différent du précédent
  int candidate{random(0, NUM_BUTTONS)};
  while (candidate == current_btn)
    candidate = random(0, NUM_BUTTONS);
  // Initialise le bouton
  current_btn = candidate;
  set_color(current_btn, CRGB::White);
  printf("Init random btn %d\n", current_btn);
}

bool game_stopped{false};
void stop_game() {
  game_stopped = true;
}

void game_loop()
{
  printf("Game loop\n");
  unsigned long display_sec{10000};
  unsigned long sec{millis() - started_at};

  // Initialise le premier bouton
  while((!game_stopped) && (sec < 60000))
  {
    // Si le bon bouton est pressé
    if (btn_flags[current_btn]) {
      score += 1;
      set_color(current_btn, CRGB::Black);
      init_random_btn();
    }
    // Réinitialisation des boutons
    for (int btn{0} ; btn<NUM_BUTTONS ; btn++) {
      btn_flags[btn] = false;
    }
    // Met à jour le timer
    sec = millis() - started_at;
    if (sec / 1000 != display_sec) {
      display_sec = sec / 1000;
      set_2digit_number(static_cast<uint8_t>(display_sec));
    }
  }
  printf("Fin de game loop\n");
}

void end_animation() {
  unsigned long start{millis()};
  while ((!game_stopped) && (millis() - start < 15000)) {
    int s = (millis() - start) / 1000;
    for (int led{0} ; led<NUM_LEDS ; led++)
      if ((s % 2) == 0)
        set_color(led, CRGB::Red);
      else
        set_color(led, CRGB::Black);
    delay(50);
  }
}


void debug_mode() {
  printf("DEBUG mode\n");

  // Wiring test
  for (int led{0} ; led<NUM_LEDS ; led++)
    leds[led] = CRGB::Yellow;
  FastLED.show();

  delay(5000);

  // Low level function test
  for (int btn{0} ; btn<NUM_BUTTONS ; btn++)
    set_color(btn, CRGB::Blue);

  delay(5000);
}


/* ------------ Lvl 3 - couche controleur ------------ */

void setup() {
  init_hardware();
  delay(500);
  debug_mode();
  init_game();
}


void loop() {
  // Jeu en cours
  game_loop();
  // Affichage du score
  set_2digit_number(score);
  // Animation de fin de partie
  end_animation();
  // Mode debug lancé
  if (game_stopped) {
    debug_mode();
    game_stopped = false;
  }
  // Initialisation d'un nouveau jeu
  init_game();
}
