#include "TestPacketManager.hpp"
#include <queue>
#include <iostream>
#include <random>

struct TestTransportLayer : public pckt::Transport {
    public:
    TestTransportLayer() {}

    int read(uint8_t* data, size_t len) {
        int r = 0;

        for (size_t i = 0; i < len && available(); i++) {
            data[i] = buffer[0];
            buffer.erase(buffer.begin());
            r++;
        }

        return r;
    }

    size_t write(const uint8_t* data, size_t len) {
        size_t r = 0;

        for (size_t i = 0; i < len; i++) {
            buffer.push_back(data[i]);
            r++;
        }

        return r;
    }

    bool available() {
        return !buffer.empty();
    }

    std::vector<uint8_t> buffer;
};

struct TestSuite {
    public:
    static size_t elapsed;
    static size_t received;
    static size_t failed;
    static uint8_t payload[MAX_PAYLOAD_SIZE];

    static void Handler(const pckt::Packet& packet) {
        if (
            packet.payload[0] == 0xCC &&
            packet.payload[1] == 0xCC &&
            packet.payload[2] == 0xCC &&
            packet.payload[3] == 0xFF &&
            packet.payload[4] == 0xFF &&
            packet.payload[5] == 0xFF &&
            packet.payload[6] == 0xAA &&
            packet.payload[7] == 0xAA
        ) {
            TestSuite::received++;
        } else {
            TestSuite::failed++;
        }
    }
    
    static void T1TestManager(size_t packetsToSend) {
        TestSuite::elapsed = 0;
        TestSuite::received = 0;
        TestSuite::failed = 0;
        TestTransportLayer transport;
        pckt::PacketManager txManager(transport);
        pckt::PacketManager rxManager(transport);

        std::cout << "Running T1 (" << packetsToSend << " packets):\n";

        rxManager.Callback(pckt::Type::DataPacket, Handler);
        

        for (size_t i = 0; i < packetsToSend; i++) {
            txManager.Send(pckt::Type::DataPacket, TestSuite::payload, MAX_PAYLOAD_SIZE);
            rxManager.Update();
            TestSuite::elapsed++;
        }

        // cleanup for any remaining data
        while (transport.available()) {
            rxManager.Update();
            TestSuite::elapsed++;
        }

        double failedPercent = 100.0 * (double)TestSuite::failed / (double)packetsToSend;
        double recvPercent =   100.0 * (double)TestSuite::received / (double)packetsToSend;

        std::cout << "\t" << TestSuite::elapsed << " updates elapsed\n";
        std::cout << "\tFailed: " << TestSuite::failed << "/" << packetsToSend << " packets (" << failedPercent << "%)\n";
        std::cout << "\tRecieved " << TestSuite::received << "/" << packetsToSend << " packets (" << recvPercent << "%)\n\n";
    }
    static void T2TestManager(size_t packetsToSend, size_t interference) {
        TestSuite::elapsed = 0;
        TestSuite::received = 0;
        TestSuite::failed = 0;
        TestTransportLayer transport;
        pckt::PacketManager txManager(transport);
        pckt::PacketManager rxManager(transport);

        std::cout << "Running T2 (" << packetsToSend << " packets, " << interference << " bytes interference):\n";

        rxManager.Callback(pckt::Type::DataPacket, Handler);
        

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0x00, 0xFF);

        for (size_t i = 0; i < packetsToSend; i++) {

            // apply pre magic number interference
            for (size_t p = 0; p < interference; p++) {
                transport.buffer.push_back(dist(gen));
            }

            txManager.Send(pckt::Type::DataPacket, TestSuite::payload, MAX_PAYLOAD_SIZE);
            rxManager.Update();
            TestSuite::elapsed++;
        }

        // cleanup for any remaining data
        while (transport.available()) {
            rxManager.Update();
            TestSuite::elapsed++;
        }

        double failedPercent = 100.0 * (long double)TestSuite::failed /   (long double)packetsToSend;
        double recvPercent =   100.0 * (long double)TestSuite::received / (long double)packetsToSend;

        std::cout << "\t" << TestSuite::elapsed << " updates elapsed\n";
        std::cout << "\tFailed: " << TestSuite::failed << "/" << packetsToSend << " packets (" << failedPercent << "%)\n";
        std::cout << "\tRecieved " << TestSuite::received << "/" << packetsToSend << " packets (" << recvPercent << "%)\n\n";
    }
    static void T3TestManager(size_t packetsToSend) {
        TestSuite::elapsed = 0;
        TestSuite::received = 0;
        TestSuite::failed = 0;
        TestTransportLayer transport;
        pckt::PacketManager txManager(transport);
        pckt::PacketManager rxManager(transport);

        std::cout << "Running T3 (" << packetsToSend << " packets, " << packetsToSend/2 << " malformed):\n";

        rxManager.Callback(pckt::Type::DataPacket, Handler);

        for (size_t i = 0; i < packetsToSend; i++) {
            txManager.Send(pckt::Type::DataPacket, TestSuite::payload, MAX_PAYLOAD_SIZE);
            
            // malform every other packet
            if (i%2 == 0) transport.buffer.back() = 0xff;

            rxManager.Update();
            TestSuite::elapsed++;
        }

        // cleanup for any remaining data
        while (transport.available()) {
            rxManager.Update();
            TestSuite::elapsed++;
        }

        double failedPercent = 100.0 * (long double)TestSuite::failed /   (long double)packetsToSend;
        double recvPercent = 100.0 *   (long double)TestSuite::received / (long double)((packetsToSend+1)/2);

        std::cout << "\t" << TestSuite::elapsed << " updates elapsed\n";
        std::cout << "\tFailed: " << TestSuite::failed << "/" << packetsToSend/2 << " packets (" << failedPercent << "%)\n";
        std::cout << "\tRecieved " << TestSuite::received << "/" << (packetsToSend+1)/2 << " packets (" << recvPercent << "%)\n\n";
    }
    static void T4TestManager(size_t packetsToSend) {
        TestSuite::elapsed = 0;
        TestSuite::received = 0;
        TestSuite::failed = 0;
        TestTransportLayer transport;
        pckt::PacketManager txManager(transport);
        pckt::PacketManager rxManager(transport);

        std::cout << "Running T4 (" << packetsToSend << " packets):\n";

        rxManager.Callback(pckt::Type::DataPacket, Handler);

        // send first half of packet
        txManager.Send(pckt::Type::DataPacket, TestSuite::payload, MAX_PAYLOAD_SIZE);
        transport.buffer.erase(transport.buffer.begin()+transport.buffer.size()/2, transport.buffer.end());

        for (size_t i = 0; i < packetsToSend; i++) {
            txManager.Send(pckt::Type::DataPacket, payload, MAX_PAYLOAD_SIZE);
            rxManager.Update();
            TestSuite::elapsed++;
        }

        // cleanup for any remaining data
        while (transport.available()) {
            rxManager.Update();
            TestSuite::elapsed++;
        }

        double failedPercent = 100.0 * (long double)TestSuite::failed /   (long double)packetsToSend;
        double recvPercent =   100.0 * (long double)TestSuite::received / (long double)packetsToSend;

        std::cout << "\t" << TestSuite::elapsed << " updates elapsed\n";
        std::cout << "\tFailed: " << TestSuite::failed << "/" << packetsToSend << " packets (" << failedPercent << "%)\n";
        std::cout << "\tRecieved " << TestSuite::received << "/" << packetsToSend << " packets (" << recvPercent << "%)\n\n";
    }
    static void T5TestManager(size_t packetsToSend) {
        TestSuite::elapsed = 0;
        TestSuite::received = 0;
        TestSuite::failed = 0;
        TestTransportLayer transport;
        pckt::PacketManager txManager(transport);
        pckt::PacketManager rxManager(transport);

        std::cout << "Running T5 (" << packetsToSend << " packets):\n";

        rxManager.Callback(pckt::Type::DataPacket, Handler);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dist(0x00, 0xFF);

        for (size_t i = 0; i < packetsToSend; i++) {
            txManager.Send(pckt::Type::DataPacket, TestSuite::payload, MAX_PAYLOAD_SIZE);

            // malformed packet, correct headers
            for (size_t r = transport.buffer.size()-10; r < transport.buffer.size(); r++) {
                transport.buffer[r] = dist(gen);
            }

            rxManager.Update();
            TestSuite::elapsed++;
        }

        // cleanup for any remaining data
        while (transport.available()) {
            rxManager.Update();
            TestSuite::elapsed++;
        }

        double failedPercent = 100.0 * (double)TestSuite::failed / (double)packetsToSend;

        std::cout << "\t" << TestSuite::elapsed << " updates elapsed\n";
        std::cout << "\tFailed: " << TestSuite::failed << "/" << packetsToSend << " packets (" << failedPercent << "%)\n";
        std::cout << "\tRecieved " << TestSuite::received << "/0" << " packets\n\n";
    }
};

size_t TestSuite::received = 0;
size_t TestSuite::elapsed = 0;
size_t TestSuite::failed = 0;
uint8_t TestSuite::payload[MAX_PAYLOAD_SIZE] = { 0xCC, 0xCC, 0xCC, 0xFF, 0xFF, 0xFF, 0xAA, 0xAA };

int main() {
    TestSuite::T1TestManager(5000000);
    TestSuite::T2TestManager(5000000, 2*sizeof(pckt::Packet));
    TestSuite::T3TestManager(5000000);
    TestSuite::T4TestManager(5000000);
    TestSuite::T5TestManager(5000000);
}
