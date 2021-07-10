#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <ThingESP.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Hash.h>
#include <FS.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "EmonLib.h"   //https://github.com/openenergymonitor/EmonLib
#include <BlynkSimpleEsp8266.h>


ThingESP8266 thing("Kadir","EmonHome","ESP8266");

AsyncWebServer server(80);

float currCalibration_input=53;
float Volt_input=240.00;
EnergyMonitor emon;


BlynkTimer timer;

char auth[] = "sUqvrFO__RChzEwI59y_Y9x6fA55WQw5";

char ssid[] = "D-link EXT";
char pass[] = "KadirOm@d-link100ext";

const char* UNITS_INPUT = "unitsinput";
const char* CALIBRATION_INPUT = "calibinput";
const char* VOLT_INPUT = "voltinput";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Energy Monitor Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <form action="/get">
    UNITS : <input type="number" placeholder="0.00" step="0.01" name="unitsinput"/>
    <input type="submit" value="Submit">
  </form><br>
   <form action="/get">
    CALIBRATION (Leave it... Don't enter any value) : <input type="number" placeholder="0.00" step="0.01" name="calibinput"/>
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    VOLT (Leave it... Don't enter any value) : <input type="number" placeholder="0.00" step="0.01" name="voltinput"/>
    <input type="submit" value="Submit">
  </form><br>
  <h7>&emsp;&emsp; Calibration = current_calibration* main reading / Emon reading </h7>
</body></html>)rawliteral";


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}



String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, float Storevalue){
//  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(Storevalue)){
//    Serial.println("- file written");
    Serial.println(file.size());
  } else {
  //  Serial.println("- write failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char * path){
   Serial.printf("Deleting file: %s\r\n", path);
   if(fs.remove(path)){
      Serial.println("− file deleted");
   } else {
      Serial.println("− delete failed");
   }
}

double price = 0.00;
float kWh = 0.00;
double Current = 0.00;
double Power = 0.00;
unsigned long lastmillis = millis();
unsigned long lasttimemillis=0;
unsigned long loopmillis=0;
unsigned long looptimemillis=0;
unsigned long periodicMillis=0;
const long INTERVAL=1000*86400;
int looptimes=1;
int times=1;

String HandleResponse(String query){
  if(query=="test"){
      return "Yes! im online.";
    }else if(query=="units"){
      return "Units: "+String(kWh);
    }else if(query=="current"){
      return "Current: "+String(Current)+" Amps";
    }else if(query=="power"){
      return "Power: "+String(Power)+" kWh";
    }else if(query=="price"){
      return "Price: "+String(price)+" Rupees";}
    else if(query=="status"){
      return "Status: \n Units: "+String(kWh)
              +"\n Current: "+String(Current)+" Amps"
              +"\n Power: "+String(Power)+" kWh"
              +"\n Price: "+String(price)+" Rupees"
              ;}
    else return "Enter a valid query!  queries: Units, Current, Power, Price, Status ";

  }



void calc_all(){
  double Irms = emon.calcIrms(1480);
  Current=Irms;
//  Blynk.virtualWrite(V1, Irms);
  Serial.print("\tIrms: ");
  Serial.println(Irms, 6);
//  Blynk.virtualWrite(V2, Irms * Volt_input);
  Serial.print("\tkWh: ");
  Serial.println(Irms * Volt_input, 6);
  Power=Irms*Volt_input;
  Serial.print(millis()-lastmillis);
  
//  lasttimemillis+=millis()-lastmillis;
//  Serial.print("\t");
//  Serial.print(lasttimemillis);
//  Serial.print("\t");
//  Serial.print(times);
//  Serial.print("\t");
//  Serial.print(lasttimemillis/times);
//  times++;
  
  Serial.print("\tUnits Consumed: ");
  kWh = kWh + (Power) * (millis()-lastmillis) / 3600000000.0;
  lastmillis = millis();
  Serial.print(kWh, 4);
  Serial.println(" Units");
//  Blynk.virtualWrite(V3, kWh);
  Serial.print("\tCalibration: ");
  Serial.print(currCalibration_input);
  /*     Type code for Price calculation  */
  if (kWh >= 0 and kWh <= 100)
  {
    price = 0;
  }
  else if (kWh > 100 and kWh <= 200)
  {
    price = (kWh * 1.50) + 20;
  }
  else if (kWh > 200 and kWh <= 500)
  {
    price = 150 - 150 + 200 + ((kWh - 200) * 3.0) + 30;
  }
  else
  {
    price = 150 - 150 + 350 + 1380 + ((kWh - 500) * 6.60) + 50;
  }
//  Blynk.virtualWrite(V4, price);

//  // LED Display
//  if (Irms < 3)
//  {
//    pinMode(0, OUTPUT);
//  }
//  else if (Irms < 5)
//  {
//    pinMode(1, OUTPUT);
//  }
//  else if (Irms < 8)
//  {
//    pinMode(2, OUTPUT);
//  }
//  else if (Irms > 8)
//  {
//    pinMode(3, OUTPUT);
//  }
//writeFile(SPIFFS, "/StoreUnits.txt", kWh);
delay(1000);
}

void myTimerEvent() {
//  Serial.println("MyTimerEvent started");
  Blynk.virtualWrite(V0, analogRead(A0));
//  double Irms = emon.calcIrms(1480);
//  Current=Irms;
  Blynk.virtualWrite(V1, Current);
//  Serial.print("\tIrms: ");
//  Serial.println(Irms, 6);
  Blynk.virtualWrite(V2, Power);
//  Serial.print("\tkWh: ");
//  Serial.println(Irms * Volt_input, 6);
//  Power=Irms*Volt_input;
//  Serial.print(millis()-lastmillis);
  
//  lasttimemillis+=millis()-lastmillis;
//  Serial.print("\t");
//  Serial.print(lasttimemillis);
//  Serial.print("\t");
//  Serial.print(times);
//  Serial.print("\t");
//  Serial.print(lasttimemillis/times);
//  times++;
  
//  Serial.print("\tUnits Consumed: ");
//  kWh = kWh + (Power) * (5640) / 3600000000.0;
//  Serial.print(kWh, 4);
//  Serial.println(" Units");
  Blynk.virtualWrite(V3, kWh);
//  Serial.print("\tCalibration: ");
//  Serial.print(currCalibration_input);
  /*     Type code for Price calculation  */
//  if (kWh >= 0 and kWh <= 100)
//  {
//    price = 0;
//  }
//  else if (kWh > 100 and kWh <= 200)
//  {
//    price = (kWh * 1.50) + 20;
//  }
//  else if (kWh > 200 and kWh <= 500)
//  {
//    price = 150 - 150 + 200 + ((kWh - 200) * 3.0) + 30;
//  }
//  else
//  {
//    price = 150 - 150 + 350 + 1380 + ((kWh - 500) * 6.60) + 50;
//  }
  Blynk.virtualWrite(V4, price);

//  // LED Display
//  if (Irms < 3)
//  {
//    pinMode(0, OUTPUT);
//  }
//  else if (Irms < 5)
//  {
//    pinMode(1, OUTPUT);
//  }
//  else if (Irms < 8)
//  {
//    pinMode(2, OUTPUT);
//  }
//  else if (Irms > 8)
//  {
//    pinMode(3, OUTPUT);
//  }

writeFile(SPIFFS, "/StoreUnits.txt", kWh);
//lastmillis = millis();
}



const char Streamdata_html[] PROGMEM = R"(
<!DOCTYPE HTML><html><head>
      <title>ESP Smart Energy Monitor</title>
      <h1>&emsp;&emsp;&emsp;&emsp;ESP Smart Energy Monitor</h1>
      <meta name="viewport" content=\"width=device-width, initial-scale=1>
      </head><body>
       <script>
            setInterval(function() {AjaxFunction();}, 100);
            function AjaxFunction()
            {
            //Streaming Price data....
            var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("PriceStream").innerHTML="&emsp; Price: "+this.responseText+" Rupees";
                    } else {
                        console.warn('Warning');
                    }
                };

                //ajax.open("GET", "", true);
                //ajax.send();
               ajax.open("GET", "StreamPriceData", true);
               ajax.send();

             //Streaming Current data...................................................
              var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("CurrentStream").innerHTML="&emsp; Current: "+this.responseText+" Amps";
                    } else {
                        console.warn('Warning');
                    }
                };
               ajax.open("GET", "StreamCurrentData", true);
               ajax.send();
            
               
             //Streaming Units data...................................................
              var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("UnitsStream").innerHTML="&emsp; Units Consumed: "+this.responseText+" Units";
                    } else {
                        console.warn('Warning');
                    }
                };
               ajax.open("GET", "StreamUnitsData", true);
               ajax.send();

               
             //Streaming Power data...................................................
              var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("PowerStream").innerHTML="&emsp; Power: "+this.responseText+" kWh";
                    } else {
                        console.warn('Warning');
                    }
                };
               ajax.open("GET", "StreamPowerData", true);
               ajax.send();
            

             //Streaming Calibration data...................................................
              var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("CalibStream").innerHTML="&emsp; Calibration: "+this.responseText+" ";
                    } else {
                        console.warn('Warning');
                    }
                };
               ajax.open("GET", "StreamCalibrationData", true);
               ajax.send();


             //Streaming Volt data...................................................
              var ajax = new XMLHttpRequest();

                ajax.onreadystatechange = function() {
                    if(this.readyState == 4 &&this.status==200) {
                        document.getElementById("VoltStream").innerHTML="&emsp; Volt: "+this.responseText+" V";
                    } else {
                        console.warn('Warning');
                    }
                };
               ajax.open("GET", "StreamVoltData", true);
               ajax.send();
             
             
            //...........................................................................
            }

        </script>   
      <br></h1><br><h1  style="color: #ed9d00" id="PriceStream"></h1>
      <br><h1 style="color: #05c0f7" id="UnitsStream">Streaming...</h1>
      <br><h1 style="color: #23c48e" id="CurrentStream">Streaming...</h1>
      <br><h1 style="color: #d3435c" id="PowerStream">Streaming...</h1>
      <br><h1 style="color: #4B0082" id="VoltStream">Streaming...</h1>
      <br><h1 style="color: yellow" id="CalibStream">Streaming...</h1>
      <br><a href="/">Click Here to Change values</a></body> Streaming...</html></h1>
  
</body></html>)";




void setup() {
  Serial.begin(9600);
  thing.SetWiFi(ssid,pass);
  thing.initDevice();
  Serial.println("Booting");
//   if(MDNS.begin("ESP")){
//    Serial.println("MDNS Started");
//    }else{
//    Serial.println("MDNS Failed");  
//      }
//  WiFi.softAP("EnergyMonitor","");
//  Serial.println("SoftAP Started ");
//  Serial.println(WiFi.softAPIP()); //192.168.4.1
  
  if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }
    
  WiFi.mode(WIFI_STA);
  WiFi.hostname("Emon");
  WiFi.begin(ssid, pass);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
 
  
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(UNITS_INPUT)) {
      inputMessage = request->getParam(UNITS_INPUT)->value();
      inputParam = UNITS_INPUT;
      kWh=inputMessage.toFloat();
    }   else if (request->hasParam(CALIBRATION_INPUT)) {
      inputMessage = request->getParam(CALIBRATION_INPUT)->value();
      inputParam = CALIBRATION_INPUT;
      currCalibration_input=inputMessage.toFloat();
      emon.current(A0, currCalibration_input);
//       if(currCalibration_input<52 or NULL){
//        currCalibration_input=52;
//        emon.current(A0, currCalibration_input);  // Current: input pin, calibration.
//        }else{
//        emon.current(A0, currCalibration_input);
//       }
    }  else if (request->hasParam(VOLT_INPUT)) {
      inputMessage = request->getParam(VOLT_INPUT)->value();
      inputParam = VOLT_INPUT;
      Volt_input=inputMessage.toFloat();
       if(Volt_input<230.00 or NULL){
        Volt_input=230.00;
        }else{
        Volt_input=Volt_input;
       }
       } else {
      inputMessage = "No message sent";
      inputParam = "none";
    } 
    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field ("
                  + inputParam + ") with value: " + inputMessage +
                  "<br><a href=\"/\">Return to Home Page</a><meta http-equiv=\"refresh\" content=\"1; URL=/Streamdata\" />");
  });

  server.on("/Streamdata", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send_P(200, "text/html",Streamdata_html);
  });

  server.on("/StreamPriceData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(price));
  });

  server.on("/StreamUnitsData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(kWh));
  });

  server.on("/StreamCurrentData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(Current));
  });

  server.on("/StreamPowerData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(Power));
  });

  server.on("/StreamCalibrationData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(currCalibration_input));
  });

  server.on("/StreamVoltData", HTTP_GET, [] (AsyncWebServerRequest * request) {
    request->send(200,"text/strings",String(Volt_input));
  });
  
  emon.current(A0,currCalibration_input);
    
  server.onNotFound(notFound);
  server.begin();
//  MDNS.addService("http", "tcp", 80);

   float StoredUnits = readFile(SPIFFS, "/StoreUnits.txt").toFloat();
   kWh = StoredUnits;
   Serial.print(" Stored Units: ");
   Serial.println(StoredUnits);
//  deleteFile(SPIFFS, "/StoreUnits.txt");
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);
}

void loop() {
//  Serial.print("\n");
//  Serial.print(millis()-loopmillis);
//  looptimemillis+=millis()-lastmillis;
//  Serial.print("\t");
//  Serial.print(looptimemillis);
//  Serial.print("\t");
//  Serial.print(looptimes);
//  Serial.print("\t");
//  Serial.print(loopmillis/looptimes);
//  looptimes++;
  MDNS.update();
  ArduinoOTA.handle();
  calc_all();
  Blynk.run();
  timer.run();
  thing.Handle();
  if(millis()-periodicMillis>=INTERVAL){
     periodicMillis = millis();

     String msg = "Price: \n"+String(price);
     

     thing.sendMsg("+918778580892",msg);
    }
//  loopmillis=millis();
}
