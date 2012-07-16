// Copyright (c) 2012 Wyss Institute at Harvard University
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// http://www.opensource.org/licenses/mit-license.php

// ptwebserver.ino -- derived from Web_Demo.ino sample code for Webduino server 
//                      library
// dependency Webduino webserver library
// https://github.com/sirleech/Webduino
 
// http://host/
// This URL brings up a display of the values READ on digital pins 0-9
// and analog pins 0-5.  This is done with a call to defaultCmd.
//
// http://host/form
// This URL also brings up a display of the values READ on digital pins 0-9
// and analog pins 0-5.  But it's done as a form,  by the "formCmd" function,
// and the digital pins are shown as radio buttons you can change.
// When you click the "Submit" button,  it does a POST that sets the
// digital pins,  re-reads them,  and re-displays the form.
//
// http://host/json
// return: response JSON formatted system state with respect to arduino pins
// 
// http://host/ptrain
// input: GET request to turn on a PulseTrain device
// return: response JSON formatted system state


#define SERVER_IP 192,168,1,145
#define MAC_ADDRESS  0x30, 0xA2, 0xDA, 0x00, 0x75, 0xC1

#define PREFIX ""
#define NAMELEN 32
#define VALUELEN 32

#define P_USE_TIMER1

// Analog Pins
 
// Digital Pins
#define PTRAIN_PIN   3

// Macros
/////////
#define ARE_STRINGS_EQ(s1, s2) (strncmp(s1, s2, sizeof(s2)-1) == 0)
#define GET_INIT    0
#define GET_ACCEPT  1
#define GET_FAIL    2

#include "SPI.h"
#include "avr/pgmspace.h" // new include
#include <string.h>
#include "Ethernet.h"
#include "WebServer.h"
#include "pulsetrain.h"
#define VERSION_STRING "0.1"

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

// CHANGE THIS TO YOUR OWN UNIQUE VALUE
static uint8_t mac[] = {  MAC_ADDRESS };

// CHANGE THIS TO MATCH YOUR HOST NETWORK
static uint8_t ip[] = {SERVER_IP}; 

WebServer webserver(PREFIX, 80);

//
uint8_t waterp = pNewPTrain();

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void returnState(WebServer &server, WebServer::ConnectionType type) 
{
    // return system state.  
    server << "{ ";
    server << "\"PERIOD\": " << pGetPeriodCounts(waterp);
    server << ",\"PULSEWIDTH\": " << pGetPulseCounts(waterp);
    server << ",\"NUM_PERIODS\": " << pGetPeriodNumber(waterp);
    server << " }";
}

void returnFailState(WebServer &server, WebServer::ConnectionType type, uint8_t fail) 
{
    server << "{ \"FAIL\": " << fail << " }";
}

// Takes a function pointer to a processor
void getCmd(WebServer &server, WebServer::ConnectionType type, 
        char *url_tail, bool tail_complete, void (*myCmd)(char*, char*, uint8_t*, uint8_t*))
{
    URLPARAM_RESULT rc;
    char name[NAMELEN];
    int  name_len;
    char value[VALUELEN];
    int value_len;
    uint8_t arg = 0;
    uint8_t ret_value = GET_INIT;
    
    /* this line sends the standard "we're all OK" headers back to the
       browser */
    server.httpSuccess("application/json");

    /* if we're handling a GET, we can output our data here.
       For a HEAD request, we just stop after outputting headers. */
    if (type == WebServer::HEAD) {
        return;
    }

    if (strlen(url_tail)) {
        while (strlen(url_tail)) {
            rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
            if (rc != URLPARAM_EOS) {
                (*myCmd)(name, value, &arg, &ret_value); // execute the function ptr
            } // end if
        } // end while
    } // end if
    switch (ret_value) {
        case GET_INIT:
        case GET_ACCEPT:
            returnState(server,type);
            break;
        case GET_FAIL:
            returnFailState(server,type, arg);
    } 
}

void pTrainParse(char* name, char* value, uint8_t* pt, uint8_t* ret_value)
{
    int value_copy;
    switch (*ret_value) {
        case GET_INIT:
            if (ARE_STRINGS_EQ(name, "VALVE")) {
                value_copy =  atoi(value);
                if (pIsValidPTrain(value_copy)){
                    *pt = value_copy;
                    *ret_value =  GET_ACCEPT;
                    break;
                } // end if
            } // end if
            *pt = 7;
            *ret_value =  GET_FAIL;
            break;
        case GET_ACCEPT:
            if (ARE_STRINGS_EQ(name, "PULSE")) {
                value_copy =  atoi(value);
                pSetPulseOnlyUS(*pt, (uint16_t) value_copy);
                pReloadToTimer(waterp); 
            } // end 
            else if (ARE_STRINGS_EQ(name, "PERIOD")) {
                value_copy =  atoi(value);
                pSetPeriodOnlyUS(*pt, (uint16_t) value_copy);
                pReloadToTimer(*pt); 
            } // end if
            else if (ARE_STRINGS_EQ(name, "COUNT")) {
                value_copy =  atoi(value);
                pSetPeriodNumberOnly(*pt, (uint16_t) value_copy);
                pReloadToTimer(*pt); 
            } // end if
            else if (ARE_STRINGS_EQ(name, "START")) {
                pStartTimer(PTIMER1);
            } // end if
    }
}

void pulseTrainCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
    getCmd(server, type, url_tail, tail_complete, pTrainParse);
}

// from Web_Demo.ino 
void jsonCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
    if (type == WebServer::POST) {
        server.httpFail();
        return;
    }

    server.httpSuccess("application/json");

    if (type == WebServer::HEAD)
        return;

    int i;    
    server << "{ ";
    for (i = 0; i <= 9; ++i) {
        // ignore the pins we use to talk to the Ethernet chip
        int val = digitalRead(i);
        server << "\"d" << i << "\": " << val << ", ";
    }

    for (i = 0; i <= 5; ++i) {
        int val = analogRead(i);
        server << "\"a" << i << "\": " << val;
        if (i != 5) { 
            server << ", ";
        }
    }
  
    server << " }";
}

// from Web_Demo.ino for debugging use only 
void outputPins(WebServer &server, WebServer::ConnectionType type, bool addControls = false)
{
    P(html_head) =
    "<html>"
    "<head>"
    "<title>Evolvulator Web Server</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: sans-serif }"
    "H1 { font-size: 14pt; text-decoration: underline }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>";
    P(digital_pin_header) = "<h1>Digital Pins</h1><p>";
    P(analog_pin_header) = "</p><h1>Analog Pins</h1><p>";
    
    int i;
    server.httpSuccess();
    server.printP(html_head);

    if (addControls) {
        server << "<form action='" PREFIX "/form' method='post'>";
    }

    server.printP(digital_pin_header);

    for (i = 0; i <= 9; ++i) {
        // ignore the pins we use to talk to the Ethernet chip
        int val = digitalRead(i);
        server << "Digital " << i << ": ";
        if (addControls) {
            char pin_name[4];
            pin_name[0] = 'd';
            itoa(i, pin_name + 1, 10);
            server.radioButton(pin_name, "1", "On", val);
            server << " ";
            server.radioButton(pin_name, "0", "Off", !val);
        }
        else {
            server << (val ? "HIGH" : "LOW");
        }
        server << "<br/>";
    }

    server.printP(analog_pin_header);
    
    for (i = 0; i <= 5; ++i) {
        int val = analogRead(i);
        server << "Analog " << i << ": " << val << "<br/>";
    }

    server << "</p>";

    if (addControls) {
        server << "<input type='submit' value='Submit'/></form>";
    }
    server << "</body></html>";
}

// from Web_Demo.ino for debugging use only 
void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) 
{
    if (type == WebServer::POST) {
        bool repeat;
        char name[16], value[16];
        do {
            repeat = server.readPOSTparam(name, 16, value, 16);
            if (name[0] == 'd') {
                int pin = strtoul(name + 1, NULL, 10);
                int val = strtoul(value, NULL, 10);
                digitalWrite(pin, val);
            }
        } while (repeat);
        server.httpSeeOther(PREFIX "/form");
    }
    else {
        outputPins(server, type, true);
    }
}

// from Web_Demo.ino for debugging use only 
void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
    outputPins(server, type, false);  
}

void setup()
{
    // set pins for digital output
    pSetupTimers();
    pSetPulseUS(waterp, 20000, 10000, 100);
    pAttach(waterp, PTRAIN_PIN, PTIMER1);

    Ethernet.begin(mac, ip);
    webserver.begin();

    webserver.setDefaultCommand(&defaultCmd);
    webserver.addCommand("json", &jsonCmd);
    webserver.addCommand("ptrain", &pulseTrainCmd);
    webserver.addCommand("form", &formCmd);
}

void loop()
{
    char buff[64];
    int len = 64;
    // process incoming connections one at a time forever
    webserver.processConnection(buff, &len);
    // if you wanted to do other work based on a connecton, it would go here
}
