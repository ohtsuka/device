#include "Arduino.h"
#include <SoftwareSerial.h>
#include "BGLib.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"

#define LED_PIN 13

// BLE112 module connections:
// - BLE P0_2 -> GND (CTS tied to ground to bypass flow control)
// - BLE P0_4 -> Arduino Digital Pin 6 (BLE TX -> Arduino soft RX)
// - BLE P0_5 -> Arduino Digital Pin 5 (BLE RX -> Arduino soft TX)

// NOTE: this demo REQUIRES the BLE112 be programmed with the UART connected
// to the "api" endpoint in hardware.xml, have "mode" set to "packet", and be
// configured for 38400 baud, 8/N/1. This may change in the future, but be
// aware. The BLE SDK archive contains an /examples/uartdemo project which is
// a good starting point for this communication, though some changes are
// required to enable packet mode and change the baud rate. The BGLib
// repository also includes a project you can use for this in the folder
// /BLEFirmware/BGLib_U1A1P_38400_noflow_wake16_hwake15.

// iMote git:8fa00b089894132e3f6906fea1009a4e53ce5834
SoftwareSerial ble112uart( 6, 5 ); // RX, TX
// create BGLib object:
//  - use SoftwareSerial por for module comms
//  - use nothing for passthrough comms (0 = null pointer)
//  - enable packet mode on API protocol since flow control is unavailable
BGLib ble112((HardwareSerial *)&ble112uart, 0, 1);

// ================================================================
// INTERNAL BGLIB CLASS CALLBACK FUNCTIONS
// ================================================================

void onBusy() {
    // turn LED on when we're busy
    digitalWrite(LED_PIN, HIGH);
}

void onIdle() {
    // turn LED off when we're no longer busy
    digitalWrite(LED_PIN, LOW);
}

void onTimeout() {
    Serial.println(P("!!!\tTimeout occurred!"));
}

void onBeforeTXCommand() {
}

void onTXCommandComplete() {
}

// ================================================================
// USER-DEFINED BGLIB RESPONSE CALLBACKS
// ================================================================

void my_rsp_system_hello(const ble_msg_system_hello_rsp_t *msg) {
    Serial.println(P("<--\tsystem_hello"));
}

void my_rsp_gap_set_scan_parameters(const ble_msg_gap_set_scan_parameters_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_scan_parameters: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_discover(const ble_msg_gap_discover_rsp_t *msg) {
    Serial.print(P("<--\tgap_discover: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_end_procedure(const ble_msg_gap_end_procedure_rsp_t *msg) {
    Serial.print(P("<--\tgap_end_procedure: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_gap_set_mode(const ble_msg_gap_set_mode_rsp_t *msg) {
    Serial.print(P("<--\tgap_set_mode: { "));

    // 0x020C: Command Disallowed
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);

    Serial.println(P(" }"));
}

void my_rsp_attributes_write(const ble_msg_attributes_write_rsp_t *msg) {
    Serial.print(P("<--\tattributes_write: { "));
    Serial.print(P("result: ")); Serial.print((uint16_t)msg -> result, HEX);
    Serial.println(P(" }"));
}

void my_rsp_connection_get_rssi(const ble_msg_connection_get_rssi_rsp_t *msg) {
    Serial.print(P("<--\tconnection_get_rssi: { "));
    Serial.print(P("conn: "));   Serial.print((uint8)msg -> connection, HEX);
    Serial.print(P(", rssi: ")); Serial.print((uint8)msg -> rssi);
    Serial.println(P(" }"));
}

// ================================================================
// USER-DEFINED BGLIB EVENT CALLBACKS
// ================================================================

void my_evt_system_boot(const ble_msg_system_boot_evt_t *msg) {
    Serial.print(P("###\tsystem_boot: { "));
    Serial.print(P("major: ")); Serial.print(msg -> major, HEX);
    Serial.print(P(", minor: ")); Serial.print(msg -> minor, HEX);
    Serial.print(P(", patch: ")); Serial.print(msg -> patch, HEX);
    Serial.print(P(", build: ")); Serial.print(msg -> build, HEX);
    Serial.print(P(", ll_version: ")); Serial.print(msg -> ll_version, HEX);
    Serial.print(P(", protocol_version: ")); Serial.print(msg -> protocol_version, HEX);
    Serial.print(P(", hw: ")); Serial.print(msg -> hw, HEX);
    Serial.println(P(" }"));
}

void my_evt_gap_scan_response(const ble_msg_gap_scan_response_evt_t *msg) {
    Serial.print(P("###\tgap_scan_response: { "));
    Serial.print(P("rssi: ")); Serial.print(msg -> rssi);
    Serial.print(P(", packet_type: ")); Serial.print((uint8_t)msg -> packet_type, HEX);
    Serial.print(P(", sender: "));
    // this is a bd_addr data type, which is a 6-byte uint8_t array
    for (uint8_t i = 0; i < 6; i++) {
        if (msg -> sender.addr[i] < 16) Serial.write('0');
        Serial.print(msg -> sender.addr[i], HEX);
    }
    Serial.print(P(", address_type: ")); Serial.print(msg -> address_type, HEX);
    Serial.print(P(", bond: ")); Serial.print(msg -> bond, HEX);
    Serial.print(P(", data: "));
    // this is a uint8array data type, which is a length byte and a uint8_t* pointer
    for (uint8_t i = 0; i < msg -> data.len; i++) {
        if (msg -> data.data[i] < 16) Serial.write('0');
        Serial.print(msg -> data.data[i], HEX);
    }
    Serial.println(P(" }"));
}

void my_evt_connection_status_evt_t(const ble_msg_connection_status_evt_t *msg) {
    Serial.print(P("###\tconnection_status: { "));
    Serial.print(P("conn: "));    Serial.print(msg -> connection, HEX);
    Serial.print(P(", flags: ")); Serial.print(msg -> flags, HEX);
    Serial.print(P(", address: "));
    // this is a bd_addr data type, which is a 6-byte uint8_t array
    for (uint8_t i = 0; i < 6; i++) {
        if (msg -> address.addr[i] < 16) Serial.write('0');
        Serial.print(msg -> address.addr[i], HEX);
    }
    Serial.print(P(", address_type: ")); Serial.print(msg -> address_type, HEX);
    Serial.print(P(", intvl: "));        Serial.print(msg -> conn_interval, HEX);
    Serial.print(P(", timeout: "));      Serial.print(msg -> timeout, HEX);
    Serial.print(P(", latency: "));      Serial.print(msg -> latency, HEX);
    Serial.print(P(", bonding: "));      Serial.print(msg -> bonding, HEX);
    Serial.println(P(" }"));
}

void my_evt_connection_disconnected(const ble_msg_connection_disconnected_evt_t *msg) {
    Serial.println( P("###\tdisconnected") );

    // ble112.ble_cmd_gap_set_mode( BGLIB_GAP_GENERAL_DISCOVERABLE, BGLIB_GAP_UNDIRECTED_CONNECTABLE );
}

void setup() {
    // initialize status LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // USB serial
    Serial.begin(115200);

    // welcome!
    Serial.println(P("BLE112 BGAPI Scanner Demo"));

    // set up internal status handlers
    // (these are technically optional)
    ble112.onBusy = onBusy;
    ble112.onIdle = onIdle;
    ble112.onTimeout = onTimeout;

    // ONLY enable these if you are using the <wakeup_pin> parameter in your firmware's hardware.xml file
    ble112.onBeforeTXCommand = onBeforeTXCommand;
    ble112.onTXCommandComplete = onTXCommandComplete;

    // set up BGLib response handlers (called almost immediately after sending commands)
    // (these are also technicaly optional)
    ble112.ble_rsp_system_hello            = my_rsp_system_hello;
    ble112.ble_rsp_gap_set_scan_parameters = my_rsp_gap_set_scan_parameters;
    ble112.ble_rsp_gap_discover            = my_rsp_gap_discover;
    ble112.ble_rsp_gap_end_procedure       = my_rsp_gap_end_procedure;
    ble112.ble_rsp_gap_set_mode            = my_rsp_gap_set_mode;
    ble112.ble_rsp_attributes_write        = my_rsp_attributes_write;
    ble112.ble_rsp_connection_get_rssi     = my_rsp_connection_get_rssi;

    // set up BGLib event handlers (called at unknown times)
    ble112.ble_evt_system_boot             = my_evt_system_boot;
    ble112.ble_evt_gap_scan_response       = my_evt_gap_scan_response;
    ble112.ble_evt_connection_status       = my_evt_connection_status_evt_t;
    ble112.ble_evt_connection_disconnected = my_evt_connection_disconnected;

    // set the data rate for the SoftwareSerial port
    ble112uart.begin(38400);
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    Serial.println(P("Operations Menu:"));
    Serial.println(P("0) Reset BLE112 module"));
    Serial.println(P("1) Say hello to the BLE112 and wait for response"));
    Serial.println(P("3) Set gap mode(2,2)"));
    Serial.println(P("4) Write attributes"));
    Serial.println(P("5) Get rssi"));
    Serial.println(P("6) Indicate new value"));
    Serial.println(P("Command?"));
    while (1) {
        // keep polling for new data from BLE
        ble112.checkActivity();

        // check for input from the user
        if (Serial.available()) {

            Serial.print(P("free:"));
            Serial.println( freeMemory() );

            uint8_t newCharacter = Serial.read();
            Serial.print(P("0x"));
            Serial.println( newCharacter, HEX );

            uint8_t status;
            if (newCharacter == '0') {
                // Reset BLE112 module
                Serial.println(P("-->\tsystem_reset: { boot_in_dfu: 0 }"));
                ble112.ble_cmd_system_reset(0);

                while ((status = ble112.checkActivity(1000)));
                // system_reset doesn't have a response, but this BGLib
                // implementation allows the system_boot event specially to
                // set the P("busy") flag to false for this particular case
            }
            else if (newCharacter == '1') {
                // Say hello to the BLE112 and wait for response
                Serial.println(P("-->\tsystem_hello"));
                ble112.ble_cmd_system_hello();

                while ((status = ble112.checkActivity(1000)));
                // response should come back within milliseconds
            }
            else if (newCharacter == '3') {
                Serial.println(P("-->\tgap_set_mode: { discover: 0x2, connect: 0x2 }"));
                ble112.ble_cmd_gap_set_mode( BGLIB_GAP_GENERAL_DISCOVERABLE, BGLIB_GAP_UNDIRECTED_CONNECTABLE );
                while ((status = ble112.checkActivity(1000)));
            }
            else if (newCharacter == '4') {
                Serial.println(P("-->\tattributes_write"));

                uint8_t data[] = {
                    lastCharacter,
                    (lastCharacter+1)%256,
                    (lastCharacter+2 )%256,
                    (lastCharacter+3 )%256,
                    (lastCharacter+4 )%256,
                    (lastCharacter+5 )%256,
                    (lastCharacter+6 )%256,
                    (lastCharacter+7 )%256,
                    (lastCharacter+8 )%256,
                    (lastCharacter+9 )%256,
                    (lastCharacter+10)%256,
                    (lastCharacter+11)%256,
                    (lastCharacter+12)%256,
                    (lastCharacter+13)%256,
                    (lastCharacter+14)%256,
                    (lastCharacter+15)%256,
                    (lastCharacter+16)%256,
                    (lastCharacter+17)%256,
                    (lastCharacter+18)%256
                };
                ble112.ble_cmd_attributes_write( (uint16)0x0014,       // handle
                                                 (uint8)0,             // offset
                                                 (uint8)sizeof(data),  // value_len
                                                 (const uint8*)&data   // value_data
                                                 );
                while ((status = ble112.checkActivity(1000)));
            }
            else if (lastCharacter == '5') {
                Serial.println(P("-->\tget_rssi"));
                ble112.ble_cmd_connection_get_rssi( 0x00 ); // connection handle
                while ((status = ble112.checkActivity(1000)));
            }

            lastCharacter = newCharacter;
        }
    }
}