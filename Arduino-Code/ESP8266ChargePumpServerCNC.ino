#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

enum RelayState
{
  ON,
  OFF
};

AsyncWebServer server(80);
const char *ssid = "CHANGE ME";
const char *password = "CHANGE ME";

// Change values according to your needs.
const byte MEASURE_PIN = 4;
const byte RELAY_PIN = 5;
const int THRESHOLD_FREQ = 25000;
const int MAX_DEVIATION = 5000;
const int MIN_REPS = 2;
const int TIMEOUT = 250;

RelayState currentRelayState = RelayState::OFF;
int reps = 0;
long currentFreq = 0;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  pinMode(MEASURE_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  // Change the relay state according to your needs.
  digitalWrite(RELAY_PIN, LOW);
  currentRelayState = RelayState::OFF;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected with SSID: ");
  Serial.println(ssid);
  Serial.print("Device ip-address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    String html = "<!DOCTYPE html><html><head><title>Mach3 Safety Charge Pump Server ESP8266</title>";
    html += "<meta http-equiv='refresh' content='1'>";   //refresh page after every 1 sec
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<style>";
    html += "body { font-family: 'Arial', sans-serif; margin: 0; padding: 0; background-color: #f4f4f4; color: #333; }";
    html += "h1 { background-color: #4CAF50; color: white; text-align: center; padding: 20px 0; margin: 0; }";
    html += ".container { padding: 20px; display: flex; justify-content: center; flex-wrap: wrap; }";
    html += ".card { background-color: white; box-shadow: 0 4px 8px 0 rgba(0,0,0,0.2); margin: 10px; padding: 20px; flex-basis: 300px; }";
    html += ".sensor, .relay { font-size: 20px; text-align: center; }";
    html += ".relay { color: " + String(currentRelayState == RelayState::ON ? "green" : "red") + "; }";
    html += "</style></head><body>";
    html += "<h1>Mach3 Safety Charge Pump Server ESP8266</h1>";
    html += "<div class='container'>";
    html += "<div class='card'><p class='sensor'>Current frequency: " + String(currentFreq) + " Hz</p></div>";
    html += "<div class='card'><p class='relay'>Relay: " +  String(currentRelayState == RelayState::ON ? "ON" : "OFF") + "</p></div>";
    html += "</div></body></html>";
    request->send(200, "text/html", html); 
  });

  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("Server started");
}

void loop()
{
  AsyncElegantOTA.loop();
  currentFreq = getFrequency(MEASURE_PIN);

  if (currentFreq > (THRESHOLD_FREQ - MAX_DEVIATION) && currentFreq < (THRESHOLD_FREQ + MAX_DEVIATION))
  {
    reps++;
    if (reps >= MIN_REPS)
    {
      // Change the relay state according to your needs.
      digitalWrite(RELAY_PIN, HIGH);
      currentRelayState = RelayState::ON;
    }
  }
  else
  {
    reps = 0;
    // Change the relay state according to your needs.
    digitalWrite(RELAY_PIN, LOW);
    currentRelayState = RelayState::OFF;
  }
}

long getFrequency(int pin)
{
  // reduce the number of samples for faster frequency read if needed
  int samples = 4096;
  long totalPulseTime = 0;
  long pulseTime;
  int validSamples = 0;

  for (int j = 0; j < samples; j++)
  {
    pulseTime = pulseIn(pin, HIGH, TIMEOUT);
    if (pulseTime > 0)
    {
      totalPulseTime += pulseTime;
      validSamples++;
    }
  }

  if (validSamples == 0)
  {
    return 0;
  }

  long averagePulseTime = totalPulseTime / validSamples;
  long frequency = 1000000L / averagePulseTime;
  return frequency;
}
