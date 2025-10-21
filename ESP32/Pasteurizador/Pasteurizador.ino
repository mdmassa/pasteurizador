#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
#else
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#define TEMP_PASTEURIZACAO 65
#define TEMP_REFRIGERACAO 5

#define ONE_WIRE_BUS 5
#define RELE_EBULIDOR 15
#define RELE_MOTOR 19
#define RELE_PELTIER 18

float tempAquecedorMedia = 0;

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

int numberOfDevices;

DeviceAddress tempDeviceAddress; 

String tempAquecedorA = "";
String tempAquecedorB = "";
String tempRefrigerador = "";

unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;

const char* ssid = "Largatixa_2.4";
const char* password = "batatafrita";

const char* mqtt_server = "192.168.100.2";
const int mqtt_port = 1883;
const char* mqtt_user = "massa";
const char* mqtt_pass = "admin";

#define TOPICO_PUB_TEMP_A "pasteurizador/temperatura/aquecedorA"
#define TOPICO_PUB_TEMP_B "pasteurizador/temperatura/aquecedorB"
#define TOPICO_PUB_TEMP_R "pasteurizador/temperatura/refrigerador"
#define TOPICO_PUB_TEMP_MEDIA "pasteurizador/temperatura/media"
#define TOPICO_SUB_RELE_EBULIDOR "pasteurizador/comando/ebulidor"
#define TOPICO_SUB_RELE_PELTIER "pasteurizador/comando/peltier"

AsyncWebServer server(80);
WiFiClient espClient;        
PubSubClient client(espClient);
unsigned long lastMsg = 0;     

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>Painel de Controle</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#F67E7F;"></i> 
    <span class="ds-labels">Aquecedor A</span> 
    <span id="tempaquecedora">%TEMPAQUECEDORA%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#F67E7F;"></i> 
    <span class="ds-labels">Aquecedor B</span>
    <span id="tempaquecedorb">%TEMPAQUECEDORB%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#7BCCF1;"></i> 
    <span class="ds-labels">Refrigerador</span>
    <span id="temprefrigerador">%TEMPREFRIGERADOR%</span>
    <sup class="units">&deg;C</sup>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempaquecedora").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempaquecedora", true);
  xhttp.send();
}, 10000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("tempaquecedorb").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/tempaquecedorb", true);
  xhttp.send();
}, 10000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temprefrigerador").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temprefrigerador", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

String processor(const String& var){
  if (var == "TEMPAQUECEDORA"){
    return tempAquecedorA;
  }
  else if (var == "TEMPAQUECEDORB"){
    return tempAquecedorB;
  } else if (var == "TEMPREFRIGERADOR"){
    return tempRefrigerador;
  }
  return String();
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Mensagem recebida no tópico: ");
  Serial.println(topic);
  Serial.print("Conteúdo: ");
  Serial.println(message);

  if (String(topic) == TOPICO_SUB_RELE_EBULIDOR) {
    if(message == "LIGAR") {
      digitalWrite(RELE_EBULIDOR, HIGH);
      Serial.println("EBULIDOR LIGADO via MQTT");
    }
    else if(message == "DESLIGAR") {
      digitalWrite(RELE_EBULIDOR, LOW);
      Serial.println("EBULIDOR DESLIGADO via MQTT");
    }
  }
  else if (String(topic) == TOPICO_SUB_RELE_PELTIER) {
    if(message == "LIGAR") {
      digitalWrite(RELE_PELTIER, HIGH);
      Serial.println("PELTIER LIGADO via MQTT");
    }
    else if(message == "DESLIGAR") {
      digitalWrite(RELE_PELTIER, LOW);
      Serial.println("PELTIER DESLIGADO via MQTT");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");
    
    if (client.connect("ESP32_Pasteurizador", mqtt_user, mqtt_pass)) {
      Serial.println("Conectado!");
      
      client.subscribe(TOPICO_SUB_RELE_EBULIDOR);
      client.subscribe(TOPICO_SUB_RELE_PELTIER);
      Serial.println("Subscrito nos tópicos de comando");
      
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println();
  sensors.begin();

  pinMode(RELE_EBULIDOR, OUTPUT);
  pinMode(RELE_MOTOR, OUTPUT);
  pinMode(RELE_PELTIER, OUTPUT);

  numberOfDevices = sensors.getDeviceCount();

  Serial.print("Localizando dispositivos...");
  Serial.print("Encontrados ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" dispositivos.");

  for(int i=0;i<numberOfDevices; i++){
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Dispositivo ");
      Serial.print(i, DEC);
      Serial.print(" Endereço: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Dispositivo fantasma encontrado em ");
      Serial.print(i, DEC);
      Serial.println(" - Verifique alimentação e cabos");
    }
  }

  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  Serial.printf("Conectando a: %s\n", ssid);
  WiFi.begin(ssid, password);
  
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 40) {
    Serial.print('.');
    delay(500);
    i++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nNão conectado! Motivos possíveis:");
    Serial.println("- SSID incorreto");
    Serial.println("- Senha incorreta");
    Serial.println("- Sinal muito fraco");
    Serial.println("- Roteador bloqueou dispositivo");
    
    Serial.printf("Status WiFi: %d\n", WiFi.status());
    Serial.println("Reiniciando em 10s...");
    delay(10000);
    ESP.restart();
  }
  
  Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/tempaquecedora", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", tempAquecedorA.c_str());
  });
  server.on("/tempaquecedorb", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", tempAquecedorB.c_str());
  });
  server.on("/temprefrigerador", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", tempRefrigerador.c_str());
  });
  
  server.begin();
}
 
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  sensors.requestTemperatures();
    
  if(sensors.getAddress(tempDeviceAddress, 0)){
    tempAquecedorA = String(sensors.getTempC(tempDeviceAddress));
    Serial.print("Temperatura Aquecedor A: ");
    Serial.print(tempAquecedorA);
    Serial.println(" ºC");
  }

  if(sensors.getAddress(tempDeviceAddress, 1)){
    tempAquecedorB = String(sensors.getTempC(tempDeviceAddress));
    Serial.print("Temperatura Aquecedor B: ");
    Serial.print(tempAquecedorB);
    Serial.println(" ºC");
  }

  if(sensors.getAddress(tempDeviceAddress, 2)){
    tempRefrigerador = String(sensors.getTempC(tempDeviceAddress));
    Serial.print("Temperatura Refrigerador: ");
    Serial.print(tempRefrigerador);
    Serial.println(" ºC");
  }

  tempAquecedorMedia = (tempAquecedorA.toFloat() + tempAquecedorB.toFloat())/2.0;
  Serial.print("Temperatura Media: ");
  Serial.println(tempAquecedorMedia);
    
  if(tempAquecedorMedia < 65.0) {
    digitalWrite(RELE_EBULIDOR, HIGH);
  } else if(tempAquecedorMedia >= 65.0) {
    digitalWrite(RELE_EBULIDOR, LOW);
  }

  unsigned long now = millis();
  if (now - lastMsg > 5000) { 
    lastMsg = now;
    
    client.publish(TOPICO_PUB_TEMP_A, tempAquecedorA.c_str());
    client.publish(TOPICO_PUB_TEMP_B, tempAquecedorB.c_str());
    client.publish(TOPICO_PUB_TEMP_R, tempRefrigerador.c_str());
    
    char tempMediaStr[8];
    dtostrf(tempAquecedorMedia, 1, 2, tempMediaStr);
    client.publish(TOPICO_PUB_TEMP_MEDIA, tempMediaStr);
    
    Serial.println("Dados publicados via MQTT");
  }

  delay(1000);
}
