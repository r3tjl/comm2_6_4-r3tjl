
/*
Сервер для Wifi коммутатора на 2 рабочих места
R3TJL V1.00 2023 г. - управление низким уровнем !!!
r3tjl@mail.ru
*/


#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Wire.h>
#include <PCF8574.h>   //или PCF8574_ESP.h           
 
const char *softAP_ssid = "RKconsole_r3tjl";
const char *softAP_password = "1234567890";
const char *myHostname = "esp8266";
 
char ssid[32] = "";
String brd1 = "Unkown";
String brd2 = "Unkown";

bool brd11 = false;
bool brd22 = false;

char password[32] = "";
char label0[32] ="";
char label1[32] ="";
char label2[32] ="";
char label3[32] ="";
char label4[32] ="";
char label5[32] ="";
char label6[32] ="";
char label7[32] ="";
char label8[32] ="";
String currentlabel ="";
String currentlabel1 ="";
String currentlabel2 ="";
String webPage = "";
String webPage2 = "";
String ourPage = "";
String Page3="";
String javaScript, XML, SVG;

int gpio8_pin = 2; //пин зажигает светодиод при подключении к wifi


char stat[9] =  ""; // массив переменных stat: каждый элемент (9 коммутируемых выходов) принимает значения 0 - не подкл., 1 - подкл. к 1 р/м, 2 - ко 2 р/м 
int tim = 0;
int sec = 0;
int minute = 0;
int hour = 0;
int day = 0;
int flagAP=0;
int flag_off=0;

const byte DNS_PORT = 53;
DNSServer dnsServer;

ESP8266WebServer server(80);

IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
boolean connect;
unsigned long lastConnectTry = 0;
unsigned int status = WL_IDLE_STATUS;
PCF8574 pcf1(0x20); // первая плата расширения
PCF8574 pcf2(0x21); // вторая плата расширения

void setup(void){
  delay(100);
  pinMode(gpio8_pin, OUTPUT);
  digitalWrite(gpio8_pin, HIGH);
 
  Wire.pins(4,5); //пины интерфейса i2c (4,5)
  Wire.begin(4,5);
    
  // инициализация первой платы расширения
  pcf1.pinMode(P0, OUTPUT);
  pcf1.digitalWrite(P0, HIGH);
  pcf1.pinMode(P1, OUTPUT);
  pcf1.digitalWrite(P1, HIGH);
  pcf1.pinMode(P2, OUTPUT);
  pcf1.digitalWrite(P2, HIGH);
  pcf1.pinMode(P3, OUTPUT);
  pcf1.digitalWrite(P3, HIGH);
  pcf1.pinMode(P4, OUTPUT);
  pcf1.digitalWrite(P4, HIGH);
  pcf1.pinMode(P5, OUTPUT);
  pcf1.digitalWrite(P5, HIGH);
  pcf1.pinMode(P6, OUTPUT);
  pcf1.digitalWrite(P6, HIGH);
  pcf1.pinMode(P7, OUTPUT);
  pcf1.digitalWrite(P7, HIGH);
  
  Serial.begin(115200);
  Serial.println();

  Serial.print("Init Ext PortBoard-1...");
  if (pcf1.begin())
  {
    Serial.println("Ok"); 
    brd1 = "Ok !";
    brd11 = true;
  }
  else
  {
    Serial.println("Error"); 
    brd1 = "Error";
    brd11 = false;
  }
  delay(50);

  // инициализация второй платы расширения
  pcf2.pinMode(P0, OUTPUT);
  pcf2.digitalWrite(P0, HIGH);
  pcf2.pinMode(P1, OUTPUT);
  pcf2.digitalWrite(P1, HIGH);
  pcf2.pinMode(P2, OUTPUT);
  pcf2.digitalWrite(P2, HIGH);
  pcf2.pinMode(P3, OUTPUT);
  pcf2.digitalWrite(P3, HIGH);
  pcf2.pinMode(P4, OUTPUT);
  pcf2.digitalWrite(P4, HIGH);
  pcf2.pinMode(P5, OUTPUT);
  pcf2.digitalWrite(P5, HIGH);
  pcf2.pinMode(P6, OUTPUT);
  pcf2.digitalWrite(P6, HIGH);
  pcf2.pinMode(P7, OUTPUT);
  pcf2.digitalWrite(P7, HIGH);
  
  Serial.println();

  Serial.print("Init Ext PortBoard-2...");
  if (pcf2.begin())
  {
    Serial.println("Ok"); 
    brd2 = "Ok !";
    brd22 = true;
  }
  else
  {
    Serial.println("Error"); 
    brd2 = "Error";
    brd22 = false;
  }
  delay(50);
  
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
 
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/label", handleLabel);
  server.on("/labelsave", handleLabelSave);
  server.on("/place1", handlePlace1);
  server.on("/place2", handlePlace2);
  server.on("/wifisave", handleWifiSave);
  server.on("/generate_204", handleRoot);
  server.on("/fwlink", handleRoot);  
  server.on("/xml", handleXML);
  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
  Serial.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  loadLabels();
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
  dnsServer.processNextRequest();
  server.handleClient();
  
  server.on("/APon", [](){
    if (flag_off==1)
    {
        WiFi.softAPConfig(apIP, apIP, netMsk);
        WiFi.softAP(softAP_ssid, softAP_password);
        delay(500); 
        flag_off=0;
    }
    Page3 += "<script> document.location.href = \"/wifi\"</script>";
    server.send(200, "text/html", Page3);
    handleWifi(); 
  });

  server.on("/APoff", [](){
    flag_off=1;
    WiFi.softAPdisconnect(true);
    Page3 += "<script> document.location.href = \"/wifi\"</script>";
    server.send(200, "text/html", Page3);
    handleWifi();
  });

  // *************************** Включение антенн 1-го Р/М *********************************
  server.on("/pl1s0On", [](){
    if (stat[0] != '2') 
    {
      turnOffPlace1();
      stat[0] = '1';
      currentlabel1=String(label0);
      currentlabel=currentlabel1 + " / " + currentlabel2;
      brd11 = pcf1.digitalWrite(P0, LOW);
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  // Отключение всех антенн 1-го Р/М
  server.on("/1stOff", [](){
    turnOffPlace1();
    currentlabel1="OFF";
    currentlabel=currentlabel1 + " / " + currentlabel2;
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });
  server.on("/pl1s1On", [](){
    if (stat[1] != '2')
    {
      turnOffPlace1();
      stat[1] = '1';
      currentlabel1=String(label1);
      currentlabel=currentlabel1 + " / " + currentlabel2;
      brd11 = pcf1.digitalWrite(P1, LOW);
    }  
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s2On", [](){
    if (stat[2] != '2')
    {
      turnOffPlace1();
      stat[2] = '1';
      brd11 = pcf1.digitalWrite(P2, LOW);
      currentlabel1=String(label2);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s3On", [](){
    if (stat[3] != '2')
    {
      turnOffPlace1();
      stat[3] = '1';
      brd11 = pcf1.digitalWrite(P3, LOW);
      currentlabel1=String(label3);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s4On", [](){
    if (stat[4] != '2')
    {
      turnOffPlace1();
      stat[4] = '1';
      brd11 = pcf1.digitalWrite(P4, LOW);
      currentlabel1=String(label4);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s5On", [](){
    if (stat[5] != '2' && stat[6] != '2' && stat [7] != '2' && stat[8] != '2')
    {
      turnOffPlace1();
      stat[5] = '1';
      brd11 = pcf1.digitalWrite(P5, LOW);
      brd11 = pcf1.digitalWrite(P6, LOW);
      currentlabel1=String(label5);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s6On", [](){
     if (stat[5] != '2' && stat[6] != '2' && stat [7] != '2' && stat[8] != '2')
    {
      turnOffPlace1();
      stat[6] = '1';
      brd11 = pcf1.digitalWrite(P5, LOW);
      brd11 = pcf1.digitalWrite(P7, LOW);
      currentlabel1=String(label6);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });

  server.on("/pl1s7On", [](){
   if (stat[5] != '2' && stat[6] != '2' && stat [7] != '2' && stat[8] != '2')
    {
      turnOffPlace1();
      stat[7] = '1';
      brd11 = pcf1.digitalWrite(P5, LOW);
      brd22 = pcf2.digitalWrite(P6, LOW);
      currentlabel1=String(label7);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });
  server.on("/pl1s8On", [](){
    if (stat[5] != '2' && stat[6] != '2' && stat [7] != '2' && stat[8] != '2')
    {
      turnOffPlace1();
      stat[8] = '1';
      brd11 = pcf1.digitalWrite(P5, LOW);
      brd22 = pcf2.digitalWrite(P7, LOW);
      currentlabel1=String(label8);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place1\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace1();
  });


  // ********************************* Включение антенн 2-го Р/М ************************************
  server.on("/pl2s0On", [](){
    if (stat[0] != '1')
    {
      turnOffPlace2();
      stat[0] = '2';
      currentlabel2=String(label0);
      currentlabel=currentlabel1 + " / " + currentlabel2;
      brd22 = pcf2.digitalWrite(P0, LOW);
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  // Отключение всех антенн 2-го Р/М
  server.on("/2ndOff", [](){
    turnOffPlace2();
    currentlabel2="OFF";
    currentlabel=currentlabel1 + " / " + currentlabel2;
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });
  server.on("/pl2s1On", [](){
    if (stat[1] != '1')
    {
      turnOffPlace2();
      stat[1] = '2';
      currentlabel2=String(label1);
      currentlabel=currentlabel1 + " / " + currentlabel2;
      brd22 = pcf2.digitalWrite(P1, LOW);
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s2On", [](){
    if (stat[2] != '1')
    {
      turnOffPlace2();
      stat[2] = '2';
      brd22 = pcf2.digitalWrite(P2, LOW);
      currentlabel2=String(label2);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s3On", [](){
    if (stat[3] != '1')
    {
      turnOffPlace2();
      stat[3] = '2';
      brd22 = pcf2.digitalWrite(P3, LOW);
      currentlabel2=String(label3);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s4On", [](){
    if (stat[4] != '1')
    {
      turnOffPlace2();
      stat[4] = '2';
      brd22 = pcf2.digitalWrite(P4, LOW);
      currentlabel2=String(label4);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s5On", [](){
    if (stat[5] != '1' && stat[6] != '1' && stat [7] != '1' && stat[8] != '1')
    {
      turnOffPlace2();
      stat[5] = '2';
      brd22 = pcf2.digitalWrite(P5, LOW);
      brd11 = pcf1.digitalWrite(P6, LOW);
      currentlabel2=String(label5);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s6On", [](){
    if (stat[5] != '1' && stat[6] != '1' && stat[7] != '1' && stat[8] != '1')
    {
      turnOffPlace2();
      stat[6] = '2';
      brd22 = pcf2.digitalWrite(P5, LOW);
      brd11 = pcf1.digitalWrite(P7, LOW);
      currentlabel2=String(label6);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/pl2s7On", [](){
    if (stat[5] != '1' && stat[6] != '1' && stat [7] != '1' && stat[8] != '1')
    {
      turnOffPlace2();
      stat[7] = '2';
      brd22 = pcf2.digitalWrite(P5, LOW);
      brd22 = pcf2.digitalWrite(P6, LOW);
      currentlabel2=String(label7);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });
  server.on("/pl2s8On", [](){
    if (stat[5] != '1' && stat[6] != '1' && stat [7] != '1' && stat[8] != '1')
    {
      turnOffPlace2();
      stat[8] = '2';
      brd22 = pcf2.digitalWrite(P5, LOW);
      brd22 = pcf2.digitalWrite(P7, LOW);
      currentlabel2=String(label8);
      currentlabel=currentlabel1 + " / " + currentlabel2;
    }
    webPage += "<script> document.location.href = \"/place2\"</script>";
    server.send(200, "text/html", webPage);
    handlePlace2();
  });

  server.on("/softreset", [](){
    webPage += "<script> document.location.href = \"/\"</script>";
    server.send(200, "text/html", webPage);
    softreset();
    handleRoot();
  });  

  server.begin();
  Serial.println("HTTP server started");
  //добавил сброс всего
  turnOffPlace1();
  turnOffPlace2();
}


void loop(void){
  if (connect) {
    Serial.println("Connect requested");
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    unsigned int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 60000)) {
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print("Status: ");
      Serial.println(s);
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        digitalWrite(gpio8_pin, LOW);
        flagAP=1;
      } else if (s == WL_NO_SSID_AVAIL) {
        digitalWrite(gpio8_pin, LOW);
        flagAP=0;
        WiFi.disconnect();
      }
    }
  }
  if (flagAP==1) 
  {
    tim++; 
    if (tim==1000) {sec++; tim=0;}
    if (sec==60) {minute++; sec=0;}
    if (minute==60) {hour++; minute=0;}
    if (hour==24) {day++; hour=0;}
  }
  if ((flagAP==0)&&(tim>0)) tim=sec=minute=hour=day=0;
  server.handleClient();
  delay(1);
  if (brd11)
  {
    brd1 = "Ok !";
  }
    else
    {
      Serial.println("Pcf-2...Error"); 
      brd1 = "Error";
      delay(1);
    }
  if (brd22)
  {
    brd2 = "Ok !";
  }
    else
    {
      Serial.println("Pcf-2...Error"); 
      brd2 = "Error";
      delay(5);
    }
  
}

void CheckStat () // Присвоение значения меткам включенных антенн - не соответствует, ПЕРЕДЕЛАТЬ !!!
/* Он используется в блоке, где принудительно включаются выходы. Либо он вообще не нужен, либо его переделать
   на вывод значений в основных блоках и вызывать оттуда 
   выводится это всё процедурой buildXML () под меткой 'runtime' как response */
{
  currentlabel ="";
  if (stat[0] == '1') currentlabel+=String(label0)+ " ";
  if (stat[1] == '1') currentlabel+=String(label1)+ " ";
  if (stat[2] == '1') currentlabel+=String(label2)+ " ";
  if (stat[3] == '1') currentlabel+=String(label3)+ " ";
  if (stat[4] == '1') currentlabel+=String(label4)+ " ";
  if (stat[5] == '1') currentlabel+=String(label5)+ " ";
  if (stat[6] == '1') currentlabel+=String(label6)+ " ";
  if (stat[7] == '1') currentlabel+=String(label7);
}

void handleXML() {
  buildXML();
  server.send(200, "text/xml", XML);
}


void BuildSVG()
{
  SVG = "<svg version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\"";
  SVG += F("width=\"154.000000pt\" height=\"152.000000pt\" viewBox=\"0 0 154.000000 152.000000\""
  "preserveAspectRatio=\"xMidYMid meet\">"
"<g transform=\"translate(0.000000,152.000000) scale(0.100000,-0.100000)\""
"fill=\"#000000\" stroke=\"none\">"
"<path d=\"M642 1459 c-131 -22 -273 -99 -377 -205 -121 -124 -184 -273 -193 -455 -6 -137 14 -228 76 -352 169 -338 599 -477 938 -303 170 86 290 227 351 411 24 71 27 96 27 215 0 122 -2 143 -28 215 -65 189 -182 322 -361 411 -135 67 -282 88 -433 63z m355 -57 c217 -81 377 -259 428 -477 21 -88 19 -236 -5 -329 -30 -118 -82 -208 -176 -302 -67 -68 -96 -89 -175 -127 -120 -57 -180 -71 -304 -71 -188 2 -335 64 -471 199 -132 131 -197 288 -197 475 0 307 200 568 499 651 55 15 94 18 194 16 110 -3 135 -8 207 -35z\"/>"
"<path d=\"M754 1372 c-7 -4 -14 -35 -15 -70 -4 -57 -2 -62 16 -62 18 0 20 7 20 70 0 71 -1 75 -21 62z\"/>"
"<path d=\"M845 1343 c0 -21 -4 -53 -9 -70 -6 -26 -4 -33 7 -33 10 0 17 16 21 50 4 27 14 56 22 65 13 13 13 15 1 15 -9 0 -21 2 -29 5 -9 4 -13 -5 -13 -32z\"/>"
"<path d=\"M630 1356 c0 -2 11 -6 25 -8 14 -3 25 -1 25 3 0 5 -11 9 -25 9 -14 0 -25 -2 -25 -4z\"/>"
"<path d=\"M910 1356 c0 -2 7 -9 15 -16 12 -10 15 -10 15 4 0 9 -7 16 -15 16 -8 0 -15 -2 -15 -4z\"/>"
"<path d=\"M588 1337 c11 -12 28 -94 24 -109 -1 -5 10 -8 26 -8 15 0 22 3 15 8 -6 4 -15 20 -19 36 -6 27 -5 29 18 24 22 -4 24 -2 20 16 -3 11 -7 15 -10 8 -2 -6 -12 -12 -23 -12 -14 0 -19 7 -19 25 0 19 -5 25 -21 25 -17 0 -19 -3 -11 -13z\"/>"
"<path d=\"M1012 1325 c8 -8 -24 -75 -47 -98 -13 -14 -11 -18 19 -36 42 -26 80 -27 106 -1 25 25 26 61 0 91 l-19 24 11 -32 c20 -55 -21 -104 -72 -88 -11 3 -20 13 -20 21 0 19 46 91 61 96 6 2 -2 10 -17 17 -16 7 -26 10 -22 6z\"/>"
"<path d=\"M490 1305 c-14 -8 -19 -14 -13 -15 16 0 65 -95 55 -105 -4 -5 1 -5 11 -2 10 4 20 7 22 7 2 0 -10 25 -27 55 -16 30 -27 59 -23 65 8 13 5 12 -25 -5z\"/>"
"<path d=\"M899 1312 c-14 -6 -14 -9 -4 -19 10 -9 14 -8 18 8 3 10 4 19 4 18 -1 0 -9 -3 -18 -7z\"/>"
"<path d=\"M370 1224 l-31 -27 34 -36 c19 -20 39 -44 44 -54 9 -17 10 -16 29 4 10 12 14 20 8 16 -12 -7 -51 29 -41 39 3 3 22 1 42 -6 23 -8 40 -9 46 -3 6 6 -5 14 -30 22 -29 8 -39 16 -34 26 5 15 -13 45 -27 45 -5 0 -23 -12 -40 -26z m45 -23 c0 -31 -21 -38 -35 -11 -13 24 -5 42 18 38 10 -2 17 -13 17 -27z\"/>"
"<path d=\"M890 1240 l-25 -8 28 -1 c15 -1 27 4 27 9 0 6 -1 10 -2 9 -2 -1 -14 -5 -28 -9z\"/>"
"<path d=\"M629 1177 c-88 -30 -171 -94 -220 -170 -51 -79 -70 -144 -70 -243 1 -262 237 -466 494 -424 263 42 426 302 349 557 -71 232 -322 359 -553 280z m324 -40 c87 -43 151 -107 194 -195 26 -52 28 -66 28 -172 0 -104 -3 -121 -27 -173 -57 -122 -180 -214 -317 -238 -267 -47 -518 207 -471 474 28 154 128 275 273 328 40 14 73 18 148 16 88 -2 102 -6 172 -40z\"/>"
"<path d=\"M627 912 c-10 -10 -17 -23 -17 -28 0 -5 5 -1 11 9 25 43 73 1 49 -43 -17 -31 -2 -39 21 -11 24 29 24 53 1 74 -24 22 -45 21 -65 -1z\"/>"
"<path d=\"M755 852 c-3 -8 -5 -21 -4 -30 0 -13 3 -11 11 6 6 12 19 22 30 22 16 0 18 -9 18 -93 0 -59 -4 -97 -12 -105 -9 -9 -3 -12 28 -12 31 1 36 3 22 11 -15 8 -18 25 -18 104 0 89 1 95 20 95 12 0 24 -10 30 -22 8 -21 9 -21 9 7 l1 30 -65 0 c-45 0 -66 -4 -70 -13z\"/>"
"<path d=\"M438 853 c17 -7 17 -199 0 -206 -7 -3 5 -5 27 -5 22 0 34 2 28 5 -15 5 -18 103 -4 103 5 0 17 -24 27 -52 16 -43 24 -54 44 -56 15 -2 20 0 13 5 -6 4 -20 31 -31 59 -17 45 -17 52 -4 61 15 9 20 63 7 83 -3 5 -32 10 -63 9 -31 0 -51 -3 -44 -6z m85 -25 c14 -21 -6 -68 -28 -68 -11 0 -15 11 -15 46 0 37 3 45 16 42 9 -2 22 -11 27 -20z\"/>"
"<path d=\"M913 849 c14 -9 17 -26 17 -118 0 -79 -4 -112 -15 -127 -8 -11 -15 -14 -15 -6 0 17 -30 15 -30 -2 0 -37 38 -29 63 13 13 22 17 53 17 128 0 64 4 103 12 111 9 9 3 12 -27 12 -32 -1 -37 -3 -22 -11z\"/>"
"<path d=\"M998 853 c17 -7 17 -199 0 -206 -7 -3 15 -6 50 -6 60 -1 62 0 62 24 l0 25 -21 -21 c-35 -35 -44 -20 -47 81 -2 68 1 96 10 102 8 4 -3 7 -27 7 -22 0 -34 -3 -27 -6z\"/>"
"<path d=\"M631 796 l-21 -23 29 5 c56 8 80 -71 41 -135 -23 -38 -27 -39 -52 -16 -20 18 -34 12 -25 -12 16 -40 87 -2 113 61 19 46 18 77 -6 106 -28 36 -54 40 -79 14z\"/>"
"<path d=\"M1244 1133 c6 -11 -81 -70 -96 -65 -13 3 -9 -6 15 -43 32 -50 69 -57 82 -14 4 10 15 19 26 19 28 0 25 48 -7 85 -14 17 -23 25 -20 18z m34 -56 c5 -22 -24 -33 -44 -16 -14 12 -14 14 2 26 22 16 38 12 42 -10z m-50 -34 c4 -28 -23 -49 -43 -33 -20 17 -19 37 3 49 23 14 36 9 40 -16z\"/>"
"<path d=\"M263 1103 c-9 -16 -21 -35 -27 -43 -6 -8 -6 -12 -2 -8 10 9 73 -25 97 -52 14 -16 16 -16 33 13 12 22 13 27 2 18 -11 -10 -19 -9 -35 1 -12 7 -21 16 -21 20 0 5 -6 8 -14 8 -7 0 -19 6 -25 14 -10 12 -8 19 8 35 12 12 17 21 11 21 -5 0 -17 -12 -27 -27z\"/>"
"<path d=\"M310 1086 c0 -9 8 -14 20 -13 11 1 20 2 20 4 0 1 -9 7 -20 13 -16 8 -20 8 -20 -4z\"/>"
"<path d=\"M376 1065 c4 -8 8 -15 10 -15 2 0 4 7 4 15 0 8 -4 15 -10 15 -5 0 -7 -7 -4 -15z\"/>"
"<path d=\"M197 969 c-3 -10 0 -16 7 -13 6 2 27 -9 46 -25 l35 -30 -35 -2 c-19 -1 -44 -3 -55 -3 -11 -1 -21 -8 -23 -17 -2 -13 7 -13 68 -5 38 5 70 13 70 17 0 4 -10 13 -22 21 -13 9 -37 29 -55 45 -27 26 -32 27 -36 12z\"/>"
"<path d=\"M1331 985 c-1 -5 -15 -25 -32 -43 -24 -25 -38 -33 -56 -29 -18 3 -23 1 -19 -10 3 -8 6 -19 6 -23 0 -5 12 -3 26 5 15 7 33 11 42 8 9 -4 13 -2 9 4 -8 13 22 55 34 48 11 -7 11 3 -1 30 -5 11 -9 16 -9 10z\"/>"
"<path d=\"M1350 881 c0 -6 5 -13 10 -16 6 -3 10 1 10 9 0 9 -4 16 -10 16 -5 0 -10 -4 -10 -9z\"/>"
"<path d=\"M310 563 c0 -7 -16 -37 -35 -68 -32 -49 -33 -55 -16 -55 11 0 21 9 25 23 3 13 16 35 30 50 21 23 22 28 10 44 -8 10 -13 13 -14 6z\"/>"
"<path d=\"M1207 543 c-3 -5 6 -16 20 -27 l25 -19 -17 27 c-19 28 -21 30 -28 19z\"/>"
"<path d=\"M364 482 c3 -5 0 -13 -6 -15 -8 -3 -6 -6 5 -6 19 -1 23 14 5 25 -6 4 -8 3 -4 -4z\"/>"
"<path d=\"M1162 469 c-15 -24 -15 -25 6 -25 9 0 44 -4 77 -8 53 -7 58 -6 46 8 -7 8 -16 15 -19 14 -30 -3 -73 4 -85 16 -11 12 -15 11 -25 -5z\"/>"
"<path d=\"M308 453 c7 -3 16 -2 19 1 4 3 -2 6 -13 5 -11 0 -14 -3 -6 -6z\"/>"
"<path d=\"M403 434 c4 -6 -12 -32 -36 -56 l-42 -45 33 -28 c25 -21 36 -25 45 -16 8 8 5 11 -11 11 -41 0 -38 30 5 71 l41 38 -21 18 c-11 10 -17 13 -14 7z\"/>"
"<path d=\"M1114 398 c3 -13 6 -27 6 -33 0 -5 5 -3 10 5 7 11 12 12 21 3 12 -12 3 -37 -10 -28 -5 2 -3 -13 2 -34 8 -29 13 -36 22 -27 10 8 9 14 -4 23 -12 9 -14 16 -5 29 15 25 29 28 47 10 12 -12 17 -13 26 -3 8 10 -5 21 -56 46 -65 32 -66 32 -59 9z\"/>"
"<path d=\"M480 295 c-24 -24 -47 -42 -52 -39 -4 3 -8 3 -8 0 0 -3 10 -10 21 -17 16 -8 20 -8 14 0 -9 15 14 43 28 34 5 -3 7 -1 4 4 -4 6 0 16 7 23 12 10 15 5 14 -32 0 -24 -4 -46 -9 -49 -5 -3 1 -10 13 -16 23 -10 23 -9 25 63 1 41 -1 74 -5 74 -4 0 -28 -20 -52 -45z\"/>"
"<path d=\"M989 328 c-14 -10 -12 -17 17 -61 18 -28 30 -55 27 -60 -3 -6 13 1 35 13 23 13 42 31 41 39 0 12 -4 10 -16 -6 -8 -13 -21 -23 -28 -23 -14 0 -67 94 -58 104 10 9 0 6 -18 -6z\"/>"
"<path d=\"M613 255 c-1 -24 -7 -53 -12 -64 -8 -15 -7 -20 5 -25 27 -10 29 -7 30 48 0 30 6 62 14 70 11 14 9 16 -11 16 -21 0 -24 -5 -26 -45z\"/>"
"<path d=\"M880 285 c-19 -23 -7 -49 30 -61 39 -13 48 -28 30 -50 -11 -12 -10 -13 3 -8 22 8 31 42 17 60 -7 8 -27 18 -44 22 -39 10 -42 37 -3 37 15 -1 27 3 27 7 0 14 -47 9 -60 -7z\"/>"
"<path d=\"M688 274 c31 -22 29 -68 -5 -100 l-27 -25 29 11 c36 14 45 26 45 67 0 30 -28 63 -54 63 -6 0 0 -7 12 -16z\"/>"
"<path d=\"M778 271 c10 -6 13 -26 10 -70 -3 -52 -1 -61 14 -61 15 0 17 10 15 70 -2 67 -3 70 -27 70 -18 0 -21 -3 -12 -9z\"/>"
"<path d=\"M886 175 c4 -8 11 -15 16 -15 6 0 5 6 -2 15 -7 8 -14 15 -16 15 -2 0 -1 -7 2 -15z\"/>"
"</g>"
"</svg>");
 
}

void buildJavascript() {
  javaScript = "<SCRIPT>\n";
  javaScript += "var xmlHttp=createXmlHttpObject();\n";

  javaScript += "function createXmlHttpObject(){\n";
  javaScript += " if(window.XMLHttpRequest){\n";
  javaScript += "    xmlHttp=new XMLHttpRequest();\n";
  javaScript += " }else{\n";
  javaScript += "    xmlHttp=new ActiveXObject('Microsoft.XMLHTTP');\n";
  javaScript += " }\n";
  javaScript += " return xmlHttp;\n";
  javaScript += "}\n";

 
  javaScript += "function process(){\n";
  javaScript += " if(xmlHttp.readyState==0 || xmlHttp.readyState==4){\n";
  javaScript += "   xmlHttp.open('PUT','xml',true);\n";
  javaScript += "   xmlHttp.onreadystatechange=handleServerResponse;\n";
  javaScript += "   xmlHttp.send(null);\n";
  javaScript += " }\n";
  javaScript += " setTimeout('process()',0.1);\n";
  javaScript += "}\n";

  javaScript += "function handleServerResponse(){\n";
  javaScript += " if(xmlHttp.readyState==4 && xmlHttp.status==200){\n";
  javaScript += "   xmlResponse=xmlHttp.responseXML;\n";
  javaScript += "   xmldoc = xmlResponse.getElementsByTagName('response');\n";
  javaScript += "   message = xmldoc[0].firstChild.nodeValue;\n";
  javaScript += "   document.getElementById('runtime').innerHTML=message;\n";
  javaScript += " }\n";
  javaScript += "}\n";

  javaScript += "</SCRIPT>\n";
}

void buildXML() {
 XML = "<?xml version='1.0'?>";
  XML += "<response>";
  XML += currentlabel;
  XML += "</response>";
}

void handlePlace1()  //Web-интерфейс 1-го рабочего места
{
  buildJavascript();
  webPage = "<!DOCTYPE HTML> <html> <head> <title>1st WorkPlace Remote Console R3TJL</title>";
  webPage += "<meta http-equiv=\'refresh\' content=\'5\'>"; // обновление страницы
  webPage += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  webPage += "<style type=\"text/css\">";
  webPage += ".btn {background-color: #4CAF50; border: none; color: white; padding: 8px 16px; text-align: center; text-decoration: none; display: inline-block; font-size: 12px; margin: 1px 2px; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer;}";
  webPage += ".btn-on {background-color: white; color: black; border: 2px solid #4CAF50;}";
  webPage += ".btn-off {background-color: white; color: black; border: 2px solid #f44336;}";
  webPage += ".btn-blk {background-color: red; color: black; border: 2px solid #f44336;}";
  webPage += ".btn-blk2 {background-color: gray; color: black; border: 2px solid #f44336;}"; 
  
  // класс "btn-blk2" будет у неактивных антенн дальней группы, если одна из этих антенн активирована другим рабочим местом (и обозхначена классом "btn-blk")
    
  //webPage += ".btn-green {background-color: green; color: black; border: 2px solid #f44336;}";
  webPage += ".btn-on:active {background: green;}";
  webPage += ".btn-off:active {background: red;}";
  webPage += "</style> </head>";

  if (brd1 == "Ok !" && brd2 == "Ok !") {
  
  webPage += javaScript;
  webPage += "<body bgcolor=\"#c7daed\">";
  webPage += "<BODY onload='process()'>";
  /*webPage += "<h1>Remote console</h1>";*/
  webPage += "<p><h2><font color=\"green\" face=\"Arial\"> Our: </font><font color=\"red\" face=\"Arial\">First (1) </font><font color=\"green\" face=\"Arial\">WorkPlace!!! </font></h2></p>";
  webPage += "</BODY>";
  webPage += "<table border=\"1\" width=\"600\"><tbody>";
  
  // Ближняя группа антенн (5 выходов)
  webPage += "<tr><td colspan=2><p><h2><font color=\"red\">HOUSE Antenna'S Group </font></td></tr>"; 
    
  if (stat[0] == '1') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label0) + "<br></td>";}
    else if (stat[0] == '2') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label0) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl1s0On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label0) + "<br></td>";} 
  if (stat[1] == '1') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label1) + "<br></td></tr>";}
    else if (stat[1] == '2') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label1) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl1s1On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label1) + "<br></td></tr>";} 
  if (stat[2] == '1') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label2) + "<br></td>";}
    else if (stat[2] == '2') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label2) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl1s2On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label2) + "<br></td>";} 
  if (stat[3] == '1') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label3) + "<br></td></tr>";}
    else if (stat[3] == '2') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button></a>&nbsp; " + String(label3) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl1s3On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label3) + "<br></td></tr>";}
  if (stat[4] == '1') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label4) + "<br></td>";}
    else if (stat[4] == '2') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button></a>&nbsp; " + String(label4) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl1s4On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label4) + "<br></td>";} 
          
  webPage += "<td>&nbsp; <h3><a href=\"1stOff\"><button class=\"btn btn-off\">All 1st Ant OFF</button></a></h3></td></tr>";

 // Удаленная группа антенн (4 выхода) 
  webPage += "<tr><td colspan=2><p><h2><font color=\"red\">REMOTE Antenna'S Group </font></td></tr>"; 
   
  if (stat[5] == '1') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label5) + "<br></td>";}
    else if (stat[5] == '2') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label5) + "<br></td>";}
        else if (stat[6] == '2' || stat[7] == '2' || stat [8] == '2') {webPage += "<tr><td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label5) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl1s5On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label5) + "<br></td>";} 
  if (stat[6] == '1') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label6) + "<br></td></tr>";}
    else if (stat[6] == '2') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label6) + "<br></td></tr>";}
        else if (stat[5] == '2' || stat[7] == '2' || stat [8] == '2') {webPage += "<td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label6) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl1s6On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label6) + "<br></td></tr>";}
  if (stat[7] == '1') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label7) + "<br></td>";}
    else if (stat[7] == '2') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label7) + "<br></td>";}
        else if (stat[5] == '2' || stat[6] == '2' || stat [8] == '2') {webPage += "<tr><td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button></a>&nbsp; " + String(label7) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl1s7On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label7) + "<br></td>";} 
  
  if (stat[8] == '1') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label8) + "<br></td></tr>";}
    else if (stat[8] == '2') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label8) + "<br></td></tr>";}
        else if (stat[5] == '2' || stat[6] == '2' || stat [7] == '2') {webPage += "<td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button></a>&nbsp; " + String(label8) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl1s8On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label8) + "<br></td></tr>";} 
  webPage += "</tbody></table>";
  }
  else 
    {
      webPage += "<p><h2><font color=\"red\" face=\"Arial\">!!! ERROR Init ExtBoards </font></h2></p>";
      webPage += "<p><a href =\"softreset\">RESET controller</a></p>";
    }
  webPage += "<p><a href ='/'>Return to the home page</a></p>";  
  server.send(200, "text/html", webPage);
  }


void handlePlace2()  //Web-интерфейс 2-го рабочего места
{
  buildJavascript();
  webPage = "<!DOCTYPE HTML> <html> <head> <title>2nd WorkPlace Remote Console R3TJL</title>";
  webPage += "<meta http-equiv=\'refresh\' content=\'5\'>"; //обновление страницы
  webPage += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  webPage += "<style type=\"text/css\">";
  webPage += ".btn {background-color: #4CAF50; border: none; color: white; padding: 8px 16px; text-align: center; text-decoration: none; display: inline-block; font-size: 12px; margin: 1px 2px; -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer;}";
  webPage += ".btn-on {background-color: white; color: black; border: 2px solid #4CAF50;}";
  webPage += ".btn-off {background-color: white; color: black; border: 2px solid #f44336;}";
  webPage += ".btn-blk {background-color: red; color: black; border: 2px solid #f44336;}";
  webPage += ".btn-blk2 {background-color: gray; color: black; border: 2px solid #f44336;}"; 
  
  // класс "btn-blk2" будет у неактивных антенн дальней группы, если одна из этих антенн активирована другим рабочим местом (и обозхначена классом "btn-blk")
    
    webPage += ".btn-on:active {background: green;}";
  webPage += ".btn-off:active {background: red;}";
  webPage += "</style> </head>";

  if (brd1 == "Ok !" && brd2 == "Ok !") {
  
  webPage += javaScript;
  webPage += "<body bgcolor=\"#c7daed\">";
  webPage += "<BODY onload='process()'>";
  /*webPage += "<h1>Remote console</h1>";*/
  webPage += "<p><h2><font color=\"green\" face=\"Arial\"> Our: </font><font color=\"red\" face=\"Arial\">Second (2) </font><font color=\"green\" face=\"Arial\">WorkPlace!!! </font></h2></p>";
  webPage += "</BODY>";
  webPage += "<table border=\"1\" width=\"600\"><tbody>";
  
  // Ближняя группа антенн (5 выходов)
  webPage += "<tr><td colspan=2><p><h2><font color=\"red\">HOUSE Antenna'S Group </font></td></tr>"; 
  
  if (stat[0] == '2') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label0) + "<br></td>";}
    else if (stat[0] == '1') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label0) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl2s0On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label0) + "<br></td>";} 
  if (stat[1] == '2') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label1) + "<br></td></tr>";}
    else if (stat[1] == '1') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label1) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl2s1On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label1) + "<br></td></tr>";} 
  if (stat[2] == '2') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label2) + "<br></td>";}
    else if (stat[2] == '1') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label2) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl2s2On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label2) + "<br></td>";} 
  if (stat[3] == '2') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label3) + "<br></td></tr>";}
    else if (stat[3] == '1') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label3) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl2s3On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label3) + "<br></td></tr>";}
  if (stat[4] == '2') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label4) + "<br></td>";}
    else if (stat[4] == '1') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label4) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl2s4On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label4) + "<br></td>";} 
          
  webPage += "<td>&nbsp; <h3><a href=\"2ndOff\"><button class=\"btn btn-off\">All 2nd Ant OFF</button></a></h3></td></tr>";

 // Удаленная группа антенн (4 выхода) 
  webPage += "<tr><td colspan=2><p><h2><font color=\"red\">REMOTE Antenna'S Group </font></td></tr>"; 
    
  if (stat[5] == '2') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label5) + "<br></td>";}
    else if (stat[5] == '1') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label5) + "<br></td>";}
        else if (stat[6] == '1' || stat[7] == '1' || stat [8] == '1') {webPage += "<tr><td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label5) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl2s5On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label5) + "<br></td>";} 
  if (stat[6] == '2') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label6) + "<br></td></tr>";}
    else if (stat[6] == '1') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label6) + "<br></td></tr>";}
        else if (stat[5] == '1' || stat[7] == '1' || stat [8] == '1') {webPage += "<td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label6) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl2s6On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label6) + "<br></td></tr>";}
  if (stat[7] == '2') {webPage += "<tr><td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label7) + "<br></td>";}
    else if (stat[7] == '1') {webPage += "<tr><td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label7) + "<br></td>";}
        else if (stat[5] == '1' || stat[6] == '1' || stat [8] == '1') {webPage += "<tr><td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label7) + "<br></td>";}
          else {webPage += "<tr><td><p><h3><a href=\"pl2s7On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label7) + "<br></td>";} 
  
  if (stat[8] == '2') {webPage += "<td><p><font color=\"green\" face=\"Arial\"><h3><button class=\"btn btn-green\">ON</button>&nbsp; " + String(label8) + "<br></td></tr>";}
    else if (stat[8] == '1') {webPage += "<td><p><font color=\"red\" face=\"Arial\"><h3><button class=\"btn btn-blk\">BLK</button>&nbsp; " + String(label8) + "<br></td></tr>";}
        else if (stat[5] == '1' || stat[6] == '1' || stat [7] == '1') {webPage += "<td><p><font color=\"gray\" face=\"Arial\"><h3><button class=\"btn btn-blk2\">BLK</button>&nbsp; " + String(label8) + "<br></td></tr>";}
          else {webPage += "<td><p><h3><a href=\"pl2s8On\"><button class=\"btn btn-on\">OFF</button></a>&nbsp; " + String(label8) + "<br></td></tr>";} 

  webPage += "</tbody></table>";

  }
    else
      {
        webPage += "<p><h2><font color=\"red\" face=\"Arial\">!!! ERROR Init ExtBoards </font></h2></p>";
        webPage += "<p><a href =\"softreset\">RESET controller</a></p>";
      }
  
  webPage += "<p><a href ='/'>Return to the home page</a></p>";  
  server.send(200, "text/html", webPage);
  }


void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
 /* server.send(200, "text/html", "");*/
 String Page;
  BuildSVG(); // это построение "логотипа" 
  Page =F(
    "<!DOCTYPE HTML> <html><head></head><body>"
    "<h1>Start page R3TJL Remote console</h1>"    
  );
  if (server.client().localIP() == apIP) {
    Page += (String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  } else {
    Page += (String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  
  Page += (String("<p>Initialize First ExtBoard: ") + brd1 + String("  Second ExtBoard: ") + brd2 + "</p>");
  if (brd1 == "Ok !" && brd2 == "Ok !") { 
    Page += "<h2>Please SELECT our WorkPlace: ";
    Page += "(Left - 1st, right - 2nd)</h2>";
    Page += F(
      "<p><h1><a href='/place1'>1st WorkPlace</a></h1></p>"
      "<p><h1><a href='/place2'>2nd WorkPlace</a></h1></p>"
      "<p><a href='/wifi'>Config the wifi connection</a></p>"
      "<p><a href='/label'>Config name of Ant labels</a></p>");
  }
    else
    {
      Page += "<p><h2><font color=\"red\" face=\"Arial\">!!! ERROR Init ExtBoards </font></h2></p>";
      Page += "<p><a href =\"softreset\">RESET controller</a></p>"; 
    }
      
    Page += SVG; //вывод логотипа
    Page +="</body></html>";
 
  server.send(200, "text/html", Page);
}

void handleLabel() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  /*server.setContentLength(CONTENT_LENGTH_UNKNOWN);*/
  
  String Page2;
  Page2 = F(
    "<html><head></head><body>"
    "<h1>Name of labels configuration</h1>"
  );
   Page2 += F(
    "\r\n<br /><form method='POST' action='labelsave'><h3>Label's captions:</h3>"
    "<br />HOME Antenna group"
    "<br /><input type='text' placeholder='label0' name='l0'/>"
    "<br /><input type='text' placeholder='label1' name='l1'/>"
    "<br /><input type='text' placeholder='label2' name='l2'/>"
    "<br /><input type='text' placeholder='label3' name='l3'/>"
    "<br /><input type='text' placeholder='label4' name='l4'/>"
    "<br />REMOTE Antenna group"
    "<br /><input type='text' placeholder='label5' name='l5'/>"
    "<br /><input type='text' placeholder='label6' name='l6'/>"
    "<br /><input type='text' placeholder='label7' name='l7'/>"
    "<br /><input type='text' placeholder='label8' name='l8'/>"
    "<br /><input type='submit' value='Save captions'/></form>"
//  //"<p><a href='/'>Return to the home page</a>.</p>"
    "</body></html>"
  );
  Page2 += "<h5><p>L0: " + String(label0);
  Page2 += "  L1: " + String(label1);
  Page2 += "  L2: " + String(label2) + "</p>";
  Page2 += "<p>L3: " + String(label3);
  Page2 += "  L4: " + String(label4) + "</p>";
  Page2 += "<p>L5: " + String(label5);
  Page2 += "  L6: " + String(label6);
  Page2 += "  L7: " + String(label7);
  Page2 += "  L8: " + String(label8) + "</p>";
  Page2 += "<p><h3><a href='/'>Return to the home page</a>.</p>";
   /*server.client().stop(); */
  server.send(200, "text/html", Page2);
}

void handleLabelSave() {
  Serial.println("labels save");
  server.arg("l0").toCharArray(label0, sizeof(label0) - 1);
  server.arg("l1").toCharArray(label1, sizeof(label1) - 1);
  server.arg("l2").toCharArray(label2, sizeof(label2) - 1);
  server.arg("l3").toCharArray(label3, sizeof(label3) - 1);
  server.arg("l4").toCharArray(label4, sizeof(label4) - 1);
  server.arg("l5").toCharArray(label5, sizeof(label5) - 1);
  server.arg("l6").toCharArray(label6, sizeof(label6) - 1);
  server.arg("l7").toCharArray(label7, sizeof(label7) - 1);
  server.arg("l8").toCharArray(label8, sizeof(label8) - 1);
  server.sendHeader("Location", "label", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveLabels();
  /*connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID*/
}

void loadLabels() {
  EEPROM.begin(512);
  EEPROM.get(100, label0);
  EEPROM.get(100 + sizeof(label0), label1);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1), label2);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2), label3);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3), label4);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4), label5);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5), label6);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6), label7);
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6) + sizeof(label7), label8);
  char ok[7 + 1];
  EEPROM.get(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6) + sizeof(label7) + sizeof(label8), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    label0[0] = 0;
    label1[0] = 0;
    label2[0] = 0;
    label3[0] = 0;
    label4[0] = 0;
    label5[0] = 0;
    label6[0] = 0;
    label7[0] = 0;  
    label8[0] = 0;    
  }
  Serial.println("Labels:");
  Serial.println(label0);
  Serial.println(label1);
  Serial.println(label2);
  Serial.println(label3);
  Serial.println(label4);
  Serial.println(label5);
  Serial.println(label6);
  Serial.println(label7);
  Serial.println(label8);
}

/** Store WLAN credentials to EEPROM */
void saveLabels() {
  EEPROM.begin(512);
  EEPROM.put(100, label0);
  EEPROM.put(100 + sizeof(label0), label1);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1), label2);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2), label3);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3), label4);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4), label5);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5), label6);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6), label7);
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6) + sizeof(label7), label8);
  char ok[8 + 1] = "OK";
  EEPROM.put(100 + sizeof(label0) + sizeof(label1) + sizeof(label2) + sizeof(label3) + sizeof(label4) + sizeof(label5) + sizeof(label6) + sizeof(label7) + sizeof(label8), ok);
  EEPROM.commit();
  EEPROM.end();
}

void handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  /*server.setContentLength(CONTENT_LENGTH_UNKNOWN);*/
   

  Page3 = F(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (server.client().localIP() == apIP) {
    Page3 +=(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  } else {
    Page3 +=(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
    
  }
  Page3 +=("<p>Uptime - " + String(day) + " day " + String(hour) + " h " + String(minute) + " m " + String(sec) + " s ""</p>");
  Page3 += ("<p><h3><a href=\"APon\"><button class=\"btn btn-on\">ON</button></a>&nbsp;<a href=\"APoff\"><button class=\"btn btn-off\">OFF</button></a> Turn on/off Access Point</h3></p>");
  if (flag_off==1) Page3 += ("<p><h3> AP OFF</h3></p>");
  if (flag_off==0) Page3 += ("<p><h3> AP ON</h3></p>");
  Page3 += F(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  Page3 +=(String() + "<tr><td>SSID " + String(softAP_ssid) + "</td></tr>");
  Page3 +=(String() + "<tr><td>IP " + WiFi.softAPIP().toString() + "</td></tr>");
 
  Page3 += F(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  Page3 +=(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  Page3 +=(String() + "<tr><td>IP " + WiFi.localIP().toString() + "</td></tr>");
  Page3 += F(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      Page3 +=(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : " *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    Page3 +=(String() + "<tr><td>No WLAN found</td></tr>");
  }
  Page3 += F(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p><a href='/'>Return to the home page</a>.</p>"
    "</body></html>"
  );
  /*server.client().stop(); */
  server.send(200, "text/html", Page3);
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}

void handleWifiSave() {
  Serial.println("wifi save");
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0 + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0 + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}

void turnOffPlace1()
{
  currentlabel1="OFF";
  //for (uint8_t i = 0; i < 8; i++) {
  if (stat[0] != '2') {stat[0]='0';  brd11 = pcf1.digitalWrite(P0, HIGH);}
  if (stat[1] != '2') {stat[1]='0';  brd11 = pcf1.digitalWrite(P1, HIGH);}
  if (stat[2] != '2') {stat[2]='0';  brd11 = pcf1.digitalWrite(P2, HIGH);}
  if (stat[3] != '2') {stat[3]='0';  brd11 = pcf1.digitalWrite(P3, HIGH);}    
  if (stat[4] != '2') {stat[4]='0';  brd11 = pcf1.digitalWrite(P4, HIGH);}    
  if (stat[5] != '2') {stat[5]='0';  brd11 = pcf1.digitalWrite(P5, HIGH); brd11 = pcf1.digitalWrite(P6, HIGH); }    
  if (stat[6] != '2') {stat[6]='0';  brd11 = pcf1.digitalWrite(P5, HIGH); brd11 = pcf1.digitalWrite(P7, HIGH); }    
  if (stat[7] != '2') {stat[7]='0';  brd11 = pcf1.digitalWrite(P5, HIGH); brd22 = pcf2.digitalWrite(P6, HIGH); }        
  if (stat[8] != '2') {stat[8]='0';  brd11 = pcf1.digitalWrite(P5, HIGH); brd22 = pcf2.digitalWrite(P7, HIGH); }    
} 

void turnOffPlace2()
{
  currentlabel2="OFF";
  
  if (stat[0] != '1') {stat[0]='0';  brd22 = pcf2.digitalWrite(P0, HIGH);}
  if (stat[1] != '1') {stat[1]='0';  brd22 = pcf2.digitalWrite(P1, HIGH);}
  if (stat[2] != '1') {stat[2]='0';  brd22 = pcf2.digitalWrite(P2, HIGH);}
  if (stat[3] != '1') {stat[3]='0';  brd22 = pcf2.digitalWrite(P3, HIGH);}    
  if (stat[4] != '1') {stat[4]='0';  brd22 = pcf2.digitalWrite(P4, HIGH);}    
  if (stat[5] != '1') {stat[5]='0';  brd22 = pcf2.digitalWrite(P5, HIGH); brd11 = pcf1.digitalWrite(P6, HIGH); }    
  if (stat[6] != '1') {stat[6]='0';  brd22 = pcf2.digitalWrite(P5, HIGH); brd11 = pcf1.digitalWrite(P7, HIGH); }    
  if (stat[7] != '1') {stat[7]='0';  brd22 = pcf2.digitalWrite(P5, HIGH); brd22 = pcf2.digitalWrite(P6, HIGH); }        
  if (stat[8] != '1') {stat[8]='0';  brd22 = pcf2.digitalWrite(P5, HIGH); brd22 = pcf2.digitalWrite(P7, HIGH); }    
  
} 

void softreset() {
  webPage += "<script> document.location.href = \"/\"</script>";
  server.send(200, "text/html", webPage);
  ESP.restart();
  //ESP.reset();
  handleRoot();
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int connRes = WiFi.waitForConnectResult();
  Serial.print("connRes: ");
  Serial.println(connRes);
}