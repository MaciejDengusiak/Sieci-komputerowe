// Zadanie programistyczne 1 - traceroute
// Maciej Dengusiak
// 338811

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <netinet/ip_icmp.h>
#include <cstring> 
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <set>
#include <arpa/inet.h>

#define POLL_TIMEOUT 1000 
char* _ip;
bool validate_ipv4(const std::string& ip) {
    struct in_addr addr;
    return inet_pton(AF_INET, ip.c_str(), &addr) == 1;
}
void ERROR(const char* str)
{
    fprintf(stderr, "%s: %s\n", str, strerror(errno));
    exit(EXIT_FAILURE);
}
void print_as_bytes (unsigned char* buff, ssize_t length)
{
    for (ssize_t i = 0; i < length; i++, buff++)
        printf("%.2x ", *buff);
}
u_int16_t compute_icmp_checksum(const void *buff, int length)
{
    const u_int16_t* ptr = static_cast<const uint16_t*>(buff);
    u_int32_t sum = 0;
    assert (length % 2 == 0);
    for (; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16U) + (sum & 0xffffU);
    return static_cast<u_int16_t>(~(sum + (sum >> 16U)));
}
void icmp_send(int ttl, char* ip){
    struct icmp header; 
    header.icmp_type = ICMP_ECHO;
    header.icmp_code = 0;
    header.icmp_hun.ih_idseq.icd_id = htons(getpid());
    header.icmp_hun.ih_idseq.icd_seq = htons(ttl);
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum (
    (u_int16_t*)&header, sizeof(header));

    struct sockaddr_in recipient;
    memset (&recipient, 0, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton (AF_INET, ip, &recipient.sin_addr);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    setsockopt (sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    sendto (
    sockfd,
    &header,
    sizeof(header),
    0,
    (struct sockaddr*)&recipient,
    sizeof(recipient)
    );
}

void icmp_receive(int actual_ttl, int cnt, char* ip) {
    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0)
        ERROR("socket error");

    pollfd fds[1];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;

    const std::chrono::milliseconds total_time{1000};
    auto start_time = std::chrono::steady_clock::now();

    std::vector<int> responses_time;
    std::set<std::string> responses_ip;
    for(;;){
        auto elapsed_time = std::chrono::steady_clock::now() - start_time;
        auto remaining_time = total_time - elapsed_time;
        if (remaining_time <= std::chrono::milliseconds{0}) {
            break;
        }

        int timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(remaining_time).count();
        int ret = poll(fds, 1, timeout_ms);
        if (ret == -1) {
            ERROR("poll error");
        } else if (ret == 0) {
            std::cout << "*" << std::endl;
            return;
        } else {
            if (fds[0].revents & POLLIN) {
                struct sockaddr_in sender;
                socklen_t sender_len = sizeof(sender);
                u_int8_t buffer[IP_MAXPACKET];

                ssize_t packet_len = recvfrom(sock_fd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&sender, &sender_len);
                if (packet_len < 0)
                    ERROR("recvfrom error");

                char sender_ip_str[20];
                inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));

                // making sure that this is my packet
                struct ip* ip_header = (struct ip*) buffer;
                ssize_t	ip_header_len = 4 * (ssize_t)(ip_header->ip_hl);
                struct icmp* icmp_header = (struct icmp*)(buffer + ip_header_len);

                bool is_my_packet = false;
                if(icmp_header->icmp_type == 11){ // ICMP time exceeded
                    icmp_header = (struct icmp*)(buffer + ip_header_len + 28);
                }
                uint16_t icd_id = icmp_header->icmp_hun.ih_idseq.icd_id;
                uint16_t icd_seq = icmp_header->icmp_hun.ih_idseq.icd_seq;
                if(icd_id == htons(getpid()) && icd_seq == htons(actual_ttl)) {
                    is_my_packet = true;
                }
                
                if(is_my_packet){ // Save time, and ip
                    auto response_time = std::chrono::steady_clock::now() - start_time;
                    responses_ip.insert(sender_ip_str);
                    responses_time.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(response_time).count());
                }

                // We have all packs so break immediately
                int responses_num = responses_time.size();
                if(responses_num == cnt){
                    break;
                }
            }
        }
    }
    close(sock_fd);

    // Print all ip for ttl
    for (auto itr = responses_ip.begin(); itr != responses_ip.end(); itr++){
        std::cout << *itr << " ";
    }

    // Print reponse time in ms
    if(responses_time.size() != 3){
        std::cout << "???\n";
    }
    else{
        int response_time_avg = 0;
        for(auto time : responses_time){
            response_time_avg += time;
        }
        std::cout << response_time_avg/3 << "ms\n";
    }

    if(responses_ip.find(ip) != responses_ip.end()){ // We found last id
        exit(0);
    }
}

void trace_route(int ttl_max, char* ip){
    int packs_cnt = 3;
    for(int ttl=1; ttl<=ttl_max; ttl++){
        for(int i=0; i<packs_cnt; i++){
            icmp_send(ttl, ip);
        }
        std::cout << ttl << ". ";
        icmp_receive(ttl, packs_cnt, ip);
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2 || argc > 2){
        std::cout << "Error accured! You should use command './traceroute `ip`'.";
        return 1;
    }
    if(!validate_ipv4(argv[1])){
        std::cout << "Error accured! Invalid IP address.";
        return 1;
    }
    // Sprawdzić czy argv to adres IP
    trace_route(30, argv[1]);
}