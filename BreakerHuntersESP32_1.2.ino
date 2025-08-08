/*
  W5500 é o modulo ethernet, necessario para comunicacao com leitor qrcode
  Pinagem: SCLK=21, MISO=19, MOSI=18, CS=W5500_CS(22), RST=W5500_RST(23)

  Rele octoacoplador 2 canais
  Leitor Keyence XR100W(?) alimentação 24V
  Sinaleiro alimentação 24V

*/



// ==== Bibliotecas ====
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Ethernet.h>
#include "secrets.h"

// === Pinos ===
#define W5500_CS 22       // Chip Select do ethernet (W5500)
#define W5500_RST 23      // Reset do W5500
const int relayPin = 13;  // Pino rele
const int lampPin = 14;   // Pino lampada

WiFiServer wifiServer(80);                 // Porta para conexões WiFi (HTTP)
WiFiClient wifiFacility;                   // Utilizado para GET

// === Configuração Ethernet ===

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Endereço físico do modulo
IPAddress ethIP(192, 168, 0, 20);                     // IP estático da interface Ethernet
EthernetServer ethServer(9004);                       // Porta para conexões Ethernet

// === Variáveis de controle ===

unsigned long wifiRetryStart = 0;  // Tempo de tentativa de reconexao
unsigned long ethRetryStart = 0;   //

bool wifiRetry = false;  // Tentando reconectar
bool ethRetry = false;   // 
bool newRead = false;    // Nova leitura para checagem

String familyLaser = "";  // Familia recebida na laser
String msgReader = "";    // QRcode lido
String familyRead = "";   // Familia do QRcode lido

// === Funções ===

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(lampPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(relayPin, HIGH); // Rele aciona com LOW
  digitalWrite(lampPin, HIGH);
  digitalWrite(LED_BUILTIN, LOW); // Led azul interno aciona com LOW

  initNetworks();
}

void initNetworks() {
  // --- Wi-Fi ---
  Serial.print("Conectando ao Wi-Fi");
  WiFi.disconnect(true);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("\nFalha ao configurar IP estático Wi-Fi");
  }
  WiFi.begin(ssid, password);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nWi-Fi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nWi-Fi não conectado");
  }
  wifiServer.begin();
  Serial.println("Servidor Wi-Fi iniciado");

  // --- Ethernet ---
  resetW5500();  // forca reinicio
  SPI.begin(21, 19, 18, W5500_CS);
  Ethernet.init(W5500_CS);
  Ethernet.begin(mac, ethIP);
  delay(500);
  if (Ethernet.localIP()[0] != 0) {
    Serial.printf("Ethernet OK. IP: %s\n", Ethernet.localIP().toString().c_str());
    ethServer.begin();
  } else {
    Serial.println("Falha ao iniciar Ethernet");
  }
}

void WifiCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    if (!wifiRetry) {
      Serial.println("WiFi desconectado. Tentando reconectar...");
      WiFi.disconnect(true);
      WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
      WiFi.begin(ssid, password);
      wifiRetryStart = millis();
      wifiRetry = true;
    }

    if (millis() - wifiRetryStart > 5000) {
      Serial.println("Falha na reconexão WiFi");
      wifiRetry = false;
      Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    }
    return;
  }

  if (wifiRetry) {
    Serial.printf("WiFi reconectado com sucesso! IP: %s\n", WiFi.localIP().toString().c_str());
    wifiRetry = false;
    if (!wifiServer) {
      wifiServer.begin();
      Serial.println("Servidor Wi-Fi iniciado após reconexão");
    }
  }
}

void EthCheck() {
  if (Ethernet.localIP()[0] == 0) {
    if (!ethRetry) {
      Serial.println("Sem IP Ethernet. Iniciando tentativa de reconexão...");
      ethRetryStart = millis();
      ethRetry = true;
      ethernetInit();  // Tenta reinicializar imediatamente
    } else if (millis() - ethRetryStart > 2000) {
      Serial.println("Nova tentativa de reinicialização Ethernet...");
      ethernetInit();
      ethRetryStart = millis();  // Atualiza tempo para nova tentativa
    }
  } else {
    ethRetry = false;  // Reset flag se Ethernet voltou
  }
}

void ethernetInit() {
  resetW5500();

  SPI.begin(21, 19, 18, W5500_CS);  // SCLK=21, MISO=19, MOSI=18, SS=W5500_CS
  Ethernet.init(W5500_CS);

  Ethernet.begin(mac, ethIP);
  delay(500);

  if (Ethernet.localIP()[0] != 0) {
    Serial.printf("Ethernet reconectada. IP: %s\n", Ethernet.localIP().toString().c_str());
    ethServer.begin();  // reinicia servidor se necessário
  } else {
    Serial.println("Falha ao reconectar Ethernet");
  }
}

void resetW5500() {
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(300);
  digitalWrite(W5500_RST, HIGH);
  delay(300);
}

void Laser() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client = wifiServer.available();
  if (client) {
    Serial.println("\nCliente conectado via WiFi");
    unsigned long startTime = millis();
    String request = "";
    while (client.connected() && !request.endsWith("\r\n\r\n")) {
      if (client.available()) {
        request += (char)client.read();
      }
      if (millis() - startTime > 3000) {
        Serial.println("Timeout aguardando fim do cabeçalho");
        Serial.println("request: " + request);
        client.stop();
        return;
      }
    }

    int pos = request.indexOf("familia=");
    if (pos != -1) {
      String msg = request.substring(pos + 8);
      int fim = msg.indexOf(' ');
      if (fim != -1) familyLaser = msg.substring(0, fim);
      Serial.println("familia (WiFi): " + familyLaser);
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Produto " + familyLaser + " carregado com sucesso!");
    client.stop();

    Serial.println("Cliente desconectado WiFi");
  }
}

void Reader() {
  if (Ethernet.localIP()[0] == 0) return;

  EthernetClient client = ethServer.available();
  if (client) {
    Serial.println("\nCliente conectado via Ethernet");
    String request = "";
    unsigned long startTime = millis();
    while (client.connected() && millis() - startTime < 2000) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\r') {
          break;  // fim da mensagem detectado
        }
      }
    }

    Serial.println("Dados recebidos brutos:");
    Serial.println(request);
    request.trim();       // Remove \r se estiver no começo e no fim
    msgReader = request;  //

    // Pegar apenas o que vem antes do ponto
    int dot = request.indexOf('.');
    if (dot != -1) {
      familyRead = request.substring(0, dot);
    } else {
      familyRead = request;
      Serial.println("Ponto não encontrado no dado recebido.");
    }
    Serial.println("familyRead: " + familyRead);
    newRead = true;

    // Envia resposta padrão
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println();
    client.println("OK");

    client.stop();
    Serial.println("Cliente desconectado Ethernet");
  }
}

void Relay() {
  if (newRead) {
    Serial.printf("familyRead %s e familyLaser %s\n", familyRead.c_str(), familyLaser.c_str());
    if (familyRead != "" && familyRead != familyLaser) {
      digitalWrite(relayPin, LOW);
      digitalWrite(lampPin, LOW);
      Serial.println("Rele ligado - valores diferentes");
    } else {
      digitalWrite(relayPin, HIGH);
      digitalWrite(lampPin, HIGH);
      Serial.println("Rele desligado - valores iguais");
    }
    newRead = false;
  }
}

void loop() {
  WifiCheck();
  Laser();
  EthCheck();
  Reader();
  Relay();
}
