# 📡 Smart Attendance System IoT (ESP32-S3 + Firebase)

![Status Project](https://img.shields.io/badge/Status-Active-success) ![Platform](https://img.shields.io/badge/Platform-ESP32-blue) ![License](https://img.shields.io/badge/License-MIT-green)

A **Smart Attendance System** based on the Internet of Things (IoT) that integrates a fingerprint sensor, dual-storage logging (SD Card & Firebase), and automated WhatsApp notifications. This device is designed to streamline real-time and transparent attendance monitoring.



## ✨ Key Features

* **👆 Precision Biometrics:** Utilizes an optical fingerprint sensor for fast and accurate identification.
* **💾 Dual Logging System:**
    * **Offline:** Saves attendance logs as CSV files on a MicroSD Card (safe during internet outages).
    * **Online:** Real-time data synchronization to Firebase Realtime Database.
* **📱 WhatsApp Notifications:** Automatically sends attendance alerts to admins or parents upon a successful scan (via Fonnte API).
* **🖥️ Interactive UI:** The ST7789 TFT display shows the real-time Clock, Date, User Name, and Attendance Status (On Time / Late).
* **⏰ Accurate Timekeeping:** Equipped with a DS3231 RTC module that automatically syncs with an internet NTP Server.
* **☁️ Remote Management:** Fingerprint enrollment and deletion are controlled remotely via database commands.

---

## 🛠️ Hardware Components & Wiring Pinout

Below is the list of components and their pin connections to the **ESP32-S3 (Board: Bos Kecil / DevKit V1)**:

| Component | ESP32 Pin | Protocol | Notes |
| :--- | :--- | :--- | :--- |
| **TFT LCD ST7789** | `CS:21`, `RST:48`, `DC:47`, `MOSI:6`, `SCK:7` | SPI (SW) | *Check voltage (usually 3.3V)* |
| **MicroSD Module** | `CS:10`, `MOSI:11`, `MISO:13`, `SCK:12` | SPI (HW) | *Format the SD card to FAT32* |
| **Fingerprint Sensor** | `RX:18`, `TX:17` | UART | *Sensor TX to ESP RX, Sensor RX to ESP TX* |
| **RTC DS3231** | `SDA:8`, `SCL:9` | I2C | *CMOS battery must be installed* |
| **WiFi LED** | `GPIO 4` | Digital Out | *Internet connection indicator* |
| **Success LED** | `GPIO 5` | Digital Out | *Successful scan indicator* |

---

## ⚙️ Installation & Configuration

### 1. Software Requirements
Make sure you have installed the **Arduino IDE** and the following libraries:
* `Adafruit Fingerprint Sensor Library`
* `Adafruit GFX Library` & `Adafruit ST7789 Library`
* `RTClib` (by Adafruit)
* `ArduinoJson` (by Benoit Blanchon)
* `WiFi`, `HTTPClient`, `SPI`, `SD`, `Wire` (Built-in ESP32 core libraries)

### 2. Credential Configuration
Open the `.ino` file and adjust the following credentials to match your setup:

```cpp
// WiFi Configuration
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Firebase Configuration (Realtime Database)
#define DATABASE_URL "[https://your-project-id-default-rtdb.firebaseio.com](https://your-project-id-default-rtdb.firebaseio.com)"
#define DATABASE_SECRET "Your_Firebase_Database_Secret"

// WhatsApp Configuration (Fonnte API)
#define FONNTE_TOKEN "Your_Fonnte_API_Token" 
#define NOMOR_TUJUAN "08xxxxxxxxxx" // Target Phone Number

3. Firebase Setup
Create a Realtime Database project in the Firebase Console. Ensure your database rules allow read/write access for the device (using the Database Secret handles authentication based on the code provided).

🚀 User Manual
A. Attendance Mode (Standby)
Turn on the device. The screen will display "BOOTING...".

Once connected to WiFi, the screen will show a Digital Clock and the text "TEMPEL JARI" (PLACE FINGER).

How to log attendance: Place a registered finger on the sensor.

✅ Success: The screen turns Green, displays the user's name, and sends a WhatsApp notification.

❌ Failed: The screen turns Red. Please try again.

B. New Fingerprint Enrollment Mode
Since this device lacks physical menu buttons, enrollment is triggered remotely via Firebase:

Open the Firebase Console > Realtime Database.

Edit the /perintah (command) node with the following JSON format:

JSON
{
  "mode": "DAFTAR",
  "id": 10,
  "nama": "John Doe"
}
The device will read the command, and the screen will change to "MODE DAFTAR" (ENROLL MODE).

Enrollment Steps:

Place your finger on the sensor (Screen: Step 1/2).

Remove your finger.

Place the exact same finger again (Screen: Step 2/2).

Upon success, the data is saved to both the Sensor's memory and the SD Card.

C. Delete Data Mode
To delete a specific fingerprint:

Send the following command via Firebase:

JSON
{
  "mode": "HAPUS",
  "id": 10
}
The device will automatically delete ID 10 from the sensor's database.

📂 Data Structure
1. On the SD Card
/database.txt: Stores ID and Name pairs (Format: 1,John).

/absensi.csv: Attendance log history (Format: Date,Time,Name,Status).

2. On Firebase
JSON
{
  "riwayat": {
    "2023-10-27": {
      "push_id": {
        "nama": "John",
        "jam": "07:00:00",
        "status": "TEPAT WAKTU"
      }
    }
  },
  "status_alat": "ALAT ONLINE",
  "perintah": { "mode": "STANDBY" }
}
🐛 Troubleshooting
Git Init Error on Drive D: Ensure proper folder navigation in the terminal (type D: then cd FolderName). It is highly recommended to use Git Bash in VS Code.

SD Card Mount Failed: Check if the MicroSD card is formatted to FAT32 and verify the SPI wiring connections.

Clock Not Displaying: Check the CMOS coin-cell battery on the RTC DS3231 module.

No WhatsApp Notifications: Verify that your Fonnte Token is active and you have sufficient API quota.

👨‍💻 Author
[Yanuar Rahmansyah]

Instrumentation Engineering College student

Institu Teknologi Sepuluh Nopember University
.
