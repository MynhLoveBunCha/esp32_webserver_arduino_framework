#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

// Replace with your network credentials
const char *ssid = "MyESP32";
const char *password = "123456789";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Configure IP addresses of the local access point
IPAddress local_IP(192, 168, 1, 25);
IPAddress gateway(192, 168, 1, 2);
IPAddress subnet(255, 255, 255, 0);

// variables
static int count = 0;
const int sensor_pin1 = 25;
const int sensor_pin2 = 26;
volatile bool ir1 = false;
volatile bool ir2 = false;
bool hit1 = true;
bool hit2 = true;
int state = 1;
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
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 Server</h2>
  <p>
    <i class="fas fa-cat"></i>
    <span class="object_count">Count</span> 
    <span id="count">%COUNT%</span>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("count").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/count", true);
  xhttp.send();
}, 10 ) ;
</script>
</html>)rawliteral";

// interupts handlers
void IRAM_ATTR ISR1()
{
    int check1 = digitalRead(sensor_pin1);
    if (check1 == 0)
    {
        ir1 = true;
    }
    else
    {
        ir1 = false;
    }
}

void IRAM_ATTR ISR2()
{
    int check2 = digitalRead(sensor_pin2);
    if (check2 == 0)
    {
        ir2 = true;
    }
    else
    {
        ir2 = false;
    }
}

String update_count()
{
    if (ir1 && (state == 1) && hit1)
    {
        state++;
        hit1 = false;
    }
    else if (ir2 && (state == 2) && hit2)
    {
        count++;
        state = 1;
        hit2 = false;
    }
    if (digitalRead(sensor_pin1))
    {
        hit1 = true;
    }
    if (digitalRead(sensor_pin2))
    {
        hit2 = true;
    }
    return String(count);
}

// Replaces placeholder with count values
String processor(const String &var)
{
    // Serial.println(var);
    if (var == "COUNT")
    {
        return update_count();
    }
    return String();
}

void setup()
{
    // pin mode
    pinMode(sensor_pin1, INPUT_PULLUP);
    pinMode(sensor_pin2, INPUT_PULLUP);

    // init serial port for debugging
    Serial.begin(115200);

    Serial.print("Setting up Access Point ... ");
    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

    Serial.print("Starting Access Point ... ");
    Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");

    Serial.print("IP address = ");
    Serial.println(WiFi.softAPIP());

    // interupts
    attachInterrupt(digitalPinToInterrupt(sensor_pin1), ISR1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(sensor_pin2), ISR2, CHANGE);

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html, processor); });
    server.on("/count", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", update_count().c_str()); });

    // Start server
    server.begin();
}

void loop()
{
    // put your main code here, to run repeatedly:
}
