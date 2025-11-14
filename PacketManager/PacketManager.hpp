#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>

#define MAX_PAYLOAD_SIZE 8
#define PACKET_COUNT 3

namespace pckt {

    /// @brief Types of packets that can be sent / recieved
    enum class Type {
        None,
        DataPacket,
        AckPacket,
    }; // enum Type


    /// @brief General packet format
    struct Packet {
        public:
        uint8_t type;
       
        // 8th bit -> | checksum | critical | tbd | tbd | user#4 | user#3 | user#2 | user#1 | <- 1st bit
        uint8_t flags;

        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint8_t checksum;
    }; // struct Packet


    /// @brief Transport layer abstraction
    struct Transport {
        public:
        virtual int read(uint8_t* data, size_t len) = 0;
        virtual size_t write(const uint8_t* data, size_t len) = 0;

        virtual bool available() = 0;
        virtual void clear() = 0;
    }; // struct Transport


    /// @brief Transport layer implementation for SoftwareSerial
    struct SerialTransport : public Transport {
        public:
        SerialTransport(SoftwareSerial& serial) : serial(serial) {}

        size_t write(const uint8_t* data, size_t len) override { return serial.write((char*)data, len); }
        int read(uint8_t* data, size_t len) override { return serial.readBytes(data, len); }
        bool available() override { return serial.available(); }
        void clear() override { serial.flush(); }

        private:
        SoftwareSerial& serial;
    }; // struct SerialTransport


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
            if (!transport.available()) { return; }

            size_t n = transport.read((uint8_t*)&rxPacket, sizeof(rxPacket));
            if (n != sizeof(rxPacket)) {
                transport.clear();
                return;
            }

            // if packet has a checksum, verify it
            if (HasChecksum(rxPacket)) {
               if (rxPacket.checksum != ComputeChecksum(rxPacket)) {
                transport.clear();
                return;
               }
            }

            // packet has been verified, call user defined handler
            if (handlers[rxPacket.type]) {
                handlers[rxPacket.type](rxPacket);
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


        /// @brief Send packet via Transport
        /// @param type Type of packet to send
        /// @param payload Packet payload
        /// @param len Number of bytes in packet payload
        inline void Send(Type type, const uint8_t* payload, size_t len) const {
            txPacket.type = (int)type;
            txPacket.flags = 0b10000000;

            memset(txPacket.payload, 0, MAX_PAYLOAD_SIZE);

            if (payload) {
                memcpy(txPacket.payload, payload, len);
            }

            txPacket.checksum = ComputeChecksum(txPacket);
            transport.write((uint8_t*)&txPacket, sizeof(txPacket));
        }


        /// @brief Sets user defined flags
        /// @tparam flag Flag [0-3] to set
        /// @param v Value [0-1] to set flag to
        template <uint8_t flag> SetFlag(uint8_t v) {
            static_assert(flag >= 0 && flag < 4);
            txPacket.flags |= ((v << flag ) & 1);
        }


        private:
        Handler handlers[PACKET_COUNT];
        Transport& transport;

        Packet txPacket;
        Packet rxPacket;


        /// @brief Compute mod 256 checksum over packet
        /// @param packet Packet to compute checksum from
        /// @return Computed checksum
        static inline uint8_t ComputeChecksum(const Packet& packet) {
            uint8_t sum = 0;

            sum += packet.type;
            sum += packet.flags;
            for (size_t i = 0; i < sizeof(packet.payload); i++) {
                sum += packet.payload[i];
            }

            return sum;
        }


        static inline bool HasChecksum(const Packet& packet) { return packet.flags & 0b10000000; }
        static inline bool HasCritical(const Packet& packet) { return packet.flags & 0b01000000; }
    }; // struct PacketManager


} // namespace pckt