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
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>

#define WIFI_SSID "IZZI-A94E"
#define WIFI_PASSWORD "FZYWCJNWZNML"

const char* mqtt_server = "broker.hivemq.com";
const char* mqtt_topic_anim = "kth_matrix/my_secret_id/anim";
const char* mqtt_topic_msg = "kth_matrix/my_secret_id/message";
const char* mqtt_topic_ota = "kth_matrix/my_secret_id/ota";
const char* mqtt_topic_brightness = "kth_matrix/my_secret_id/brightness";
const char* mqtt_topic_ack = "kth_matrix/my_secret_id/ack";
const char* mqtt_topic_live = "kth_matrix/my_secret_id/live";

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


unsigned long previousMillis = 0;
int frame = 0;
int cursorX = 16;
int currentAnim = 0;

void saveState() {
  File file = SPIFFS.open("/state.bin", FILE_WRITE);
  if (file) {
    file.write((uint8_t*)&currentAnim, sizeof(currentAnim));
    if (currentAnim == -1) {
      file.write((uint8_t*)leds, NUM_LEDS * sizeof(CRGB));
    }
    file.close();
  }
}

void loadState() {
  if (SPIFFS.exists("/state.bin")) {
    File file = SPIFFS.open("/state.bin", FILE_READ);
    if (file) {
      file.read((uint8_t*)&currentAnim, sizeof(currentAnim));
      if (currentAnim == -1) {
        file.read((uint8_t*)leds, NUM_LEDS * sizeof(CRGB));
        matrix->show();
      }
      file.close();
    }
  }
}

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




 void renderBday() {

matrix->fillScreen(matrix->Color(0, 0, 0));

matrix->setTextColor(matrix->Color(255, 20, 147));

matrix->setCursor(cursorX, 4);

matrix->print("Happy Birthday Mi Amor");

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

const uint8_t hachiware_blue[] PROGMEM = {
  0b00000000, 0b00000000, // Row 0
  0b00000000, 0b00000000, // Row 1
  0b00010000, 0b00010000, // Row 2
  0b00011000, 0b00110000, // Row 3
  0b00011111, 0b11110000, // Row 4
  0b01111110, 0b11111100, // Row 5
  0b01110000, 0b00011100, // Row 6
  0b00000000, 0b00000000, // Row 7
  0b00000000, 0b00000000, // Row 8
  0b00000000, 0b00000000, // Row 9
  0b00000000, 0b00000000, // Row 10
  0b00000000, 0b00000000, // Row 11
  0b00000000, 0b00000000, // Row 12
  0b00000000, 0b00000000, // Row 13
  0b00000000, 0b00000000, // Row 14
  0b00000000, 0b00000000  // Row 15
};


const uint8_t hachiware_white[] PROGMEM = {
  0b00000000, 0b00000000, // Row 0
  0b00000000, 0b00000000, // Row 1
  0b00000000, 0b00000000, // Row 2
  0b00000000, 0b00000000, // Row 3
  0b00000000, 0b00000000, // Row 4
  0b00000001, 0b00000000, // Row 5
  0b00000111, 0b11000000, // Row 6
  0b01111111, 0b11111100, // Row 7
  0b11110111, 0b11011110, // Row 8
  0b11101011, 0b10101110, // Row 9
  0b11100011, 0b10001110, // Row 10
  0b10000110, 0b11000010, // Row 11
  0b10001100, 0b01100010, // Row 12
  0b01111111, 0b11111100, // Row 13
  0b01111110, 0b11111100, // Row 14
  0b00011111, 0b11110000 // Row 15

};

const uint8_t hachiware_pink[] PROGMEM = {
  0b00000000, 0b00000000, // Row 0
  0b00000000, 0b00000000, // Row 1
  0b00000000, 0b00000000, // Row 2
  0b00000000, 0b00000000, // Row 3
  0b00000000, 0b00000000, // Row 4
  0b00000000, 0b00000000, // Row 5
  0b00000000, 0b00000000, // Row 6
  0b00000000, 0b00000000, // Row 7
  0b00000000, 0b00000000, // Row 8
  0b00000000, 0b00000000, // Row 9
  0b00000000, 0b00000000, // Row 10
  0b01110000, 0b00011100, // Row 11
  0b01110000, 0b00011100, // Row 12
  0b00000000, 0b00000000, // Row 13
  0b00000000, 0b00000000, // Row 14
  0b00000000, 0b00000000  // Row 15
};

const uint8_t tera_bmp[] PROGMEM = {
  0b00000000, 0b00000000, // Row 0
  0b00000000, 0b10000000, // Row 1
  0b00000000, 0b10000000, // Row 2
  0b00111111, 0b11111100, // Row 3
  0b00000000, 0b10000000, // Row 4
  0b11111111, 0b11111111, // Row 5
  0b00000000, 0b00000000, // Row 6
  0b00000000, 0b00001000, // Row 7
  0b01111111, 0b11111110, // Row 8
  0b00000000, 0b00001000, // Row 9
  0b00000000, 0b00001000, // Row 10
  0b00001000, 0b00001000, // Row 11
  0b00000100, 0b00001000, // Row 12
  0b00000010, 0b00001000, // Row 13
  0b00000000, 0b00010000, // Row 14
  0b00000011, 0b11100000  // Row 15
};

void renderHachiware() {
  matrix->fillScreen(matrix->Color(0, 0, 0));

  uint16_t colorWhite = matrix->Color(255, 255, 255);
  uint16_t colorBlue  = matrix->Color(40, 100, 200); 
  uint16_t colorPink  = matrix->Color(255, 150, 150);
  uint16_t colorBackground = matrix->Color(0, 0, 0);

  // Blinking timing logic
  bool isBlinking = (millis() % 3000) < 150;

  // 2. Render the layers
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      int byteIndex = (y * 2) + (x / 8);
      int bitPosition = 1 << (7 - (x % 8));

      // Check each layer sequentially. Negative space remains black.
      if (pgm_read_byte(&(hachiware_blue[byteIndex])) & bitPosition) {
        matrix->drawPixel(x, y, colorBlue);
      } else if (pgm_read_byte(&(hachiware_white[byteIndex])) & bitPosition) {
        matrix->drawPixel(x, y, colorWhite);
      } else if (pgm_read_byte(&(hachiware_pink[byteIndex])) & bitPosition) {
        matrix->drawPixel(x, y, colorPink);
      }
    }
  }

  // 3. Apply blinking animation mask
  if (isBlinking) {
    // Overwrite left eye with white, then draw a closed slit
    matrix->fillRect(4, 7, 2, 3, colorWhite);
    matrix->drawFastHLine(4, 8, 2, colorBackground);

    // Overwrite right eye with white, then draw a closed slit
    matrix->fillRect(9, 7, 2, 3, colorWhite);
    matrix->drawFastHLine(9, 8, 2, colorBackground);
  }

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
  uint16_t colorKanji   = matrix->Color(255, 255, 255);
  
  if (frame < 10) {
    matrix->drawBitmap(0, 0, tera_bmp, 16, 16, colorKanji);
  } else {
    int animFrame = frame - 10;
    int leftX, rightX;
    bool showHeart = false;
    
    if (animFrame < 5) {
      leftX = -4 + animFrame;        
      rightX = 12 - animFrame;      
    } else if (animFrame >= 5 && animFrame < 15) {
      leftX = 0;
      rightX = 8;
      showHeart = (animFrame % 2 == 0); 
    } else {
      leftX = 0 - (animFrame - 15);
      rightX = 8 + (animFrame - 15);
    }
    
    matrix->drawBitmap(leftX, 8, person_left, 8, 8, colorPerson1);
    matrix->drawBitmap(rightX, 8, person_right, 8, 8, colorPerson2);
    
    if (showHeart) {
      matrix->drawBitmap(4, 0, heart_bmp, 8, 8, colorHeart);
    }
  }
  
  matrix->show(); 
  frame++;
  
  if (frame > 30) {
    frame = 0;
  }
}

void renderHeart() {
  matrix->fillScreen(matrix->Color(0, 0, 0)); 
  
  // Create a pulse value from 0 to 255
  uint8_t pulse = beatsin8(50, 0, 255); 
  
  // Red increases as pulse increases, Blue increases as pulse decreases
  uint16_t pulseColor = matrix->Color(pulse, 0, 255 - pulse);   
  
  matrix->drawBitmap(0, 0, big_heart, 16, 16, pulseColor);
  matrix->show();
}

String customMessage = "Waiting for message...";
int messageCursorX = 16;

void renderMessage() {
  matrix->fillScreen(matrix->Color(0, 0, 0));
  matrix->setTextColor(matrix->Color(255, 255, 255)); 
  matrix->setCursor(messageCursorX, 4);
  matrix->print(customMessage);
  matrix->show();

  messageCursorX--;
  
  // Calculate width: length of string * 6 pixels per character
  int textWidth = customMessage.length() * 6;
  
  if (messageCursorX < -textWidth) {
    messageCursorX = 16; 
  }
}

void performOTA(const String& firmwareUrl) {
  matrix->fillScreen(matrix->Color(0, 0, 0));
  matrix->setTextColor(matrix->Color(0, 255, 0));
  matrix->setCursor(0, 4);
  matrix->print("OTA");
  matrix->show();

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); 
  
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  
  t_httpUpdate_return ret = httpUpdate.update(secureClient, firmwareUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      break;
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  String topicStr = String(topic);

  if (topicStr == mqtt_topic_msg) {
    customMessage = messageTemp;
    messageCursorX = 16; 
    currentAnim = 99; 
    client.publish(mqtt_topic_ack, "ACK: Message received and rendering");
  } 
  else if (topicStr == mqtt_topic_ota) {
    client.publish(mqtt_topic_ack, "ACK: OTA sequence initiated");
    performOTA(messageTemp);
  }
  else if (topicStr == mqtt_topic_anim) {
    currentAnim = messageTemp.toInt();
    saveState();
    client.publish(mqtt_topic_ack, "ACK: Animation updated");
  } 
  else if (topicStr == mqtt_topic_brightness) {
    int brightnessValue = messageTemp.toInt();
    if (brightnessValue >= 0 && brightnessValue <= 255) {
      FastLED.setBrightness(brightnessValue);
      client.publish(mqtt_topic_ack, "ACK: Brightness adjusted");
    } else {
      client.publish(mqtt_topic_ack, "ERROR: Brightness value out of bounds (0-255)");
    }
  }else if (topicStr == mqtt_topic_live) {
    if (length == 768) {
      // Full frame update
      currentAnim = -1;
      for (int i = 0; i < 256; i++){
        int x = i % 16;
        int y = i / 16;
        matrix->drawPixel(x, y, matrix->Color(payload[i*3], payload[i*3+1], payload[i*3+2]));
      }
      matrix->show();
      saveState();
      client.publish(mqtt_topic_ack, "ACK: Full frame received");
    } else if (length == 4) {
      // Single pixel update [index, R, G, B]
      currentAnim = -1;
      int index = (uint8_t)payload[0];
      int r = (uint8_t)payload[1];
      int g = (uint8_t)payload[2];
      int b = (uint8_t)payload[3];
      int x = index % 16;
      int y = index / 16;
      matrix->drawPixel(x, y, matrix->Color(r, g, b));
      matrix->show();
      client.publish(mqtt_topic_ack, "ACK: Pixel updated");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32MatrixClient")) {
      client.subscribe(mqtt_topic_anim);
      client.subscribe(mqtt_topic_msg);
      client.subscribe(mqtt_topic_ota);
      client.subscribe(mqtt_topic_brightness);
      client.subscribe(mqtt_topic_live);
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
  }

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(20);

  loadState();

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("IZZI-A94E", "FZYWCJNWZNML");
  wifiMulti.addAP("IZZI-A94E-5G", "FZYWCJNWZNML");
  wifiMulti.addAP("Foo","12345678");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (wifiMulti.run() != WL_CONNECTED) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 500) {
      lastBlink = millis();
      static bool state = false;
      state = !state;
      matrix->fillScreen(0);
      matrix->drawPixel(15, 0, state ? matrix->Color(255, 0, 0) : matrix->Color(0, 0, 0));
      matrix->show();
    }
    return;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 100) { 
    previousMillis = currentMillis;

    switch(currentAnim) {
      case -1: /* Live Drawing Mode: Do not clear or redraw */ break;
      case 0: renderHachiware(); break;
      case 1: renderBday(); break;
      case 2: renderChiikawa(); break;
      case 3: renderPacman(); break;
      case 4: renderKiss(); break;
      case 5: renderHeart(); break;
      case 99: renderMessage(); break;
      default: matrix->fillScreen(matrix->Color(0,0,0)); matrix->show(); break;
    }
  }
}