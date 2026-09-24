//
// Created by Kanishk on 18-09-2026.
//

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <fstream>
#include <string>
#include <regex>
#include <mutex>

static std::atomic<int> ERROR = 0;
static std::atomic<int> WARNING= 0;
static std::atomic<int> INFO = 0;

static std::regex pattern_error(R"(ERROR)");
static std::regex pattern_warning(R"(WARNING)");
static std::regex pattern_info(R"(INFO)");

static void file_reading(const std::string& fill, std::streamsize start, std::streamsize end) {
    std::ifstream file(fill, std::ios::binary);
    file.seekg(start);
    std::string line;

    if (start != 0) {
        std::string dummy;
        std::getline(file, dummy);
    }

    while (file.tellg() >= 0 && file.tellg() < end && std::getline(file,line)) {
        if (std::regex_search(line, pattern_error)) ++ERROR;
        else if (std::regex_search(line, pattern_warning)) ++WARNING;
        else if (std::regex_search(line, pattern_info)) ++INFO;
    };
}

int main() {
    std::string fill = "data.txt";
    std::ifstream file(fill, std::ios::binary);
    file.seekg(0,std::ios::end);
    std::streamsize size = file.tellg();
    file.close();
    int no_of_chunks = 32;

    std::vector<std::thread> threads;
    std::streamsize chunk_size = size / no_of_chunks;

    for (int i = 0; i < no_of_chunks; i++) {
        std::streamsize start = chunk_size * i;
        std::streamsize end = start + chunk_size;
        threads.emplace_back(&file_reading, fill, start, end);
    }
    for (int i = 0; i < no_of_chunks; i++) {
        threads[i].join();
    }

    std::cout << "ERROR: " << ERROR << std::endl;
    std::cout << "WARNING: " << WARNING << std::endl;
    std::cout << "INFO: " << INFO << std::endl;
}