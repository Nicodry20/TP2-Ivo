#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Configura el DHT11
#define DHTPIN 4 // Pin conectado al DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configura tu red WiFi
const char* ssid = "IoTB";
const char* password = "inventaronelVAR";

// Configura la API con tu clave
const String apiEndpoint = "http://api.weatherapi.com/v1/current.json?key=d283ca83f3f8406da95131506241909=Buenos Aires";

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Conéctate a la red WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conectado a WiFi");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error al conectar a WiFi");
  }

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Lee la temperatura y humedad del DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error al leer el DHT11");
    return;
  }

  // Consulta la API
  HTTPClient http;
  http.begin(apiEndpoint);
  int httpCode = http.GET();

  String apiResponse;
  if (httpCode > 0) {
    apiResponse = http.getString();
    Serial.println("Respuesta de la API:");
    Serial.println(apiResponse); // Imprime la respuesta completa para inspección
  } else {
    Serial.println("Error al consultar la API");
  }
  http.end();

  // Parseo de JSON
  String apiTemp = "Desconocida";
  String apiHumidity = "Desconocida";

  DynamicJsonDocument doc(2048); // Aumentado el tamaño del buffer para JSON
  DeserializationError error = deserializeJson(doc, apiResponse);

  if (!error) {
    JsonObject current = doc["current"];
    if (current.containsKey("temp_c")) {
      apiTemp = current["temp_c"].as<String>();
    }
    if (current.containsKey("humidity")) {
      apiHumidity = current["humidity"].as<String>();
    }
  } else {
    Serial.println("Error al parsear JSON:");
    Serial.println(error.c_str());
  }

  // Responde a la solicitud HTTP
  String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n";
  response += "<!DOCTYPE HTML><html>";
  response += "<h1>Datos del DHT11</h1>";
  response += "<p>Temperatura: " + String(t) + " °C</p>";
  response += "<p>Humedad: " + String(h) + " %</p>";
  
  response += "<h1>Clima en Buenos Aires</h1>";
  response += "<p>Temperatura: " + apiTemp + " °C</p>";
  response += "<p>Humedad: " + apiHumidity + " %</p>";

  response += "</html>";

  client.print(response);
  delay(10000); // Actualiza cada 10 segundos
}
