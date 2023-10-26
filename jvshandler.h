#ifndef jvshandler_h
#define jvshandler_h

#include <Arduino.h>
#include <constants.h>
#include <Adafruit_TinyUSB.h>

struct JVSData {
    byte* payload;
    uint length;
    bool status;
};

class JVSHandler {
    public:
        JVSHandler();
        ~JVSHandler();
        JVSData receive();
        bool send(byte* payload, uint length);  
    private:
        JVSData recvdata;
        uint calc(uint length,byte* payload);
        uint sum;
};

#endif