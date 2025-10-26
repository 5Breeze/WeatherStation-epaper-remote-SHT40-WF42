/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch
*/

#include "TimeClient.h"

TimeClient::TimeClient(float utcOffset, const char * Serverurl) {
  myUtcOffset = utcOffset;
  server=Serverurl;
}
void TimeClient::updateTime(String line) {
  if (line.startsWith("DATE: ")) {
    // 找到最后一个空格，后面就是 "23:28:30"
    int spaceIndex = line.lastIndexOf(' ');
    String timePart = line.substring(spaceIndex + 1); // "23:28:30"

    int firstColon = timePart.indexOf(':');
    int secondColon = timePart.lastIndexOf(':');

    int parsedHours = timePart.substring(0, firstColon).toInt();
    int parsedMinutes = timePart.substring(firstColon + 1, secondColon).toInt();
    int parsedSeconds = timePart.substring(secondColon + 1).toInt();

    localEpoc = (parsedHours * 3600 + parsedMinutes * 60 + parsedSeconds) * 1000 - millis();

    Serial.printf("Parsed Time: %02d:%02d:%02d\n", parsedHours, parsedMinutes, parsedSeconds);
  }
}


void TimeClient::updateTime() {
  WiFiClient client;
  const int httpPort = 80;
  Serial.println("Updating time...");

if (!client.connect(server, 80)) {
    Serial.println("Connection failed");
    return;
  }

  Serial.println("Requesting URL:/time.php");

  // This will send the request to the server
  client.print(String("GET /time.php")+ " HTTP/1.1\r\n" +
             "Host: " + server + "\r\n" +
             "Connection: close\r\n" +
             "\r\n" );

  
  /*if (!client.connect("duckduckweather.esy.es",80)) {
    ////Serial.println("connection failed");
    return;
  }
  
  // This will send the request to the server
  client.print(String("GET")+"/time.php"+"HTTP/1.1\r\n" +
               String("Host:duckduckweather.esy.es\r\n") + 
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while(!client.available() && repeatCounter < 100) {
    delay(500); 
    ////Serial.println(".");
    repeatCounter++;
  }*/

  String line;
  int size = 0;
  client.setNoDelay(false);
  while(client.connected()) {
   Serial.println("Time server connected");
   //Serial.println(client.available());
    delay(200);
    while((size = client.available()) > 0) {
      delay(20);
       
      line = client.readStringUntil('\n');
      //Serial.println(line);
      line.toUpperCase(); 
      // example: 
      // date: Thu, 19 Nov 2015 20:25:40 GMT
      if (line.startsWith("DATE: ")) {
        ////Serial.println(line.substring(23, 25) + ":" + line.substring(26, 28) + ":" +line.substring(29, 31));
        int parsedHours = line.substring(23, 25).toInt();
        int parsedMinutes = line.substring(26, 28).toInt();
        int parsedSeconds = line.substring(29, 31).toInt();
        Serial.println(String(parsedHours) + ":" + String(parsedMinutes) + ":" + String(parsedSeconds));

        localEpoc = (parsedHours * 60 * 60 + parsedMinutes * 60 + parsedSeconds);
        Serial.printf("localEpoc:%d\n",localEpoc);
        localMillisAtUpdate = millis();
      }
    }
  }

}

String TimeClient::getHours() {
    if (localEpoc == 0) {
      return "--";
    }
    int hours = ((getCurrentEpochWithUtcOffset()  % 86400000L) / 3600000) % 24;
    if (hours < 10) {
      return "0" + String(hours);
    }
    return String(hours); // print the hour (86400 equals secs per day)

}
String TimeClient::getMinutes() {
    if (localEpoc == 0) {
      return "--";
    }
    int minutes = ((getCurrentEpochWithUtcOffset() % 3600000) / 60000);
    if (minutes < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      return "0" + String(minutes);
    }
    return String(minutes);
}
String TimeClient::getSeconds() {
    if (localEpoc == 0) {
      return "--";
    }
    int seconds = getCurrentEpochWithUtcOffset() % 60000/1000;
    if ( seconds < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      return "0" + String(seconds);
    }
    return String(seconds);
}
byte TimeClient::getHours_byte()
{
   if (localEpoc == 0) {
      return 0;
    }
    int hours = ((getCurrentEpochWithUtcOffset()  % 86400000L) / 3600000) % 24;
    return (byte) hours;
  }
byte TimeClient::getMinutes_byte()
{
   if (localEpoc == 0) {
      return 0;
    }
    int minutes = ((getCurrentEpochWithUtcOffset() % 3600000) / 60000);
    return (byte)minutes;
  
  }

byte TimeClient::TimeClient::getSeconds_byte() {
    if (localEpoc == 0) {
      return 0;
    }
    int seconds = getCurrentEpochWithUtcOffset() % 60000/1000;
    
    return byte(seconds);
}
String TimeClient::getFormattedTime() {
  return getHours() + ":" + getMinutes();
}

long TimeClient::getCurrentEpoch() {
  return localEpoc + ((millis() - localMillisAtUpdate) );
}

long TimeClient::getCurrentEpochWithUtcOffset() {
  return long(round(getCurrentEpoch() + 3600 * long(myUtcOffset) + 86400000L) )% 86400000L;
}
