#include <iostream>
#include <PcapFileDevice.h>
#include <Packet.h>
#include <PayloadLayer.h>
#include <RawPacket.h>

int main() {
    std::unique_ptr<pcpp::IFileReaderDevice> reader(pcpp::IFileReaderDevice::tryCreateReader("dude.pcap"));

    if (reader == nullptr) {
        std::cerr << "dude.pcap doesn't exist!" << std::endl;
        return 1;
    }
    if (!reader->open()) {
        std::cerr << "Unable to open dude.pcap" << std::endl;
        return 1;
    }
    pcpp::RawPacket raw_packet;

    while (reader->getNextPacket(raw_packet)) {
        pcpp::Packet parsed_Packet(&raw_packet);
        auto* payloadLayer = parsed_Packet.getLayerOfType<pcpp::PayloadLayer>();

        if (payloadLayer != nullptr) {
            const uint8_t* packet_data = payloadLayer->getPayload();
            size_t packet_size = payloadLayer->getPayloadLen();
            std::string message(reinterpret_cast<const char*>(packet_data), packet_size);
            std::cout << message << std::endl;
        }
    }

    reader->close();
    return 0;
}