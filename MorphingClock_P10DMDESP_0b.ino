/*
 * Morphing Clock pe P10 monocrom (HUB12) - ESP8266
 * Adaugat: Animatie puncte (:) - Blink la 1 secunda
 * based requirements of Nicu FLORICA (niq_ro)
 */

// --- 1. PINII TAI ---
#define DMDESP_PIN_A    16   // D0
#define DMDESP_PIN_B    12   // D6
#define DMDESP_PIN_CLK  14   // D5
#define DMDESP_PIN_SCLK 0    // D3
#define DMDESP_PIN_R    13   // D7
#define DMDESP_PIN_NOE  15   // D8

#include <DMDESP.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* ssid     = "bbk2";
const char* password = "internet2";

DMDESP dmd(1, 1); 

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ro.pool.ntp.org", 7200, 60000);

const byte segMap[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// Coordonate cu distanta intre cifre (modificate de tine)
const int digitX[4] = {0, 8, 17, 25};
const int digitY = 1; 

String lastTimeStr = "";
int oldDigits[4] = {-1, -1, -1, -1};
int newDigits[4] = {-1, -1, -1, -1};

// Variabile pentru animatia cifrelor MORPHING
bool isAnimating = false;
int currentFrame = 0;
const int TOTAL_FRAMES = 8;
unsigned long lastFrameTime = 0;
const int FRAME_DELAY = 40;
unsigned long lastNTPUpdate = 0;

// --- NOU: Variabile pentru animatia punctelor (BLINK) ---
bool showColon = true;
unsigned long lastColonBlinkTime = 0;
const int COLON_BLINK_DELAY = 500; // 500ms aprins, 500ms stins = 1 secunda total

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Conectat!");

  timeClient.begin();
  timeClient.update();

  dmd.start(); 
  dmd.setBrightness(50); 

  String timp = getTimeString();
  parseDigits(timp, oldDigits);
  drawStaticTime(oldDigits);
  lastTimeStr = timp;
}

void loop() {
  // 1. Actualizare NTP sigura
  if (millis() - lastNTPUpdate >= 60000) {
    timeClient.update();
    lastNTPUpdate = millis();
  }

  // 2. LOGICA ANIMATIE PUNCTE (BLINK)
  if (millis() - lastColonBlinkTime >= COLON_BLINK_DELAY) {
    showColon = !showColon; // Inverseaza starea (daca e aprins, il face stins si invers)
    lastColonBlinkTime = millis();
    
    // Daca NU facem morphing la cifre, redesenam ecranul pentru a arata punctele stinse/aprinse
    if (!isAnimating) {
      drawStaticTime(oldDigits);
    }
  }

  String timp = getTimeString();

  // 3. Detectare schimbare minut (Morphing cifre)
  if (timp != lastTimeStr && lastTimeStr != "") {
    parseDigits(timp, newDigits);
    isAnimating = true;
    currentFrame = 1;
    lastFrameTime = millis();
    lastTimeStr = timp;
  }

  // 4. Logica desenare animatie morphing
  if (isAnimating) {
    if (millis() - lastFrameTime >= FRAME_DELAY) {
      if (currentFrame > TOTAL_FRAMES) {
        isAnimating = false;
        for(int i=0; i<4; i++) oldDigits[i] = newDigits[i];
        drawStaticTime(newDigits);
      } else {
        drawAnimationFrame(oldDigits, newDigits, currentFrame);
        currentFrame++;
        lastFrameTime = millis();
      }
    }
  }

  // 5. Scanare continua
  dmd.loop();
}

// ====================================
// FUNCȚII DE DESENARE
// ====================================

void drawStaticTime(int digits[4]) {
  dmd.clear(false);
  for (int i = 0; i < 4; i++) {
    byte segs = segMap[digits[i]];
    for (int s = 0; s < 7; s++) {
      if (segs & (1 << s)) drawSegment(digitX[i], digitY, s, 2); 
    }
  }
  // Puncte animate - se deseneaza DOAR daca variabila showColon este true
  if (showColon) {
    dmd.setPixel(15, 4, true); dmd.setPixel(16, 4, true);
    dmd.setPixel(15, 10, true); dmd.setPixel(16, 10, true);
  }
}

void drawAnimationFrame(int oldD[4], int newD[4], int f) {
  dmd.clear(false);
  float progress = (float)f / TOTAL_FRAMES;
  
  // Puncte animate si in timpul morphing-ului cifrelor
  if (showColon) {
    dmd.setPixel(15, 4, true); dmd.setPixel(16, 4, true);
    dmd.setPixel(15, 10, true); dmd.setPixel(16, 10, true);
  }

  for (int i = 0; i < 4; i++) {
    byte oldSeg = segMap[oldD[i]];
    byte newSeg = segMap[newD[i]];
    
    for (int s = 0; s < 7; s++) {
      bool wasOn = oldSeg & (1 << s);
      bool isOn  = newSeg & (1 << s);
      int drawStep = 0; 
      
      if (wasOn && isOn) drawStep = 2;
      else if (!wasOn && !isOn) drawStep = 0;
      else if (!wasOn && isOn) drawStep = (progress < 0.5) ? 1 : 2;
      else if (wasOn && !isOn) drawStep = (progress < 0.5) ? 1 : 0;
      
      drawSegment(digitX[i], digitY, s, drawStep);
    }
  }
}

void drawSegment(int ox, int oy, int seg, int step) {
  int thick = 2;
  switch(seg) {
    case 0: drawHLine(ox+2, oy, 3, thick, step); break;     // A
    case 1: drawVLine(ox+5, oy+2, 4, thick, step); break;   // B
    case 2: drawVLine(ox+5, oy+8, 4, thick, step); break;   // C
    case 3: drawHLine(ox+2, oy+12, 3, thick, step); break;  // D
    case 4: drawVLine(ox, oy+8, 4, thick, step); break;     // E
    case 5: drawVLine(ox, oy+2, 4, thick, step); break;     // F
    case 6: drawHLine(ox+2, oy+6, 3, thick, step); break;   // G
  }
}

void drawHLine(int x, int y, int len, int thick, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x + 1, y, true);
    dmd.setPixel(x + 1, y + 1, true);
  } else {
    for (int i = 0; i < len; i++) {
      for (int j = 0; j < thick; j++) {
        dmd.setPixel(x + i, y + j, true);
      }
    }
  }
}

void drawVLine(int x, int y, int len, int thick, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x, y + 1, true);   dmd.setPixel(x + 1, y + 1, true);
    dmd.setPixel(x, y + 2, true);   dmd.setPixel(x + 1, y + 2, true);
  } else {
    for (int i = 0; i < len; i++) {
      for (int j = 0; j < thick; j++) {
        dmd.setPixel(x + j, y + i, true);
      }
    }
  }
}

// ====================================
// UTILITARE
// ====================================

void parseDigits(String timeStr, int digits[4]) {
  digits[0] = timeStr.substring(0, 1).toInt();
  digits[1] = timeStr.substring(1, 2).toInt();
  digits[2] = timeStr.substring(3, 4).toInt();
  digits[3] = timeStr.substring(4, 5).toInt();
}

String getTimeString() {
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  return (h < 10 ? "0" : "") + String(h) + ":" + (m < 10 ? "0" : "") + String(m);
}
