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
