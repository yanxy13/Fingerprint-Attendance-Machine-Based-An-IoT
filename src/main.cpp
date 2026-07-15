#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h> 
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Fingerprint.h>
#include <ArduinoJson.h> 
#include "time.h" 

// --- TAMBAHAN LIBRARY LCD ---
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 

// ==========================================
// 1. CONFIG WIFI & FIREBASE
// ==========================================
#define WIFI_SSID "MUTIARA HATI"
#define WIFI_PASSWORD "R1o040508"

#define DATABASE_URL "https://absensi-mutiara-default-rtdb.asia-southeast1.firebasedatabase.app"
#define DATABASE_SECRET "qiaCFp87SwrjsofwplHmoGNHzSgZ9P7XSri4MCDO"

// ==========================================
// 2. CONFIG WHATSAPP (FONNTE)
// ==========================================
#define FONNTE_TOKEN "Swo3Bj89DmodBTuLT7Yr" 
#define NOMOR_TUJUAN "081352381040" 

// ==========================================
// 3. PIN & SETUP HARDWARE
// ==========================================
#define TFT_CS    21
#define TFT_RST   48
#define TFT_DC    47
#define TFT_MOSI  6 
#define TFT_SCLK  7 

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200; // WIB
const int   daylightOffset_sec = 0;

#define LED_WIFI    4  
#define LED_SUKSES  5  

#ifdef SD_CS_PIN
 #undef SD_CS_PIN
#endif
#define SD_CS_PIN 10
#define SPI_MOSI  11
#define SPI_SCK   12
#define SPI_MISO  13

#define FP_RX_PIN 18  
#define FP_TX_PIN 17  

HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
RTC_DS3231 rtc;

unsigned long lastCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long wifiCheckTimer = 0;
bool modeDaftar = false;
int idDaftar = 0;
String namaDaftar = "";

// --- [MODIFIKASI] VARIABEL BARU ---
bool isStandby = true;       // Penanda apakah layar sedang standby
unsigned long lastClock = 0; // Timer untuk update jam

// ==========================================
// 4. FUNGSI DISPLAY (VISUAL)
// ==========================================

// --- [MODIFIKASI] TAMPILAN STANDBY ---
void tampilkanStandby() {
  isStandby = true; // Set mode ke standby agar jam bisa update
  tft.fillScreen(ST77XX_BLACK);
  
  // Garis Header
  tft.fillRect(0, 0, 240, 40, ST77XX_BLUE);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(70, 12); // Geser sedikit ke tengah
  tft.print("ABSENSI");

  // Info Utama
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(3);
  tft.setCursor(65, 70); // Posisi agak naik
  tft.println("TEMPEL");
  tft.setCursor(80, 100);
  tft.println("JARI");

  // Footer
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(80, 220);
  tft.print("Mutiara Hati");
}

// --- [MODIFIKASI] FUNGSI BARU UNTUK UPDATE JAM ---
void updateJamLCD() {
  // Hanya update jam jika layar sedang standby (tidak sedang menampilkan sukses/gagal)
  if (!isStandby) return;

  DateTime now = rtc.now();
  
  // Format Jam: HH:MM:SS
  char jamBuffer[10];
  sprintf(jamBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // Format Tanggal: DD-MM-YYYY
  char tglBuffer[15];
  sprintf(tglBuffer, "%02d-%02d-%04d", now.day(), now.month(), now.year());

  // Tampilkan Tanggal (Kecil)
  tft.setTextSize(2);
  tft.setCursor(60, 140);
  // Pakai warna background (ST77XX_BLACK) agar tulisan lama tertimpa otomatis (anti-flicker)
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); 
  tft.print(tglBuffer);

  // Tampilkan Jam (Besar)
  tft.setTextSize(3);
  tft.setCursor(50, 170);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.print(jamBuffer);
}

void tampilkanSukses(String nama, String status) {
  isStandby = false; // Matikan update jam agar tidak menimpa layar sukses
  tft.fillScreen(ST77XX_GREEN);
  
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.println("BERHASIL!");

  // Nama
  tft.setTextSize(3);
  tft.setCursor(10, 80);
  tft.println(nama);

  // Status
  tft.setTextSize(2);
  if(status == "TERLAMBAT") tft.setTextColor(ST77XX_RED);
  else tft.setTextColor(ST77XX_BLUE);
  
  tft.setCursor(10, 150);
  tft.println(status);
}

void tampilkanGagal() {
  isStandby = false; // Matikan update jam sementara
  tft.fillScreen(ST77XX_RED);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(70, 80);
  tft.println("GAGAL");
  tft.setTextSize(2);
  tft.setCursor(50, 120);
  tft.println("Coba Lagi");
}

void tampilkanInfo(String teks1, String teks2) {
  isStandby = false; // Matikan update jam sementara
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.println(teks1);
  tft.setCursor(10, 120);
  tft.println(teks2);
}

// ==========================================
// 5. FUNGSI WA & DATABASE (TIDAK BERUBAH)
// ==========================================
void kirimWhatsApp(String pesan) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://api.fonnte.com/send");
    http.addHeader("Authorization", FONNTE_TOKEN);
    http.addHeader("Content-Type", "application/json");
    StaticJsonDocument<1024> doc;
    doc["target"] = NOMOR_TUJUAN;
    doc["message"] = pesan;
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    http.POST(jsonOutput);
    http.end();
  }
}

void kirimKeFirebase(String path, String method, String dataJson) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(DATABASE_URL) + path + ".json?auth=" + DATABASE_SECRET;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    if (method == "PUT") http.PUT(dataJson);
    else if (method == "POST") http.POST(dataJson);
    else if (method == "PATCH") http.PATCH(dataJson);
    http.end();
  }
}

void updateStatusWeb(String pesan) {
  kirimKeFirebase("/status_alat", "PUT", "\"" + pesan + "\"");
}

void kirimNotifKecil(String pesan) {
  String json = "{\"pesan\":\"" + pesan + "\", \"id\":" + String(millis()) + "}";
  kirimKeFirebase("/notif_kecil", "PUT", json);
}

void kirimHeartbeat() {
  kirimKeFirebase("/heartbeat", "PUT", "{\".sv\": \"timestamp\"}");
}

void cekPerintah() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(DATABASE_URL) + "/perintah.json?auth=" + DATABASE_SECRET;
    http.begin(url);
    if (http.GET() == 200) {
      String payload = http.getString();
      if (payload != "null") { 
        StaticJsonDocument<512> doc;
        deserializeJson(doc, payload);
        String mode = doc["mode"];
        if (mode == "DAFTAR") {
          idDaftar = doc["id"];
          namaDaftar = doc["nama"].as<String>();
          modeDaftar = true;
          updateStatusWeb("MODE DAFTAR: " + namaDaftar);
          tampilkanInfo("MODE DAFTAR", namaDaftar); 
        } 
        else if (mode == "HAPUS") {
          int idHapus = doc["id"];
          if (finger.deleteModel(idHapus) == FINGERPRINT_OK) updateStatusWeb("SUKSES HAPUS ID " + String(idHapus));
          else updateStatusWeb("GAGAL HAPUS");
        } 
        else if (mode == "HAPUS_SEMUA") {
           updateStatusWeb("RESET SEMUA...");
           tampilkanInfo("RESET DATA", "Mohon Tunggu");
           if (finger.emptyDatabase() == FINGERPRINT_OK) {
             SD.remove("/database.txt");
             SD.remove("/absensi.csv");
             updateStatusWeb("DATABASE BERSIH");
             tampilkanInfo("SUKSES", "Data Bersih");
             delay(2000); tampilkanStandby();
           } else updateStatusWeb("GAGAL RESET");
        }
        kirimKeFirebase("/perintah", "PUT", "{\"mode\":\"STANDBY\"}");
      }
    }
    http.end();
  }
}

// ==========================================
// 6. FUNGSI LOGIKA UTAMA
// ==========================================
void simpanNamaKeSD(int id, String nama) {
  File db = SD.open("/database.txt", FILE_APPEND);
  if(db) { db.println(String(id) + "," + nama); db.close(); }
}

String getNama(int id) {
  if(id == idDaftar && namaDaftar != "") return namaDaftar; 
  File db = SD.open("/database.txt");
  if(db) {
    while(db.available()) {
      String line = db.readStringUntil('\n'); line.trim();
      int comma = line.indexOf(',');
      if(comma > 0) {
        String idStr = line.substring(0, comma);
        String namaStr = line.substring(comma + 1);
        if(idStr.toInt() == id) { db.close(); return namaStr; }
      }
    }
    db.close();
  }
  return "ID-" + String(id);
}

void kirimLogAbsen(int id, String nama, String tgl, String jam, bool sdStatus, String statusKehadiran) {
  StaticJsonDocument<300> doc;
  doc["id"] = id; doc["nama"] = nama; doc["jam"] = jam; doc["status"] = statusKehadiran;
  String json; serializeJson(doc, json);
  kirimKeFirebase("/riwayat/" + tgl, "POST", json);
  
  updateStatusWeb("BERHASIL: " + nama);
  if (sdStatus) kirimNotifKecil("✅ Data Masuk (SD+Web)");
  else kirimNotifKecil("⚠️ Masuk Web Saja (SD Gagal)");

  StaticJsonDocument<200> docStat;
  docStat["terakhir_nama"] = nama; docStat["terakhir_jam"] = jam;
  String jsonStat; serializeJson(docStat, jsonStat);
  kirimKeFirebase("/status", "PATCH", jsonStat);

  String pesanWA = "*ABSENSI BARU* (" + statusKehadiran + ")\n\n👤 Nama: " + nama + "\n🕒 Jam: " + jam + "\n📅 Tgl: " + tgl;
  kirimWhatsApp(pesanWA);

  // UPDATE LCD: SUKSES
  tampilkanSukses(nama, statusKehadiran);

  digitalWrite(LED_SUKSES, HIGH); delay(2000); digitalWrite(LED_SUKSES, LOW);
  
  // Balik ke Standby
  tampilkanStandby();
}

bool simpanKeSD(String tgl, String jam, String nama, String statusKehadiran) {
  File log = SD.open("/absensi.csv", FILE_APPEND);
  if(log){
    log.print(tgl); log.print(","); log.print(jam); log.print(","); 
    log.print(nama); log.print(","); log.println(statusKehadiran);
    log.close(); return true; 
  }
  return false; 
}

void cekKoneksi() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_WIFI, HIGH);
  } else {
    digitalWrite(LED_WIFI, LOW); 
    WiFi.reconnect();
  }
}

void syncTimeOnline() {
  if(WiFi.status() != WL_CONNECTED) return;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) return;
  rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
}

// ==========================================
// 7. DAFTAR JARI
// ==========================================
uint8_t getFingerprintEnroll() {
  int p = -1;
  finger.deleteModel(idDaftar);
  
  updateStatusWeb("DAFTAR: TEMPEL JARI 1...");
  tampilkanInfo("TEMPEL JARI", "Langkah 1/2");
  
  while (p != FINGERPRINT_OK) { p = finger.getImage(); cekKoneksi(); } 
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) { tampilkanGagal(); delay(1000); return p; }
  
  updateStatusWeb("ANGKAT JARI");
  tampilkanInfo("ANGKAT JARI", "Lepaskan...");
  p = 0; while (p != FINGERPRINT_NOFINGER) { p = finger.getImage(); }
  delay(1000);
  
  updateStatusWeb("TEMPEL JARI LAGI...");
  tampilkanInfo("TEMPEL LAGI", "Langkah 2/2");
  
  p = -1; while (p != FINGERPRINT_OK) { p = finger.getImage(); }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) { tampilkanGagal(); delay(1000); return p; }

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    p = finger.storeModel(idDaftar);
    if (p == FINGERPRINT_OK) {
      updateStatusWeb("SUKSES DAFTAR: " + namaDaftar);
      simpanNamaKeSD(idDaftar, namaDaftar);
      kirimWhatsApp("🎉 *PENDAFTARAN SUKSES*\n\n👤 Nama: " + namaDaftar + "\n🆔 ID: " + String(idDaftar));
      
      tampilkanSukses(namaDaftar, "TERDAFTAR!");
      digitalWrite(LED_SUKSES, HIGH); delay(1000); digitalWrite(LED_SUKSES, LOW);
      return 1; 
    }
  } 
  tampilkanGagal(); delay(1000);
  return 0;
}

// ==========================================
// 8. SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9); 
  pinMode(LED_WIFI, OUTPUT); pinMode(LED_SUKSES, OUTPUT);
  digitalWrite(LED_WIFI, LOW); digitalWrite(LED_SUKSES, LOW);

  // START LCD
  tft.init(240, 320); 
  tft.setRotation(3); 
  tft.fillScreen(ST77XX_BLACK);
  tampilkanInfo("BOOTING...", "Mohon Tunggu");

  if (!rtc.begin()) Serial.println("RTC Gagal!");
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS_PIN);
  if(!SD.begin(SD_CS_PIN, SPI, 4000000)) Serial.println("SD Gagal");
  
  mySerial.begin(57600, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
  finger.verifyPassword();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) { 
    digitalWrite(LED_WIFI, !digitalRead(LED_WIFI)); 
    delay(300); retry++;
  }

  cekKoneksi(); 
  if(WiFi.status() == WL_CONNECTED) {
    syncTimeOnline();
    kirimWhatsApp("🤖 *ALAT ABSENSI AKTIF*\nLayar LCD Siap!");
  }
  
  updateStatusWeb("ALAT ONLINE");
  tampilkanStandby(); 
}

void loop() {
  // 1. Cek Koneksi WiFi berkala
  if (millis() - wifiCheckTimer > 1000) { cekKoneksi(); wifiCheckTimer = millis(); }
  
  // 2. Kirim Heartbeat ke Firebase berkala
  if (millis() - lastHeartbeat > 5000) { kirimHeartbeat(); lastHeartbeat = millis(); }
  
  // 3. Cek perintah dari web (Daftar/Hapus)
  if (millis() - lastCheck > 2000) { cekPerintah(); lastCheck = millis(); }

  // --- [MODIFIKASI] UPDATE JAM DI LAYAR SETIAP 1 DETIK ---
  if (millis() - lastClock > 1000) {
    updateJamLCD();
    lastClock = millis();
  }

  // 4. Mode Daftar (jika diaktifkan dari web)
  if (modeDaftar) {
    getFingerprintEnroll();
    modeDaftar = false; delay(2000);
    tampilkanStandby(); return;
  }

  // 5. Cek Fingerprint
  int p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    if (finger.image2Tz() == FINGERPRINT_OK) {
      if (finger.fingerFastSearch() == FINGERPRINT_OK) {
        
        DateTime now = rtc.now();
        char jam[20]; sprintf(jam, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        char tgl[20]; sprintf(tgl, "%04d-%02d-%02d", now.year(), now.month(), now.day());
        String nama = getNama(finger.fingerID);

        String statusKehadiran = "TEPAT WAKTU";
        if (now.hour() > 7 || (now.hour() == 7 && now.minute() >= 26)) statusKehadiran = "TERLAMBAT";
        
        // Simpan & Kirim
        bool sdStatus = simpanKeSD(tgl, jam, nama, statusKehadiran);
        
        // Di dalam fungsi ini sudah memanggil tampilkanSukses(nama, ...)
        kirimLogAbsen(finger.fingerID, nama, tgl, jam, sdStatus, statusKehadiran);
        
      } else {
        updateStatusWeb("GAGAL: TIDAK DIKENAL");
        tampilkanGagal(); 
        for(int i=0; i<3; i++) { digitalWrite(LED_SUKSES, HIGH); delay(50); digitalWrite(LED_SUKSES, LOW); delay(50); }
        delay(1000);
        tampilkanStandby(); 
      }
    }
  }
}