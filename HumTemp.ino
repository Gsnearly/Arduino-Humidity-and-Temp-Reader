#include <Wire.h>
#include <WiFiS3.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "dht_nonblocking.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHT_SENSOR_TYPE DHT_TYPE_11
static const int DHT_SENSOR_PIN = 2;
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Network credentials
const char* ssid = "Your SSID";
const char* pass = "Your Password";

WiFiServer server(80);

void setup()
{
  Serial.begin(9600);

  // Initialize the 0.96-inch OLED display (I2C address 0x3C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Connecting to WiFi..."));
  display.display();

  // Check for the WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("WiFi Module Failed!"));
    display.display();
    while (true);
  }

  // Connect to Wi-Fi network
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(5000);
  }

  server.begin();

  // Display Connected & IP Address briefly during setup
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(F("WiFi Connected!"));
  display.setCursor(0, 20);
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.display();
  delay(3000); // Show IP for 3 seconds before starting loop
}

static bool measure_environment(float *temperature, float *humidity)
{
  static unsigned long measurement_timestamp = millis();

  /* Measure once every four seconds.[cite: 1] */
  if (millis() - measurement_timestamp > 3000ul)
  {
    if (dht_sensor.measure(temperature, humidity) == true)
    {
      measurement_timestamp = millis();
      return(true);
    }
  }

  return(false);
}

void loop()
{
  float temperature;
  float humidity;

  // Static variables to keep track of the latest readings for the web server
  static float latest_f = 0.0;
  static float latest_h = 0.0;

  /* Measure temperature and humidity using the non-blocking state machine.[cite: 1] */
  if (measure_environment(&temperature, &humidity) == true)
  {
    // Convert Celsius to Fahrenheit
    latest_f = (temperature * 9.0 / 5.0) + 32.0;
    latest_h = humidity;

    // Print to Serial Monitor
    Serial.print("T = ");
    Serial.print(latest_f, 1);
    Serial.print(" deg. F, H = ");
    Serial.print(latest_h, 1);
    Serial.println("%");

    // Update the 0.96-inch OLED Display (including local IP at the bottom)
    display.clearDisplay();
    
    // Title header (Yellow section)
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Environment Monitor"));
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    // Temperature reading (Blue section)
    display.setCursor(0, 18);
    display.setTextSize(1);
    display.print(F("Temp:"));
    display.setCursor(36, 18);
    display.setTextSize(1);
    display.print(latest_f, 1);
    display.print((char)247); 
    display.print(F("F"));

    // Humidity reading
    display.setCursor(0, 38);
    display.setTextSize(1);
    display.print(F("Humidity: "));
    display.print(latest_h, 1);
    display.print(F("%"));

    // IP Address at the very bottom row
    display.setCursor(0, 54);
    display.setTextSize(1);
    display.print(F("IP:"));
    display.print(WiFi.localIP());

    display.display();
  }

  // Handle incoming web server clients continuously
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // HTML Web Page content
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta http-equiv=\"refresh\" content=\"5\"></head>");
            client.println("<body><h1>Arduino Uno R4 Environment</h1>");
            client.print("<p>Temperature: <strong>");
            client.print(latest_f, 1);
            client.println(" &deg;F</strong></p>");
            client.print("<p>Humidity: <strong>");
            client.print(latest_h, 1);
            client.println(" %</strong></p>");
            client.println("</body></html>");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}
