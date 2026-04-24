/*
 * Morphing Clock HH:MM:SS pe P10 monocrom (32x16) - ESP8266
 * Font personalizat fara colturi (conform desenului ASCII)
 * Orizontale: 2px (retrase), Verticale: 1px x 3px
 */

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

// Coordonate pixel-perfect pentru 32 pixeli latime
// Fiecare cifra: 4px. Punctele ":" au cate 1px.
// H1(0-3) H2(5-8) :(10) M1(12-15) M2(17-20) :(22) S1(24-27) S2(29-32->31)
const int digitX[6] = {0, 5, 12, 17, 24, 29}; 
const int digitY = 3; // Centrare verticala

const int colonX[2] = {10, 22}; 
const int colonY1 = 6;
const int colonY2 = 8;

String lastTimeStr = "";
int oldDigits[6] = {-1, -1, -1, -1, -1, -1};
int newDigits[6] = {-1, -1, -1, -1, -1, -1};

bool isAnimating = false;
int currentFrame = 0;
const int TOTAL_FRAMES = 8;
unsigned long lastFrameTime = 0;
unsigned long lastNTPUpdate = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
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
  if (millis() - lastNTPUpdate >= 60000) {
    timeClient.update();
    lastNTPUpdate = millis();
  }

  String timp = getTimeString();

  if (timp != lastTimeStr) {
    parseDigits(timp, newDigits);
    isAnimating = true;
    currentFrame = 1;
    lastFrameTime = millis();
    lastTimeStr = timp;
  }

  if (isAnimating) {
    if (millis() - lastFrameTime >= 40) {
      if (currentFrame > TOTAL_FRAMES) {
        isAnimating = false;
        for(int i=0; i<6; i++) oldDigits[i] = newDigits[i];
        drawStaticTime(newDigits);
      } else {
        drawAnimationFrame(oldDigits, newDigits, currentFrame);
        currentFrame++;
        lastFrameTime = millis();
      }
    }
  }

  dmd.loop();
}

// ====================================
// FUNCȚII DE DESENARE (FONT FARA COLȚURI)
// ====================================

void drawStaticTime(int digits[6]) {
  dmd.clear(false);
  for (int i = 0; i < 6; i++) {
    byte segs = segMap[digits[i]];
    for (int s = 0; s < 7; s++) {
      if (segs & (1 << s)) drawSegment(digitX[i], digitY, s, 2); 
    }
  }
  dmd.setPixel(colonX[0], colonY1, true); dmd.setPixel(colonX[0], colonY2, true);
  dmd.setPixel(colonX[1], colonY1, true); dmd.setPixel(colonX[1], colonY2, true);
}

void drawAnimationFrame(int oldD[6], int newD[6], int f) {
  dmd.clear(false);
  float progress = (float)f / TOTAL_FRAMES;
  
  dmd.setPixel(colonX[0], colonY1, true); dmd.setPixel(colonX[0], colonY2, true);
  dmd.setPixel(colonX[1], colonY1, true); dmd.setPixel(colonX[1], colonY2, true);

  for (int i = 0; i < 6; i++) {
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

// Geometrie exacta din desenul ASCII: latime 4, inaltime 9, fara colturi
void drawSegment(int ox, int oy, int seg, int step) {
  switch(seg) {
    // Orizontale: incep de la ox+1 si au fix 2 pixeli
    case 0: drawHLine(ox+1, oy, 2, step); break;       // A (Sus)
    case 3: drawHLine(ox+1, oy+8, 2, step); break;     // D (Jos)
    case 6: drawHLine(ox+1, oy+4, 2, step); break;     // G (Mijloc)
    
    // Verticale: au latime 1 si inaltime 3
    case 1: drawVLine(ox+3, oy+1, 3, step); break;     // B (Dr Sus)
    case 2: drawVLine(ox+3, oy+5, 3, step); break;     // C (Dr Jos)
    case 4: drawVLine(ox, oy+5, 3, step); break;       // E (Stg Jos)
    case 5: drawVLine(ox, oy+1, 3, step); break;       // F (Stg Sus)
  }
}

// Orizontala: lungime fixa 2 pixeli
void drawHLine(int x, int y, int len, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x, y, true); // Morph jumatate: 1 pixel
  } else {
    dmd.setPixel(x, y, true); 
    dmd.setPixel(x + 1, y, true); // Complet: 2 pixeli
  }
}

// Verticala: inaltime fixa 3 pixeli
void drawVLine(int x, int y, int len, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x, y + 1, true); // Morph jumatate: pixelul din mijloc
  } else {
    dmd.setPixel(x, y, true); 
    dmd.setPixel(x, y + 1, true); // Complet: 3 pixeli
    dmd.setPixel(x, y + 2, true);
  }
}

// ====================================
// UTILITARE
// ====================================

void parseDigits(String timeStr, int digits[6]) {
  digits[0] = timeStr.substring(0, 1).toInt();
  digits[1] = timeStr.substring(1, 2).toInt();
  digits[2] = timeStr.substring(3, 4).toInt();
  digits[3] = timeStr.substring(4, 5).toInt();
  digits[4] = timeStr.substring(6, 7).toInt();
  digits[5] = timeStr.substring(7, 8).toInt();
}

String getTimeString() {
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();
  int s = timeClient.getSeconds();
  return (h < 10 ? "0" : "") + String(h) + ":" + (m < 10 ? "0" : "") + String(m) + ":" + (s < 10 ? "0" : "") + String(s);
}
