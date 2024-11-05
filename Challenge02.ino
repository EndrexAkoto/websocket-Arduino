#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>

// Replace with your network credentials
const char* ssid = "ALX The Piano 01";
const char* password = "Thepiano01";

// Initialize the server on port 80 and WebSocket server on port 81
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// GPIO pins for LEDs
const int LED1_PIN = 16;
const int LED2_PIN = 19;

// Variables to store LED states and brightness levels
bool led1State = false;
bool led2State = false;
int led1Brightness = 0; // Range: 0-255
int led2Brightness = 0; // Range: 0-255

// WebSocket event handler
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_DISCONNECTED) {
    Serial.printf("Client %u disconnected\n", num);
  } else if (type == WStype_CONNECTED) {
    Serial.printf("Client %u connected\n", num);
    // Send initial states
    webSocket.sendTXT(num, led1State ? "LED1 ON" : "LED1 OFF");
    webSocket.sendTXT(num, "LED1 Brightness: " + String(led1Brightness));
    webSocket.sendTXT(num, led2State ? "LED2 ON" : "LED2 OFF");
    webSocket.sendTXT(num, "LED2 Brightness: " + String(led2Brightness));
  } else if (type == WStype_TEXT) {
    String request = (char*) payload;
    
    if (request.startsWith("toggleLED1")) {
      led1State = !led1State;
      digitalWrite(LED1_PIN, led1State ? HIGH : LOW);
      webSocket.sendTXT(num, led1State ? "LED1 ON" : "LED1 OFF");
    } else if (request.startsWith("toggleLED2")) {
      led2State = !led2State;
      digitalWrite(LED2_PIN, led2State ? HIGH : LOW);
      webSocket.sendTXT(num, led2State ? "LED2 ON" : "LED2 OFF");
    } else if (request.startsWith("brightness1:")) {
      led1Brightness = request.substring(12).toInt();
      led1Brightness = constrain(led1Brightness, 0, 255); // Ensure brightness is within range
      led1State ? analogWrite(LED1_PIN, led1Brightness) : digitalWrite(LED1_PIN, LOW);
      webSocket.sendTXT(num, "LED1 Brightness: " + String(led1Brightness));
    } else if (request.startsWith("brightness2:")) {
      led2Brightness = request.substring(12).toInt();
      led2Brightness = constrain(led2Brightness, 0, 255); // Ensure brightness is within range
      led2State ? analogWrite(LED2_PIN, led2Brightness) : digitalWrite(LED2_PIN, LOW);
      webSocket.sendTXT(num, "LED2 Brightness: " + String(led2Brightness));
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LEDs as outputs
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // WebSocket server on event callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  // Serve HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          html { font-family: Helvetica; text-align: center; }
          .button { padding: 20px; margin: 10px; font-size: 20px; }
          .on { background-color: green; color: white; }
          .off { background-color: red; color: white; }
        </style>
      </head>
      <body>
        <h1>ESP32 Web Server with WebSocket</h1>
        <p>LED 1 - State: <span id="state1">OFF</span></p>
        <button id="toggle1" class="button off" onclick="toggleLED('toggleLED1')">Toggle LED 1</button>
        <input type="range" id="brightness1" min="0" max="255" value="0" onchange="setBrightness(1)">
        
        <p>LED 2 - State: <span id="state2">OFF</span></p>
        <button id="toggle2" class="button off" onclick="toggleLED('toggleLED2')">Toggle LED 2</button>
        <input type="range" id="brightness2" min="0" max="255" value="0" onchange="setBrightness(2)">
        
        <script>
          var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
          ws.onmessage = function(event) {
            let msg = event.data;
            if (msg === "LED1 ON") {
              document.getElementById("state1").innerText = "ON";
              document.getElementById("toggle1").className = "button on";
            } else if (msg === "LED1 OFF") {
              document.getElementById("state1").innerText = "OFF";
              document.getElementById("toggle1").className = "button off";
            } else if (msg.startsWith("LED1 Brightness:")) {
              // Handle brightness update
              console.log(msg);
            } else if (msg === "LED2 ON") {
              document.getElementById("state2").innerText = "ON";
              document.getElementById("toggle2").className = "button on";
            } else if (msg === "LED2 OFF") {
              document.getElementById("state2").innerText = "OFF";
              document.getElementById("toggle2").className = "button off";
            } else if (msg.startsWith("LED2 Brightness:")) {
              // Handle brightness update
              console.log(msg);
            }
          };
          
          function toggleLED(command) {
            ws.send(command);
          }

          function setBrightness(ledNumber) {
            let brightness = document.getElementById("brightness" + ledNumber).value;
            ws.send("brightness" + ledNumber + ":" + brightness);
          }
        </script>
      </body>
      </html>
    )rawliteral");
  });

  // Start the server
  server.begin();
}

void loop() {
  // Handle WebSocket connections
  webSocket.loop();
}
