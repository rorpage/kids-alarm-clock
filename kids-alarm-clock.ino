#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <Losant.h>

const int screenWidth = 16;
const int screenHeight = 2;

const int secondsBetweenChecks = 60000;

// WiFi credentials.
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

uint8_t degree[8]  = { 140, 146, 146, 140, 128, 128, 128, 128 };

const int LED_PIN = 0;

// Construct an LCD object and pass it the
// I2C address, width (in characters) and
// height (in characters). Depending on the
// Actual device, the IC2 address may change.
LiquidCrystal_I2C lcd(0x3F, screenWidth, screenHeight);

WiFiClientSecure wifiClient;

void setup() {
  Serial.begin(115200);

  // Giving it a little time because the serial monitor doesn't
  // immediately attach. Want the workshop that's running to
  // appear on each upload.
  delay(2000);

  Serial.println();
  Serial.println("Running LCD Kit Firmware.");

  // The begin call takes the width and height. This
  // Should match the number provided to the constructor.
  lcd.begin(screenWidth, screenHeight);
  lcd.init();

  // Turn on the backlight.
  lcd.backlight();

  lcd.createChar(0, degree);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  connect();
}

int timeSinceLastRead = 60001;

void loop() {

  bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  if (timeSinceLastRead > secondsBetweenChecks) {
    getWeatherAndTime();

    timeSinceLastRead = 0;
  }

  delay(1000);
  timeSinceLastRead += 1000;
}

void displayMessage(String message, int row, bool shouldClear) {
  if (shouldClear) {
    lcd.clear();
  }

  lcd.setCursor(0, row);
  lcd.print(message);
}

/**
 * Connect to WiFi and Losant
 */
void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(WIFI_SSID);
      Serial.print("Password: ");
      Serial.println(WIFI_PASS);
      Serial.println();
      Serial.println("Trying again...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(10000);
    }

    delay(500);
    Serial.println("...");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void getWeatherAndTime() {
  HTTPClient http;
  http.begin("http://rorpage.azurewebsites.net/api/IoTWeatherAndTime?code=qkynrKsM5V5GyquXtScnFO6Sayrf5oI781w5K9Y1Uz7aJBCMl1b17w==");

  int httpCode = http.GET();

  if(httpCode == HTTP_CODE_OK) {
    String response = http.getString();

    StaticJsonBuffer<512> jsonBuffer;
    JsonObject& jsonObject = jsonBuffer.parseObject(response);

    String dateDisplay = (jsonObject["date"]).asString();
    String timeDisplay = (jsonObject["time"]).asString();
    displayMessage(dateDisplay + " " + timeDisplay, 0, true);

    String tempF = (jsonObject["temperature_f"]).asString();
    displayMessage("Temp: " + tempF, 1, false);
    lcd.write(byte(0));
    lcd.print("F");

    if (jsonObject["current_hour"] == 13) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
  }
}
