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

// const int PACKET_SIZE = 1024;
// const int WINDOW_SIZE = 20;
int actual_start = 0;
int actual_size = 256;
// int actual_size = 12;
// struct PacketInfo {
//     size_t start;  
//     size_t length; 
//     bool received;
// };
// struct PacketWindow{

// }
// std::map<size_t, PacketInfo> window_packets;


void ERROR(const char* str){
    fprintf(stderr, "%s: %s\n", str, strerror(errno)); 
    exit(EXIT_FAILURE);
}


int validate_buffer(const uint8_t* buffer, ssize_t packet_len){
    if (packet_len < 5 || std::strncmp(reinterpret_cast<const char*>(buffer), "DATA ", 5) != 0) {
        return -1;
    }
    const char* ptr = reinterpret_cast<const char*>(buffer) + 5;
    const char* end = reinterpret_cast<const char*>(buffer) + packet_len;
    
    int val1 = 0;
    const char* num1_start = ptr;
    while (ptr < end && *ptr != ' ') {
        if (!std::isdigit(*ptr)) {
            return -1;
        }
        val1 = val1 * 10 + (*ptr - '0');
        ptr++;
    }
    if (ptr == num1_start || ptr >= end || *ptr != ' ') {
        return -1;
    }
    if(val1 != actual_start){
        return -1;
    }

    ptr++;
    int val2 = 0;
    while (ptr < end && *ptr != '\n') {
        if (!std::isdigit(*ptr)) {
            return -1;
        }
        val2 = val2 * 10 + (*ptr - '0');
        ptr++;
    }
    if (*ptr != '\n') {
        return -1;
    }
    if(val2 != actual_size){
        return -1;
    }

    return ptr - reinterpret_cast<const char*>(buffer) + 1;
}
bool validate_udp_packet(const uint8_t* buffer, ssize_t packet_len, sockaddr_in& sender,
                         const char* expected_ip, uint16_t expected_port, std::string& file_data) 
{
    struct in_addr expected_addr;
    inet_pton(AF_INET, expected_ip, &expected_addr); 
    uint16_t source_port = ntohs(sender.sin_port);

    if (sender.sin_addr.s_addr != expected_addr.s_addr or expected_port != source_port) {
        return false;
    }

    int start_len = validate_buffer(buffer, packet_len);
    if(start_len == -1){
        return false;
    }
    std::vector<uint8_t> content(buffer + start_len, buffer + packet_len);
    file_data = std::string(content.begin(), content.end());
    return true;
}

void appendBinary(FILE* file, const char* data, size_t length) {
    if (file) {
        fwrite(data, sizeof(char), length, file);
    } else {
        ERROR("There is no file!");
    }
}

bool move_block(int max_size){
    // We create all blocks
    if(actual_start + actual_size == max_size){
        return true;
    }
    actual_start += actual_size;
    if(actual_start + actual_size > max_size){
        actual_size = max_size - actual_start;
    }
    return false;
}

void udp_send(int sock_fd, const char* ip_addr, int port, int start, int size){
    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip_addr, &server_address.sin_addr);

    std::string msg = "GET " + std::to_string(start) + " " + std::to_string(size) + "\n";
    const char* message = msg.c_str();
    size_t message_len = msg.size();

    // std::cout << "WYSŁANO: " << message;
    ssize_t sent_len = sendto(sock_fd, message, message_len, 0, 
                             (struct sockaddr*)&server_address, sizeof(server_address));
    
    if (sent_len != (ssize_t)message_len)
        ERROR("sendto error");

}

bool udp_receive(FILE* file, int sock_fd, const char* ip_addr, int port, int size_in_bytes){
    uint8_t buffer[65536];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    ssize_t packet_len = recvfrom(sock_fd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&sender, &sender_len);
    if (packet_len < 0)
        ERROR("recvfrom error");
    std::string file_data;
    if (validate_udp_packet(buffer, packet_len, sender, ip_addr, port, file_data)) {
        // std::cout << file_data << "\n";
        // Tu wykonać zapis i zwrocic true jesli wszystkie fragmenty zostały zwrocone
        appendBinary(file, file_data.c_str(), file_data.length());
        if(move_block(size_in_bytes)){
            return true;
        }
        std::cout << "[Received package]: actual_start = " << actual_start << ", actual_size = " <<  actual_size << "\n";
    }
    return false;
}

void main_program(FILE* file, int sock_fd, const char* ip_addr, int port, int size_in_bytes, int timeout_ms = 20){
    struct pollfd fds[1];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    auto last_request = std::chrono::steady_clock::now();

    while (true) {
        int ret = poll(fds, 1, 5);
        if (ret < 0)
            ERROR("poll error");
        if (ret > 0) {
            if (fds[0].revents & POLLIN) {
                if(udp_receive(file, sock_fd, ip_addr, port, size_in_bytes)){
                    break;
                }
            }
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_request).count() >= timeout_ms) {
            udp_send(sock_fd, ip_addr, port, actual_start, actual_size);
            last_request = now;
        }
    }
}


int main(int argc, char* argv[]){
    if(argc < 5 || argc > 5)
        ERROR("Wrong arguments! You should use command './transport `ip` `port` `file_name` `size_in_bytes`'.\n");
    
    // Trzeba dodać, że size_in_bytes maksymalnie 10MB> 
    const char* ip_addr = argv[1];
    size_t port = 0;
    size_t size_in_bytes = 0;
    try {
        port = std::stoi(argv[2]);
    } catch (const std::exception& e) {
        ERROR("Port number error");
    }
    try {
        size_in_bytes = std::stoi(argv[4]);
    } catch (const std::exception& e) {
        ERROR("Size in bytes error");
    }

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
        ERROR("Socket error");
    FILE* file = fopen(argv[3], "wb");
    actual_size = std::min(actual_size, int(size_in_bytes));

    main_program(file, sock_fd, ip_addr, port, size_in_bytes);

    close(sock_fd);
    fclose(file);
}