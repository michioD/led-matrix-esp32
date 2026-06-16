#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <esp_wifi.h>

#define WIFI_SSID "IZZI-A94E-5G"
#define WIFI_PASSWORD "FZYWCJNWZNML"

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic = "kth_matrix/my_secret_id/anim";

WiFiClient espClient;
PubSubClient client(espClient);
WiFiMulti wifiMulti;

#define DATA_PIN    13
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define MATRIX_W    16
#define MATRIX_H    16
#define NUM_LEDS    256

CRGB leds[NUM_LEDS];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, MATRIX_W, MATRIX_H, 
  NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + 
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);

enum AnimationState { ANIM_OFF, ANIM_PACMAN, ANIM_KISS, ANIM_HEART, ANIM_CHIIKAWA, ANIM_BDAY };

AnimationState currentState = ANIM_BDAY;

unsigned long previousMillis = 0;
int frame = 0;
int cursorX = 16;

const uint8_t ghost_bmp[] PROGMEM = {
  0b00111100, 0b01111110, 0b11011011, 0b11111111,
  0b11111111, 0b11111111, 0b11111111, 0b10100101
};

const uint8_t pacman_open[] PROGMEM = {
  0b00111100, 0b01111110, 0b11011111, 0b11111111,
  0b11100000, 0b11100000, 0b01111110, 0b00111100
};

const uint8_t pacman_closed[] PROGMEM = {
  0b00111100, 0b01111110, 0b11011111, 0b11111111,
  0b11111111, 0b11111111, 0b01111110, 0b00111100
};

const uint8_t person_left[] PROGMEM = {
  0b00111000, 0b01111100, 0b01111000, 0b01111100,
  0b00111000, 0b00111000, 0b01111100, 0b11111110
};

const uint8_t person_right[] PROGMEM = {
  0b00011100, 0b00111110, 0b00011110, 0b00111110,
  0b00011100, 0b00011100, 0b00111110, 0b01111111
};

const uint8_t heart_bmp[] PROGMEM = {
  0b00000000, 0b01100110, 0b11111111, 0b11111111,
  0b01111110, 0b00111100, 0b00011000, 0b00000000
};

const uint8_t big_heart[] PROGMEM = {
  0b00000000, 0b00000000, // Row 0
  0b00000000, 0b00000000, // Row 1
  0b00000000, 0b00000000, // Row 2
  0b00000000, 0b00000000, // Row 3
  0b00000000, 0b00000000, // Row 4
  0b00111000, 0b00011100, // Row 5
  0b01111100, 0b00111110, // Row 6
  0b01111110, 0b01111110, // Row 7
  0b01111111, 0b11111110, // Row 8
  0b01111111, 0b11111110, // Row 9
  0b00111111, 0b11111100, // Row 10
  0b00011111, 0b11111000, // Row 11
  0b00001111, 0b11110000, // Row 12
  0b00000111, 0b11100000, // Row 13
  0b00000011, 0b11000000, // Row 14
  0b00000001, 0b10000000  // Row 15
};

const uint8_t chiikawa_bmp[] PROGMEM = {
  0x00, 0x00, 0x30, 0x0c, 0x78, 0x1e, 0x7f, 0xfe,
  0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7,
  0xff, 0xff, 0xfa, 0x5f, 0xfd, 0xbf, 0x7f, 0xfe,
  0x3f, 0xfc, 0x1f, 0xf8, 0x18, 0x18, 0x00, 0x00
};

const uint8_t hachiware[] PROGMEM = {
    0b00000000, 0b00000000, // Row 0
    0b00000000, 0b00000000, // Row 1
    0b00010000, 0b00010000, // Row 2
    0b00011000, 0b00110000, // Row 3
    0b00011111, 0b11110000, // Row 4
    0b00111111, 0b11111000, // Row 5
    0b01110111, 0b11011100, // Row 6
    0b01111111, 0b11111100, // Row 7
    0b11110111, 0b11011110, // Row 8
    0b11101011, 0b10101110, // Row 9
    0b11100011, 0b10001110, // Row 10
    0b11110110, 0b11011110, // Row 11
    0b11111100, 0b01111110, // Row 12
    0b01111111, 0b11111100, // Row 13
    0b01111110, 0b11111100, // Row 14
    0b00011111, 0b11100000  // Row 15
};

void callback(char* topic, byte* message, unsigned int length) {
  String incoming = "";
  for (int i = 0; i < length; i++) {
    incoming += (char)message[i];
  }
  
  incoming.trim();
  incoming.toLowerCase();
  
  if (incoming == "pacman") {
    currentState = ANIM_PACMAN;
    cursorX = 16; 
  } else if (incoming == "kiss") {
    currentState = ANIM_KISS;
    frame = 0; 
  } else if (incoming == "heart") {
    currentState = ANIM_HEART;
  } else if (incoming == "chiikawa") {
    currentState = ANIM_CHIIKAWA;
  } else if (incoming == "bday") { 
    currentState = ANIM_BDAY;
    cursorX = 16; 
  } else if (incoming == "off") {
    currentState = ANIM_OFF;
  } else if (incoming == "stats") {
    String telemetry = "Free RAM: " + String(ESP.getFreeHeap() / 1024) + " KB | " +
                       "Free OTA Flash: " + String(ESP.getFreeSketchSpace() / 1024) + " KB";
    client.publish("kth_matrix/my_secret_id/stats", telemetry.c_str());
  }
}

void reconnect() {
  while (!client.connected()) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe(mqtt_topic);
    } else {
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println("\nInitializing Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(1000);

  wifi_country_t wifi_country = {.cc="MX", .schan=1, .nchan=11, .policy=WIFI_COUNTRY_POLICY_AUTO};
  esp_wifi_set_country(&wifi_country);
  
  WiFi.setSleep(false);
  
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Attempting to connect to registered APs ");
  
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- BOOTING ---");
  
  setup_wifi();
  
  ArduinoOTA.setHostname("esp32-matrix"); 
  ArduinoOTA.begin();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); 
  matrix->begin();
  
  matrix->setTextWrap(false); 
  matrix->setBrightness(15); 
  
  Serial.println("Matrix initialization complete. Starting animation loop.");
}

void renderBday() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  
  matrix->setTextColor(matrix->Color(255, 20, 147)); 
  
  matrix->setCursor(cursorX, 4); 
  matrix->print("happy birthday mi amor");
  
  matrix->show();
  
  cursorX--;
  
  if (cursorX < -135) {
    cursorX = 16; 
  }
}

void renderChiikawa() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  int yOffset = beatsin8(30, 0, 2); 
  
  uint16_t colorBody = matrix->Color(255, 255, 255); 
  uint16_t colorCheeks = matrix->Color(255, 100, 150); 
  
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int byteIndex = (y * 2) + (x / 8);
      uint8_t b = pgm_read_byte(&(chiikawa_bmp[byteIndex]));
      if (b & (1 << (7 - (x % 8)))) {
        matrix->drawPixel(x, y + yOffset, colorBody);
      }
    }
  }
  
  matrix->drawPixel(2, 8 + yOffset, colorCheeks);
  matrix->drawPixel(3, 8 + yOffset, colorCheeks);
  matrix->drawPixel(12, 8 + yOffset, colorCheeks);
  matrix->drawPixel(13, 8 + yOffset, colorCheeks);

  matrix->show(); 
}

void renderHachiware() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  int yOffset = beatsin8(30, 0, 2); 
  
  uint16_t colorBody = matrix->Color(255, 255, 255); 
  uint16_t colorCheeks = matrix->Color(255, 100, 150); 
  
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int byteIndex = (y * 2) + (x / 8);
      uint8_t b = pgm_read_byte(&(hachiware[byteIndex]));
      if (b & (1 << (7 - (x % 8)))) {
        matrix->drawPixel(x, y + yOffset, colorBody);
      }
    }
  }
  matrix->drawPixel(2, 8 + yOffset, colorCheeks);
  matrix->drawPixel(3, 8 + yOffset, colorCheeks);
  matrix->drawPixel(12, 8 + yOffset, colorCheeks);
  matrix->drawPixel(13, 8 + yOffset, colorCheeks);
  matrix->show(); 
}

void renderPacman() {
  matrix->fillScreen(matrix->Color(0, 0, 0));

  uint16_t colorGhost = matrix->Color(0, 255, 255);
  uint16_t colorPacman = matrix->Color(255, 255, 0);
  // Ghost
  matrix->drawBitmap(cursorX, 4, ghost_bmp, 8, 8, colorGhost);

  // Pac-Man behind the ghost (moving left -> right)
  if ((cursorX % 4) < 2) {
    matrix->drawBitmap(cursorX - 10, 4, pacman_open, 8, 8, colorPacman);
  } else {
    matrix->drawBitmap(cursorX - 10, 4, pacman_closed, 8, 8, colorPacman);
  }
  matrix->show();
  cursorX++;  // move right
  if (cursorX >= 26) {  // 16px display + 10px spacing
    cursorX = -10;
  }
}

void renderKiss() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  uint16_t colorPerson1 = matrix->Color(0, 150, 255); 
  uint16_t colorPerson2 = matrix->Color(255, 0, 150); 
  uint16_t colorHeart   = matrix->Color(255, 0, 0);   
  
  int leftX, rightX;
  bool showHeart = false;
  
  if (frame < 5) {
    leftX = -4 + frame;        
    rightX = 12 - frame;      
  } else if (frame >= 5 && frame < 15) {
    leftX = 0;
    rightX = 8;
    showHeart = (frame % 2 == 0); 
  } else {
    leftX = 0 - (frame - 15);
    rightX = 8 + (frame - 15);
  }
  
  matrix->drawBitmap(leftX, 8, person_left, 8, 8, colorPerson1);
  matrix->drawBitmap(rightX, 8, person_right, 8, 8, colorPerson2);
  
  if (showHeart) {
    matrix->drawBitmap(4, 0, heart_bmp, 8, 8, colorHeart);
  }
  
  matrix->show(); 
  frame++;
  if (frame > 20) frame = 0;
}

void renderHeart() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  uint8_t pulse = beatsin8(50, 0, 255); 
  uint16_t pulseColor = matrix->Color(255, pulse, pulse);   
  matrix->drawBitmap(0, 0, big_heart, 16, 16, pulseColor);
  matrix->show(); 
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  
  int interval = 150; 
  if (currentState == ANIM_PACMAN) interval = 120;
  if (currentState == ANIM_HEART) interval = 30; 
  if (currentState == ANIM_CHIIKAWA) interval = 50; 
  if (currentState == ANIM_BDAY) interval = 80;

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    if (currentState == ANIM_PACMAN) {
      renderPacman();
    } else if (currentState == ANIM_KISS) {
      renderKiss();
    } else if (currentState == ANIM_HEART) {
      renderHeart();
    } else if (currentState == ANIM_CHIIKAWA) {
      renderChiikawa();
    } else if (currentState == ANIM_BDAY) {
      renderBday();
    } else if (currentState == ANIM_OFF) {
      matrix->fillScreen(0);
      matrix->show();
    }
  }
}