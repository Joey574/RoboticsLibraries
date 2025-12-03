#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>

#define READ_TIMEOUT 100
#define MAX_PAYLOAD_SIZE 8
#define PACKET_COUNT 3
#define MAGIC_NUM 0xAA

namespace pckt {

    /// @brief Types of packets that can be sent / recieved
    enum class Type {
        None,
        DataPacket,
        AckPacket,
    }; // enum Type


    /// @brief General packet format
    struct __attribute__((packed)) Packet {
        public:
        uint8_t magic = MAGIC_NUM;
        uint8_t type;
       
        // 8th bit -> | critical | tbd | tbd | tbd | user#4 | user#3 | user#2 | user#1 | <- 1st bit
        uint8_t flags;

        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint16_t checksum;

        inline uint8_t* self() { return reinterpret_cast<uint8_t*>(this); }
        inline const uint8_t* self() const { return reinterpret_cast<const uint8_t*>(this); }
    }; // struct Packet


    /// @brief Transport layer abstraction
    struct Transport {
        public:
        virtual int read(uint8_t* data, size_t len) = 0;
        virtual size_t write(const uint8_t* data, size_t len) = 0;

        virtual bool available() = 0;
    }; // struct Transport


    /// @brief Manages sending and recieving packets
    struct PacketManager {
        using Handler = void(*)(const Packet&);

        public:
        PacketManager(Transport& transport) : transport(transport) {
            for(size_t i = 0; i < PACKET_COUNT; i++) {
                handlers[i] = nullptr;
            }

            memset(&txPacket, 0, sizeof(Packet));
            memset(&rxPacket, 0, sizeof(Packet));
            txPacket.magic = MAGIC_NUM;

            ResetState();
        }

        /// @brief Checks transport buffer for data and attempts to parse packet
        inline void Update() {
            if (!transport.available()) {

                // message timed out, reset state
                if (reading && (millis()-receivedAt) > READ_TIMEOUT) {
                    ResetState();
                }

                return; 
            }

            while (transport.available()) { TryReadPacket(); }
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
        inline void Send(Type type, const uint8_t* payload, size_t len) {
            txPacket.magic = MAGIC_NUM;
            txPacket.type = (uint8_t)type;
            memset(txPacket.payload, 0, MAX_PAYLOAD_SIZE);

            if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
            if (payload) memcpy(txPacket.payload, payload, len);
            
            txPacket.checksum = ComputeChecksum(txPacket);
            transport.write((uint8_t*)&txPacket, sizeof(Packet));
        }


        /// @brief Checks it a user defined flag was set
        /// @tparam flag Flag [0-3] to check
        /// @return The state of the flag
        template <uint8_t idx> inline static bool HasFlag(const Packet& packet) {
            static_assert(idx < 4, "flag must be [0, 3]");
            return (packet.flags & (1u << idx));
        }


        /// @brief Sets user defined flags
        /// @tparam flag Flag [0-3] to set
        /// @param v Value [0-1] to set flag to
        template <uint8_t flag> inline void SetFlag(bool v) {
            static_assert(flag < 4, "flag must be [0,3]");
            if (v) txPacket.flags |= (1u << flag);
            else txPacket.flags &= ~(1u << flag);
        }

        
        /// @brief Sets the critical bit flag for the txPacket
        /// @param v Value to set critical bit
        inline void SetCritical(bool v) { 
            if (v) txPacket.flags |= 0b10000000;
            else txPacket.flags &= 0b01111111;
        }


        private:
        bool reading;
        size_t bytesRead;
        unsigned long receivedAt;

        Handler handlers[PACKET_COUNT];
        Transport& transport;

        Packet txPacket;
        Packet rxPacket;


        /// @brief Resets internal state
        inline void ResetState() {
            rxPacket.magic = 0;
            reading = false;
            receivedAt = 0;
            bytesRead = 0;
        }


        /// @brief Compute the fletcher16 checksum
        /// @param packet Packet to compute checksum from
        /// @return Computed checksum
        static inline uint16_t ComputeChecksum(const Packet& packet) {
            uint16_t sum1 = 0;
            uint16_t sum2 = 0;

            for (size_t i = 0; i < sizeof(Packet)-sizeof(packet.checksum); i++) {
                sum1 = (sum1 + ((uint8_t*)&packet)[i]) % 255;
                sum2 = (sum2 + sum1) % 255;
            }

            return (sum2 << 8) | sum1;
        }

        /// @brief Updates the rxPacket buffer head to the next magic number
        inline void MoveHeadToNextMagic() {
            // search for magic num in bytes already read
            for (size_t i = 1; i < bytesRead; i++) {
                if (rxPacket.self()[i] == MAGIC_NUM) {
                    memmove(rxPacket.self(), rxPacket.self()+i, bytesRead-i);
                    bytesRead -= i;
                    return;
                }
            }

            // no magic found, reset, keep buffer to keep looking
            ResetState();
        }


        /// @brief Attempts to read packet from transport buffer
        inline void TryReadPacket() {
            // if first byte read, set state
            if (!reading) {
                bytesRead = 0;
                reading = true;
                receivedAt = millis();
            }

            size_t remaining = sizeof(Packet) - bytesRead;
            int recv = transport.read(rxPacket.self()+bytesRead, remaining);
            if (recv <= 0) return;

            // read enough for magic num, verify it, before we continue
            bytesRead += recv;
            if (bytesRead >= sizeof(rxPacket.magic)) {
                if (rxPacket.magic != MAGIC_NUM) {
                    MoveHeadToNextMagic();
                    return;
                }
            }

            // not enough for a full packet, wait for more
            if (bytesRead != sizeof(Packet)) {
                return;
            }

            // verify checksum
            if (rxPacket.checksum != ComputeChecksum(rxPacket)) {
                MoveHeadToNextMagic();
                return;
            }

            // packet has been verified, call user defined handler
            const uint8_t t = rxPacket.type;
            if (t < PACKET_COUNT) {
                if (handlers[rxPacket.type]) handlers[rxPacket.type](rxPacket);
            } else {
                // malformed, type of out range, move to next magic
                MoveHeadToNextMagic();
            }

            // reset state for next packet
            ResetState();
        }


        static inline bool HasCritical(const Packet& packet) { return packet.flags & 0b10000000; }
    }; // struct PacketManager


} // namespace pckt

namespace trns {

    struct SerialTransport : public pckt::Transport {
        public:
        SerialTransport(SoftwareSerial& serial) : serial(serial) {}

        size_t write(const uint8_t* data, size_t len) override { return serial.write(reinterpret_cast<const char*>(data), len); }
        int read(uint8_t* data, size_t len) override { return serial.readBytes(reinterpret_cast<char*>(data), len); }
        bool available() override { return serial.available(); }

        private:
        SoftwareSerial& serial;
    }; // struct SerialTransport

} // namespace trns
