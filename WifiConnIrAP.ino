#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include <Arduino.h>
#include <U8g2lib.h>      // make sure to add U8g2 library and restart Arduino IDE  
#include <SPI.h>
#include <Wire.h>

#define OLED_SDA  2
#define OLED_SCL 14
#define OLED_RST  4
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , OLED_RST);

ESP8266WebServer server(80);

const char* ssid = "";
const char* passphrase = "";
const char* APname = "ESP8266-AP";
String esid;  //SSID from EEPROM 
char* temp = "";
String st;
String content;
int statusCode;
IPAddress APip = WiFi.softAPIP();
IPAddress LOCip = WiFi.localIP();

void setup() {
  // Start Serial communication
  Serial.begin(115200);
  u8g2.begin();
  set_oled("Startup",APname);
  // Start Eeprom
  EEPROM.begin(512);
  delay(10);
  
  Serial.println();
  Serial.println("Startup");
    
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");

  
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);

  set_oled("SSID: ", esid.c_str());

  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  delay(100);
  
  if ( esid.length() > 1 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
      if (testWifi()) {
        launchWeb(0);
        return;
      } else {setupAP(); }
  } 
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  set_oled("Waiting for", "Wifi to connect");  
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) { return true; } 
    delay(500);
    Serial.print(".");    
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  set_oled("Connect timed out", "opening AP");
  return false;
} 

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  LOCip = WiFi.localIP();
  set_oled(esid.c_str(), LOCip.toString().c_str());
  
  createWebServer(webtype);
  
  // Start the server
  server.begin();
  Serial.println("Server started"); 
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0){
    Serial.println("no networks found");
    set_oled("ERR:", "no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*ENC*");
      delay(10);
     }
  }
  Serial.println(""); 

  st = " ";
  for (int i = 0; i < n; ++i)
    {
     
      st += "<option value\"";      
      st += WiFi.SSID(i);
     
      st += "\"> ";
      st += WiFi.SSID(i);
   
      st += "</option>";
    }

  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(APname);
  Serial.println("softap");
  set_oled(APname, APip.toString().c_str());
  launchWeb(1);
  Serial.println("over");
}

void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
      
      set_oled(APname, APip.toString().c_str());

    server.on("/", []() {
      
        content = "<!DOCTYPE HTML>\r\n<html><head><title>";
        content +=  APname;
        content +=  "</title>\r\n<style>body{font-size:20px;} input, select{width:100%; height:35px;}</style></head>\r\n<body><h1>Set-up ";
        content +=  APname;
        content += " at ";
        content += "";
        content += APip;
        content += "</h1><form method='get' action='setting'><p><label>Choose wifi-ssid:</label><br/>";
        content += "<select name=\"ssidsel\" onchange=\"changeSSID(this)\">";
        content += st;
        content += "</select><br/>";
        content += "<input id=ssid name=ssid><br/>";
        content += "\r\n<label>Password: </label><br/><input name='pass' length=64><br/><input type='submit'></p></form>";
        content += "<br/><script>function changeSSID(selOpt){ var ssid = document.getElementById('ssid'); ssid.value = selOpt.value;}</script><br/><a href=\"/cleareeprom\">Clearing boot-set-up</a>";
        content += "</body></html>";
        server.send(200, "text/html", content);  
    });
    server.on("/setting", []() {
        String qsid = server.arg("ssid");        
        String qpass = server.arg("pass");
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
          statusCode = 200;
          ESP.reset();
        } else {
          content = "{\"Error\":\"404 not found\"}";
          statusCode = 404;
          Serial.println("Sending 404");
        }
        server.send(statusCode, "application/json", content);
    });


    
  } else if (webtype == 0) {

    //Normal webserver
 
      set_oled(esid.c_str(), LOCip.toString().c_str());

      
    server.on("/", []() {

        //String ledvalue = server.arg("led1");
        String selval = "";
        String ledval = "";

        if(server.arg("led1")!="1")
        {
          ledval = "1";
          selval = "checked";
        }else{
          ledval = "0";
          selval = "unchecked";
        }
  
      set_oled("Message: ", server.arg("message").c_str());
        
      
        content = "<!DOCTYPE HTML>\r\n<html>\r\n<head>\r\n";
        content += "<title>ESP8266 IOT</title></head>";
        
        content += "\r\n<body>";
        content += "<h1>Welcome to ";
        content +=  esid;
        content += " at ";
        content += LOCip.toString().c_str();
        content += "</h1>";
        
        
        content += "\r\n<form method=\"get\"";
        content += " action=\"/\" ";
        content += "name=\"ledform\">";
        
        content += "\r\n<p style=\"border:1px solid grey; padding:10px; \"><br/>\r\n<label>LED:</label>";        
        //TODO fix switch
        //content += "\r\n<input type=\"checkbox\" name=\"led1\" value=\"";
        //content += ledval;
        //content += "\" ";
        //content += selval;        
        //content += " onClick=\"this.form.submit();\"  style=\"border:5px solid gray; width:100px; height:100px;\"></p>";

        content +="\r\n<p><input type=\"text\" name=\"message\" placeholder=\"your message\" value=\"";
        if(server.arg("message")!="") content += server.arg("message");
        content +="\"/></p>";

        content += "\r\n<input type=\"submit\" value=\"SEND\"></form>";
        
        
        content += "\r\n<br/><br/>[";
        content += ledval;
        content += "]<br/><br/>\r\n\r\n";
        
        

        content += "<br/><a href=\"?led1=1\">LED <b>On</b></a>";

        content += "<br/><a href=\"?led1=0\">LED <b>off</b></a>";
          
        
        content += "</a>";
        content += "\r\n</body></html>";
                
        server.send(200, "text/html", content);  
    });
  
    server.on("/test", []() {
        //IPAddress ip = WiFi.softAPIP();
        //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        //IPAddress ip = WiFi.localIP();
        //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        //set_oled("/test", ipStr.c_str());
        
        
        content = "<!DOCTYPE HTML>\r\n<html><h1>TEST ";
        content +=  APname;
        content += " at ";
        content += "";
        //content += ipStr;
        content += "</h1><p>Choose wifi-ssid:";
        content += "<select name='ssid'>";
        content += st;
        content += "</select>";
        content += "</p><form method='get' action='setting'><input name='xssid' length=32 style='display: none;'><input name='pass' length=64><input type='submit'></form>";
        content += "</html>";
                
        server.send(200, "text/html", content);  
    });

    server.on("/setting", []() {
        String qsid = server.arg("ssid");
        String qpass = server.arg("pass");
 
        if (qsid.length() > 0 && qpass.length() > 0) {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
            
          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
            {
              EEPROM.write(i, qsid[i]);
              Serial.print("Wrote: ");
              Serial.println(qsid[i]); 
            }
          Serial.println("writing eeprom pass:"); 
          for (int i = 0; i < qpass.length(); ++i)
            {
              EEPROM.write(32+i, qpass[i]);
              Serial.print("Wrote: ");
              Serial.println(qpass[i]); 
            }    
          EEPROM.commit();
          content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
          statusCode = 200;
          ESP.reset();
        }});
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
      EEPROM.commit();
      WiFi.disconnect();
      delay(1000);
      ESP.restart();
    });
  }
}

void loop() {
  server.handleClient();
}

void set_oled(const char* text1, const char* text2)
{  
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_8x13B_mf); // choose a suitable font
  u8g2.drawStr(0, 10, text1); // write something to the internal memory
  u8g2.drawStr(0, 28, text2); // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display
}
