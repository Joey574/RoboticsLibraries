# Arduino Packet Manager
A simple packet manager for arduino, originally made to handle bluetooth connections

## Installation
The packet manager is a header only library, just navigate over to *src/PacketManager.hpp* and download the file, pasting it into your local Arduino libraries folder

## Basic Usage

<br>

RX Code
``` C++
SoftwareSerial BT(10, 11); // <- creates a stream for our data to cross through
trns::SerialTransport transport(BT); // <- creates an abstraction layer for our data
pckt::PacketManager manager(transport); // <- creates packet manager

/// @brief This function will get called evertime we recieve a data packet
/// @param packet This is the packet we recieved
void dataCallback(const pckt::Packet& packet) {
    printf("We recieved a packet!\n");

    // we can get the passed data by indexing into packet.payload
    uint8_t x = packet.payload[0];
    uint8_t y = packet.payload[1];
}

void setup() {
    // This tells the manager to call dataCallback everytime a packet of type DataPacket is recieved
    manager.Callback(pckt::Type::DataPacket, &dataCallback);
}

void loop() {
    // This is where most of the work is handled and is where the packet manager will check to see if there
    // is any packets to be read
    manager.Update();
}
```

<br>

TX Code
``` C++
// same setup as before
SoftwareSerial BT(10, 11);
trns::SerialTransport transport(BT);
pckt::PacketManager manager(transport);

void setup() {
    // this time we don't need to do anything in setup
}

void loop() {
    // this defines the payload, ie data, we want to send, it can be up to MAX_PAYLOAD_SIZE
    // default is 8, but you can change that as needed in the source
    // in this case we are sending the numbers 1 and 2
    uint8_t payload[2] = { 1, 2 };
    size_t len = 2;

    // this will actually send the payload, setting its type to DataPacket, and telling the
    // manager how many bytes payload contains
    manager.Send(pckt::Type::DataPacket, payload, len);
}

```
