/*
 * Morphing Clock HH:MM:SS + data ca text deplasabil pe P10 monocrom (32x16) - ESP8266
 * Data sus: text deplasabil cu font de 3x5 pixeli
 * Ceas jos: Font personalizat fara colturi, segmente 4x3
 * based requirements of Nicu FLORICA (niq_ro)
 * v.3e - AI added the scrolling text (name of the day and data) in upser side
 * v.3f - used my custom Font (height = 5) - https://app.fanselectronics.com/dmd-font-generator/
 * v.3g - added name of the day also in english, next in romanian, etc
 * v.4 - added open-meteo.com info also using AI (but just for basic sketch)
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
#include "Fontnou5x.h"
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> // Necesar pentru HTTPS
#include <ArduinoJson.h>

const char* ssid     = "bbk2";
const char* password = "internet2";

// Coordonatele orașului tău (Exemplu: București)
// Poți găsi coordonatele mergând pe Google Maps, dând click dreapta pe locație'
// Craiova 
float latitude = 44.3302; //44.4268;
float longitude = 23.7949;  // 26.1025;

// --- FUNCȚIE PENTRU TRADUCEREA CODURILOR WMO ---
// Open-Meteo folosește coduri numerice pentru starea vremii
String traducereVreme(int cod) {
  switch(cod) {
    case 0: return "Cer senin";
    case 1: return "Predominant senin";
    case 2: return "Partial noros";
    case 3: return "Innorat";
    case 45: case 48: return "Ceata";
    case 51: case 53: case 55: return "Burnita";
    case 56: case 57: return "Burnita înghetata";
    case 61: case 63: case 65: return "Ploaie";
    case 66: case 67: return "Ploaie inghetata";
    case 71: case 73: case 75: return "Ninsoare";
    case 77: return "Granule de gheata";
    case 80: case 81: case 82: return "Averse de ploaie";
    case 85: case 86: return "Averse de ninsoare";
    case 95: return "Furtuna";
    case 96: case 99: return "Furtuna cu grindina";
    default: return "Necunoscut";
  }
}

String directieVant(int grade) {
  const char* directii[] = {"Nord", "Nord-Est", "Est", "Sud-Est", "Sud", "Sud-Vest", "Vest", "Nord-Vest"};
  int index = int((float)(grade / 45.)) % 8;
  return directii[index];
}

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

byte cetext = 0;  // romanian and english

String meteo1, meteo2, meteo3, meteo4, meteo5;
byte gata = 1;
unsigned long tpactualizare;
unsigned long tpactualizare0 = 1200000;

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
  OpenMeteo();
}

void loop() {
  if (millis() - lastNTPUpdate >= 60000) 
  {
    timeClient.update();
    lastNTPUpdate = millis();
  //  OpenMeteo();
  }

  String timp = getTimeString();
  String data = getDateString();
  dmd.setFont(Fontnou5x);

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
    /*
    if (cetext%2 == 0)
    textDeAfisat = getRomanianDateStr(); 
    else
    textDeAfisat = getEnglishDateStr();
    */
    if (cetext == 0)
      textDeAfisat = getRomanianDateStr(); 
    else
    if (cetext == 1)
      textDeAfisat = meteo1; 
    else    
    if (cetext == 2)
      textDeAfisat = meteo2; 
    else  
    if (cetext == 3)
      textDeAfisat = meteo3; 
    else  
    if (cetext == 4)
      textDeAfisat = meteo4; 
    else
    if (cetext == 5)
      textDeAfisat = meteo5; 
 //   else
 //   if (cetext == 6)
 //     textDeAfisat = "Caut date meteo noi!";      

    int fullScroll = dmd.textWidth(textDeAfisat) + dmd.width();
    // Stergem zona de sus
    dmd.drawFilledRect(0, 0, dmd.width(), 4, false);
    
    if (scroll_x < fullScroll) 
    {
      ++scroll_x;
    } 
    else 
    {
      scroll_x = 0;
      cetext = cetext + 1;  
      if (cetext > 5)
      {
        cetext = 0;
      //  dmd.drawFilledRect(0, 0, dmd.width(), dmd.height(), false); 
        gata = 0;    
        if ( millis()- tpactualizare > tpactualizare0)
        {    
        OpenMeteo();
        }
      }
    }
    
    // Desenam la Y = -3 (stie ca lasa "urme" in memorie mai jos)
    dmd.drawText(dmd.width() - scroll_x, -3, textDeAfisat); 
  }

  dmd.loop();
} // end main loop

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

void drawClockDigits1(int digits[6]) {
  for (int i = 0; i < 4; i++) {
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


// Wheater info
void OpenMeteo()
{ 
  gata = 0;
  Serial.println("Meteo data is searching !");   
  dmd.drawFilledRect(0, 0, dmd.width(), dmd.height(), false); 
  dmd.loop();
     
    WiFiClientSecure client;
    HTTPClient http;
    
    // Pentru Open-Meteo este obligatoriu să setăm clientul ca "insecure" 
    // (ignoră verificarea certificatului SSL). Pe ESP8266 este necesar pentru a nu da eroare.
    client.setInsecure();

    // Construim URL-ul cererii. 
    // Parametrul "current=" cere exact datele care ne interesează în acest moment.
/*
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 4) + 
                 "&longitude=" + String(longitude, 4) + 
               //  "&current=temperature_2m,relative_humidity_2m,surface_pressure,weather_code&timezone=auto";
                 "&current=temperature_2m,relative_humidity_2m,surface_pressure,weather_code,wind_speed_10m,wind_direction_10m&timezone=auto";
*/
   String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 4) + 
                 "&longitude=" + String(longitude, 4) + 
                 "&current=temperature_2m,relative_humidity_2m,surface_pressure,pressure_msl,weather_code,wind_speed_10m,wind_direction_10m&timezone=auto";
    
    http.begin(client, url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      String payload = http.getString();
      
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.print("Eroare la parsarea JSON: ");
        Serial.println(error.c_str());
      } else {
        float temp = doc["current"]["temperature_2m"].as<float>();
        float umiditate = doc["current"]["relative_humidity_2m"].as<float>();
        int codVreme = doc["current"]["weather_code"].as<int>();
        String timp = doc["current"]["time"].as<String>();

 //     // Traducem codul numeric în text românesc
        String descriere = traducereVreme(codVreme);     
        
        // Citim ambele tipuri de presiune (in hPa)
        float presiuneSol_hPa = doc["current"]["surface_pressure"].as<float>();
        float presiuneMSL_hPa = doc["current"]["pressure_msl"].as<float>();
        
        // Convertim in mmHg (1 hPa = 0.75006 mmHg)
        float presiuneSol_mmHg = presiuneSol_hPa * 0.75006;
        float presiuneMSL_mmHg = presiuneMSL_hPa * 0.75006;

        float vitezaVant = doc["current"]["wind_speed_10m"].as<float>();
        int directieGrade = doc["current"]["wind_direction_10m"].as<int>();
        
        // Afișare
        int temp0 = temp*10.;
        int temp1 = temp0/10; 
        int temp2 = temp0%10;
        int directieGrade1 = directieGrade;
        int vitezaVant0 = vitezaVant*10.;
        int vitezaVant1 = vitezaVant0/10;
        int vitezaVant2 = vitezaVant0%10;
        
        meteo1 = descriere;
        meteo2 = "Temperatura: ";
        meteo2 = meteo2 + temp1 + "." + temp2 + "#C";  // # is degree on the display
        meteo3 = "Umiditate: ";
        meteo3 = meteo3 + int(umiditate) + "%";  
        meteo4 = "Presiune: ";
        meteo4 = meteo4 + int(presiuneMSL_mmHg+0.5) + " mmHg";
        meteo5 = "Vant: ";
        meteo5 = meteo5 + vitezaVant1 + "." + vitezaVant2  + " km/h din " + directieVant(directieGrade1) + " !"; 
        Serial.println("\n=========================================");
        Serial.printf("Locatie: %.4f, %.4f\n", latitude, longitude);
        Serial.printf("Ora masuratoarii: %s\n", timp.c_str());
        Serial.println("-----------------------------------------");
        Serial.printf("Stare: %s (Cod %d)\n", descriere.c_str(), codVreme);
        Serial.printf("Temperatura: %.1f °C\n", temp);
        Serial.printf("Umiditate: %.0f %%\n", umiditate);     
        Serial.printf("Presiune_nivelul marii: %.0f mmHg\n", presiuneMSL_mmHg);
         // Afișăm presiunea în mmHg cu o zecimală (%.1f)
        Serial.printf("Presiune sol: %.1f mmHg\n", presiuneSol_mmHg);
        Serial.printf("Vant: %.1f km/h din %s (%d grade)\n", vitezaVant, directieVant(directieGrade).c_str(), directieGrade);
        Serial.println("=========================================\n");
      }
    } else {
      Serial.printf("Eroare la cererea HTTPS: %s\n", http.errorToString(httpCode).c_str());
    }   
    http.end();
    Serial.println("Meteo data was found !"); 
    gata = 1;
    tpactualizare = millis();
}
