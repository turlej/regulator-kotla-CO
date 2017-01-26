#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "hasla.h"
#include "FS.h"

WiFiClient client;

WiFiUDP udp;
ESP8266WebServer server(80);

unsigned int czas, b=0;
int analog=500, temperatura, t1, t2, a1, a2;
String server2;// = "192.168.1.132";
File f;
String temp;

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void handleParametry(){
  String message;
  message += "Method: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "<br>Arguments: ";
  message += server.args();
  message += "<br>";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "<br>";
    if (server.argName(i)=="serwer")
    {
      server2=server.arg(i);
      f = SPIFFS.open("/serwer.txt", "w");
      f.println(server2);
      f.close();
    }
    if (server.argName(i)=="ssid")
    {
      f = SPIFFS.open("/wifi.txt", "w");
      f.println(server.arg("ssid"));
      f.println(server.arg("pass"));
      f.close();
    }
    if (server.argName(i)=="t1")
    {
      f = SPIFFS.open("/skalowanie.txt", "w");
      f.println(server.arg("t1"));
      f.println(server.arg("t2"));
      f.println(server.arg("a1"));
      f.println(server.arg("a2"));
      f.close();
      f = SPIFFS.open("/skalowanie.txt", "r");
      temp = f.readStringUntil('\r');
      t1=atoi(temp.c_str());
      f.seek(1, SeekCur);
      temp = f.readStringUntil('\r');
      t2=atoi(temp.c_str());
      f.seek(1, SeekCur);
      temp = f.readStringUntil('\r');
      a1=atoi(temp.c_str());
      f.seek(1, SeekCur);
      temp = f.readStringUntil('\r');
      a2=atoi(temp.c_str());
      f.close();
      analog=(a2-a1)*((float)(temperatura-t1)/(t2-t1))+a1;
      analogWrite(5, analog);
    }
  }
  String strona;
  String do_wyslania;
  String form;
  String dane;
  do_wyslania += "<html><body>";
  strona += "<b>Regulator pogodowy kotla CO</b><br><br>";
  strona += "<b>WiFi SSID: ";
  strona += WiFi.SSID();
  strona += "</b><br>";
  strona += "<b>WiFi RSSI: ";
  strona += WiFi.RSSI();
  strona += "</b><br>";
  strona += "Rozmiar pamieci: ";
  strona += ESP.getFlashChipRealSize()/1024;
  strona += " KB";
  strona += "<br>Czas pracy: ";
  if (millis()/3600000<10) strona += "0";
  strona += millis()/3600000;
  strona += ":";
  if (millis()/60000%60<10) strona += "0";
  strona += millis()/60000%60;
  strona += ":";
  if (millis()/1000%60<10) strona += "0";
  strona += millis()/1000%60;

  form += "<br><br><form>";
  form += "Serwer: <input type='text' name='serwer' value='";
  form += server2;
  form += "'>";
  form += "<button type='submit' formaction='/' formmethod='GET'><font size='2'>Zapisz</font></button>";
  form += "</form>";

  form += "<form>";
  form += "SSID: <input type='text' name='ssid' value='";
  f = SPIFFS.open("/wifi.txt", "r");
  form += f.readStringUntil('\r');
  f.seek(1, SeekCur);
  form += "'>";
  form += "<br>Haslo: <input type='text' name='pass' value='";
  form += f.readStringUntil('\r');
  f.close();
  form += "'>";
  form += "<br><button type='submit' formaction='/' formmethod='GET'><font size='2'>Zapisz</font></button>";
  form += "</form>";

  form += "<form>";
  form += "t1: <input type='number' name='t1' min='0' max='90' value='";
  f = SPIFFS.open("/skalowanie.txt", "r");
  form += f.readStringUntil('\r');
  f.seek(1, SeekCur);
  form += "'>";
  form += " t2: <input type='number' name='t2' min='0' max='90' value='";
  form += f.readStringUntil('\r');
  f.seek(1, SeekCur);
  form += "'>";
  form += "<br>a1: <input type='number' name='a1' min='0' max='1023' value='";
  form += f.readStringUntil('\r');
  f.seek(1, SeekCur);
  form += "'>";
  form += " a2: <input type='number' name='a2' min='0' max='1023' value='";
  form += f.readStringUntil('\r');
  form += "'>";
  form += "<br><button type='submit' formaction='/' formmethod='GET'><font size='2'>Zapisz</font></button>";
  form += "</form>";
  f.close();
  
  dane += "Zadana temperatura CO: ";
  dane += temperatura;
  dane += " st.C<br>";
  dane += "Wyjscie analogowe: ";
  dane += analog;
  dane += "<br><br>";
  
  do_wyslania += strona + form + dane;
  do_wyslania += "<a href='/'>Odswiez</a><br><br><a href='/reboot'>Reset ESP CPU</a></body></html>";
  server.send(200, "text/html", do_wyslania);
}

byte scanNetworks()
{
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();
  byte znaleziono = 0;
  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++)
  {
    Serial.print(thisNet);
    Serial.print(") Network: ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print(" - RSSI: ");
    Serial.print(2*(WiFi.RSSI(thisNet)+100));
    Serial.println(" %");
    String siec = WiFi.SSID(thisNet);
    if (siec == "WLAN") znaleziono = 1;
    if (siec == "automatycy") znaleziono = 2;
    if (siec == "Domowa") znaleziono = 3;
    f = SPIFFS.open("/wifi.txt", "r");
    if (siec == f.readStringUntil('\r')) znaleziono = 4;
    f.close();
  }
  return znaleziono;
}

void setup(void)
{
  pinMode(5, OUTPUT);
  analogWrite(5, analog);
  SPIFFS.begin();
//  SPIFFS.format();



  f = SPIFFS.open("/serwer.txt", "r");
  server2 = f.readStringUntil('\r');
  f.close();
  f = SPIFFS.open("/skalowanie.txt", "r");
  temp = f.readStringUntil('\r');
  t1=atoi(temp.c_str());
  f.seek(1, SeekCur);
  temp = f.readStringUntil('\r');
  t2=atoi(temp.c_str());
  f.seek(1, SeekCur);
  temp = f.readStringUntil('\r');
  a1=atoi(temp.c_str());
  f.seek(1, SeekCur);
  temp = f.readStringUntil('\r');
  a2=atoi(temp.c_str());
  f.close();
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.println("");
  Serial.println("Start...");
  
  switch (scanNetworks())
  {
    case 0:
    //display.clearDisplay();
    Serial.println("Brak znanej sieci WiFi");
    delay(15000);
    Serial.println("Restart...");
    delay(1500);
    ESP.restart();
    break;
    case 1:
    WiFi.begin(ssid, password);
    WiFi.config(IPAddress(192,168,1,124), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    break;
    case 2:
    WiFi.begin(ssid2, password2);
    WiFi.config(IPAddress(192,168,1,124), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    break;
    case 3:
    WiFi.begin(ssid3, password3);
    WiFi.config(IPAddress(192,168,2,124), IPAddress(192,168,2,1), IPAddress(255,255,255,0));
    break;
    case 4:
    f = SPIFFS.open("/wifi.txt", "r");
    String ssid4 = f.readStringUntil('\r');
    f.seek(1, SeekCur);
    String password4 =f.readStringUntil('\r');
    unsigned int dlugosc=ssid4.length()+1;
    char ssid44[dlugosc];
    ssid4.toCharArray(ssid44,dlugosc);
    dlugosc=password4.length()+1;
    char password44[dlugosc];
    password4.toCharArray(password44,dlugosc);
    WiFi.begin(ssid44, password44);
    f.close();
    break;
  }
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  Serial.println("Polaczono");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("regulator");

  // No authentication by default
  ArduinoOTA.setPassword(haslo_ota);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  server.on("/reboot", [](){
    server.send(200, "text/html", "<html><body>Restarting<br><a href='/'>Odswiez</a></body></html>");
    delay(1000);
    ESP.restart();
  });
  
  server.on("/", HTTP_GET, handleParametry);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([](){
    handleNotFound();
//      server.send(404, "text/plain", "FileNotFound");
  });

  server.begin();
  
  Serial.println("Uruchomiono");
}
 
void loop(void)
{
  server.handleClient();
  ArduinoOTA.handle();
  b = czas;       
  czas = millis()/60000;   
  if (czas != b) 
  {
      if (WiFi.status() == WL_CONNECTED)
      {
        unsigned int dlugosc=server2.length()+1;
        char adres[dlugosc];
        server2.toCharArray(adres,dlugosc);
        if (client.connect(adres,80))
        {
          client.println("GET /regulator.php HTTP/1.1"); 
          client.print("Host: ");
          client.println(server2);
          client.println("Connection: close");
          client.println(); 
        }
      } else
      {
        Serial.println("Brak sieci WiFi");
      }
  }

    
    if (client.connected())
    {
      if (client.available())
      {
        String naglowek = client.readStringUntil('\r');
        if (naglowek=="HTTP/1.1 302 Found")
        {
          client.readStringUntil('/'); client.read();
          String server3 = client.readStringUntil('/');
          client.stop();
          unsigned int dlugosc3=server3.length()+1;
          char adres3[dlugosc3];
          server3.toCharArray(adres3,dlugosc3);
          if (client.connect(adres3,80))
          {
            client.println("GET /regulator.php HTTP/1.1"); 
            client.print("Host: ");
            client.println(server3);
            client.println("Connection: close");
            client.println();
          }
        }
        if (naglowek=="HTTP/1.1 200 OK")
        {
          client.readStringUntil('>'); client.read();
          temp = client.readStringUntil('<');
          if (temp.length()<5) {temperatura=atoi(temp.c_str());}

          client.stop();

          analog=(a2-a1)*((float)(temperatura-t1)/(t2-t1))+a1;
          
          //analog = temperatura*4.2667+384;
          
          analogWrite(5, analog);
        }
      }
    }
}
