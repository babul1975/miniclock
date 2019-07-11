#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

extern "C" {
  #include "user_interface.h"  // Required for wifi_station_connect() to work
}

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

char ssid[] = "ssid";        // your network SSID (name)
char pass[] = "pw";          // your network password

unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServerIP;
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
int cb;
byte count;

byte try_count = 15; // 1 count = 2 seconds
char buffer[80];
char Trigger[4];

WiFiUDP udp;

void setup() {

  Serial.begin(9600);  // max speed as using softwareserial on Arduino

  udp.begin(localPort);

}

void loop() {

  if (readline(Serial.read(), buffer, 80) > 0) {
    if ((buffer[0] == 'N') && (buffer[1] == 'T') && (buffer[2] == 'P')) {
      Serial.println("NTP request received from Arduino");
      WiFiOn();
      GetNTPTime();
      WiFiOff();
    }
  }
  delay(1000);

}

void GetNTPTime() {
  
  WiFi.hostByName(ntpServerName, timeServerIP); 

  count = 1;
  sendNTPpacket(timeServerIP);
  delay(50);
  cb = udp.parsePacket();
  int i = 0;
  while (!cb) {
    if (count > try_count) {
      Serial.println("Failed");
      break;
    }
    if (i == 400) { //resend NTP packet every 2 seconds
      count++;
      sendNTPpacket(timeServerIP);
      delay(5);
      i = 0;
    }
    delay(5);
    cb = udp.parsePacket();
    i++;
  }
  
  if (cb) {
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    Serial.print("UNIX");
    Serial.println(epoch);
  }
  
}

void WiFiOn() {

  wifi_fpm_do_wakeup();
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  wifi_connect();

}

void WiFiOff() {

  Serial.print("Disabling WiFi..");
  //client.disconnect();
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
  Serial.println("..ok");

}

void wifi_connect() {

  Serial.println();
  // attempt to connect to Wifi network:
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to SSID: ");
  Serial.print(ssid);
  Serial.print("..");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    delay(500);
    // add counter here ######################
  }
  Serial.println("..connected");
  printWifiStatus();

}

void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());

  // print your WiFi IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(", IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");

}

unsigned long sendNTPpacket(IPAddress& address) {
  
  Serial.print("Sending NTP packet to: ");
  Serial.print(address);
  Serial.print(" [attempt ");
  Serial.print(count);
  Serial.println("]");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  udp.beginPacket(address, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

}

//used to readline from serial output
int readline(int readch, char *buffer, int len) {

  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\r': // Ignore CR
      break;
      case '\n': // Return on new-line
      rpos = pos;
      pos = 0;  // Reset position index ready for next time
      return rpos;
      default:
      if (pos < len-1) {
        buffer[pos++] = readch;
        buffer[pos] = 0;
      }
    }
  }
  return 0;

}
