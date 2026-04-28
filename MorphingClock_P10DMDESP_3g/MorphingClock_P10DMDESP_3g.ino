/*
 * Morphing Clock HH:MM:SS + data ca text deplasabil pe P10 monocrom (32x16) - ESP8266
 * Data sus: text deplasabil cu font de 3x5 pixeli
 * Ceas jos: Font personalizat fara colturi, segmente 4x3
 * based requirements of Nicu FLORICA (niq_ro)
 * v.3e - AI added the scrolling text (name of the day and data) in upser side
 * v.3f - used my custom Font (height = 5) - https://app.fanselectronics.com/dmd-font-generator/
 * v.3g - added name of the day also in english, next in romanian, etc
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
#include "NewFont5x3.h"

const char* ssid     = "bbk2";
const char* password = "internet2";

DMDESP dmd(1, 1); 
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "ro.pool.ntp.org", 7200, 60000); // winter time
NTPClient timeClient(ntpUDP, "ro.pool.ntp.org", 10800, 60000); // summer time


const byte segMap[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// --- ZILELE SAPTAMANII IN ROMANA ---
const char* zileSaptamana[7] = {
  "Duminica", "Luni", "Marti", "Miercuri", "Joi", "Vineri", "Sambata"
};

// --- Name of the day  ---
const char* NameofDay[7] = {
  "Sunday", "Monday",  "TueSday", "WedneSday", "ThurSday", "Friday", "Saturday"
};

// --- COORDONATE CEAS (MUTATE MAI JOS) ---
const int digitX[6] = {0, 5, 12, 17, 23, 28}; 
const int digitY = 7; // Mutat de la 3 la 7 pentru a face loc datei

const int colonX[2] = {10, 22}; 
const int colonY1 = 10; // Recalculat pentru noua pozitie
const int colonY2 = 12;

String lastTimeStr = "";
String lastDateStr = "";
int oldDigits[6] = {-1, -1, -1, -1, -1, -1};
int newDigits[6] = {-1, -1, -1, -1, -1, -1};

bool isAnimating = false;
int currentFrame = 0;
const int TOTAL_FRAMES = 8;
unsigned long lastFrameTime = 0;
unsigned long lastNTPUpdate = 0;

// --- VARIABILE PENTRU TEXT DEPLASABIL ---
static uint32_t scroll_pM = 0;
static uint32_t scroll_x = 0;

byte limba = 0;  // romanian and english

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

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

  dmd.setFont(NewFont5x3);
 
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
  } 
  
  if((millis() - scroll_pM) > 150) { 
    scroll_pM = millis();
    
    // Calculam textul DOAR cand se misca (eficient pentru memorie)
    String textDeAfisat = "";
    if (limba%2 == 0)
    textDeAfisat = getRomanianDateStr(); 
    else
    textDeAfisat = getEnglishDateStr();
    int fullScroll = dmd.textWidth(textDeAfisat) + dmd.width();
    
    // Stergem zona de sus
    dmd.drawFilledRect(0, 0, dmd.width(), 4, false);
    
    if (scroll_x < fullScroll) {
      ++scroll_x;
    } else {
      scroll_x = 0;
      limba = limba + 1;  
    }
    
    // Desenam la Y = -3 (stie ca lasa "urme" in memorie mai jos)
    dmd.drawText(dmd.width() - scroll_x, -3, textDeAfisat); 
  }
  
  dmd.loop();
}

// ====================================
// FUNCȚII DATA ÎN ROMÂNĂ
// ====================================

String getRomanianDateStr() {
  unsigned long epoch = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epoch);
  
  String ziuaSapt = zileSaptamana[ptm->tm_wday];
  int zi = ptm->tm_mday;
  int luna = ptm->tm_mon + 1;
  int an = ptm->tm_year + 1900;
  
  String data1 = (zi < 10 ? "0" : "") + String(zi) + "." + 
                (luna < 10 ? "0" : "") + String(luna) + "." + String(an);
                
  return ziuaSapt + " " + data1; 
}

// ====================================
// DATA IN ENGLISH
// ====================================

String getEnglishDateStr() {
  unsigned long epoch = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epoch);
  
  String dayWeek = NameofDay[ptm->tm_wday];
  int zi = ptm->tm_mday;
  int luna = ptm->tm_mon + 1;
  int an = ptm->tm_year + 1900;
  
  String data2 = (zi < 10 ? "0" : "") + String(zi) + "." + 
                (luna < 10 ? "0" : "") + String(luna) + "." + String(an);
                
  return dayWeek + " " + data2; 
}


// ====================================
// FUNCȚII CEAS
// ====================================

// ====================================
// FUNCȚII DE DESENARE COMBINATE
// ====================================

void drawStaticTime(int digits[6], String dateStr) {
//  dmd.clear(false);
// Stergem zona de jos
    dmd.drawFilledRect(0, digitY, dmd.width(), dmd.height(), false);
   drawClockDigits(digits);
}

void drawAnimationFrame(int oldD[6], int newD[6], int f, String dateStr) {
//  dmd.clear(false);
 dmd.drawFilledRect(0, digitY, dmd.width(), dmd.height(), false); 
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
