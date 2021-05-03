#include <WiFi.h>
#include <WiFiManager.h>         
#include <ESPmDNS.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h" 
#include <WebSocketsServer.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputMixer.h"
#include "PCF8574.h"
#include <LiquidCrystal_I2C.h>

//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
//#include "logo.h"
//
//#define SCREEN_WIDTH 128 // OLED display width, in pixels
//#define SCREEN_HEIGHT 64 // OLED display height, in pixels
//#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
char buff[10];

String message, XML,songs="",jadwal="0";
String lastSd;
String lastSt;
boolean sends=true;
RtcDS3231<TwoWire> Rtc(Wire);
RtcDateTime now;
LiquidCrystal_I2C lcd(0x27, 16, 2);  
AudioFileSourceSD *source = NULL;
AudioOutputI2S *output = NULL;
AudioGeneratorMP3 *decoder = NULL;

AudioOutputMixer *mixer;
AudioOutputMixerStub *stub[2];
int state = HIGH;

char weekDay[][7] = {"MINGGU", "SENIN", "SELASA", "RABU", "KAMIS", "JUM'AT", "SABTU", "MINGGU"}; // array hari, dihitung mulai dari senin, hari senin angka nya =0,
char monthYear[][4] = { "DES", "JAN", "FEB", "MAR", "APR", "MEI", "JUN", "JUL", "AGU", "SEP", "OKT", "NOV", "DES" };

String namaJadwal="";
void buildXML(){
  RtcDateTime now = Rtc.GetDateTime();
  RtcTemperature temp = Rtc.GetTemperature();
  XML="<?xml version='1.0'?>";
  XML+="<t>";
    XML+="<rYear>";
    XML+=now.Year();
    XML+="</rYear>";
    XML+="<rMonth>";
    XML+=now.Month();
    XML+="</rMonth>";
    XML+="<rDay>";
    XML+=now.Day();
    XML+="</rDay>";
    XML+="<rHour>";
    XML+=now.Hour();
    XML+="</rHour>";
    XML+="<rMinute>";
    XML+=now.Minute();
    XML+="</rMinute>";
    XML+="<rSecond>";
    XML+=now.Second();
    XML+="</rSecond>";
    XML+="<rTemp>";
    XML+= temp.AsFloatDegC();
    XML+="</rTemp>";
    XML+="<rSongs>";
    XML+= songs;
    XML+="</rSongs>";
    XML+="<rJadwal>";
    XML+= jadwal;
    XML+="</rJadwal>";
  XML+="</t>"; 
}


int LED_BUILTIN = 2;

const int ledPin = 2;
String ledState;
AsyncWebServer  server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

PCF8574 pcf1(0x20);
PCF8574 pcf2(0x21),pcf3(0x22),pcf4(0x23);//,pcf5(0x24);
PCF8574 pcf[] = {pcf1,pcf2,pcf3,pcf4};
int pcfCount = sizeof(pcf)/sizeof(pcf1);
//int pin[] = {15,0,4,16,17,5,18,  19,21,3,22, 23,13,12,14, 27,26,25,33, 32,35,34,39};
//int ss = sizeof(pin)/sizeof(pin[0]);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  //Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      {
        String text = (char *) payload;
        //Serial.println(text);
        if (text.startsWith("play_")){
          text.replace("play_","");
          const char* string1 = text.c_str();
          //Serial.println(text);
          playFile(string1);
          sends=false;
        }else if (text.startsWith("stop_")){
          stopPlayer();
        }else if (text.startsWith("save_")){
          text.replace("save_","");
          saveJadwal(text);
        }else if (text.startsWith("update_")){
          text.replace("update_","");
          updateJadwal(text);
        }else if (text.startsWith("delete_")){
          text.replace("delete_","");
          deleteJadwal(text);
        }else if (text.startsWith("addRooms_")){
          text.replace("addRooms_","");
          addRooms(text);
        }else if (text.startsWith("updateNamaRooms_")){
          text.replace("updateNamaRooms_","");
          updateNamaRooms(text);
        }else if (text.startsWith("updateRooms_")){
          text.replace("updateRooms_","");
          updateRooms(text);
        }else if (text.startsWith("deleteRooms_")){
          text.replace("deleteRooms_","");
          deleteRooms(text);
        }else if (text.startsWith("deleteSongs_")){
          text.replace("deleteSongs_","");
          deleteSongs(text);
        }else if (text.startsWith("request_data")){
          getSongs();
          getRooms();
          getNamaRooms();
          getSpeakerState();
          getJadwal();
          getTime();
        }else if (text.startsWith("get_jadwal")){
          getJadwal();
        }else if (text.startsWith("updateDate_")){
          text.replace("updateDate_","");
          updateDate(text);
        }else if (text.startsWith("updateTime_")){
          text.replace("updateTime_","");
          updateTime(text);
        }else if (text.startsWith("changeState_")){
          text.replace("changeState_","");
          changeSpeakerState(text);
        }else{
          turnSpeaker(text);
        }
        break;
      }
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\r\n", num, length);
      break;
    default:
      //Serial.println("Invalid WStype");
      break;
  }
}

String processor(const String& var){
    //Serial.println(var);
  if(var == "STATE"){
    //if(digitalRead(ledPin)){
    //  ledState = "ON";
    //}
    //else{
      ledState = "OFF";
    //}
    Serial.print(ledState);
    return ledState;
  }
  return String();
}
void setup() {
  
  Serial.begin(115200);
  
  startRTC();
  //RTC D3231

//  WiFiManager wifiManager;
//  wifiManager.autoConnect("MiSenTek WSAS", "B1sm1llah");
  WiFi.softAP("MiSenTek WSAS", "B1sm1llah");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  //Serial.println(myIP);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
  if (MDNS.begin("http://misentek.local/")) {
    //Serial.println("MDNS responder started");
  }
  Serial.print("Init pcf8574...");
  for (int x=0;x<pcfCount;x++){
    for(int i=0;i<8;i++) {
      pcf[x].pinMode(i, OUTPUT);
      //pcf[x].pinMode(i, OUTPUT);
    }
    if (pcf[x].begin()){
      Serial.println(" PCF " + String(x)+" OK");
    }else{
      //Serial.println(" PCF " + String(x)+" FAILED");
    }
  }
  
  
  if(!SPIFFS.begin(true)){
    //Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  SD.begin();
  File root = SD.open("/");
  if(!root){
    Serial.println("sd card failed");
  }
  
  source = new AudioFileSourceSD();
  output = new AudioOutputI2S(0,1,8,0);
  decoder = new AudioGeneratorMP3();

  //Serial.println("SCL : " + String(digitalRead(SCL)));
  //Serial.println("SDA : " + String(digitalRead(SDA)));


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/favicon.ico", "image/x-icon");
  });
  
  server.on("/play.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/play.png", "image/png");
  });
  server.on("/stop.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/stop.png", "image/png");
  });
 
  // Route to load style.css file
  server.on("/sb-admin-2.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/sb-admin-2.min.css", "text/css");
  });
  // Route to load style.css file
  server.on("/font.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/font.css", "text/css");
  });
  // Route to load style.css file
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquery.min.js", "text/css");
  });
  // Route to load style.css file
  server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/bootstrap.bundle.min.js", "text/javascript");
  });
  // Route to load style.css file
  server.on("/jquery.easing.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/jquery.easing.min.js", "text/javascript");
  });
  // Route to load style.css file
  server.on("/sb-admin-2.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/sb-admin-2.min.js", "text/javascript");
  });
  // Route to load style.css file
  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/settings.json", "text/javascript");
  });
  // Route to load style.css file
  server.on("/setting_jam.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_jam.html", String(), false, processor);
  });
  server.on("/setting_jadwal.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_jadwal.html", String(), false, processor);
  });
  server.on("/setting_wifi.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_wifi.html", String(), false, processor);
  });
  server.on("/setting_backup.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_backup.html", String(), false, processor);
  });
  server.on("/setting_nada.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_nada.html", String(), false, processor);
  });
  server.on("/setting_ruangan.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/setting_ruangan.html", String(), false, processor);
  });
  server.on("/xml", HTTP_GET, [](AsyncWebServerRequest *request){
    buildXML();
    request->send(200,"text/xml",XML);
  });
  
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, handleUpload);
  server.begin();
  Serial.print("Connected to ");  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  lcd.init();
    // turn on LCD backlight                      
  lcd.backlight();
//  //Serial.println("display begin");
//  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
//    //Serial.println("display failed");  
//  }  
//  //Serial.println("display connected");  
//  // Clear the buffer.
//  display.clearDisplay();
//
//  // Display bitmap
//  //Serial.println("drawing bitmap");
//  display.drawBitmap(0, 0,  MarilynMonroe, 128, 64, WHITE);
//  display.display();
  changeSpeakerState("speaker_off;Semua Ruangan");
  getJadwal();
  
}
void loop() {
  webSocket.loop();
  belScheduler();
  if (decoder) {
    if (decoder->isRunning()){
      if (!decoder->loop()) {
        stopPlayer();
        namaJadwal="";
      }else{
        if (namaJadwal!=""){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(namaJadwal);
        }
      }
    }
  }

  
} 

void startRTC(){
  int rtn = I2C_ClearBus(); // clear the I2C bus first before calling Wire.begin()
  if (rtn != 0) {
    //Serial.println(F("I2C bus error. Could not clear"));
    if (rtn == 1) {
      //Serial.println(F("SCL clock line held low"));
    } else if (rtn == 2) {
      //Serial.println(F("SCL clock line held low by slave clock stretch"));
    } else if (rtn == 3) {
      //Serial.println(F("SDA data line held low"));
    }
  } else { // bus clear, re-enable Wire, now can start Wire Arduino master
    Wire.begin();
  }
  
  Rtc.Begin();

  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true);
  }
  
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  
  //Serial.println("Setup RTC selesai");
}

void belScheduler() {
  ////Serial.println("Start scheduler");
  RtcDateTime now = Rtc.GetDateTime();
  char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
  int tahun = now.Year();
  int bulan = now.Month();
  int tanggal = now.Day();
  int hari = now.DayOfWeek();
  int jamRTC = now.Hour();
  int menitRTC = now.Minute();
  int detikRTC = now.Second();
  
  lcd.setCursor(0,0);
  lcd.print(daysOfTheWeek[now.DayOfWeek()]);
  lcd.print(",");
  printAngka(tanggal);
  lcd.print("-");
  printAngka(bulan);
  lcd.print("-");
  printAngka(tahun);  
    
  lcd.setCursor(4,1);
  printAngka(jamRTC);
  lcd.print(":");
  printAngka(menitRTC);
  lcd.print(":");
  printAngka(detikRTC);
  delay(100);

  
  
  int count;
  int posLine= 0;
  int posDel=0;

  if (sizeof(jadwal)>0){
    ////Serial.println("jadwal ada");
    while (posLine<= jadwal.lastIndexOf("\n")) {
      if (jadwal.indexOf("\n",posLine)>0){
        boolean ketemuHari=false;
        ////Serial.println("ketemu char " + String(jadwal.indexOf("\n",posLine)) +" dari "+ String(jadwal.lastIndexOf("\n")));
        posDel=posLine;
        String nama, haris, jams, nada, ruangan;
        String intHari, intJam, intMenit;
        for (int i=0;i<=4;i++){
          String val = jadwal.substring(posDel,jadwal.indexOf(";",posDel));
         
          val.replace("jadwal__","");
          if (i==0){
            nama=val;
          }else if(i==1){
            haris=val;
            int posHari=0;
            for (int x=0;x<7;x++){
              String valHari=val.substring(posHari,val.indexOf(",",posHari));
              
              ////Serial.println("valhari : " + valHari);
              if (valHari.endsWith("_on")){
                
               // //Serial.println("hari sekarang : " + String(hari));
               // //Serial.println("hari yang on " + String(x+1));
                if (hari==x+1){
                  ketemuHari = true;
                 // //Serial.println("ketemu jadwal hari ini " + String(valHari));
                  break;
                }
              }
              posHari=val.indexOf(",",posHari)+1;
            }
          }else if(i==2){
            intJam = val.substring(0,val.indexOf(":"));
            intMenit = val.substring(val.indexOf(":")+1,sizeof(val)+1);
          }else if(i==3){
            nada=val;
          }else if(i==4){
            ruangan=val;
             
          }

          posDel=jadwal.indexOf(";",posDel)+1;
         // //Serial.println("barisnya " + String(val));
        }
        if (ketemuHari){
          
          ////Serial.println("ketemu hari");
          ////Serial.println("jam sekarang" + String(jamRTC) + ":" + String(menitRTC)+ ":" + String(detikRTC));
          ////Serial.println("jadwal :" + String(intJam) + ":" + String(intMenit));
          if (jamRTC==intJam.toInt() && menitRTC==intMenit.toInt() && detikRTC==0){
            namaJadwal=nama;
            Serial.println("menyalakan ruangan " + ruangan);
            turnRoomsOn(ruangan, nada);
            sends=false;
            //Serial.println("mainkan " + nada);
          }
          
        }
        
        count++;
        posLine=jadwal.indexOf("\n",posLine)+1;
      }
    }
  }
}

void printAngka(int digits){
   if(digits < 10){
   lcd.print('0');
   lcd.print(digits);
   }
   else lcd.print(digits);
}
 
void playFile(const char* filename){
  
  if (decoder) {
    if (decoder->isRunning()) {
      decoder->stop();
    }
  }else{
    decoder = new AudioGeneratorMP3();
  } 
  source->close();
  if (source->open(filename)) { 
    Serial.printf_P(PSTR("Playing '%s' from SD card...\n"), filename);
    decoder->begin(source, output);
    //decoder->begin(source, stub[0]);
  } else {
    Serial.printf_P(PSTR("Error opening '%s'\n"), filename);
  } 
}

void stopPlayer(){
  if ((decoder) && (decoder->isRunning())) {
    decoder->stop();
    String udah = "udah_";
    const char* c= udah.c_str();
    webSocket.broadcastTXT(c);
    File f2 = SPIFFS.open("/speakerstate.txt", "r");
    if (!f2) {
      //Serial.println("open failed");
    }
    String rooms="";
    //Serial.println("open oke");
    while(f2.available()) {
      //Serial.println("mulai membaca rooms ");
      String line = f2.readStringUntil('\n');
      rooms += line;
      Serial.println("isi rooms : " + rooms);
    }
    f2.close();
    if (rooms.startsWith("speaker_off")){
      turnSpeaker("speaker_all_off");  
    }
  }
}



void getTime(){
  RtcDateTime now = Rtc.GetDateTime();
  RtcTemperature temp = Rtc.GetTemperature();
  String waktu = "waktu__"+String(now.Year())+","+String(now.Month()-1)+","+String(now.Day())+","+String(now.Hour())+","+String(now.Minute())+","+String(now.Second())+";"+String(temp.AsFloatDegC());
  //Serial.println("getTime : " + waktu);
  const char* c= waktu.c_str();
  
  webSocket.broadcastTXT(c);
}

void getSongs(){
  Serial.println("opening sd card ");
  File root = SD.open("/");
  songs="";
  if(!root){
    Serial.println("sd card failed");
    return;
  }
  if(!root.isDirectory()){
    //Serial.println("sd card failed : not directory");
    return;
  }
  //Serial.println("open sd card success");
  File file = root.openNextFile();
  while(file){
    if (String(file.name()).endsWith(".mp3")){
      //Serial.println(String(file.isDirectory()?"Dir ":"File ")+String(file.name())+"\t");
      songs += String(file.name())+",";  
    }
    file = root.openNextFile();
  }
  file.close();
  songs = "songs__"+songs;
  const char* c= songs.c_str();
  webSocket.broadcastTXT(c);
}

void saveJadwal(String strJadwal){
  //Serial.println("appending " + strJadwal);
  File f = SPIFFS.open("/f.txt", "a");
  
  if (!f) {
    //Serial.println("File doesn't exist yet. Creating it");
    File f = SPIFFS.open("/f.txt", "w");
    if (!f) {
      //Serial.println("file creation failed");
    }else{
      f.println(strJadwal);  
      //Serial.println("bikin file baru berhasil");
    }
  }else{
    //Serial.println("file ketemu, isinya : " + f.read());
    f.println(strJadwal);
  }
  f.close();
  getJadwal();
}


void updateJadwal(String strJadwal){
  //Serial.println("updating " + strJadwal);
  String nama = strJadwal.substring(0,strJadwal.indexOf(";"));
  //Serial.println("updating jadwal " + nama);
  
  File f2 = SPIFFS.open("/f.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  jadwal="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca jadwal ");
    String line = f2.readStringUntil('\n');
    String n= line.substring(0,line.indexOf(";"));
    if (nama==n){
      jadwal += strJadwal+"\n";
    }else{
      jadwal += line +"\n";
    }
    //Serial.println("isi jadwal baru : " + jadwal);
  }
  f2.close();
  File f = SPIFFS.open("/f.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.print(jadwal);  
    //Serial.println("bikin file baru berhasil");
  }
  f.close();
  getJadwal();
}

void deleteJadwal(String strJadwal){
  String nama = strJadwal.substring(0,strJadwal.indexOf(";"));
  //Serial.println("Menghapus jadwal " + nama);
  
  File f2 = SPIFFS.open("/f.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  jadwal="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca jadwal ");
    String line = f2.readStringUntil('\n');
    String n= line.substring(0,line.indexOf(";"));
    if (nama!=n){
      jadwal += line +"\n";
    }
    //Serial.println("isi jadwal baru : " + jadwal);
  }
  f2.close();
  File f = SPIFFS.open("/f.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.print(jadwal);  
    //Serial.println("bikin file baru berhasil");
  }
  f.close();
  getJadwal();
}

void getJadwal(){
  File f2 = SPIFFS.open("/f.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  jadwal="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca jadwal ");
    String line = f2.readStringUntil('\n');
    jadwal += line +"\n";
    //Serial.println("isi jadwal : " + jadwal);
  }
  jadwal = "jadwal__"+jadwal;
  const char* c= jadwal.c_str();
  webSocket.broadcastTXT(c);
  f2.close();
}

void getJadwalHari(String hari){
  File f2 = SPIFFS.open("/f.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  jadwal="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca jadwal ");
    String line = f2.readStringUntil('\n');
    
    jadwal += line +"\n";
    //Serial.println("isi jadwal : " + jadwal);
  }
  jadwal = "jadwal__"+jadwal;
  const char* c= jadwal.c_str();
  webSocket.broadcastTXT(c);
  f2.close();
}

void updateDate(String strDate){

    uint16_t tahun;
    uint8_t bulan;
    uint8_t tanggal;      
    String sd= strDate;
    
    //Serial.println("mengubah tanggal : " + sd);
    
//    tahun = ((sd[1]-'0')*1000)+((sd[2]-'0')*100)+((sd[3]-'0')*10)+(sd[4]-'0');
//    bulan = ((sd[6]-'0')*10)+(sd[7]-'0');
//    tanggal = ((sd[9]-'0')*10)+(sd[10]-'0');

    String intTahun = sd.substring(0,4);
    String intBulan = sd.substring(5,7);
    String intTanggal = sd.substring(8,10);

    //Serial.println("mengubah tanggal : " + intTahun + "-" +intBulan+ "-"+intTanggal);

    tahun=intTahun.toInt();
    bulan=intBulan.toInt();
    tanggal=intTanggal.toInt();
    RtcDateTime now = Rtc.GetDateTime();
    uint8_t jam = now.Hour();
    uint8_t menit = now.Minute();
    Rtc.SetDateTime(RtcDateTime(tahun, bulan, tanggal, jam, menit, 0));
    
    //Serial.println("berhasil ubah tanggal : " + sd);
    getTime();
}


void updateTime(String strTime){
   
      String st= strTime;
//      uint8_t jam = ((st[1]-'0')*10)+(st[2]-'0');
//      uint8_t menit = ((st[4]-'0')*10)+(st[5]-'0');
      
      String intJam = st.substring(0,st.indexOf(":"));
      String intMenit = st.substring(st.indexOf(":")+1,sizeof(st)+1);

      
      uint8_t jam = intJam.toInt();
      uint8_t menit = intMenit.toInt();
      //Serial.println("mengubah waktu : " + st);
       RtcDateTime now = Rtc.GetDateTime();
       uint16_t tahun = now.Year();
       uint8_t bulan = now.Month();
       uint8_t tanggal = now.Day();
       Rtc.SetDateTime(RtcDateTime(tahun, bulan, tanggal, jam, menit, 0));
    
      //Serial.println("berhasil ubah waktu   : " + st);
      getTime();
}

void addRooms(String strRooms){
  //Serial.println("appending " + strRooms);
  File f = SPIFFS.open("/rooms.txt", "a");
  
  if (!f) {
    //Serial.println("File doesn't exist yet. Creating it");
    File f = SPIFFS.open("/rooms.txt", "w");
    if (!f) {
      //Serial.println("file creation failed");
    }else{
      f.println(strRooms);  
      //Serial.println("bikin file baru berhasil");
    }
  }else{
    //Serial.println("file ketemu, isinya : " + f.read());
    f.println(strRooms);
  }
  
  f.close();
  getRooms();
}


void updateRooms(String strRooms){
  //Serial.println("updating " + strRooms);
  String nama = strRooms.substring(0,strRooms.indexOf("~"));
  //Serial.println("updating ruangan " + nama);
  
  File f2 = SPIFFS.open("/rooms.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  String rooms="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca ruangan ");
    String line = f2.readStringUntil('\n');
    String n= line.substring(0,line.indexOf("~"));
    if (nama==n){
      rooms += strRooms+"\n";
    }else{
      rooms += line +"\n";
    }
    //Serial.println("isi ruangan baru : " + rooms);
  }
  f2.close();
  File f = SPIFFS.open("/rooms.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.print(rooms);  
    //Serial.println("bikin file baru berhasil");
  }
  f.close();
  getRooms();
}

void deleteRooms(String strRooms){
 // //Serial.println("DELETING " + strRooms);
  String nama = strRooms.substring(0,strRooms.indexOf("~"));
  //Serial.println("Menghapus ruangan " + nama);
  
  File f2 = SPIFFS.open("/rooms.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  String rooms="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca ruangan ");
    String line = f2.readStringUntil('\n');
    String n= line.substring(0,line.indexOf("~"));
    n.replace("rooms:","");
    if (nama!=n){
      rooms += line +"\n";
    }
    //Serial.println("isi ruangan baru : " + rooms);
  }
  f2.close();
  File f = SPIFFS.open("/rooms.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.print(rooms);  
    //Serial.println("bikin file baru berhasil");
  }
  f.close();
  getRooms();
}


void getRooms(){
  File f2 = SPIFFS.open("/rooms.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  String rooms="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca rooms ");
    String line = f2.readStringUntil('\n');
    rooms += line +"\n";
    //Serial.println("isi rooms : " + rooms);
  }
  rooms = "rooms__"+rooms;
  const char* c= rooms.c_str();
  webSocket.broadcastTXT(c);
  f2.close();
}


void updateNamaRooms(String strRooms){
  Serial.println("updating" + strRooms);

  File f = SPIFFS.open("/namarooms.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.println(strRooms);  
    Serial.println("bikin file baru berhasil");
  }
 
  f.close();
  getNamaRooms();
}

void getNamaRooms(){
  File f2 = SPIFFS.open("/namarooms.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  String rooms="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca rooms ");
    String line = f2.readStringUntil('\n');
    rooms += line +"\n";
    Serial.println("isi rooms : " + rooms);
  }
  rooms = "roomsname__"+rooms;
  const char* c= rooms.c_str();
  webSocket.broadcastTXT(c);
  f2.close();
}

void getSpeakerState(){
  File f2 = SPIFFS.open("/speakerstate.txt", "r");
  if (!f2) {
    //Serial.println("open failed");
  }
  String rooms="";
  //Serial.println("open oke");
  while(f2.available()) {
    //Serial.println("mulai membaca rooms ");
    String line = f2.readStringUntil('\n');
    rooms += line;
    Serial.println("isi rooms : " + rooms);
  }
  rooms = "speakerstate__"+rooms;
  const char* c= rooms.c_str();
  webSocket.broadcastTXT(c);
  f2.close();
}

void turnRoomsOn(String rooms, String nada){
  rooms=rooms.substring(0,rooms.indexOf("\n"));
  int count;
  int posRuangan= 0;
  int posDel=0;
  
  Serial.println("mulai membaca ruangan ");
  if (rooms=="Semua Ruangan"){
    turnSpeaker("speaker_all_on");
  }else{
    File f3 = SPIFFS.open("/rooms.txt", "r");
    if (!f3) {
      Serial.println("open failed");
    }
    String ruangan="";
    //Serial.println("open oke");
    
    while(f3.available()) {
      String line = f3.readStringUntil('\n');
      if (sizeof(line)>0){
        Serial.println("line : " + line);
        String n=line.substring(0,line.indexOf("~"));
        rooms.trim();
        n.replace("rooms:","");
        n.trim();
        Serial.println("n : " + n + " = rooms : " + rooms);
        if (rooms.equals(n)){
          ruangan=line.substring(line.indexOf("~"));
          break;
        }
      }
      
    }
    f3.close();
    
    Serial.println("ruangan : " + ruangan);
    if (sizeof(ruangan)>0){
      for (int x=0;x<32;x++){
        String valRuangan=ruangan.substring(posRuangan,ruangan.indexOf(",",posRuangan));
        
        Serial.println("valRuangan : " + valRuangan);
        turnSpeaker(valRuangan);
        posRuangan=ruangan.indexOf(",",posRuangan)+1;
      }
    }
    
  }
  delay(50);
  const char* string1 = nada.c_str();
  playFile(string1);
}

void turnSpeaker(String text){
  if (text.endsWith("on")){
    state=LOW;
  }else{
    state=HIGH;
  }
  if (text.startsWith("speaker_all")){
    for (int i=0;i<pcfCount;i++){
      for (int x=0;x<8;x++){
        pcf[i].digitalWrite(x,state);
      }
    }
    Serial.println("turn speaker all speaker "+ String(state));
  }else{
    text.replace("speaker_","");
    text.replace("_on","");
    text.replace("_off","");
    int no = text.toInt();
    int y=0;
    for (int i=0;i<pcfCount;i++){
      for (int x=0;x<8;x++){
        if (y==no){
          pcf[i].digitalWrite(x,state);
          Serial.println("turn speaker no " +String(x)+" " + String(state));
          break;
        }
        y++;
      }
    }
  }
}

void changeSpeakerState(String text){
  Serial.println("updating" + text);

  File f = SPIFFS.open("/speakerstate.txt", "w");
  if (!f) {
    //Serial.println("file creation failed");
  }else{
    f.print(text);  
    Serial.println("bikin file baru berhasil");
  }
 
  f.close();
}


void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  //Serial.println(logmessage);
  filename.replace(",","");

  if(!filename.startsWith("/")) filename = "/"+filename;
  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    
    Serial.print("Upload File Name: "); 
    //Serial.println(filename);
    SD.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SD.open(filename, FILE_WRITE);
    //Serial.println(logmessage);
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    //Serial.println(logmessage);
  }

  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    // close the file handle as the upload is now done
    request->_tempFile.close();
    //Serial.println(logmessage);
    request->send(200,"text",logmessage);
    //request->redirect("/");
    getSongs();
  }
}

void deleteSongs(String filename){
  File dataFile = SD.open(filename, FILE_READ); // Now read data from SD Card 
  Serial.print("Deleting file: "); //Serial.println(filename);
  if (dataFile){
    if (SD.remove("/"+filename)) {
      //Serial.println(F("File deleted successfully"));
    }
    else{ 
      //Serial.println(F("File Gagal dihapus"));
    }
  } 
  getSongs();
}

int I2C_ClearBus() {
  
#if defined(TWCR) && defined(TWEN)
  TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif

  pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(SCL, INPUT_PULLUP);

  delay(2500);  // Wait 2.5 secs. This is strictly only necessary on the first power
  // up of the DS3231 module to allow it to initialize properly,
  // but is also assists in reliable programming of FioV3 boards as it gives the
  // IDE a chance to start uploaded the program
  // before existing sketch confuses the IDE by sending Serial data.

  boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
  if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master. 
    return 1; //I2C bus error. Could not clear SCL clock line held low
  }

  boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
  int clockCount = 20; // > 2x9 clock

  while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
    clockCount--;
  // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
    pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
    pinMode(SCL, OUTPUT); // then clock SCL Low
    delayMicroseconds(10); //  for >5uS
    pinMode(SCL, INPUT); // release SCL LOW
    pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
    // do not force high as slave may be holding it low for clock stretching.
    delayMicroseconds(10); //  for >5uS
    // The >5uS is so that even the slowest I2C devices are handled.
    SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
    int counter = 20;
    while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
      counter--;
      delay(100);
      SCL_LOW = (digitalRead(SCL) == LOW);
    }
    if (SCL_LOW) { // still low after 2 sec error
      return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
    }
    SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
  }
  if (SDA_LOW) { // still low
    return 3; // I2C bus error. Could not clear. SDA data line held low
  }

  // else pull SDA line low for Start or Repeated Start
  pinMode(SDA, INPUT); // remove pullup.
  pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
  // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
  /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
  delayMicroseconds(10); // wait >5uS
  pinMode(SDA, INPUT); // remove output low
  pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
  delayMicroseconds(10); // x. wait >5uS
  pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
  pinMode(SCL, INPUT);
  return 0; // all ok
}
