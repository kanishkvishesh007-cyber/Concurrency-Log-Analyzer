#include <iostream>
#include <PcapFileDevice.h>
#include <Packet.h>
#include <RawPacket.h>
#include <thread>
#include <atomic>
#include <UdpLayer.h>
#include <cstring>
#include <unordered_set>
#include <memory>
#include <functional>

struct User_Demand {
    size_t type_A;
    size_t type_E;
    size_t type_D;
    uint32_t cost;
    uint32_t executed_funds;
};

struct Msg {
    uint8_t  type;
    uint16_t instr_id;
    // tracking number
    uint64_t timestamp;
    uint64_t order_ref;
    uint8_t  side;
    uint32_t shares;
    // stocks
    uint32_t price;
};

uint16_t ret16(const uint8_t* data) {
    uint16_t ret;
    memcpy(&ret, data, 2);
    return __builtin_bswap16(ret);
}
uint32_t ret32(const uint8_t* data) {
    uint32_t ret;
    memcpy(&ret, data, 4);
    return __builtin_bswap32(ret);
}
uint64_t ret48(const uint8_t* data) {
    return (uint64_t)data[0] << 40 | (uint64_t)data[1] << 32 | (uint64_t)data[2] << 24 | (uint64_t)data[3] << 16 | (uint64_t)data[4] << 8 | data[5];
}
uint64_t ret64(const uint8_t* data) {
    uint64_t ret;
    memcpy(&ret, data, 8);
    return __builtin_bswap64(ret);
}

struct SPSCQueue {
    Msg ring_buffer[8];
    alignas(64) std::atomic<size_t> head{0};
    alignas(64) std::atomic<size_t> tail{0};
    std::atomic<bool> block{false};

    bool push(Msg& message) {
        size_t t = tail.load(std::memory_order_relaxed);
        if (t-head.load(std::memory_order_acquire) == 8 ) return false;
        ring_buffer[t%8] = message;
        tail.store(t+1, std::memory_order_release);
        return true;
    }
    bool pop(Msg& Consumer) {
        size_t h = head.load(std::memory_order_relaxed);
        if (h-tail.load(std::memory_order_acquire) == 0) return false;
        Consumer = ring_buffer[h%8];
        head.store(h+1, std::memory_order_release);
        return true;
    }
};

void Producers(SPSCQueue& ring) {
    std::unique_ptr<pcpp::IFileReaderDevice> Producer(pcpp::IFileReaderDevice::   tryCreateReader("D:/Projects/ConcurrencyLogAnalyzer/NasdaqTexasTotalView-ITCH5.0.pcap"));

    if (Producer == nullptr) {
        std::cerr << "Unable to create producer device." << std::endl;
        ring.block.store(true, std::memory_order_release);
        return;
    }
    if (!Producer->open()) {
        std::cerr << "Unable to open producer device." << std::endl;
        ring.block.store(true, std::memory_order_release);
        return;
    }

    pcpp::RawPacket raw_packet;
    while (Producer->getNextPacket(raw_packet)) {
        pcpp::Packet parsed_Packet(&raw_packet);
        auto* UDPLayer = parsed_Packet.getLayerOfType<pcpp::UdpLayer>();

        if (UDPLayer != nullptr) {
            const uint8_t* data = UDPLayer->getLayerPayload();
            size_t len = UDPLayer->getLayerPayloadSize();

            if (len < 23) continue;
            Msg message{};
            const uint8_t* thr = data + 22;
            switch (data[22]) {
                case 'A':
                    if (len < 22+36) continue;
                    message.type = data[22];
                    message.instr_id = ret16(thr+1);
                    message.timestamp = ret48(thr+5);
                    message.order_ref = ret64(thr+11);
                    message.side = data[41];
                    message.shares = ret32(thr+20);
                    message.price = ret32(thr+32);
                    break;
                case 'E':
                    if (len < 22+31) continue;
                    message.type = data[22];
                    message.instr_id = ret16(thr+1);
                    message.timestamp = ret48(thr+5);
                    message.order_ref = ret64(thr+11);
                    message.shares = ret32(thr+19);
                    break;
                case 'D':
                    if (len < 22+19) continue;
                    message.type = data[22];
                    message.instr_id = ret16(thr+1);
                    message.timestamp = ret48(thr+5);
                    message.order_ref = ret64(thr+11);
                    break;
                default:
                    continue;
            }
            while (!ring.push(message)) {};
        }
    }
    ring.block.store(true, std::memory_order_release);
    Producer->close();
}

void Consumers(SPSCQueue& ring, User_Demand& user) {
    Msg Consumer;
    user.type_A = 0;
    user.type_E = 0;
    user.type_D = 0;
    user.cost = 0;
    user.executed_funds = 0;
    std::unordered_set<uint64_t> BlackList;
    bool done = false;
    while (!done) {
        done = ring.block.load(std::memory_order_acquire);
        while (ring.pop(Consumer)) {
            switch (Consumer.type) {
                case 'A':
                    user.type_A++;
                    user.cost+=Consumer.price;
                    break;
                case 'E':
                    user.type_E++;
                    if (BlackList.count(Consumer.order_ref) != 0) continue;
                    user.executed_funds+=Consumer.price;
                    break;
                case 'D':
                    user.type_D++;
                    BlackList.insert(Consumer.order_ref);
                    break;
                default:
                    continue;
            }
        };
    }
}

int main() {
    SPSCQueue ring;
    User_Demand user;
    std::thread producer_thread(Producers, std::ref(ring));
    std::thread consumer_thread(Consumers, std::ref(ring), std::ref(user));
    producer_thread.join();
    consumer_thread.join();
    std::cout << "Number of A: " <<  user.type_A << std::endl;
    std::cout << "Number of E: " <<  user.type_E << std::endl;
    std::cout << "Number of D: " <<  user.type_D << std::endl;
    return 0;
}