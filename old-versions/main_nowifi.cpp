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

#define WIFI_SSID "IZZI-A94E"
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

// 1. Add the new state to the enum
enum AnimationState {ANIM_OFF, ANIM_PACMAN, ANIM_KISS, ANIM_HEART, ANIM_CHIIKAWA, ANIM_BDAY };

// 2. Set the default state to the birthday text
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

const uint8_t chiikawa_bmp[] PROGMEM = {
  0x00, 0x00, 0x30, 0x0c, 0x78, 0x1e, 0x7f, 0xfe,
  0xff, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7,
  0xff, 0xff, 0xfa, 0x5f, 0xfd, 0xbf, 0x7f, 0xfe,
  0x3f, 0xfc, 0x1f, 0xf8, 0x18, 0x18, 0x00, 0x00
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
  } else if (incoming == "bday") { // 3. Add MQTT trigger for the text
    currentState = ANIM_BDAY;
    cursorX = 16; // Reset text position to the right edge
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
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(1000);

  wifi_country_t wifi_country = {.cc="MX", .schan=1, .nchan=11, .policy=WIFI_COUNTRY_POLICY_AUTO};
  esp_wifi_set_country(&wifi_country);
  
  WiFi.setSleep(false);
  
  // Register multiple access points
  wifiMulti.addAP("IZZ-A94E", "FZYWCJNWZNML");
  
  // Halt execution until one of the registered networks connects
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
  }
}


// 4. The new Birthday Text Render Function
void renderBday() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); // Black background
  
  // Set text color to Deep Pink for contrast
  matrix->setTextColor(matrix->Color(255, 20, 147)); 
  
  // Place the text vertically centered (Y=4)
  matrix->setCursor(cursorX, 4); 
  matrix->print("happy birthday mi amor");
  
  matrix->show();
  
  // Move the text left by 1 pixel per frame
  cursorX--;
  
  // Standard Adafruit characters are 6 pixels wide (5px character + 1px space)
  // "happy birthday mi amor" is 22 characters. 22 * 6 = 132 pixels.
  // Reset when the text completely leaves the left side of the screen.
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

void renderPacman() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  uint16_t colorGhost = matrix->Color(0, 255, 255); 
  uint16_t colorPacman = matrix->Color(255, 255, 0); 
  
  matrix->drawBitmap(cursorX, 4, ghost_bmp, 8, 8, colorGhost);
  
  if (abs(cursorX) % 4 < 2) {
    matrix->drawBitmap(cursorX - 10, 4, pacman_open, 8, 8, colorPacman);
  } else {
    matrix->drawBitmap(cursorX - 10, 4, pacman_closed, 8, 8, colorPacman);
  }
  
  matrix->show(); 
  cursorX++; 
  if (cursorX <= -20) cursorX = 16; 
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
  matrix->drawBitmap(4, 4, heart_bmp, 8, 8, pulseColor);
  matrix->show(); 
}


// void setup() {
//   setup_wifi();
  
//   ArduinoOTA.setHostname("esp32-matrix"); 
//   ArduinoOTA.begin();

//   client.setServer(mqtt_server, 1883);
//   client.setCallback(callback);

//   FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
//   FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); 
//   matrix->begin();
  
//   // CRITICAL: Ensure text wrapping is false so it scrolls off the screen
//   matrix->setTextWrap(false); 
//   matrix->setBrightness(15); 
// }

// void loop() {
//   ArduinoOTA.handle();

//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();

//   unsigned long currentMillis = millis();
  
//   int interval = 150; 
//   if (currentState == ANIM_PACMAN) interval = 120;
//   if (currentState == ANIM_HEART) interval = 30; 
//   if (currentState == ANIM_CHIIKAWA) interval = 50; 
  
//   // 5. Add dynamic interval for the scrolling text (80ms creates a smooth read speed)
//   if (currentState == ANIM_BDAY) interval = 80;

//   if (currentMillis - previousMillis >= interval) {
//     previousMillis = currentMillis;
    
//     if (currentState == ANIM_PACMAN) {
//       renderPacman();
//     } else if (currentState == ANIM_KISS) {
//       renderKiss();
//     } else if (currentState == ANIM_HEART) {
//       renderHeart();
//     } else if (currentState == ANIM_CHIIKAWA) {
//       renderChiikawa();
//     } else if (currentState == ANIM_BDAY) {
//       renderBday();
//     } else if (currentState == ANIM_OFF) {
//       matrix->fillScreen(0);
//       matrix->show();
//     }
//   }
// }

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- BOOTING ---");
  
  // 1. TEMPORARILY DISABLE WI-FI
  // setup_wifi(); 
  
  // ArduinoOTA.setHostname("esp32-matrix"); 
  // ArduinoOTA.begin();

  // client.setServer(mqtt_server, 1883);
  // client.setCallback(callback);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); 
  matrix->begin();
  
  matrix->setTextWrap(false); 
  matrix->setBrightness(15); 
  
  Serial.println("Matrix initialization complete. Starting animation loop.");
}

void loop() {
  // 2. TEMPORARILY DISABLE MQTT AND OTA RECONNECTS
  // ArduinoOTA.handle();
  // if (!client.connected()) {
  //   reconnect();
  // }
  // client.loop();

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