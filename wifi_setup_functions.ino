#include <SPI.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
//#include <DS3231.h>
//#include <RTCZero.h>//re-enable for MKR1010
#include <Adafruit_NeoPixel.h>
#include <FlashStorage.h>
#include <Timezone.h>

#include "RTClib.h"

/////////TODO LIST /////
// 1. need to figure out a way to retry the NTP connection if it fails (which it seems to do alot) - maybe fixed?
// 2. need to figure out why the http server stops responding if say the SSID is wrong. - maybe fixed.  not the best solution in the world
// 3. sort out refreshing the clock twice a day (may be ok with the DS3231 keeping the time, but we'll see.
// 4. Add software reset // worked once... very odd
// 5. Make sure all debug states are properly displayed via pixels and document those
// 6. make sure the temperature supports negative values and upto 3 digits (so basically -99 to +999
// 7. add ability to prevent dimming of display
// 8. add option to set brightness levels manually if dimming is turned off

RTC_DS3231 rtc;
#define CLOCK_INTERRUPT_PIN 2
unsigned int localPort = 2390;  // local port to listen for UDP packets
IPAddress timeServer(129, 6, 15, 28);
const int NTP_PACKET_SIZE = 48;      // NTP timestamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
WiFiUDP Udp;
#define LED_PIN 4
#define LED_COUNT 58
const int pResistor = A0;
int Reset_PIN = 6;
int lightvalue;
int pixels[58];
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
int Sized[] = {18, 6, 15, 15, 12, 15, 18, 9, 21, 18};
int Sizedoff[] = {3, 15, 6, 6, 9, 6, 3, 12, 0, 3};
char ssid[] = "RainbowClockSetup";  // your network SSID (name)
char pass[] = "";                   // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                   // your network key Index number (needed only for WEP)
typedef struct {
    boolean valid;
    char wifissid[100];
    char Password[100];
    char brightness[5];
    char temp[3];
    char twelvehr[3];
    char timezone[10];
    char tempUnits[3];
    char dimmer[3];
} Credentials;

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);
String readString;
int WifiSetup = 1;
FlashStorage(my_flash_store, Credentials);
Credentials owner;
Credentials owner2;

TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
TimeChangeRule usMDT = {"MDT", Second, Sun, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, Sun, Nov, 2, -420};
TimeChangeRule usCDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, Sun, Nov, 2, -360};
TimeChangeRule usPDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, Sun, Nov, 2, -480};
Timezone usET(usEDT, usEST);
Timezone usPT(usPDT, usPST);
Timezone usCT(usCDT, usCST);
Timezone usMT(usMDT, usMST);

void setup() {
    Serial.begin(9600);

    //  while (!Serial) {}
    owner = my_flash_store.read();

    strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();   // Turn OFF all pixels ASAP
    strip.clear();
    strip.setBrightness(150);

    digitalWrite(Reset_PIN, HIGH);
    delay(200);
    pinMode(Reset_PIN, OUTPUT);

    if (owner.valid == false) {
        status = WiFi.beginAP(ssid);
        if (status != WL_AP_LISTENING) {
            strip.setPixelColor(50, strip.Color(255, 150, 0));
            strip.setPixelColor(51, strip.Color(255, 150, 0));
            strip.setPixelColor(6, strip.Color(255, 150, 0));
            strip.setPixelColor(7, strip.Color(255, 150, 0));
            strip.setPixelColor(20, strip.Color(255, 150, 0));
            strip.setPixelColor(21, strip.Color(255, 150, 0));
            strip.show();
            // 48 and 49
            Serial.println("Creating access point failed");
            // don't continue
            while (true)
                ;
        }
        delay(10000);
        server.begin();
        printWiFiStatus();
        Serial.println("Access Point Web Server");
        if (WiFi.status() == WL_NO_MODULE) {
            Serial.println("Communication with WiFi module failed!");
            // Failure of Wifi Module.  all top rows are red
            strip.setPixelColor(50, strip.Color(255, 0, 0));
            strip.setPixelColor(51, strip.Color(255, 0, 0));
            strip.setPixelColor(6, strip.Color(255, 0, 0));
            strip.setPixelColor(7, strip.Color(255, 0, 0));
            strip.setPixelColor(20, strip.Color(255, 0, 0));
            strip.setPixelColor(21, strip.Color(255, 0, 0));
            strip.show();
            while (true)
                ;
        }
        String fv = WiFi.firmwareVersion();
        if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
            Serial.println("Please upgrade the firmware");
        }
        Serial.print("Creating access point named: ");
        Serial.println(ssid);
        strip.setPixelColor(50, strip.Color(255, 255, 0));
        strip.setPixelColor(51, strip.Color(255, 255, 0));
        strip.setPixelColor(36, strip.Color(255, 255, 0));
        strip.setPixelColor(37, strip.Color(255, 255, 0));

        strip.setPixelColor(6, strip.Color(255, 255, 0));
        strip.setPixelColor(7, strip.Color(255, 255, 0));
        strip.setPixelColor(20, strip.Color(255, 255, 0));
        strip.setPixelColor(21, strip.Color(255, 255, 0));
        strip.show();
    } else {
        strip.setPixelColor(11, strip.Color(255, 150, 0));
        strip.setPixelColor(10, strip.Color(255, 150, 0));
        strip.setPixelColor(24, strip.Color(255, 150, 0));
        strip.setPixelColor(25, strip.Color(255, 150, 0));

        strip.setPixelColor(40, strip.Color(255, 150, 0));
        strip.setPixelColor(41, strip.Color(255, 150, 0));
        strip.setPixelColor(54, strip.Color(255, 150, 0));
        strip.setPixelColor(55, strip.Color(255, 150, 0));
        strip.show();
        Serial.print("Stored SSID:");
        Serial.println((String)owner.wifissid);
        Serial.println((String)owner.brightness);
        WifiSetup = 0;
        validateSSID(owner);
        delay(5000);
        server.begin();
        strip.setPixelColor(28, strip.Color(255, 0, 0));
        strip.setPixelColor(29, strip.Color(255, 0, 0));
        strip.show();
        if (WiFi.status() == WL_NO_MODULE) {
            strip.setPixelColor(50, strip.Color(255, 0, 0));
            strip.setPixelColor(51, strip.Color(255, 0, 0));
            strip.setPixelColor(6, strip.Color(255, 0, 0));
            strip.setPixelColor(7, strip.Color(255, 0, 0));
            strip.setPixelColor(20, strip.Color(255, 0, 0));
            strip.setPixelColor(21, strip.Color(255, 0, 0));
            strip.show();
            Serial.println("Communication with WiFi module failed!");
            while (true)
                ;
        }
        String fv = WiFi.firmwareVersion();
        if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
            Serial.println("Please upgrade the firmware");
        }
        while (status != WL_CONNECTED) {
            strip.setPixelColor(28, strip.Color(255, 150, 0));
            strip.setPixelColor(29, strip.Color(255, 150, 0));
            strip.show();
            Serial.print("Attempting to connect to SSID: ");
            Serial.println(ssid);
            // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
            status = WiFi.begin(ssid, pass);
            // wait 10 seconds for connection:
            delay(10000);
        }
        Serial.println("Connected to WiFi");
        strip.setPixelColor(28, strip.Color(0, 255, 0));
        strip.setPixelColor(29, strip.Color(0, 255, 0));
        strip.show();
        printWifiStatus();

        Serial.println("\nStarting connection to server (first run)...");
        Udp.begin(localPort);
        sendNTPpacket(timeServer);  // send an NTP packet to a time server

        delay(3000);
        if (Udp.parsePacket()) {
            Serial.println("packet received");
            strip.setPixelColor(28, strip.Color(0, 0, 255));
            strip.setPixelColor(29, strip.Color(0, 0, 255));
            strip.show();
            // We've received a packet, read the data from it
            Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            const unsigned long seventyYears = 2208988800UL;
            unsigned long epoch = secsSince1900 - seventyYears;
            rtc.begin();  // initialize RTC
            rtc.disable32K();
            rtc.adjust(DateTime(epoch + 1));
            rtc.clearAlarm(1);
            rtc.disable32K();
            pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
            attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), alarm, FALLING);
            rtc.clearAlarm(1);
            rtc.clearAlarm(2);
            rtc.writeSqwPinMode(DS3231_OFF);
            rtc.setAlarm2(rtc.now(), DS3231_A2_PerMinute);
            rtc.setAlarm1(DateTime(0, 0, 0, 0, 0, 55), DS3231_A1_Second);

            SetTime();
        } else {
            strip.setPixelColor(28, strip.Color(255, 255, 255));
            strip.setPixelColor(29, strip.Color(255, 255, 255));
            strip.show();
        }
    }
}

void printWiFiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print where to go in a browser:
    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);
}

void alarm() {
    if (rtc.alarmFired(1)) {
        Serial.println("Alarm1 Triggered");
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
        if ((String)owner.temp == "on") {
            //      //this is for the temperature
            Serial.println(owner.temp);
            Serial.print("Temperature: ");
            Serial.print(rtc.getTemperature());
            Serial.println(" C");
            Serial.println(owner.tempUnits);
            String temp1;
            String temp2;
            String TempUnits;
            if ((String)owner.tempUnits == "F") {
                Serial.println("CaveMan Temperature");
                int Temperature = rtc.getTemperature();
                int Temperature2 = (Temperature * 1.8) + 32;
                temp1 = String(Temperature2).substring(0, 1);
                temp2 = String(Temperature2).substring(1, 2);
                Serial.println(temp1);
                Serial.println(temp2);
                TempUnits = "F";
            } else {
                temp1 = String(rtc.getTemperature()).substring(0, 1);
                temp2 = String(rtc.getTemperature()).substring(1, 2);
                Serial.println(temp1);
                Serial.println(temp2);
                TempUnits = "C";
            }
            //      Serial.println("in temp loop");
            outputDigitsTemp(temp1.toInt(), temp2.toInt(), TempUnits);
            //      //      delay(5000);
            //      //      SetTime();
            //      rtc.clearAlarm(1);
            //      rtc.clearAlarm(2);
        }
    } else {
        Serial.println("Alarm1 Triggered2");
        SetTime();
    }
}
void SetTime() {
    /*
       this is what handles the actual updating of the time.  it's an Alarm interrupt on the RTC.
    */
    Serial.println("Alarm2 Triggered");
    DateTime now = rtc.now();

    int alarmepoch = now.unixtime();
    char bufPST[40];
    char mPST[4];
    char mEST[4];
    char mMST[4];
    char mCST[4];
    TimeChangeRule *tcr;
    //    time_t tPST = usPT.toLocal(alarmepoch, &tcr);

    char PSTHour[3];
    char PSTMin[3];
    char PSTSec[3];
    if ((String)owner.timezone == "PST") {
        Serial.println("PST Time Zone...");
        time_t tPST = usPT.toLocal(alarmepoch, &tcr);
        strcpy(mPST, monthShortStr(month(tPST)));
        sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tPST), minute(tPST), second(tPST), dayShortStr(weekday(tPST)), day(tPST), mPST, year(tPST), tcr->abbrev);
        Serial.println(bufPST);
        sprintf(PSTHour, "%.2d", hour(tPST));
        sprintf(PSTMin, "%.2d", minute(tPST));
        sprintf(PSTSec, "%.2d", second(tPST));
    }

    if ((String)owner.timezone == "EST") {
        Serial.println("EST Time Zone...");
        time_t tEST = usET.toLocal(alarmepoch, &tcr);
        strcpy(mEST, monthShortStr(month(tEST)));
        sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tEST), minute(tEST), second(tEST), dayShortStr(weekday(tEST)), day(tEST), mEST, year(tEST), tcr->abbrev);
        Serial.println(bufPST);
        sprintf(PSTHour, "%.2d", hour(tEST));
        sprintf(PSTMin, "%.2d", minute(tEST));
        sprintf(PSTSec, "%.2d", second(tEST));
    }
    if ((String)owner.timezone == "CST") {
        Serial.println("CST Time Zone...");
        time_t tCST = usCT.toLocal(alarmepoch, &tcr);

        strcpy(mCST, monthShortStr(month(tCST)));
        sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tCST), minute(tCST), second(tCST), dayShortStr(weekday(tCST)), day(tCST), mCST, year(tCST), tcr->abbrev);

        Serial.println(bufPST);

        sprintf(PSTHour, "%.2d", hour(tCST));
        sprintf(PSTMin, "%.2d", minute(tCST));
        sprintf(PSTSec, "%.2d", second(tCST));
    }
    if ((String)owner.timezone == "MST") {
        Serial.println("MST Time Zone...");
        time_t tMST = usMT.toLocal(alarmepoch, &tcr);

        strcpy(mMST, monthShortStr(month(tMST)));
        sprintf(bufPST, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(tMST), minute(tMST), second(tMST), dayShortStr(weekday(tMST)), day(tMST), mMST, year(tMST), tcr->abbrev);

        Serial.println(bufPST);

        sprintf(PSTHour, "%.2d", hour(tMST));
        sprintf(PSTMin, "%.2d", minute(tMST));
        sprintf(PSTSec, "%.2d", second(tMST));
    }
    // There may be a better way to do all this, but I don't know what it is.  but since we're driving the digits in different colours, I figured we needed
    // to drive the digits entirely independently.  so in order to do this, I need to break the time down into separate digits.

    String PSTHours = String(PSTHour);
    if ((String)owner.twelvehr == "on") {
        Serial.println("12 hour Time!!!");
        if ((int)PSTHour > 12) {
            String foo = (String)PSTHour;
            int bar = foo.toInt();
            int www = bar - 12;
            if (www < 10) {
                String foo2 = (String)www;
                String prefix = "0";
                String bar2 = prefix + foo2;
                bar2.toCharArray(PSTHour, 3);
            } else {
                String bar2 = (String)www;
                bar2.toCharArray(PSTHour, 3);
            }
        }
    }
    String PSTMins = String(PSTMin);
    String PSTSeconds = String(PSTSec);
    String PSTMinute1 = String(PSTMin).substring(0, 1);
    String PSTMinute2 = String(PSTMin).substring(1);
    String PSTHour1 = String(PSTHour).substring(0, 1);
    String PSTHour2 = String(PSTHour).substring(1);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    // now that the time's been split up, we can output it all to the digits of the clock.
    // I'm doing each digit separately.  again as with before, there's probably way better ways of doing this.

    if ((PSTHour1 == "0") && (PSTHour2 == "0") && (PSTMinute1 == "0") && (PSTMinute2 == "0")) {
        // it's midnight, so re-run the time sync
        GetTime();
    }
    if ((PSTHour1 == "1") && (PSTHour2 == "2") && (PSTMinute1 == "0") && (PSTMinute2 == "0")) {
        // it's Noon, so re-run the time sync

        GetTime();
    }
    outputDigits(PSTMinute2.toInt(), PSTMinute1.toInt(), PSTHour2.toInt(), PSTHour1.toInt());
    // outputDigits (1, 1, 1, 1);
    // outputDigits (2, 2, 2, 2);
    // outputDigits (3, 3, 3, 3);
    // outputDigits (4, 4, 4, 4);
    // outputDigits (5, 5, 5, 5);
    // outputDigits (6, 6, 6, 6);
    // outputDigits (7, 7, 7, 7);
    // outputDigits (8, 8, 8, 8);
    // outputDigits (9, 9, 9, 9);
}

void outputDigitsTemp(int digit1, int digit2, String units) {
    int nums[][14] = {

        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, -1, -1},  // 0 //seems ok
        {0, 1, -1, -1, -1, -1, 6, 7, 8, 9, 10, 11, 12, 13},        // 1 //seems ok
        {-1, -1, 2, 3, -1, -1, -1, -1, 8, 9, -1, -1, -1, -1},      // 2 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, 8, 9, -1, -1, 12, 13},    // 3
        {0, 1, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, 12, 13},      // 4
        {-1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, 12, 13},    // 5////numbers represent the pixels that are NOT LIT
        {-1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1},    // 6
        {0, 1, -1, -1, -1, -1, -1, -1, 8, 9, 10, 11, 12, 13},      // 7 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 8 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13},  // 9 //seems ok
        {-1, -1, 2, 3, 4, 5, -1, -1, -1, -1, 10, 11, -1, -1},      // C (for Temp)
        {0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1},        // F (for Temp)
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},            // all off
    };

    for (int j = 30; j < 44; j++) {  // third digit.. a 7
        if (nums[digit2][j - 30] == -1) {
            pixels[j] = -1;
        } else {
            pixels[j] = nums[digit2][j - 30] + 30;
        }
    }
    for (int j = 44; j < 58; j++) {  // fourth digit... a 1
        if (nums[digit1][j - 44] == -1) {
            pixels[j] = -1;
        } else {
            pixels[j] = nums[digit1][j - 44] + 44;
        }
    }

    pixels[28] = 28;                // dots
    pixels[29] = 29;                // dots
    for (int j = 0; j < 14; j++) {  // first digit.  a 3
        pixels[j] = nums[12][j];
    }
    if (units == "F") {
        for (int j = 14; j < 28; j++) {  // second digit... a 3
            if (nums[11][j - 14] == -1) {
                pixels[j] = -1;
            } else {
                pixels[j] = nums[11][j - 14] + 14;
            }
        }
    } else {
        for (int j = 14; j < 28; j++) {  // second digit... a 3
            if (nums[10][j - 14] == -1) {
                pixels[j] = -1;
            } else {
                pixels[j] = nums[10][j - 14] + 14;
            }
        }
    }
}
void outputDigits(int digit1, int digit2, int digit3, int digit4) {
    int nums[][14] = {
        // 0   1   2   3   4   5   6   7   8   9  10  11  12  13
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, -1, -1},  // 0 //seems ok
        {0, 1, -1, -1, -1, -1, 6, 7, 8, 9, 10, 11, 12, 13},        // 1 //seems ok
        {-1, -1, 2, 3, -1, -1, -1, -1, 8, 9, -1, -1, -1, -1},      // 2 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, 8, 9, -1, -1, 12, 13},    // 3
        {0, 1, -1, -1, -1, -1, 6, 7, -1, -1, -1, -1, 12, 13},      // 4
        {-1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, 12, 13},    // 5////numbers represent the pixels that are NOT LIT
        {-1, -1, -1, -1, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1},    // 6
        {0, 1, -1, -1, -1, -1, -1, -1, 8, 9, 10, 11, 12, 13},      // 7 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},  // 8 //seems ok
        {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13},  // 9 //seems ok
        {-1, -1, 2, 3, 4, 5, -1, -1, -1, -1, 10, 11, -1, -1},      // C (for Temp)
        {0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1},        // F (for Temp)
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},            // all off
    };

    for (int j = 0; j < 14; j++) {  // first digit.  a 3
        pixels[j] = nums[digit1][j];
    }

    for (int j = 14; j < 28; j++) {  // second digit... a 3
        if (nums[digit2][j - 14] == -1) {
            pixels[j] = -1;
        } else {
            pixels[j] = nums[digit2][j - 14] + 14;
        }
    }
    pixels[28] = -1;  // dots
    pixels[29] = -1;  // dots

    for (int j = 30; j < 44; j++) {  // third digit.. a 7
        if (nums[digit3][j - 30] == -1) {
            pixels[j] = -1;
        } else {
            pixels[j] = nums[digit3][j - 30] + 30;
        }
    }
    for (int j = 44; j < 58; j++) {  // fourth digit... a 1
        if (nums[digit4][j - 44] == -1) {
            pixels[j] = -1;
        } else {
            pixels[j] = nums[digit4][j - 44] + 44;
        }
    }
}
uint32_t Wheel(byte WheelPos) {
    if (WheelPos < 85) {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}
void loop() {
    if (WifiSetup == 0) {
        //   lightvalue = analogRead(pResistor);
        uint16_t i, j;

        for (j = 0; j < 256; j++) {
            for (i = 0; i < strip.numPixels(); i++) {
                if (pixels[i] == -1) {
                    strip.setPixelColor(i, Wheel((i * 14 + j) & 255));
                } else {
                    strip.setPixelColor(i, strip.Color(0, 0, 0));
                }
            }
            strip.show();
            delay(15);
        }

        lightvalue = analogRead(pResistor);
        // Serial.println(lightvalue);
        if (lightvalue > 350) {
            strip.setBrightness(255);
            strip.show();
        }
        if ((lightvalue > 150) && (lightvalue < 350)) {
            strip.setBrightness(150);
            strip.show();
        }
        if (lightvalue < 150) {
            strip.setBrightness(20);
            strip.show();
        }
    }

    WiFiClient client = server.available();
    if (client) {
        // Serial.println("In Client Loop");
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.println(readString.length());
                // read char by char HTTP request
                if (readString.length() < 200) {
                    // store characters to string
                    readString += c;
                    // Serial.print(c); //uncomment to see in serial monitor
                }
                // Serial.println(readString);
                // if HTTP request has ended
                if (c == '\n') {
                    if (readString.indexOf("Submit=Subm") > 0) {
                        Serial.println(readString);
                        char Buf[250];
                        readString.toCharArray(Buf, 150);
                        char *token = strtok(Buf, "/?");  // Get everything up to the /if(token) // If we got something
                        {
                            char *name = strtok(NULL, "=");  // Get the first name. Use NULL as first argument to keep parsing same string
                            while (name) {
                                char *valu = strtok(NULL, "&");
                                if (valu) {
                                    if (String(name) == "?ssid") {
                                        String ssidname = valu;
                                        ssidname.toCharArray(owner.wifissid, 100);
                                    }
                                    if (String(name) == "password") {
                                        String pass = valu;
                                        pass.toCharArray(owner.Password, 100);
                                    }
                                    if (String(name) == "timezone") {
                                        String pass = valu;
                                        pass.toCharArray(owner.timezone, 10);
                                    }
                                    if (String(name) == "12hr") {
                                        String twelvehr = valu;
                                        twelvehr.toCharArray(owner.twelvehr, 3);
                                    }
                                    if (String(name) == "ShowTemp") {
                                        String temp = valu;
                                        temp.toCharArray(owner.temp, 3);
                                    }
                                    if (String(name) == "TempUnits") {
                                        String temp = valu;
                                        temp.toCharArray(owner.tempUnits, 3);
                                    }

                                    if (String(name) == "range") {
                                        String brightness = valu;
                                        brightness.toCharArray(owner.brightness, 5);
                                    }
                                    if (String(name) == "dimmer") {
                                        String dimmer = valu;
                                        dimmer.toCharArray(owner.dimmer, 3);
                                    }
                                    name = strtok(NULL, "=");  // Get the next name
                                }
                            }
                        }
                        String returnedip = validateSSID(owner);
                        if (returnedip == "0.0.0.0") {
                            Serial.println("Wifi Failed");
                            status = WiFi.beginAP(ssid);
                            strip.setPixelColor(50, strip.Color(255, 0, 0));
                            strip.setPixelColor(51, strip.Color(255, 0, 0));
                            strip.setPixelColor(36, strip.Color(255, 0, 0));
                            strip.setPixelColor(37, strip.Color(255, 0, 0));
                            strip.show();
                            printWiFiStatus();
                        } else {
                            strip.setPixelColor(50, strip.Color(0, 0, 255));
                            strip.setPixelColor(51, strip.Color(0, 0, 255));
                            strip.setPixelColor(37, strip.Color(0, 0, 255));
                            strip.setPixelColor(36, strip.Color(0, 0, 255));
                            strip.show();
                            owner.valid = true;
                            my_flash_store.write(owner);
                            owner2 = my_flash_store.read();
                            // Serial.println((String)owner2.temp);
                            Serial.println((String)owner2.dimmer);
                            Serial.println((String)owner2.twelvehr);
                            Serial.println((String)owner2.brightness);
                            ////digitalWrite(Reset_PIN, LOW);
                        }
                    }
                    String twelvehour = "";
                    if ((String)owner.twelvehr == "on") {
                        twelvehour = " selected='selected' ";
                    }
                    String ShowTempUnitsSetting = "";
                    if ((String)owner.tempUnits == "F") {
                        ShowTempUnitsSetting = " selected ";
                    }

                    String ShowTempSetting = "";
                    if ((String)owner.temp == "on") {
                        ShowTempSetting = " selected ";
                    }
                    String DimmerSetting = "";
                    if ((String)owner.dimmer == "on") {
                        DimmerSetting = " selected ";
                    }

                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println();
                    client.println("<HTML>");
                    client.println("<HEAD>");
                    client.println("<TITLE>Rainbow Clock Setup</TITLE>");
                    client.println("</HEAD>");

                    client.println("<BODY>");
                    client.println("<style type='text/css'>");
                    client.println("*,");
                    client.println("*::before,");
                    client.println("*::after {");
                    client.println("box-sizing: border-box;");
                    client.println("}");

                    client.println(":root {");
                    client.println("--select-border: #777;");
                    client.println("--select-focus: blue;");
                    client.println("--select-arrow: var(--select-border);");
                    client.println("}");

                    client.println("select {");
                    client.println("color: black;");
                    client.println("-webkit-appearance: none;");
                    client.println("-moz-appearance: none;");
                    client.println("appearance: none;");
                    client.println("background-color: transparent;");
                    client.println("border: none;");
                    client.println("padding: 0 1em 0 0;");
                    client.println("margin: 0;");
                    client.println("width: 100%;");
                    client.println("font-family: inherit;");
                    client.println("font-size: inherit;");
                    client.println("cursor: inherit;");
                    client.println("line-height: inherit;");
                    client.println("z-index: 1;");
                    client.println("outline: none;");
                    client.println("}");

                    client.println("select::-ms-expand {");
                    client.println("display: none;");
                    client.println("}");

                    client.println(".select {");
                    client.println("color: black;");
                    client.println("display: grid;");
                    client.println("grid-template-areas: 'select';");
                    client.println("align-items: center;");
                    client.println("position: relative;");
                    client.println("min-width: 15ch;");
                    client.println("max-width: 30ch;");
                    client.println("border: 1px solid var(--select-border);");
                    client.println("border-radius: 0.25em;");
                    client.println("padding: 0.25em 0.5em;");
                    client.println("font-size: 1.25rem;");
                    client.println("cursor: pointer;");
                    client.println("line-height: 1.1;");
                    client.println("background-color: #fff;");
                    client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
                    client.println("}");

                    client.println(".select select, .select::after {");
                    client.println("grid-area: select;");
                    client.println("}");

                    client.println(".select:not(.select--multiple)::after {");
                    client.println("content: '';");
                    client.println("justify-self: end;");
                    client.println("width: 0.8em;");
                    client.println("height: 0.5em;");
                    client.println("background-color: var(--select-arrow);");
                    client.println("-webkit-clip-path: polygon(100% 0%, 0 0%, 50% 100%);");
                    client.println("clip-path: polygon(100% 0%, 0 0%, 50% 100%);");
                    client.println("}");

                    client.println("select:focus + .focus {");
                    client.println("position: absolute;");
                    client.println("top: -1px;");
                    client.println("left: -1px;");
                    client.println("right: -1px;");
                    client.println("bottom: -1px;");
                    client.println("border: 2px solid var(--select-focus);");
                    client.println("border-radius: inherit;");
                    client.println("}");

                    client.println("select[multiple] {");

                    client.println("height: 6rem;");
                    client.println("}");

                    client.println("select[multiple] option {");
                    client.println("white-space: normal;");
                    client.println("outline-color: var(--select-focus);");
                    client.println("}");

                    client.println(".select--disabled {");
                    client.println("cursor: not-allowed;");
                    client.println("background-color: #eee;");
                    client.println("background-image: linear-gradient(to top, #ddd, #eee 33%);");
                    client.println("}");

                    client.println("label {");
                    client.println("font-size: 1.125rem;");
                    client.println("font-weight: 500;");
                    client.println("}");

                    client.println(".select + label {");
                    client.println("margin-top: 2rem;");
                    client.println("}");

                    client.println("body {");
                    client.println("min-height: 100vh;");
                    client.println("display: grid;");
                    client.println("place-content: center;");
                    client.println("grid-gap: 0.5rem;");
                    client.println("font-family: 'Baloo 2', sans-serif;");
                    client.println("/*background-color: #e9f2fd;*/");
                    client.println("padding: 1rem;");
                    client.println("}");

                    client.println(".formcontent label {");
                    client.println("/*color: red;*/");
                    client.println("display: block;");
                    client.println("}");

                    client.println(".formcontent {");
                    client.println("text-align: center;");
                    client.println("width: 30ch;");
                    client.println("margin: 0 auto;");
                    client.println("}");

                    client.println(".formcontent li {");
                    client.println("list-style: none;");
                    client.println("padding-top: 10px;");
                    client.println("width: 30ch;");
                    client.println("}");

                    client.println(".formcontent input {");
                    client.println("color: black;");
                    client.println("display: grid;");
                    client.println("grid-template-areas: 'select';");
                    client.println("align-items: center;");
                    client.println("position: relative;");
                    client.println("min-width: 15ch;");
                    client.println("max-width: 24ch;");
                    client.println("border: 1px solid var(--select-border);");
                    client.println("border-radius: 0.25em;");
                    client.println("padding: 0.25em 0.5em;");
                    client.println("font-size: 1.25rem;");
                    client.println("cursor: pointer;");
                    client.println("line-height: 1.1;");
                    client.println("background-color: #fff;");
                    client.println("text-align: center;");
                    client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
                    client.println("}");

                    client.println(".formcontent button {");
                    client.println("color: black;");
                    client.println("display: grid;");
                    client.println("grid-template-areas: 'select';");
                    client.println("align-items: center;");
                    client.println("position: relative;");
                    client.println("min-width: 15ch;");
                    client.println("max-width: 24ch;");
                    client.println("border: 1px solid var(--select-border);");
                    client.println("border-radius: 0.25em;");
                    client.println("padding: 0.25em 0.5em;");
                    client.println("font-size: 1.25rem;");
                    client.println("cursor: pointer;");
                    client.println("line-height: 1.1;");
                    client.println("background-color: #fff;");
                    client.println("text-align: center;");
                    client.println("background-image: linear-gradient(to top, #f9f9f9, #fff 33%);");
                    client.println("}");

                    client.println("html {");

                    client.println("background: linear-gradient(145deg,");
                    client.println("rgba(43, 9, 82, 1) 9%,");
                    client.println("rgba(2, 2, 255, 1) 18%,");
                    client.println("rgba(2, 255, 2, 1) 27%,");
                    client.println("rgba(196, 181, 0, 1) 36%,");
                    client.println("rgba(255, 165, 0, 1) 45%,");
                    client.println("rgba(162, 19, 19, 1) 63%,");
                    client.println("rgba(255, 165, 0, 1) 72%,");
                    client.println("rgba(196, 181, 0, 1) 81%,");
                    client.println("rgba(2, 255, 2, 1) 90%,");
                    client.println("rgba(2, 2, 255, 1) 99%,");
                    client.println("rgba(43, 9, 82, 1) 100%);");

                    client.println("background-size: 400% 400%;");
                    client.println("animation: gradient 5s ease infinite;");

                    client.println("height: 100vh;");
                    client.println("width: 100vw;");
                    client.println("align-items: center;");
                    client.println("justify-content: center;");
                    client.println("}");

                    client.println("@keyframes gradient {");
                    client.println("0% {");
                    client.println("background-position: 0% 50%;");
                    client.println("}");
                    client.println("50% {");
                    client.println("background-position: 100% 50%;");
                    client.println("}");
                    client.println("100% {");
                    client.println("background-position: 0% 50%;");
                    client.println("}");
                    client.println("}");

                    client.println(".loader {");
                    client.println("border: 16px solid #f3f3f3; /* Light grey */");
                    client.println("border-style: solid;");
                    client.println("border-width: 0px;");
                    client.println("/*border-top: 16px solid #3498db; !* Blue *!*/");
                    client.println("/*border-radius: 50%;*/");
                    client.println("width: 120px;");
                    client.println("height: 120px;");
                    client.println("/*animation: spin 2s linear infinite;*/");
                    client.println("}");

                    client.println("@keyframes spin {");
                    client.println("0% {");
                    client.println("transform: rotate(0deg);");
                    client.println("}");
                    client.println("100% {");
                    client.println("transform: rotate(360deg);");
                    client.println("}");
                    client.println("}");

                    client.println(".slidecontainer {width: 100%;}");

                    client.println(".slider {-webkit-appearance: none;width: 100%;height: 25px;background: #d3d3d3;outline: none;opacity: 0.7;-webkit-transition: .2s;transition: opacity .2s;}");

                    client.println(".slider:hover {opacity: 1;}");

                    client.println(".slider::-webkit-slider-thumb {-webkit-appearance: none;appearance: none;width: 25px;height: 25px;background: #04AA6D;cursor: pointer;}");

                    client.println(".slider::-moz-range-thumb {width: 25px;height: 25px;background: #04AA6D;cursor: pointer;}");

                    client.println("</style>");

                    client.println("<header class='container-col'>");
                    client.println("<H1>Enter your SSID, Password below. if I haven't fucked up, after you hit Save, it should reconnect to your main");
                    client.println("Wifi");
                    client.println("and this Wireless Access Point will vanish (if the info entered is correct)<br>You can revisit this setup page");
                    client.println("by");
                    client.println("checking your router or other tools for the clocks new ip address</H1>");
                    client.println("</header>");
                    client.print("<form method='get' action=''>");
                    client.println("<div class='formcontent'>");
                    client.println("<ul>");
                    client.println("<Li>");
                    client.println("<label for='ssid'>SSID</label>");
                    client.println("<input id='ssid' required type='text' name='ssid'");
                    client.print("value='");
                    client.print(owner.wifissid);
                    client.print("' placeholder='ssid'>");
                    client.println("</Li>");
                    client.println("<li>");
                    client.println("<label for='password'>Password</label>");
                    client.println("<input id='password'");
                    client.println("       type='password'");
                    client.println("       name='password'");
                    client.println("      value='");
                    client.print(owner.Password);
                    client.print("' placeholder='password' required><label for='password'>Password</label><br>");

                    client.println("</li>");
                    client.println("<li>");
                    client.println("<label for='ShowTemp'>Show Temperature every 1 minute</label>");
                    client.println("<div class='select'>");
                    client.print("<select name='ShowTemp' id='ShowTemp'>");
                    client.print("<option value='off'>off</option>");
                    client.print("<option value='on' ");
                    client.print(ShowTempSetting);
                    client.print(">on</option>");
                    client.print("</select>");
                    client.println("<span class='focus'></span>");
                    client.println("</div>");

                    client.println("</li>");

                    client.println("<li>");
                    client.println("<label for='TempUnits'>Temperature Units</label>");
                    client.println("<div class='select'>");
                    client.print("<select name='TempUnits' id='TempUnits'>");
                    client.print("<option value='C'>C</option>");
                    client.print("<option value='F' ");
                    client.print(ShowTempUnitsSetting);
                    client.print(">F</option>");
                    client.print("</select>");
                    client.println("<span class='focus'></span>");
                    client.println("</div>");

                    client.println("</li>");
                    client.println("<li>");
                    client.println("<label for='12hr'>Show 12hr Time</label>");
                    client.println("<div class='select'>");
                    client.println("<select name='12hr' id='12hr'>");
                    client.print("<option value='off'>off</option>");
                    client.print("<option value='on' ");
                    client.print(twelvehour);
                    client.print(">on</option>");
                    client.print("</select>");
                    client.println("<span class='focus'></span>");
                    client.println("</div>");

                    client.println("</li>");
                    client.println("<li>");
                    client.println("<label for='timezone'>Timezone</label>");
                    client.println("<div class='select'>");
                    client.println("<select name='timezone' id='timezone'>");
                    client.print("<option value='EST' ");
                    if ((String)owner.timezone == "EST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Eastern</option>");
                    client.print("<option value='CST' ");
                    if ((String)owner.timezone == "CST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Central</option>");
                    client.print("<option value='MST' ");
                    if ((String)owner.timezone == "MST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Mountain</option>");
                    client.print("<option value='PST' ");
                    if ((String)owner.timezone == "PST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Pacific</option>");
                    client.print("<option value='AKST' ");
                    if ((String)owner.timezone == "AKST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Alaska</option>");
                    client.print("<option value='HST' ");
                    if ((String)owner.timezone == "HST") {
                        client.print(" selected='selected' ");
                    }
                    client.print(">Hawaii</option>");

                    client.print(twelvehour);

                    client.print("</select>");

                    client.println("<li>");
                    client.println("<label for='dimmer'>Enable Auto Dimming</label>");
                    client.println("<div class='select'>");
                    client.println("<select name='dimmer' id='dimmer'>");
                    client.print("<option value='on'>on</option>");
                    client.print("<option value='off' ");
                    client.print(DimmerSetting);
                    client.print(">off</option>");
                    client.print("</select>");
                    client.println("<span class='focus'></span>");
                    client.println("</div>");

                    client.println("</li>");
                    client.println("<li>");

                    client.println("<label for='myrange'>Display Brightness</label>");

                    client.println("<div class='slidecontainer'>");
                    client.println("<input type='range' name='range' min='1' max='255' value='");
                    client.print(owner.brightness);

                    client.println("' class='slider' id='myRange'>");
                    client.println("<input type='text' name='rangevalue'  value='");
                    client.print(owner.brightness);
                    client.println("' id='rangevalue'>");
                    client.println(" <p>Value: <span id='demo'></span></p>");
                    client.println(" </div>");

                    client.println("</li>");

                    client.println("<li>");
                    client.println("<button id='Submit' name='Submit' value='Submit' onclick='submit();' type='submit'>Save</button>");
                    client.println("</li>");
                    client.println("</ul>");
                    client.println("</div>");
                    client.println("<script>");
                    client.println("var slider = document.getElementById('myRange');");
                    client.println("var output = document.getElementById('rangevalue');");
                    client.println("output.value = slider.value;");

                    client.println("slider.oninput = function() {");
                    client.println("  output.value = this.value;");
                    client.println("}");
                    client.println("</script>");
                    client.println("</BODY>");
                    client.println("</HTML>");

                    delay(1);
                    client.stop();
                    readString = "";
                }
            }
        }
    }
}
String validateSSID(Credentials owner) {
    status = WiFi.begin(owner.wifissid, owner.Password);
    delay(10000);
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("brightness: ");
    Serial.println(owner.brightness);
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    String LocalIP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    return (LocalIP);
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress &address) {
    Serial.println("1");
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    Serial.println("2");
    packetBuffer[0] = 0b11100011;  // LI, Version, Mode
    packetBuffer[1] = 0;           // Stratum, or type of clock
    packetBuffer[2] = 6;           // Polling Interval
    packetBuffer[3] = 0xEC;        // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;

    Serial.println("3");

    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123);  // NTP requests are to port 123
    Serial.println("4");
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Serial.println("5");
    Udp.endPacket();
    Serial.println("6");
}

void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

void GetTime() {
    strip.setPixelColor(28, strip.Color(255, 0, 0));
    strip.setPixelColor(29, strip.Color(255, 0, 0));

    strip.show();
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
        strip.setPixelColor(28, strip.Color(255, 150, 0));
        strip.setPixelColor(29, strip.Color(255, 150, 0));

        strip.show();
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(10000);
    }

    Serial.println("Connected to WiFi");
    strip.setPixelColor(28, strip.Color(0, 255, 0));
    strip.setPixelColor(29, strip.Color(0, 255, 0));

    strip.show();
    printWifiStatus();

    Serial.println("\nStarting connection to server...");
    Udp.begin(localPort);
    sendNTPpacket(timeServer);  // send an NTP packet to a time server
    delay(1000);
    if (Udp.parsePacket()) {
        Serial.println("packet received");
        strip.setPixelColor(28, strip.Color(0, 0, 255));
        strip.setPixelColor(29, strip.Color(0, 0, 255));

        strip.show();
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        const unsigned long seventyYears = 2208988800UL;
        unsigned long epoch = secsSince1900 - seventyYears;
        //   rtc.setEpoch(epoch + 1);
    }
}

void reboot() {
    //  wdt_disable();
    //  wdt_enable(WDTO_15MS);
    while (1) {
        ;
    }
}