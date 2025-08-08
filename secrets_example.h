#pragma once

// WiFi
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// API
const char* usuario = "SEU_USUARIO_API";
const char* senha = "SUA_SENHA_API";
String encodedAuth ="SUA_AUTENTIFICACAO_CODIFICADA"

// URL base da API
String urlBase = "https://seuservidor.com/api/?leitura=";

// IP Estático (opcional)
#include <IPAddress.h>  // Importante manter aqui
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
