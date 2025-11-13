#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MAX_PAYLOAD_SIZE 8
#define PACKET_COUNT 3

namespace pckt {
    public:

    /// @brief Types of packets that can be sent / recieved 
    enum class Type {
        None, 
        AckPacket,
        MovePacket,
    };


    /// @brief General packet format
    struct Packet {
        public:
        uint8_t type;
        
        // 8th bit -> | checksum | sendack |
        uint8_t flags; 

        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint8_t checksum;
    };


    /// @brief Transport layer abstraction
    class Transport {
        public:
        virtual size_t write(const uint8_t* data, size_t len) = 0;
        virtual int read(uint8_t* data, size_t len) = 0;
    };


    /// @brief Transport layer implementation for SoftwareSerial
    class SerialTransport : public Transport {
        public:
        SerialTransport(SoftwareSerial& serial) : serial(serial) {}

        size_t write(const uint8_t* data, size_t len) override { return serial.write((char*)data, len); }
        int read(uint8_t* data, size_t len) override { return serial.readBytes(data, len); }

        private:
        SoftwareSerial& serial;
    };


    /// @brief Manages sending and recieving packets
    struct PacketManager {
        using Handler = void(*)(const Packet&);

        public:
        PacketManager(Transport& transport) : transport(transport) {
            for(size_t i = 0; i < PACKET_COUNT; i++) {
                handlers[i] = nullptr;
            }
        }

        inline void Update() {
            if (transport.read(&rxPacket, sizeof(rxPacket))) {

                // if packet has a checksum, verify it
                if (HasChecksum(rxPacket)) {
                    bool stable = VerifyChecksum(rxPacket);
                    if (!stable) {
                        return;
                    }
                }

                handlers[rxPacket.type](rxPacket.payload);

                // if packet expects an ack, send one
                if (HasSendAck(rxPacket)) {

                }
            }

        }

        /// @brief Sets callback function when a packet is recieved
        /// @param type Type of packet this handler applies to
        /// @param handler Pointer to handler function
        inline void Callback(Type type, Handler handler) {
            int t = (int)type;
            if (t < 0 || t >= PACKET_COUNT) {
                return;
            }
            
            handlers[t] = handler;
        }

        inline void Send(Type type, uint8_t* payload) const {

        }


        private:
        Transport& transport;
        Packet rxPacket;

        Handler handlers[PACKET_COUNT];

        static inline uint8_t ComputeChecksum(const Packet& packet) {
            uint8_t sum = 0;

            sum += packet.type;
            sum += packet.flags;
            for (size_t i = 0; i < sizeof(packet.payload); i++) {
                sum += packet.payload[i];
            }

            return sum;
        }
        static inline bool VerifyChecksum(const Packet& packet) {
            return ComputeChecksum(packet) == packet.checksum;
        }

        static inline bool HasChecksum(const Packet& packet) {
            return packet.flags & 0b10000000;
        }
        static inline bool HasSendAck(const Packet& packet) {
            return packet.flags & 0b01000000;
        }
    };


} // namespace pckt
