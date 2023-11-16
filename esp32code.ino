#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <TinyGPS++.h>
#include <NewPing.h>
#include <ArduinoJson.h>

const char* ssid = "GalaxyJv";
const char* password = "facil123";
const char* googleMapsApiKey = "AIzaSyBEp6DFtBPKEUeBYKYPWMnjm9zvneqFfPw";
const int obstacleThreshold = 30; // Umbral de distancia en centímetros
static const int RXPin = 4, TXPin = 2;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
HardwareSerial ss(1); // Usamos Serial1 para RX y TX en el ESP32

const int trigPin = 22;
const int echoPin = 23;
NewPing sonar(trigPin, echoPin);

AsyncWebServer server(80);

double gpsLatitude = 0.0;
double gpsLongitude = 0.0;
unsigned int distance = 0;

const int buzzerPin = 13;  // Elige el pin del buzzer

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");

  // Imprimir la dirección IP en el Monitor Serie
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // Rutas de los recursos del servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Find me!</title>";
    html += "<style>body { font-family: 'Arial', sans-serif; margin: 0; padding: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; background-color: #f0f0f0; background-image: url('https://wallpapercosmos.com/w/full/a/e/1/257456-3840x2160-desktop-4k-global-map-wallpaper-photo.jpg'); background-size: cover; }";
    html += "h1 { color: #FEF9F9; margin-bottom: 20px; }";
    html += "#map { height: 300px; width: 80%; border: 2px solid #ddd; margin-bottom: 20px; border-radius: 8px; }";
    html += "p { color: #FEF9F9; margin: 10px 0; }";
    html += "</style>";
    html += "<script src='https://maps.googleapis.com/maps/api/js?key=" + String(googleMapsApiKey) + "&callback=initMap' async defer></script>";
    html += "</head><body onload='initMap()'>";
    html += "<h1>Find me!</h1>";
    html += "<div id='map'></div>";
    html += "<p id='gps'>Coordenadas GPS: </p>";
    html += "<p id='distance'>Distancia del sensor ultrasónico: </p>";
    html += "<p id='buzzer-status'>Estado del Buzzer: INACTIVO</p>";
    html += "<script>";
    html += "var map;";
    html += "var marker;";
    html += "function initMap() {";
    html += "  map = new google.maps.Map(document.getElementById('map'), { center: { lat: 0, lng: 0 }, zoom: 16 });";
    html += "  marker = new google.maps.Marker({ position: { lat: 0, lng: 0 }, map: map });";
    html += "}";
    html += "setInterval(function() {";
    html += "  fetch('/data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('gps').innerText = 'Coordenadas GPS: ' + data.gps;";
    html += "      document.getElementById('distance').innerText = 'Distancia del sensor ultrasónico: ' + data.distance + ' cm';";
    html += "      document.getElementById('buzzer-status').innerText = 'Estado del Buzzer: ' + (data.distance < 30 ? 'ACTIVO' : 'INACTIVO');";
    html += "      var coords = data.gps.split(',');";
    html += "      var latLng = new google.maps.LatLng(parseFloat(coords[0]), parseFloat(coords[1]));";
    html += "      marker.setPosition(latLng);";
    html += "      map.setCenter(latLng);";
    html += "    });";
    html += "}, 1000);";
    html += "</script>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Ruta para obtener datos en formato JSON
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"gps\": \"" + String(gpsLatitude, 6) + ", " + String(gpsLongitude, 6) + "\",";
    json += "\"distance\": " + String(getDistance());
    json += "}";
    request->send(200, "application/json", json);
  });

  // Inicia el servidor web
  server.begin();

  Serial.println(F("Servidor web iniciado"));
}

void loop() {
  smartDelay(100);
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);

  gpsLatitude = gps.location.lat();
  gpsLongitude = gps.location.lng();
  distance = getDistance();

  if (distance < obstacleThreshold) {
    // Si la distancia es menor de 10 cm, activar el buzzer
    digitalWrite(buzzerPin, HIGH);
  } else {
    // Si la distancia es mayor o igual a 10 cm, apagar el buzzer
    digitalWrite(buzzerPin, LOW);
  }
}

unsigned int getDistance() {
  unsigned int duration = sonar.ping_median(5);
  return duration / US_ROUNDTRIP_CM;
}
