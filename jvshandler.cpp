#include <jvshandler.h>

/*
SEGA Keychip-specific JVS message format

[SYNC][LEN][PAYLOAD][SUM]
  1B   2B   LEN - 1  1B

SYNC is always 0xE0
LEN is length of any data trailing itself in u16
SUM = (sum of all bytes except SYNC and SUM) MOD 256
*/


JVSHandler::JVSHandler() : recvdata({NULL, 0, false}) {   //Initialize payload pointer to NULL
    while (debug) {
        Serial1.println("--------------------------------------------");
        Serial1.println("Created JVS handler");
        break;
    }
}

JVSHandler::~JVSHandler() {
    while (debug) {
        Serial1.println("JVS Handler destroyed");
        Serial1.println("--------------------------------------------");
        break;
    }
    if (recvdata.payload != NULL) {              //Discard buffer to prevent memory leakage
        delete[] recvdata.payload;
    }
}

bool JVSHandler::send(byte* payload, uint length) {
    uint checksum;
    byte* buffer = new byte[length + 4];        //Create a buffer for all outgoing data
    byte header[3];
    header[0] = 0xE0;                           //The first byte of the header is always 0xE0 (JVS SYNC)
    header[1] = (length + 1) >> 8;              //Calculate length bytes
    header[2] = (length + 1) & 0xFF;

    memcpy(buffer, header, 3);                  //Concatenate all buffers
    memcpy(buffer + 3, payload, length);
    checksum = calc(length, payload);           //Calculate checksum
    buffer[length + 3] = checksum;

    size_t buffsize = length + 4;

    if (buffsize == Serial.write(buffer,buffsize)) {
        while (debug) {
            Serial1.println();
            Serial1.println("Successfully sent JVS payload: ");
            for (uint i=0; i<buffsize; i++) {
                if (buffer[i] < 16) {
                Serial1.print("0");
                }
                Serial1.print(buffer[i],HEX);
                Serial1.print(" ");
            }
            Serial1.println();
            break;
        }
        delete[] buffer;        //Return status and delete buffer to prevent memory leakage
        return true;
    } else {
        delete[] buffer;
        return false;
    }
}

JVSData JVSHandler::receive() {
    uint checksum;
    byte header[3];                                                  //Create a buffer for the packet header
    Serial.readBytes(header,3);                                      //Read first 3 bytes of the packet
    recvdata.length = ((header[1] << 8) | header[2]) - 1;            //Determine the length of any trailing payload

    if (recvdata.payload != NULL) {delete[] recvdata.payload;}       //Ensure we discard any existing buffer
    
    if (header[2] == Serial.available()) {                           //If length(n) matches payload length in FIFO
        recvdata.payload = new byte[recvdata.length];                //Allocate a buffer with size (length)
        Serial.readBytes(recvdata.payload,recvdata.length);          //Read payload into buffer
        while (debug) {
            Serial1.println("Payload length check passed");
            break;
        }
    }

    sum = Serial.read();                                             //Read checksum byte

    while (debug) {
        Serial1.println("Received JVS SYNC 0xE0");
        Serial1.print("Payload length is: ");
        Serial1.println(recvdata.length);
        Serial1.print("Received JVS Header: ");
        for (uint i=0;i<3;i++) {
            if (header[i] < 16) {
                Serial1.print("0");
            }
        Serial1.print(header[i],HEX);
        Serial1.print(" ");
        }
        Serial1.println();
        Serial1.print("Received JVS Payload: ");
        for (uint i=0; i<recvdata.length; i++) {
            if (recvdata.payload[i] < 16) {
                Serial1.print("0");
            }
            Serial1.print(recvdata.payload[i],HEX);
            Serial1.print(" ");
        }
        Serial1.println();
        Serial1.print("Received Checksum: ");
        Serial1.println(sum, HEX);
        break;
    }
    
    checksum = calc(recvdata.length,recvdata.payload);          //Calculate checksum
    
    if (checksum == sum) {
        recvdata.status = true;
        while (debug) {
            Serial1.println("Checksum Passed");
            break;
        }
    } else {
        recvdata.status = false;
        const byte failuremsg[] = {0xE0,0x00,0x02,0x03,0x05};         //Report Code 0x03: Checksum failure
        Serial.write(failuremsg,5);
        while (debug) {
            Serial1.println("Checksum Failed");
            break;
        }
    }

    return recvdata;
}

uint JVSHandler::calc(uint length,byte* payload) {
    uint checksum = 0;
    checksum += length;
    checksum += 1;
    for (uint i=0;i<length;i++) {
        checksum += payload[i];
    }
    checksum %= 256;
    return checksum;
}