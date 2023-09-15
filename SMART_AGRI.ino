#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h> // Include the Servo library
#define DHTPIN 14 // Pin D14 connected to the DHT sensor
#define DHTTYPE DHT11 // DHT sensor type
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
Servo servo; // Create a servo object
// WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_password";
// Pin definitions for L298N motor driver
const int motor1In1Pin = 26;
const int motor1In2Pin = 25;
const int motor2In1Pin = 33;
const int motor2In2Pin = 32;
// Ultrasonic sensor pin definitions
const int trigPin = 13;
const int echoPin = 12;
// Soil moisture sensor pin definition
const int sensorPin = 34; // Soil moisture sensor O/P pin
// HTML page content
const char* htmlPage = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
.arrows {
font-size: 70px;
color: black;
position: center;
}
.stopButton {
font-size: 25px;
position: center;
color: black;
}
.button {
width: 100px;
height: 100px;
font-size: 20px;
margin: 10px;
border-radius: 25%;
}
.distance {
color: lightgreen;
font-size: 30px;
text-align: center;
margin-bottom: 10px;
}
.obstacle {
color:red;
font-size: 34px;
text-align: center;
margin-top: 20px;
}
</style>
<script>
function updateValues(humidity, temperature, moisture) {
document.getElementById("humidityValue").textContent = humidity;
document.getElementById("temperatureValue").textContent = temperature;
document.getElementById("moistureValue").textContent = moisture;
}
function move(direction) {
var xhttp = new XMLHttpRequest();
xhttp.open("GET", "/move?dir=" + direction, true);
xhttp.send();
}
function updateDistance(distance) {
var distanceElement = document.getElementById("distance");
var obstacleTextElement = document.getElementById("obstacleText");
distanceElement.textContent = distance + " cm";
if (distance < 10) {
obstacleTextElement.style.display = "block";
} else {
obstacleTextElement.style.display = "none";
}
}
function rotateServo() {
var xhttp = new XMLHttpRequest();
xhttp.open("GET", "/rotate", true);
xhttp.send();
}
setInterval(function() {
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
if (this.readyState == 4 && this.status == 200) {
var data = JSON.parse(this.responseText);
updateValues(data.humidity, data.temperature, data.moisture);
updateDistance(data.distance);
}
};
xhttp.open("GET", "/data", true);
xhttp.send();
}, 1000);
</script>
</head>
<body class="noselect" align="center" style="background-color:black">
<h1 style="color: white; text-align: center;">Control Your Car!!</h1>
<p style="color: black; text-align: center;">Humidity: <span id="humidityValue">-
</span>%</p>
<p style="color: black; text-align: center;">Temperature: <span id="temperatureValue">-
</span>°C</p>
<p style="color: black; text-align: center;">Moisture: <span id="moistureValue">-
</span>%</p>
<p class="distance">Distance: <span id="distance">-</span></p>
<p class="obstacle" id="obstacleText" style="display: none;">Obstacle Detected</p>
<button class="button" onclick="move('forward')"><span
class="arrows">&#8679;</span></button>
<br>
<button class="button" onclick="move('left')"><span
class="arrows">&#8678;</span></button>
<button class="button" onclick="move('stop')"><span
class="stopButton">STOP</span></button>
<button class="button" onclick="move('right')"><span
class="arrows">&#8680;</span></button>
<br>
<button class="button" onclick="move('backward')"><span
class="arrows">&#8681;</span></button>
<br>
<button class="button" onclick="rotateServo()">Moist</button> <!-- New button for servo
control -->
</body>
</html>
)HTML";
AsyncWebServer server(80);
String currentDirection = "stop";
void setup() {
Serial.begin(115200);
WiFi.softAP(ssid, password);
IPAddress IP = WiFi.softAPIP();
Serial.print("AP IP address: ");
Serial.println(IP);
pinMode(motor1In1Pin, OUTPUT);
pinMode(motor1In2Pin, OUTPUT);
pinMode(motor2In1Pin, OUTPUT);
pinMode(motor2In2Pin, OUTPUT);
pinMode(trigPin, OUTPUT);
pinMode(echoPin, INPUT);
dht.begin();
servo.attach(27); // Attach the servo to pin 4
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
request->send(200, "text/html", htmlPage);
});
server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request){
String direction = request->getParam("dir")->value();
// Update currentDirection only if the direction is valid
if (direction == "forward" || direction == "backward" || direction == "left" || direction ==
"right" || direction == "stop") {
currentDirection = direction;
}
request->send(200, "text/plain", "OK");
});
server.on("/rotate", HTTP_GET, [](AsyncWebServerRequest *request){
rotateServo(); // Call the rotateServo function
request->send(200, "text/plain", "OK");
});
server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
float humidity = dht.readHumidity(); // Read humidity value
float temperature = dht.readTemperature(); // Read temperature value in Celsius
int moisture = getMoisture();
if (isnan(humidity) || isnan(temperature)) {
Serial.println("Failed to read from DHT sensor!");
return;
}
StaticJsonDocument<200> jsonDocument;
jsonDocument["humidity"] = humidity;
jsonDocument["temperature"] = temperature;
jsonDocument["moisture"] = moisture;
jsonDocument["distance"] = getDistance();
String jsonData;
serializeJson(jsonDocument, jsonData);
request->send(200, "application/json", jsonData);
});
server.begin();
}
void loop() {
if (currentDirection == "forward") {
digitalWrite(motor1In1Pin, HIGH);
digitalWrite(motor1In2Pin, LOW);
digitalWrite(motor2In1Pin, HIGH);
digitalWrite(motor2In2Pin, LOW);
} else if (currentDirection == "backward") {
digitalWrite(motor1In1Pin, LOW);
digitalWrite(motor1In2Pin, HIGH);
digitalWrite(motor2In1Pin, LOW);
digitalWrite(motor2In2Pin, HIGH);
} else if (currentDirection == "left") {
digitalWrite(motor1In1Pin, LOW);
digitalWrite(motor1In2Pin, HIGH);
digitalWrite(motor2In1Pin, HIGH);
digitalWrite(motor2In2Pin, LOW);
} else if (currentDirection == "right") {
digitalWrite(motor1In1Pin, HIGH);
digitalWrite(motor1In2Pin, LOW);
digitalWrite(motor2In1Pin, LOW);
digitalWrite(motor2In2Pin, HIGH);
} else if (currentDirection == "stop") {
digitalWrite(motor1In1Pin, LOW);
digitalWrite(motor1In2Pin, LOW);
digitalWrite(motor2In1Pin, LOW);
digitalWrite(motor2In2Pin, LOW);
}
// Rest of the code...
float humidity = dht.readHumidity(); // Read humidity value
float temperature = dht.readTemperature(); // Read temperature value in Celsius
int moisture = getMoisture();
Serial.print("Humidity: ");
Serial.print(humidity);
Serial.print("%, Temperature: ");
Serial.print(temperature);
Serial.print("°C, Moisture: ");
Serial.print(moisture);
Serial.println("%");
// Delay before next reading
delay(1000);
}
int getMoisture() {
int sensor_analog = analogRead(sensorPin);
int moisture = 100 - ((sensor_analog / 4095.0) * 100);
return moisture;
}
long getDistance() {
long duration, distance;
digitalWrite(trigPin, LOW);
delayMicroseconds(2);
digitalWrite(trigPin, HIGH);
delayMicroseconds(10);
digitalWrite(trigPin, LOW);
duration = pulseIn(echoPin, HIGH);
distance = duration * 0.034 / 2;
if (distance < 10) {
currentDirection = "stop";
}
return distance;
}
void rotateServo() {
servo.write(120); // Rotate servo to 120 degrees
delay(1000); // Wait for 5 seconds
servo.write(0); // Rotate servo back to 0 degrees
}