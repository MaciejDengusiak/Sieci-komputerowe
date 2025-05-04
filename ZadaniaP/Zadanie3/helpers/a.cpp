#include <netinet/ip.h>   
#include <netinet/udp.h>  
#include <arpa/inet.h>     
#include <sys/socket.h>  
#include <poll.h>          
#include <unistd.h>        
#include <cstring>         
#include <iostream>      
#include <string>         
#include <chrono>  
#include <map>        

void ERROR(const char* str){
    fprintf(stderr, "%s: %s\n", str, strerror(errno)); 
    exit(EXIT_FAILURE);
}

// This method is not valid - should be changed
bool validate_udp_packet(const uint8_t* buffer, ssize_t packet_len,
                         const std::string& expected_ip, uint16_t expected_port,
                         int& start, int& length, std::string& file_data) 
{
    struct ip* ip_header = (struct ip*)buffer;
    ssize_t ip_header_len = ip_header->ip_hl * 4;

    if (ip_header->ip_p != IPPROTO_UDP){
        std::cout << ip_header;
        std::cout << "- PAKIET TO NIE UDP\n";
        return false; // Not UDP
    }

    const uint8_t* udp_start = buffer + ip_header_len;

    if (packet_len < ip_header_len + 8){
        std::cout << "- Pakiet UDP ma krotszy od 8 bajow content\n";
        return false;
    }

    uint16_t source_port = ntohs(*(uint16_t*)(udp_start + 0));
    uint16_t dest_port   = ntohs(*(uint16_t*)(udp_start + 2));

    // Extract sender IP
    char sender_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip_header->ip_src), sender_ip_str, sizeof(sender_ip_str));

    // Check if sender matches expected
    if (expected_ip != sender_ip_str || expected_port != source_port){
        std::cout << "- Pakiet UDP ma błędne ip lub port\n";
        return false;
    }

    // Extract UDP payload
    size_t udp_payload_offset = ip_header_len + 8; // 8 bajtów nagłówka UDP
    size_t udp_payload_len = packet_len - udp_payload_offset;

    if (udp_payload_len <= 0){
        std::cout << "- Pakiet UDP ma błędne udp_payload_len\n";
        return false;
    }

    const char* udp_payload = (const char*)(buffer + udp_payload_offset);
    std::string message(udp_payload, udp_payload_len);

    // Message must start with "DATA start length\n"
    if (message.compare(0, 5, "DATA ") != 0){
        std::cout << "- Pakiet UDP ma wiadomość ktora nie zaczyna się od DATA\n";
        return false;
    }

    size_t first_space = message.find(' ', 5);
    size_t second_space = message.find(' ', first_space + 1);
    size_t newline_pos = message.find('\n', second_space + 1);

    if (first_space == std::string::npos || second_space == std::string::npos || newline_pos == std::string::npos)
        return false; // Malformed

    try {
        start = std::stoi(message.substr(5, first_space - 5));
        length = std::stoi(message.substr(first_space + 1, second_space - first_space - 1));
    } catch (...) {
        return false;
    }

    file_data = message.substr(newline_pos + 1);

    return true;
}


int udp_send(int sock_fd, const char* ip_addr, int port, int start, int size){
    struct sockaddr_in server_address = { 0 };
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip_addr, &server_address.sin_addr);

    std::string msg = "GET " + std::to_string(start) + " " + std::to_string(size) + "\n";
    const char* message = msg.c_str();
    size_t message_len = msg.size();

    std::cout << "WYSŁANO: " << message;
    ssize_t sent_len = sendto(sock_fd, message, message_len, 0, 
                             (struct sockaddr*)&server_address, sizeof(server_address));
    
    if (sent_len != (ssize_t)message_len)
        ERROR("sendto error");

    return sock_fd;
}

void udp_receive(int sock_fd, const char* ip_addr, int port, int timeout_ms = 3000) {
    struct pollfd fds[1];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    auto start_time = std::chrono::steady_clock::now();
    const std::chrono::milliseconds total_time(timeout_ms);
    
    while (true) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed > total_time){
            std::cout << "PRZEKROCZONO CZAS\n";
            break;
        }

        int remaining_time = std::chrono::duration_cast<std::chrono::milliseconds>(total_time - elapsed).count();
        int ret = poll(fds, 1, remaining_time);

        if (ret < 0)
            ERROR("poll error");
        else if (ret == 0){
            std::cout << "PRZEKROCZONO CZAS 2\n";
            break;
        }
        else {
            if (fds[0].revents & POLLIN) {
                std::cout << "RECEIVED MESSAGE!!!";
                uint8_t buffer[65536];
                struct sockaddr_in sender;
                socklen_t sender_len = sizeof(sender);
                ssize_t packet_len = recvfrom(sock_fd, buffer, sizeof(buffer), 0,
                                              (struct sockaddr*)&sender, &sender_len);
                if (packet_len < 0)
                    ERROR("recvfrom error");

                int start = 0;
                int length = 0;
                std::string file_data;
                if (validate_udp_packet(buffer, packet_len, ip_addr, port, start, length, file_data)) {
                    std::cout << "Received valid DATA packet:\n";
                    std::cout << "Start: " << start << ", Length: " << length << "\n";         
                    std::cout << file_data << "\n";
                    break;
                }
            }
        }
    }
}