/*
 * Morphing Clock HH:MM:SS + Data ZZ.LL pe P10 monocrom (32x16) - ESP8266
 * Data sus: Font 3x5 pixeli
 * Ceas jos: Font personalizat fara colturi, segmente 4x3
 * based requirements of Nicu FLORICA (niq_ro)
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
//NTPClient timeClient(ntpUDP, "ro.pool.ntp.org", 7200, 60000); // winter time
NTPClient timeClient(ntpUDP, "ro.pool.ntp.org", 10800, 60000); // summer time


const byte segMap[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// --- FONT MATRICEAL 3x5 PENTRU DATA (Cifre 0-9 si punct .) ---
const byte font3x5[11][5] = {
  {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
  {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
  {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
  {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
  {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
  {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
  {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
  {0x7, 0x1, 0x1, 0x1, 0x1}, // 7
  {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
  {0x7, 0x5, 0x7, 0x1, 0x7}, // 9
  {0x0, 0x0, 0x2, 0x0, 0x0}  // . (punct)
};

// --- COORDONATE CEAS (MUTATE MAI JOS) ---
const int digitX[6] = {0, 5, 12, 17, 23, 28}; 
const int digitY = 6; // Mutat de la 3 la 6 pentru a face loc datei

const int colonX[2] = {10, 22}; 
const int colonY1 = 9; // Recalculat pentru noua pozitie
const int colonY2 = 11;

String lastTimeStr = "";
String lastDateStr = "";
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
  dmd.setBrightness(2); // minimum 1..255 maximum

  String timp = getTimeString();
  String data = getDateString();
  parseDigits(timp, oldDigits);
  
  drawStaticTime(oldDigits, data);
  lastTimeStr = timp;
  lastDateStr = data;
}

void loop() {
  if (millis() - lastNTPUpdate >= 60000) {
    timeClient.update();
    lastNTPUpdate = millis();
  }

  String timp = getTimeString();
  String data = getDateString();

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
        drawStaticTime(newDigits, data);
      } else {
        drawAnimationFrame(oldDigits, newDigits, currentFrame, data);
        currentFrame++;
        lastFrameTime = millis();
      }
    }
  } else if (data != lastDateStr) {
    // Daca s-a schimbat ziua (la miezul noptii), redesenam fara animatie
    drawStaticTime(oldDigits, data);
    lastDateStr = data;
  }

  dmd.loop();
}

// ====================================
// FUNCȚII DE DESENARE COMBINATE
// ====================================

void drawStaticTime(int digits[6], String dateStr) {
  dmd.clear(false);
  drawDateText(dateStr, 0); // Deseneaza data incepand de la Y=0
  drawClockDigits(digits);
}

void drawAnimationFrame(int oldD[6], int newD[6], int f, String dateStr) {
  dmd.clear(false);
  drawDateText(dateStr, 0); // Deseneaza data si in timpul animatiei ceasului
  
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

void drawClockDigits(int digits[6]) {
  for (int i = 0; i < 6; i++) {
    byte segs = segMap[digits[i]];
    for (int s = 0; s < 7; s++) {
      if (segs & (1 << s)) drawSegment(digitX[i], digitY, s, 2); 
    }
  }
  dmd.setPixel(colonX[0], colonY1, true); dmd.setPixel(colonX[0], colonY2, true);
  dmd.setPixel(colonX[1], colonY1, true); dmd.setPixel(colonX[1], colonY2, true);
}

// ====================================
// FONT DATA 3x5 SI CEAS FARA COLTURI
// ====================================

void drawDateText(String dateStr, int startY) {
  // "DD.MM" = 5 caractere. Latime totala = 5*3 + 4*1(spate) = 19 pixeli.
  // Centrat pe 32: (32 - 19) / 2 = 6.5 -> start la X = 6
  int startX = 6; 
  int cursorX = startX;

  for (int i = 0; i < dateStr.length(); i++) {
    char c = dateStr.charAt(i);
    int charIndex = -1;

    if (c >= '0' && c <= '9') charIndex = c - '0';
    else if (c == '.') charIndex = 10; // Indexul punctului din font

    if (charIndex != -1) {
      for (int col = 0; col < 3; col++) {
        for (int row = 0; row < 5; row++) {
          // Cauta bitul in matricea fontului
          if (font3x5[charIndex][row] & (1 << (2 - col))) {
            dmd.setPixel(cursorX + col, startY + row, true);
          }
        }
      }
      cursorX += 4; // Treci la urmatorul caracter (3 latime + 1 spatiu)
    }
  }
}

void drawSegment(int ox, int oy, int seg, int step) {
  switch(seg) {
    case 0: drawHLine(ox+1, oy, 2, step); break;       
    case 3: drawHLine(ox+1, oy+8, 2, step); break;     
    case 6: drawHLine(ox+1, oy+4, 2, step); break;     
    case 1: drawVLine(ox+3, oy+1, 3, step); break;     
    case 2: drawVLine(ox+3, oy+5, 3, step); break;     
    case 4: drawVLine(ox, oy+5, 3, step); break;       
    case 5: drawVLine(ox, oy+1, 3, step); break;       
  }
}

void drawHLine(int x, int y, int len, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x, y, true); 
  } else {
    dmd.setPixel(x, y, true); dmd.setPixel(x + 1, y, true);
  }
}

void drawVLine(int x, int y, int len, int step) {
  if (step == 0) return;
  if (step == 1) {
    dmd.setPixel(x, y + 1, true); 
  } else {
    dmd.setPixel(x, y, true); dmd.setPixel(x, y + 1, true); dmd.setPixel(x, y + 2, true);
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

String getDateString() {
  unsigned long epoch = timeClient.getEpochTime();
  
  // Convertim timpul UNIX intr-o structura de timp citibila
  struct tm *ptm = gmtime ((time_t *)&epoch);
  
  int day = ptm->tm_mday;       // Ziua (1-31)
  int month = ptm->tm_mon + 1;  // Luna (0-11 in structura, deci adunam 1)
  
  // Formatare "ZZ.LL" cu zero in fata daca e necesar
  String zi = (day < 10 ? "0" : "") + String(day);
  String luna = (month < 10 ? "0" : "") + String(month);
  
  return zi + "." + luna;
}
