#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define DHTPIN 14          // Pin untuk data DHT11
#define DHTTYPE DHT11      // Tipe sensor DHT11
#define SOIL_SENSOR_PIN 34 // Pin untuk data sensor kelembaban tanah
#define RELAY_PIN 26       // Pin untuk relay

// Konfigurasi jaringan WiFi
const char* ssid = "LABOR SAMSUNG";
const char* password = "hhpsamsung02";
const char* server_address = "192.168.1.11"; // Alamat IP server Flask
const int server_port = 5000;                 // Port server Flask
const String url = "/sensor/data";            // Endpoint untuk mengirim data

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiClient client;

// Konfigurasi NTP untuk mendapatkan waktu dari internet
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void setup() {
  // Inisialisasi serial komunikasi
  Serial.begin(115200);

  // Inisialisasi sensor DHT11
  dht.begin();

  // Inisialisasi LCD I2C
  lcd.init();
  lcd.backlight();

  // Inisialisasi pin relay sebagai output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Pompa off awalnya

  // Inisialisasi koneksi WiFi
  connectToWiFi();

  // Inisialisasi NTP untuk mendapatkan waktu dari internet
  timeClient.begin();
  delay(1000);
  timeClient.update();

  // Tampilan awal di LCD
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.setCursor(0, 1);
  lcd.print("Kelembaban: ");
  lcd.setCursor(0, 2);
  lcd.print("Soil Moisture: ");
}

void loop() {
  // Ambil waktu saat ini dari NTP server
  timeClient.update();
  String timestamp = timeClient.getFormattedTime();

  // Membaca data dari DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Membaca data dari sensor kelembaban tanah
  int soilMoisture = analogRead(SOIL_SENSOR_PIN) / 4;

  // Periksa jika ada kegagalan dalam membaca data dari DHT11
  if (isnan(h) || isnan(t)) {
    Serial.println("Gagal membaca dari DHT sensor!");
    return;
  }

  // Menampilkan data ke LCD
  lcd.setCursor(6, 0);
  lcd.print(t);
  lcd.print(" C");

  lcd.setCursor(12, 1);
  lcd.print("   "); // Menghapus karakter lama
  lcd.setCursor(12, 1);
  lcd.print(h);

  lcd.setCursor(14, 2);
  lcd.print("    "); // Menghapus karakter lama
  lcd.setCursor(14, 2);
  lcd.print(soilMoisture);
  lcd.print("");

  // Logika untuk mengaktifkan atau mematikan pompa
  if (soilMoisture > 800 && t > 20) {
    digitalWrite(RELAY_PIN, HIGH); // Mengaktifkan pompa
    lcd.setCursor(0, 3);
    lcd.print("Pompa: ON ");
  } else if (soilMoisture < 600) {
    digitalWrite(RELAY_PIN, LOW); // Mematikan pompa
    lcd.setCursor(0, 3);
    lcd.print("Pompa: OFF");
  }

  // Kirim data ke server Flask
  sendSensorData(t, h, soilMoisture, timestamp);

  // Menunda sebentar sebelum melakukan pembacaan lagi
  delay(2000);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println("Connected to WiFi");
}

void sendSensorData(float temperature, float humidity, int soilMoisture, String timestamp) {
  if (!client.connect(server_address, server_port)) {
    Serial.println("Connection to server failed...");
    return;
  }

  Serial.println("Connected to server");

  // Buat JSON object untuk data sensor
  String json_data = "{\"temperature\": " + String(temperature) + ",";
  json_data += "\"humidity\": " + String(humidity) + ",";
  json_data += "\"soil_moisture\": " + String(soilMoisture) + ",";
  json_data += "\"timestamp\": \"" + timestamp + "\"}";

  // Kirim HTTP POST request ke server
  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: " + String(server_address));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(json_data.length());
  client.println();
  client.println(json_data);

  Serial.println("Data sent to server:");
  Serial.println(json_data);

  client.stop(); // Putuskan koneksi setelah selesai mengirim
}
