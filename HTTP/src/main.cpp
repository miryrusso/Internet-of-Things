/* Il dispositivo si connette a una rete Wi-Fi specificata e avvia un server web sulla porta 80.
    Il server web gestisce tre endpoint:
    /: Risponde a richieste GET sull'indirizzo radice restituendo "Hello, world" come risposta in formato testo.
    /led: Gestisce richieste GET sull'endpoint "/led". Se la richiesta include un parametro "status" con valori "on", "off" o "toggle", 
    cambia lo stato del LED incorporato sulla scheda Arduino in base al valore ricevuto. Se non viene fornito alcun parametro "status" 
    nella richiesta, restituirà "che faccio?" in formato testo.
    notFound: Gestisce le richieste a endpoint non esistenti, restituendo un messaggio di "Not found" con codice di stato 404.
    
    Il LED incorporato sulla scheda Arduino viene controllato attraverso richieste HTTP GET all'endpoint "/led" con i parametri 
    "status=on", "status=off" o "status=toggle". Se il parametro "status" non viene fornito o se è diverso da questi valori, 
    il server restituirà un messaggio di errore.
    In sostanza, questo codice permette di controllare il LED della scheda Arduino via web attraverso richieste GET inviando 
    comandi specifici per accendere, spegnere o invertire lo stato del LED.
*/
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
const char *ssid = "NomeRete";
const char *password = "Password";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello, world"); });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        String status;
        if (request->hasParam("status")) {
            status = request->getParam("status")->value();
            if(status == "on"){
              digitalWrite(LED_BUILTIN,HIGH);
            }
            else if(status=="off"){
              digitalWrite(LED_BUILTIN,LOW);
            }
            else if(status=="toggle"){
              digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));
            }
        }
        else{
            request->send(401, "text/plain", "che faccio?");
        }
        request->send(200, "text/plain", status); });

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{
}
