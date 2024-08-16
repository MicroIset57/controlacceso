#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "DHTesp.h"
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>

const char* ssid = "ISET57CLARO";  // Cambia esto por el nombre de tu red Wi-Fi
const char* password = "GONZALO1981";  // Cambia esto por la contraseña de tu red Wi-Fi

AsyncWebServer server(80);
DHTesp dht;
float humidity =0;
float temperature=0;

void NoExiste404(AsyncWebServerRequest *request) {
    request->send(200, "text", "404 no ta");
}
void PaginaAyuda(AsyncWebServerRequest * request){
   request->send(200, "text/html", R"(<h1>Pedir ayuda</h1><img src="http://www.larevista.com.ec/sites/default/files/resize/8-PSICOLOGIA-PEDIR-AYUDA%2Cphoto01-275x191.jpg" />)");
}

void PaginaRaiz(AsyncWebServerRequest * request){
    // imprimo todos los valores del header:
    for (int i=0; i <request->headers(); i++) {
      Serial.printf("Header %s\n", request->header(i).c_str());
    }

    String estado = "";

    if(request->hasArg("ESTADO")){
        if(digitalRead(23)==0)   
          estado = "boton pulsado";
        else
          estado = "boton no pulsado";
    }

    //identificar parametros:
    if(request->hasArg("LED")){
      if(request->arg("LED") == "1"){
        digitalWrite (21, HIGH);
        Serial.println("Prender");
      } else if(request->arg("LED") == "0"){
        digitalWrite (21, LOW);
        Serial.println("Apagar");
      }
    }

    String str="<h1>Bienvenido al servidor ESP32</h1>"
    "<a href=\"/?LED=1\">Prender</a><br>"
    "<a href=\"/?LED=0\">Apagar</a><br>br>"
    "<a href=\"/?ESTADO=1\">Estado del boton</a><br>" + estado+
    "<h1> TEMP " + String(temperature)+
    " HUMEDAD " + String(humidity)+" %</h1>";

    request->send(200, "text/html", str);
  
}
//IPAddress dns1(8, 8, 8, 8); // Google DNS (dns.google)
//IPAddress dns2(8, 8, 4, 4); // Google DNS (dns.google)

void setup() {
  Serial.begin(115200);
  
  pinMode (23 , INPUT_PULLUP);
  pinMode (21 , OUTPUT);
  //pinMode (34 , INPUT_PULLUP);
  
  WiFi.begin(ssid, password);
  // WiFi.config(INADDR_NONE, INADDR_NONE, dns1, dns2);
  MDNS.begin("micro");


  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red Wi-Fi...");
  }

  Serial.println("Conectado a la red Wi-Fi");
  Serial.println(WiFi.localIP());

  server.onNotFound(NoExiste404);
  server.on("/", HTTP_GET, PaginaRaiz);
 server.on("/ayuda", HTTP_GET, PaginaAyuda);
 
  dht.setup(22, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
  server.begin();

  // IPAddress ip;
  // WiFi.hostByName("www.google.com", ip);
  // Serial.println(ip);
}

void EnviarDatosAThinsgboard(){ // G3iKyKrpB9UtrWL4AmvZ
  const char* host = "thingsboard.cloud";
  String url = "/api/v1/yosoyelsensor/telemetry";

  //curl -v -X POST --data "{"temperature":42,"humidity":73}" http://thingsboard.cloud/api/v1/yosoyelsensor/telemetry --header "Content-Type:application/json"

  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("thingsboard.cloud", 443)) {
    Serial.println("Error al conectar al servidor");
    return;
  }

  String json = "{\"temperature\":"+String(temperature)+",\"humidity\":"+String(humidity)+"}";

  client.print(String("POST /api/v1/yosoyelsensor/telemetry HTTP/1.1\r\n") +
               "Host: thingsboard.cloud\r\n" +
               "Content-Type: application/json\r\n" +
               "Content-Length: " + String(json.length()) + "\r\n" +
               "Connection: close\r\n\r\n" +
               json + "\r\n");
               
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Cabeceras recibidas");
      break;
    }
  }

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  Serial.println("Solicitud HTTPS completada");
}


void loop() {
  delay(5000);

  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  Serial.printf("%f  %f\n", humidity, temperature);

  EnviarDatosAThinsgboard();
}