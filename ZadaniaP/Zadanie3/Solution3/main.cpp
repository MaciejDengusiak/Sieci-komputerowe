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
#include <iomanip>


const int DEFAULT_PACKET_SIZE = 1000;
const int WINDOW_SIZE = 1200;


int ACTUAL_PACKETS_SAVED = 0;
int PACKETS_NUM = 0;

struct PacketInfo {
    size_t start;  
    size_t length; 
    bool received;
    char* data;
};
struct PacketWindow{
    size_t window_offset;
    PacketInfo packets[WINDOW_SIZE];
};

PacketWindow packet_window;

void ERROR(const char* str) {
    fprintf(stderr, "%s: %s\n", str, strerror(errno)); 
    exit(EXIT_FAILURE);
}

bool try_accept_packet(size_t packet_start, size_t packet_size, const char* packet_data) {
    for (int i = 0; i < WINDOW_SIZE; i++) {
        int ind = (packet_window.window_offset + i) % WINDOW_SIZE;
        PacketInfo& packetInfo = packet_window.packets[ind]; 

        if (packetInfo.start == packet_start && packetInfo.length == packet_size && packetInfo.received == false) {
            packetInfo.received = true;
            packetInfo.data = new char[packet_size];
            std::memcpy(packetInfo.data, packet_data, packet_size);
            return true;
        }
    }
    return false;
}

int validate_buffer_try_save(const uint8_t* buffer, ssize_t packet_len){
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

    int start_len = ptr - reinterpret_cast<const char*>(buffer) + 1;
    std::vector<uint8_t> content(buffer + start_len, buffer + packet_len);
    std::string file_data = std::string(content.begin(), content.end());

    if(!try_accept_packet(val1, val2, file_data.c_str())){
        return -1;
    }

    return true;
}
bool validate_udp_packet_try_save(const uint8_t* buffer, ssize_t packet_len, sockaddr_in& sender,
                         const char* expected_ip, uint16_t expected_port) 
{
    struct in_addr expected_addr;
    inet_pton(AF_INET, expected_ip, &expected_addr); 
    uint16_t source_port = ntohs(sender.sin_port);

    if (sender.sin_addr.s_addr != expected_addr.s_addr or expected_port != source_port) {
        return false;
    }

    return validate_buffer_try_save(buffer, packet_len);
}
void append_binary(FILE* file, const char* data, size_t length) {
    if (file) {
        // std::cout << "[SAVING - FILE] " << data << " \n";
        fwrite(data, sizeof(char), length, file);
    } else {
        ERROR("There is no file!");
    }
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

void udp_send_all(int sock_fd, const char* ip_addr, int port, size_t size_in_bytes){
    for(int i=0; i<WINDOW_SIZE; i++){
        size_t start = packet_window.packets[i].start;
        size_t length = packet_window.packets[i].length;
        if(start + length <= size_in_bytes && packet_window.packets[i].received == false){
            udp_send(sock_fd, ip_addr, port, start, length);
        }
    }
}

bool save_blocks_and_move_window(FILE* file, size_t size_in_bytes){
    int received_pref_len = 0;
    for(int i=0; i<WINDOW_SIZE; i++){
        int ind = (packet_window.window_offset + i) % WINDOW_SIZE;
        PacketInfo& packetInfo = packet_window.packets[ind];
        if(packetInfo.received == false){
            break;
        }

        append_binary(file, packetInfo.data, packetInfo.length);
        if(packetInfo.start + packetInfo.length == size_in_bytes){
            return true;
        }

        size_t new_start = packetInfo.start + packetInfo.length * WINDOW_SIZE;
        size_t new_length = packetInfo.length;

        if(new_start + new_length > size_in_bytes){
            new_length = size_in_bytes - new_start;
        }
        if(new_start > size_in_bytes){
            new_length = 0;
        }

        delete[] packet_window.packets[ind].data;
        packet_window.packets[ind] = PacketInfo{
            .start = new_start,
            .length = new_length,
            .received = false,
            .data = nullptr
        };
        received_pref_len += 1;

        ACTUAL_PACKETS_SAVED += 1;
        if(true){
            double percent = double(ACTUAL_PACKETS_SAVED) / double(PACKETS_NUM) * 100.0;
            std::cout << std::fixed << std::setprecision(2) << percent << "% done\n";
        }
    }

    packet_window.window_offset += received_pref_len;
    return false;
}

void initialize_window(size_t size_in_bytes){
    packet_window.window_offset = 0;

    for (size_t i = 0; i < WINDOW_SIZE; ++i) {
        size_t start = i * DEFAULT_PACKET_SIZE;
        size_t length = DEFAULT_PACKET_SIZE;
        if(start + length > size_in_bytes){
            length = size_in_bytes - start;
        }
        if(start > size_in_bytes){
            length = 0;
        }
        packet_window.packets[i] = PacketInfo{
            .start = start,
            .length = length,
            .received = false,
            .data = nullptr
        };
    }

    PACKETS_NUM = size_in_bytes / DEFAULT_PACKET_SIZE + 1;
}


bool udp_receive(FILE* file, int sock_fd, const char* ip_addr, int port, int size_in_bytes){
    uint8_t buffer[65536];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    ssize_t packet_len = recvfrom(sock_fd, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&sender, &sender_len);
    if (packet_len < 0)
        ERROR("recvfrom error");
    if (validate_udp_packet_try_save(buffer, packet_len, sender, ip_addr, port)) {
        return save_blocks_and_move_window(file, size_in_bytes);
    }
    return false;
}

void main_program(FILE* file, int sock_fd, const char* ip_addr, int port, int size_in_bytes, int timeout_ms = 50){
    struct pollfd fds[1];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    auto last_request = std::chrono::steady_clock::now();

    while (true) {  
        int ret = poll(fds, 1, 20);
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
            udp_send_all(sock_fd, ip_addr, port, size_in_bytes);
            last_request = now;
        }
    }
}

int main(int argc, char* argv[]){
    if(argc < 5 || argc > 5)
        ERROR("Wrong arguments! You should use command './transport `ip` `port` `file_name` `size_in_bytes`'.\n");
    

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

    if(size_in_bytes > 10000000){
        ERROR("Size max bytes size is 10 000 000");
    }

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
        ERROR("Socket error");
    FILE* file = fopen(argv[3], "wb");

    initialize_window(size_in_bytes);
    main_program(file, sock_fd, ip_addr, port, size_in_bytes);

    close(sock_fd);
    fclose(file);
}