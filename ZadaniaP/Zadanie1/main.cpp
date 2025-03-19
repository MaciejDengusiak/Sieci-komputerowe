#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <netinet/ip_icmp.h>  // struct icmp
#include <cstring> 
#include <unistd.h>
#include <poll.h>

#define POLL_TIMEOUT 1000 

char* _ip;

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
void icmp_receive(int actual_ttl, int cnt, char* ip) {
    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0)
        ERROR("socket error");

    std::cout << "Nasłuchiwanie TTL=" << actual_ttl << ", potrzebne pakiety: " << cnt << "\n";

    while (cnt > 0) {
        struct pollfd pfd;
        pfd.fd = sock_fd;
        pfd.events = POLLIN;

        int poll_result = poll(&pfd, 1, 1000);  // 1000 ms = 1 sekunda

        if (poll_result == -1) {
            ERROR("poll error");
        } else if (poll_result == 0) {
            std::cout << "Brak pakietów ICMP w ciągu 1 sekundy.\n";
            break;  // Możesz też kontynuować, jeśli chcesz dalej czekać
        } else {
            if (pfd.revents & POLLIN) {
                struct sockaddr_in sender;
                socklen_t sender_len = sizeof(sender);
                u_int8_t buffer[IP_MAXPACKET];

                ssize_t packet_len = recvfrom(sock_fd, buffer, IP_MAXPACKET, 0,
                                              (struct sockaddr*)&sender, &sender_len);
                if (packet_len < 0)
                    ERROR("recvfrom error");

                char sender_ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));

                struct ip* ip_header = (struct ip*) buffer;
                ssize_t ip_header_len = 4 * (ssize_t)(ip_header->ip_hl);

                struct icmp* icmp_header = (struct icmp*)(buffer + ip_header_len);

                uint16_t icd_id = ntohs(icmp_header->icmp_hun.ih_idseq.icd_id);
                uint16_t icd_seq = ntohs(icmp_header->icmp_hun.ih_idseq.icd_seq);

                std::cout << icd_id << " " << icd_seq << "\n";
                // Porównaj z PID i TTL
                if (icd_id == getpid() && icd_seq == actual_ttl) {
                    printf("Otrzymano pakiet ICMP od: %s\n", sender_ip_str);
                    _ip = sender_ip_str;
                    cnt -= 1;
                    if(_ip == ip){
                        cnt = 0;
                    }
                }
            }
        }
    }
    close(sock_fd);
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
    header.icmp_hun.ih_idseq.icd_id = getpid();
    header.icmp_hun.ih_idseq.icd_seq = ttl;
    header.icmp_cksum = 0;
    header.icmp_cksum = compute_icmp_checksum (
    (u_int16_t*)&header, sizeof(header));

    struct sockaddr_in recipient;
    memset (&recipient, 0, sizeof(recipient));
    recipient.sin_family = AF_INET;
    inet_pton (AF_INET, ip, &recipient.sin_addr);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    setsockopt (sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

    ssize_t bytes_sent = sendto (
    sockfd,
    &header,
    sizeof(header),
    0,
    (struct sockaddr*)&recipient,
    sizeof(recipient)
    );
}

void send_pack(int cnt, int ttl_max, char* ip){
    for(int ttl=1; ttl<=ttl_max; ttl++){
        for(int i=0; i<cnt; i++){ // Wysyłamy jeden za drugim
            icmp_send(ttl, ip);
        }
        std::cout << ttl << ": ";
        icmp_receive(ttl, 3, ip); // Odbieramy przez sekundę, jeśli przyjdzie pakiet z tym pid i ttl to wypisujemy go, jeden koło drugiego.

        std::cout << "[TEST]" << "\n";
        if(ip == _ip){
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2 || argc > 2){
        std::cout << "Error accured! You should use command './traceroute 'ip'";
        return 1;
    }

    send_pack(3, 30, argv[1]);
}