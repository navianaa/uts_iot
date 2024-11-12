#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Pin yang digunakan untuk komponen
#define DHTPIN 15       // Pin sensor DHT
#define LED_HI_PIN 13   // LED Hijau
#define LED_YE_PIN 14   // LED Kuning
#define LED_RE_PIN 12   // LED Merah
#define RELAY_PIN 32    // Relay pompa
#define BUZZER_PIN 33   // Buzzer

// DHT sensor
#define DHTTYPE DHT11   // Jenis sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Pengaturan WiFi dan MQTT
const char* ssid = "Wokwi-GUEST";    // Nama WiFi
const char* password = "";           // Password WiFi (kosong)
const char* mqtt_server = "broker.hivemq.com"; // Alamat broker MQTT
const int mqtt_port = 1883;          // Port broker MQTT

WiFiClient espClient;
PubSubClient client(espClient);

// Fungsi untuk menghubungkan ke WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    Serial.print(WiFi.status()); // Debugging untuk menampilkan status WiFi
  }

  Serial.println("\nTerhubung ke WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Menampilkan alamat IP
}

// Fungsi untuk menghubungkan ke broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Mencoba menghubungkan ke MQTT...");

    // Mencoba untuk terkoneksi
    if (client.connect("ESP32Client")) {
      Serial.println("Terhubung");
      client.subscribe("hydroponic/control"); // Subscribing untuk menerima kontrol
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  dht.begin();
  
  pinMode(LED_HI_PIN, OUTPUT); // LED Hijau
  pinMode(LED_YE_PIN, OUTPUT); // LED Kuning
  pinMode(LED_RE_PIN, OUTPUT); // LED Merah
  pinMode(RELAY_PIN, OUTPUT);  // Pompa (Relay)
  pinMode(BUZZER_PIN, OUTPUT); // Buzzer
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Membaca suhu dan kelembapan dari sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Mengecek jika pembacaan gagal
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Gagal membaca data dari sensor DHT!");
    return;
  }

  // Menampilkan data suhu dan kelembapan di Serial Monitor
  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.print("°C  Kelembapan: ");
  Serial.print(humidity);
  Serial.println("%");

  // Menyalakan LED dan Buzzer berdasarkan suhu
  if (temperature > 35) {
    digitalWrite(LED_RE_PIN, HIGH);   // LED Merah menyala
    digitalWrite(LED_YE_PIN, LOW);    // LED Kuning mati
    digitalWrite(LED_HI_PIN, LOW);    // LED Hijau mati
    digitalWrite(RELAY_PIN, HIGH);    // Menyalakan pompa
    digitalWrite(BUZZER_PIN, HIGH);   // Menyalakan buzzer
    Serial.println("Peringatan: Suhu di atas 35°C! Pompa MENYALA, LED Merah & Buzzer MENYALA");
  } else if (temperature >= 30 && temperature <= 35) {
    digitalWrite(LED_RE_PIN, LOW);    // LED Merah mati
    digitalWrite(LED_YE_PIN, HIGH);   // LED Kuning menyala
    digitalWrite(LED_HI_PIN, LOW);    // LED Hijau mati
    digitalWrite(RELAY_PIN, LOW);     // Pompa mati
    digitalWrite(BUZZER_PIN, LOW);    // Buzzer mati
    Serial.println("Suhu antara 30°C hingga 35°C. LED Kuning MENYALA");
  } else {
    digitalWrite(LED_RE_PIN, LOW);    // LED Merah mati
    digitalWrite(LED_YE_PIN, LOW);    // LED Kuning mati
    digitalWrite(LED_HI_PIN, HIGH);   // LED Hijau menyala
    digitalWrite(RELAY_PIN, LOW);     // Pompa mati
    digitalWrite(BUZZER_PIN, LOW);    // Buzzer mati
    Serial.println("Suhu di bawah 30°C. LED Hijau MENYALA");
  }

  // Mengirim data suhu dan kelembapan ke broker MQTT
  String payload = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + "}";
  client.publish("hydroponic/data", payload.c_str());

  delay(2000); // Delay 2 detik sebelum membaca data lagi
}