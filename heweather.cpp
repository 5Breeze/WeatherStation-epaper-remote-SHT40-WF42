#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <cstring>
#include <cstdlib>
#include "heweather.h"
//#define debug 1;
//#define debug2 1;
char client_cert[] = "E";
char client_key[] = "E";
heweatherclient::heweatherclient(const char* Serverurl,const char* langstring)
{
  server=Serverurl;
  lang=langstring;
  }
byte heweatherclient::getMeteoconIcon(int weathercodeindex) {
  if(weathercodeindex==0) return 12;
  if(weathercodeindex==1) return 58;
  if(weathercodeindex==2) return 58;
  if(weathercodeindex==3) return 58;
  if(weathercodeindex==4) return 54;
  if(weathercodeindex>=5&&weathercodeindex<=18) return 0;
  if(weathercodeindex>=19&&weathercodeindex<=32) return 19;
  if(weathercodeindex>=33&&weathercodeindex<=36)   return 16;
  if(weathercodeindex>=37&&weathercodeindex<=40)   return 16;
  if(weathercodeindex==41) return 37;
  if(weathercodeindex==42) return 37;
  if(weathercodeindex==43) return 37;
  return 17;
}

void heweatherclient::whitespace(char c) {
  ////Serial.println("whitespace");
}

void heweatherclient::startDocument() {
  ////Serial.println("start document");
}

void heweatherclient::key(String key) {
   currentKey=key;
  
  //Serial.println("key: " + key);
}

void heweatherclient::value(String value) {
if(currentParent=="now")
{
  if(currentKey=="cond") now_cond=value;
  if(currentKey=="cond_index") 
  {
    now_cond_index=value;
   if(value.toInt()>=19&&value.toInt()<=40)  rain=1;
  }
  if(currentKey=="hum") now_hum=value;
  if(currentKey=="tmp") now_tmp=value;
  if(currentKey=="dir") now_dir=value;
  if(currentKey=="sc") now_sc=value;

  if(currentKey=="fl") now_fl=value;
  if(currentKey=="pcpn") now_pcpn=value;
  if(currentKey=="vis") now_vis=value;
  if(currentKey=="pres") now_pres=value;


  
  }
if(currentParent=="today")
{
  if(currentKey=="cond_d") today_cond_d=value;
  if(currentKey=="cond_d_index") {today_cond_d_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
  if(currentKey=="cond_n") today_cond_n=value;
  if(currentKey=="cond_n_index") {today_cond_n_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
  if(currentKey=="tmpmax") today_tmp_max=value;
  if(currentKey=="tmpmin") today_tmp_min=value;
  if(currentKey=="txt_d") today_txt_d=value;
  if(currentKey=="txt_n") today_txt_n=value;
  }

if(currentParent=="tomorrow")
{
  if(currentKey=="cond_d") tomorrow_cond_d=value;
  if(currentKey=="cond_d_index") {tomorrow_cond_d_index=value;if(value.toInt()>=19&&value.toInt()<=40)  rain=1;}
    
  if(currentKey=="cond_n") tomorrow_cond_n=value;
  if(currentKey=="cond_n_index") tomorrow_cond_n_index=value;
  if(currentKey=="tmpmax") tomorrow_tmp_max=value;
  if(currentKey=="tmpmin") tomorrow_tmp_min=value;
  if(currentKey=="txt_d") tomorrow_txt_d=value;
  if(currentKey=="txt_n") tomorrow_txt_n=value;
  }

if(currentParent=="thedayaftertomorrow")
{
  if(currentKey=="cond_d") thedayaftertomorrow_cond_d=value;
  if(currentKey=="cond_d_index") thedayaftertomorrow_cond_d_index=value;
   
  if(currentKey=="cond_n") thedayaftertomorrow_cond_n=value;
  if(currentKey=="cond_n_index") thedayaftertomorrow_cond_n_index=value;
  if(currentKey=="tmpmax") thedayaftertomorrow_tmp_max=value;
  if(currentKey=="tmpmin") thedayaftertomorrow_tmp_min=value;
  }
if(currentParent=="daily")
{
  if(currentKey=="tmin") tmin_array=value;
  if(currentKey=="tmax") tmax_array=value;
   
  if(currentKey=="code_d_index") code_d_array=value;
  if(currentKey=="code_n_index") code_n_array=value;
  if(currentKey=="text_d") text_d_array=value;
  if(currentKey=="text_n") text_n_array=value;

  if(currentKey=="day") date_array=value;
  if(currentKey=="week") week_array=value;
  }

  
 if(currentKey=="aqi") aqi=value;
 long aqiint = aqi.toInt();
 if(aqiint<=50) {aqitext="Excellent";airconditionbits_index=0;}
 else if(aqiint>50&&aqiint<=100){ aqitext="Good";airconditionbits_index=1;}
 else if(aqiint>100&&aqiint<=150) {aqitext="Lightly Polluted";airconditionbits_index=2;}
 else if(aqiint>150&&aqiint<=200) {aqitext="Moderately Polluted";airconditionbits_index=3;}
 else if(aqiint>250&&aqiint<=300) {aqitext="Heavily Polluted";airconditionbits_index=4;}
 else {aqitext="Severely Poluuted";airconditionbits_index=5;}
 if(currentKey=="co") co=value;
 if(currentKey=="no2") no2=value;
 if(currentKey=="o3") o3=value;
 if(currentKey=="pm10") pm10=value;
 if(currentKey=="pm25") pm25=value;
 if(currentKey=="so2") so2=value;
 if(currentKey=="city") citystr=value;
 if(currentKey=="date") date=value;
   if(currentKey=="qlty") qlty=value;
   if(currentKey=="message") message=value;
   if(currentKey=="year") year=value;
   if(currentKey=="nongli") nongli=value;
   if(currentKey=="t") t=value;
   if(currentKey=="status") unknown=value;
  Serial.println("value: " + value);
}

void heweatherclient::endArray() {
  ////Serial.println("end array. ");
}

void heweatherclient::endObject() {
  ////Serial.println("end object. ");
  currentParent="";
}

void heweatherclient::endDocument() {
  ////Serial.println("end document. ");
}

void heweatherclient::startArray() {
   ////Serial.println("start array. ");
}

void heweatherclient::startObject() {
   ////Serial.println("start object. ");
     currentParent = currentKey;
}
void heweatherclient::update()
{ 
  uint8_t fingerprint[20] = {0x21,0xac, 0xbb, 0xc2, 0x6b, 0x93, 0x3d,0xa1, 0xf3 ,0x85 ,0x24, 0x14,0x67 ,0x32, 0x98, 0x12, 0xcb, 0x15, 0x32, 0x9c};
  fingerprint[0]=0x20;//0x22,0x8c, 0xb1, 0xcd, 0xab, 0x23, 0xad,0x27, 0xbf ,0xf3 ,0x24, 0xd4,0x7c ,0xef, 0xed, 0xf5, 0x3d, 0x1e, 0x42, 0x8c
  fingerprint[1]=0xb4;//8c
  fingerprint[2]=0x42;//b1
  fingerprint[3]=0xb3;
  fingerprint[4]=0x08;
  fingerprint[5]=0xc9;
  fingerprint[6]=0x53;
  fingerprint[7]=0x30;
  fingerprint[8]=0x75;
  fingerprint[9]=0x7f;
  fingerprint[10]=0xdc;
   fingerprint[11]=0x8d;
    fingerprint[12]=0x94;
     fingerprint[13]=0xf4;
      fingerprint[14]=0x94;
       fingerprint[15]=0x3b;
        fingerprint[16]=0x03;
         fingerprint[17]=0x8a;
          fingerprint[18]=0x96;
           fingerprint[19]=0x46;
  rain=0;
  JsonStreamingParser parser;
  parser.setListener(this);
  int i=0;

  int a,b;
  a=47+9%2;//48
  b=49+9%2;
  for(i=0;i<sizeof(client_key);i++)//0to2 2to0
  {
   if(client_key[i]==char(byte(a))) {client_key[i]=char(byte(b));continue;}
   
   if(client_key[i]==char(byte(b))) {client_key[i]=char(byte(a));continue;} 
  }

  // Prepare POST body
  String get_para = "lut1="+city+"&lut2="+lang+"&lut3="+client_name+"&lut4="+ESP.getChipId()+"&lut5="+cdkey+"&bssid="+bssid+"&ssid="+ssid+"&epd_type="+epd_type;

  // Detect if `server` is an http URL (e.g. "http://host:port/path") to allow using plain HTTP for local mock server
  bool use_plain_http = false;
  char hostbuf[128] = {0};
  char pathbuf[128] = "/weather.php";
  const char* host_for_header = server;
  int port = 443;

  if (strncmp(server, "http://", 7) == 0) {
    use_plain_http = true;
    const char* p = server + 7; // skip "http://"
    const char* slash = strchr(p, '/');
    if (slash) {
      int hlen = slash - p;
      if (hlen > 127) hlen = 127;
      memcpy(hostbuf, p, hlen);
      hostbuf[hlen] = '\0';
      strncpy(pathbuf, slash, 127);
      pathbuf[127] = '\0';
    } else {
      strncpy(hostbuf, p, 127);
      hostbuf[127] = '\0';
    }
    // parse optional port in host (host:port)
    char* colon = strchr(hostbuf, ':');
    if (colon) {
      port = atoi(colon + 1);
      *colon = '\0';
    }
    host_for_header = hostbuf;
  }

  #ifdef debug
  Serial.println(get_para);
  #endif

  int pos = 0;
  boolean isBody = false;
  char c;

  if (use_plain_http) {
    // Use plain TCP client (HTTP) for local mock server
    WiFiClient client;
    client.setNoDelay(false);
    #ifdef debug2
    Serial.println("Requesting weather (plain HTTP): ");
    #endif
    if (!client.connect(hostbuf, port)) {
      Serial.println("connection failed");
      timeout = true;
      return;
    }
    client.print(String("POST ") + pathbuf +
                 " HTTP/1.1\r\n" +
                 "Host: " + String(host_for_header) + "\r\n" +
                 "Connection: close\r\n" +
                 "Content-Type: application/x-www-form-urlencoded\r\n" +
                 "Content-Length: " + get_para.length() + "\r\n\r\n" +
                 get_para);

    // read response
    while(client.connected()) {
      delay(100);
      while(client.available()) {
        c = client.read();
        Serial.print(c);
        if (c == '{' || c == '[') isBody = true;
        if (isBody) parser.parse(c);
      }
    }
  } else {
    // Use secure client (original behavior)
    BearSSL::WiFiClientSecure client;
    client.setFingerprint(fingerprint);
    BearSSL::X509List client_crt(client_cert);
    BearSSL::PrivateKey key(client_key);
    client.setClientRSACert(&client_crt, &key);
    // probe and connect using configured server name
    #ifdef debug
    Serial.println("mfln support:");
    Serial.println(client.probeMaxFragmentLength(server, 443, 512));
    #endif
    if (client.probeMaxFragmentLength(server, 443, 512) == false) { timeout = true; return; }
    client.setBufferSizes(512,512);
    #ifdef debug2
    Serial.println("Updating weather data...");
    #endif
    if (!client.connect(server, 443)) {
      Serial.println("connection failed");
      timeout = true;
      return;
    }
    #ifdef debug2
    Serial.println("Requesting weather: ");
    #endif
    client.print(String("POST /weather.php")+
               " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" +
               "Connection: close\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + get_para.length() + "\r\n\r\n" +
               get_para);

    client.setNoDelay(false);
    while(client.connected()) {
      delay(500+abs((0)*100000));
      while(client.available()) {
        c = client.read();
        Serial.print(c);
        if (c == '{' || c == '[') {
          isBody = true;
        }
        if (isBody) {
          parser.parse(c);
        }
      }
    }
  }
  #ifdef debug
  Serial.printf("Recieved %d bytes of data\n",pos);
  #endif
  }
