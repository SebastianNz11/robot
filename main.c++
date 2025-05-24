#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <ArduinoHttpClient.h>

const char* ssid = "";
const char* password = "";
const char* host = "";
const int port = 443;

WiFiSSLClient sslClient;
HttpClient client(sslClient, host, port);

const int lm35Pin = A0;

const int IN1 = 5;
const int IN2 = 6;
const int IN3 = 9;
const int IN4 = 10;

const int buzzerPin = 7;

const int velocidadIzquierda = 120;
const int velocidadDerecha = 120;

unsigned long tiempoUltimaEnvio = 0;
const unsigned long intervaloEnvio = 5000;

void setup() {
  Serial.begin(9600);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  conectarWifi();
}

unsigned long tiempoUltimaConsulta = 0;
const unsigned long intervaloConsulta = 500;

void loop() {
  unsigned long ahora = millis();

  if (ahora - tiempoUltimaEnvio > intervaloEnvio) {
    enviarTemperatura();
    tiempoUltimaEnvio = ahora;
  }

  if (ahora - tiempoUltimaConsulta > intervaloConsulta) {
    consultarComando();
    tiempoUltimaConsulta = ahora;
  }
}

void conectarWifi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  int intentos = 0;
  while ((WiFi.status() != WL_CONNECTED || WiFi.localIP() == INADDR_NONE) && intentos < 30) {
    Serial.print(".");
    delay(1000);
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != INADDR_NONE) {
    Serial.println("\nConectado a WiFi");
    Serial.print("IP asignada: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo se pudo conectar a WiFi o IP inválida.");
    while (true)
      ;
  }
}

void enviarTemperatura() {
  float temp = leerTemperatura();
  String contenido = "{\"valor\":" + String(temp, 1) + "}";
  if (temp >= 30.0) {
    tone(buzzerPin, 1200, 800);
  } else {
    noTone(buzzerPin);
  }

  client.beginRequest();
  client.post("/temperatura");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", contenido.length());
  client.beginBody();
  client.print(contenido);
  client.endRequest();

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Temperatura enviada: ");
  Serial.println(temp);
  Serial.print("Respuesta: ");
  Serial.println(response);
}

String ultimoComando = "";

void consultarComando() {
  client.get("/leer_comando");

  int statusCode = client.responseStatusCode();
  String respuesta = client.responseBody();

  if (statusCode == 200) {
    respuesta.trim();

    if (respuesta == "izquierda" || respuesta == "derecha") {
      Serial.println("Comando recibido: [" + respuesta + "]");
      moverRobot(respuesta);
    } else {
      if (respuesta != ultimoComando) {
        Serial.println("Comando recibido: [" + respuesta + "]");
        moverRobot(respuesta);
        ultimoComando = respuesta;
      }
    }
  } else {
    Serial.print("Error al consultar comando. Código HTTP: ");
    Serial.println(statusCode);
  }
}

float leerTemperatura() {
  const int numLecturas = 10;
  long sumaLecturas = 0;

  for (int i = 0; i < numLecturas; i++) {
    sumaLecturas += analogRead(lm35Pin);
    delay(5);
  }

  float lecturaPromedio = sumaLecturas / (float)numLecturas;
  float voltaje = lecturaPromedio * (5.0 / 1023.0);

  Serial.print("Lectura analógica promedio: ");
  Serial.println(lecturaPromedio);
  Serial.print("Voltaje promedio: ");
  Serial.println(voltaje, 3);

  float temperatura = 60.0 - (voltaje * 100.0);

  Serial.print("Temperatura calculada (invertida): ");
  Serial.println(temperatura);
  return temperatura;
}

void moverRobot(String comando) {
  if (comando == "adelante") {
    analogWrite(IN1, velocidadIzquierda);
    analogWrite(IN2, 0);
    analogWrite(IN3, velocidadDerecha);
    analogWrite(IN4, 0);
  } else if (comando == "atras") {
    analogWrite(IN1, 0);
    analogWrite(IN2, velocidadIzquierda);
    analogWrite(IN3, 0);
    analogWrite(IN4, velocidadDerecha);
  } else if (comando == "izquierda") {
    analogWrite(IN1, velocidadIzquierda);
    analogWrite(IN2, 0);
    analogWrite(IN3, 0);
    analogWrite(IN4, velocidadDerecha);
    delay(500);
    detenerMotores();
  } else if (comando == "derecha") {
    analogWrite(IN1, 0);
    analogWrite(IN2, velocidadIzquierda);
    analogWrite(IN3, velocidadDerecha);
    analogWrite(IN4, 0);
    delay(500);
    detenerMotores();
  } else {
    detenerMotores();
  }
}

void detenerMotores() {
  analogWrite(IN1, 0);
  analogWrite(IN2, 0);
  analogWrite(IN3, 0);
  analogWrite(IN4, 0);
}