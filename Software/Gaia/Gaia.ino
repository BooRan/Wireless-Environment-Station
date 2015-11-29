// WIFI includes
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiServer.h>
#include <LWiFiClient.h>

// SD card includes
#include <LSD.h>
#include <LStorage.h>

// Sensor includes
#include <Wire.h>
#include <HIH6130.h>
#include <Adafruit_BMP085.h>

// Pin that TEMT6000 in connected to
#define TEMT6000PIN (0)

// I2C address for HIH6130
#define HIH6130ADDRESS (0x27)

// Determines what is printed to webpage
#define PRINT_HTML_TAGS
#define PRINT_HTML_TEXT

// Wifi server and client
LWiFiServer server(80);
LWiFiClient client;

// Global accessor for HIH6130
HIH6130 rht(HIH6130ADDRESS);

// Global accessor for BMP085
Adafruit_BMP085 bmp;
int bmpPressure = 0;

// Global accessor for TEMT6000
float ambientLightLux = 0.0;

// Values to be stored for WIFI Access
String ssid;
String pass;

void setup()
{ 
  Serial.begin(115200);

  Serial.println("Starting initilization");

  Serial.println("Initializing SD card");

  if(InitSdCard() == true)
  {
    Serial.println("SD card initialization complete");
  }
  else
  {
    Serial.println("Error initializing SD card");
  }
  
  Serial.println("Initilizing sensors");
  rht.begin();
  bmp.begin();
  Serial.println("Sensor initilization complete");

  Serial.println("Initilizing wifi");
  InitWifi(ssid, pass);  
  Serial.println("Wifi initilization complete");

  Serial.println("Initilizing Server");
  server.begin();
  Serial.println("Server initilization complete");

  Serial.println("Initilization complete");
}

void loop()
{
  client = server.available();
  if (client)
  {
    if (client.available())
    {
      // Read data
      readSensors();
      
      // Ignores client request, but wait for HTTP request end
      int c = client.read();

      // Print sensor status
      printSensorStatus();     
    }
 
    // give the web browser time to receive the data
    delay(1000);

    // close the connection:
    client.stop();
  }
}

bool InitSdCard()
{
  char readChar;
  pinMode(10, OUTPUT);
  LSD.begin();
  LFile myFile = LSD.open("wifi.txt");
  
  if (myFile) {
      Serial.println("Reading wifi.txt");
      myFile.seek(0);
      
      // Read until the first space
      while (readChar != ' ')
      {
        readChar = myFile.read();
      }

      // Read until the end line
      while (readChar != '\n')
      {
        readChar = myFile.read();

        if(readChar != '\n')
        {
          ssid += readChar;
        }
      }

      // Read until the next space
      while (readChar != ' ')
      {
        readChar = myFile.read();
      }

      // Read until the end line or end of 
      // file, whichever comes first
      while ( (myFile.available()) &&
              (readChar != '\n'))
      {
        readChar = myFile.read();

        if(readChar != '\n')
        {
          pass += readChar;
        }
      }

      // Store the last character 
      if(readChar != '\n')
      {
        pass += readChar;
      }
      
      // Close the file
      myFile.close();
      return true;
  } 
  
  // If the file didn't open, return an error
  return false;
}

void InitWifi(String ssidName, String ssidPass)
{
  char ssidChars[ssidName.length()];
  char passChars[ssidPass.length()];

  ssidName.toCharArray(ssidChars, ssidName.length());
  ssidPass.toCharArray(passChars, ssidPass.length()); 
  
  LWiFi.begin();
  // keep retrying until connected to AP
  Serial.println("Connecting to AP");
  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP according to your WiFi AP configuration
  while (0 == LWiFi.connect(ssidChars, LWiFiLoginInfo(LWIFI_WPA, passChars)))
  {
    delay(1000);
  }
  printWifiStatus();
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(LWiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = LWiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("subnet mask: ");
  Serial.println(LWiFi.subnetMask());

  Serial.print("gateway IP: ");
  Serial.println(LWiFi.gatewayIP());

  // print the received signal strength:
  long rssi = LWiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void readSensors()
{
  // HIH6130
  rht.readRHT(); 

  // BMP085
  bmpPressure = bmp.readPressure();

  // TEMT6000 - 0.9765625 = 1000/1024 to give us illuminance
  ambientLightLux = (analogRead(TEMT6000PIN) * 0.9765625);  
}

void printSensorStatus()
{
  // Print a standard http response header
//  client.println("HTTP/1.1 200 OK");
//  client.println("Content-Type: text/html");
//  client.println("Connection: close");  // the connection will be closed after completion of the response
//  client.println("Refresh: 5");  // refresh the page automatically every 5 sec
//  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<body>");
  // Print each of the sensor's data

  printSensorInfo("HUMIDITY", rht.humidity, "%");
  printSensorInfo("TEMPERATURE", rht.temperature, "C");
  printSensorInfo("LIGHT", ambientLightLux, "LUX");
  printSensorInfo("PRESSURE", bmpPressure, "PA");
  
  // Close out the http tags
  client.println("</body>");
  client.println("</html>");
  client.println();
}

void printSensorInfo(String sensorName, String value, String units)
{
  // Tag
  client.print("<SENSOR ");
#ifdef PRINT_HTML_TAGS
  // Tag NAME
  client.print("NAME=\"");
  client.print(sensorName);
  client.print("\" ");
  // Tag VALUE
  client.print("VALUE=\"");
  client.print(value);
  client.print("\" ");
  // Tag UNITS
  client.print("UNITS=\"");
  client.print(units);
  client.print("\">");
#endif
#ifdef PRINT_HTML_TEXT
  // Sensor
  client.print(sensorName);
  client.print(" VALUE=\"");
  // Value
  client.print(value); 
  client.print("\" ");
  client.print("UNITS=\"");
  // Units
  client.print(units);              
  client.print("\"");
#endif
  // Tag close
  client.print("</SENSOR>");
  client.println("<br/>");
}

void printSensorInfo(String sensorName, int value, String units)
{
   // Tag
  client.print("<SENSOR ");
#ifdef PRINT_HTML_TAGS
  // Tag NAME
  client.print("NAME=\"");
  client.print(sensorName);
  client.print("\" ");
  // Tag VALUE
  client.print("VALUE=\"");
  client.print(value);
  client.print("\" ");
  // Tag UNITS
  client.print("UNITS=\"");
  client.print(units);
  client.print("\">");
#endif
#ifdef PRINT_HTML_TEXT
  // Sensor
  client.print(sensorName);
  client.print(" VALUE=\"");
  // Value
  client.print(value); 
  client.print("\" ");
  client.print("UNITS=\"");
  // Units
  client.print(units);              
  client.print("\"");
#endif
  // Tag close
  client.print("</SENSOR>");
  client.println("<br/>");
}

void printSensorInfo(String sensorName, double value, String units)
{
   // Tag
  client.print("<SENSOR ");
#ifdef PRINT_HTML_TAGS
  // Tag NAME
  client.print("NAME=\"");
  client.print(sensorName);
  client.print("\" ");
  // Tag VALUE
  client.print("VALUE=\"");
  client.print(value);
  client.print("\" ");
  // Tag UNITS
  client.print("UNITS=\"");
  client.print(units);
  client.print("\">");
#endif
#ifdef PRINT_HTML_TEXT
  // Sensor
  client.print(sensorName);
  client.print(" VALUE=\"");
  // Value
  client.print(value); 
  client.print("\" ");
  client.print("UNITS=\"");
  // Units
  client.print(units);              
  client.print("\"");
#endif
  // Tag close
  client.print("</SENSOR>");
  client.println("<br/>");
}

void printSensorInfo(String sensorName, float value, String units)
{
  // Tag
  client.print("<SENSOR ");
#ifdef PRINT_HTML_TAGS
  // Tag NAME
  client.print("NAME=\"");
  client.print(sensorName);
  client.print("\" ");
  // Tag VALUE
  client.print("VALUE=\"");
  client.print(value);
  client.print("\" ");
  // Tag UNITS
  client.print("UNITS=\"");
  client.print(units);
  client.print("\">");
#endif
#ifdef PRINT_HTML_TEXT
  // Sensor
  client.print(sensorName);
  client.print(" VALUE=\"");
  // Value
  client.print(value); 
  client.print("\" ");
  client.print("UNITS=\"");
  // Units
  client.print(units);              
  client.print("\"");
#endif
  // Tag close
  client.print("</SENSOR>");
  client.println("<br/>");
}


