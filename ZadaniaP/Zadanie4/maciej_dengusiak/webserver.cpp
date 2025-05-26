#include <sys/socket.h>    // socket(), bind(), listen(), accept()
#include <netinet/in.h>    // struct sockaddr_in, htons(), INADDR_ANY
#include <arpa/inet.h>     // inet_ntoa(), inet_pton(), inet_ntop()
#include <unistd.h>        // close()
#include <poll.h>          // poll() – do obsługi wielu połączeń
#include <cstring>         // memset(), strcmp(), memcpy()
#include <iostream>        // std::cout, std::cerr
#include <string>          // std::string, std::getline
#include <vector>          // np. do listy połączeń
#include <map>             // do mapowania ścieżek lub nagłówków
#include <sys/stat.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;
const int MAX_REQUEST_LINE_SIZE = 8192;

struct HttpRequest{
    bool is_valid;
    bool connection_close;
    std::string host;
    std::string http_standard;
};

void ERROR(const char* str) {
    fprintf(stderr, "%s: %s\n", str, strerror(errno)); 
    exit(EXIT_FAILURE);
}

bool directory_exists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

std::string get_content_type(const std::string& path) {
    static const std::unordered_map<std::string, std::string> typeMap = {
        {".txt", "text/plain; charset=utf-8n"},
        {".html", "text/html; charset=utf-8"},
        {".css", "text/css"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".pdf", "application/pdf"}
    };

    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos);
        auto it = typeMap.find(ext);
        if (it != typeMap.end()) {
            return it->second;
        }
    }

    return "application/octet-stream";
}

std::string build_response_headers(int statusCode, 
                         size_t content_length,
                         const std::string& content_type,
                         const std::string& http_standard,
                         const std::string& location = "") 
{
    std::string statusText;
    switch(statusCode) {
        case 200: statusText = "OK"; break;
        case 301: statusText = "Moved Permanently"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 501: statusText = "Not Implemented"; break;
        default:  statusText = "Unknown"; break;
    }

    std::stringstream headers;
    headers << http_standard << " " << statusCode << " " << statusText << "\r\n";
    headers << "Content-Type: " << content_type << "\r\n";
    headers << "Content-Length: " << content_length << "\r\n";
    headers << "Connection: close\r\n";
    
    if(statusCode == 301) {
        headers << "Location: " << location << "\r\n";
    }
    headers << "\r\n"; // Koniec nagłówków
    
    return headers.str();
}

bool is_forbidden(const std::string catalog, const std::string& path){
    fs::path abs_catalog = fs::weakly_canonical(fs::absolute(catalog));
    fs::path abs_path = fs::weakly_canonical(fs::absolute(path));

    return abs_path.string().find(abs_catalog.string()) != 0;
}

bool is_not_found(const std::string catalog, const std::string& path){
    if (is_forbidden(catalog, path)) {
        return false;
    }
    fs::path full_path = fs::path(catalog) / path.substr(1);

    if(path[path.size() - 1] == '/'){
        return !fs::exists(fs::absolute(path + "/index.html"));
    }
    return !fs::exists(fs::absolute(path));
}

std::string read_file_content(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Wczytujemy całą zawartość
    std::string buffer(size, '\0');
    if (!file.read(&buffer[0], size)) {
        throw std::runtime_error("Error reading file: " + filepath);
    }

    return buffer;
}

void send_response(int client_fd, const std::string& catalog, HttpRequest& http_request){
    if(http_request.is_valid == false){
        std::string content = "<h1>Invalid HTTP request syntax!<h1>";
        std::string response = build_response_headers(501, content.size(), "text/html", http_request.http_standard);
        response += content;
        send(client_fd, response.data(), response.size(), 0);
        return;
    }

    std::string path = catalog + "/" + http_request.host;
    if(is_forbidden(catalog, path)){
        std::string content = "<h1>Access to path forbidden!<h1>";
        std::string response = build_response_headers(403, content.size(), "text/html", http_request.http_standard);
        response += content;
        send(client_fd, response.data(), response.size(), 0);
        return;
    }
    if(path[path.size() - 1] == '/'){ 
        path += "index.html";
        std::string redirect_path = http_request.host.substr(http_request.host.find("/")); // Nie chcemy domeny
        std::string response = build_response_headers(301, 0, "text/html", http_request.http_standard, redirect_path + "index.html");
        send(client_fd, response.data(), response.size(), 0);
    }

    if(is_not_found(catalog, path)){
        std::string content = "<h1>File not found!<h1>";
        std::string response = build_response_headers(404, content.size(), "text/html", http_request.http_standard);
        response += content;
        send(client_fd, response.data(), response.size(), 0);
        return;
    }
    
    std::string content = read_file_content(path);
    std::string content_type = get_content_type(path);
    bool is_text_content = (content_type.find("text/") != std::string::npos);
        
    if (is_text_content) {
        std::string response = build_response_headers(200, content.size(), content_type, http_request.http_standard);
        response += content;
        send(client_fd, response.data(), response.size(), 0);
    } else {
        std::string headers = build_response_headers(200, content.size(), content_type, http_request.http_standard);
        send(client_fd, headers.data(), headers.size(), 0);
        send(client_fd, content.data(), content.size(), 0);
    }
}

HttpRequest parse_http_request(const std::string& raw_request){
    HttpRequest request = HttpRequest{
        .is_valid = true,
        .connection_close = true,
        .host = "",
        .http_standard = "HTTP/1.1"
    };
    if(raw_request.size() > MAX_REQUEST_LINE_SIZE){
        return request;
    }
    bool first_line = true;
    std::string line;
    std::istringstream stream(raw_request);
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (first_line) {
            if(line.size() < 5 || line.substr(0, 4) != "GET ") {
                request.is_valid = false;
                return request;
            }
            std::string rest = line.substr(5);
            int pos = rest.find(" ");
            request.host = rest.substr(0, pos);
            request.http_standard = rest.substr(pos + 1);
            first_line = false;
            continue;
        }
        
        if (line.empty()) break;

        size_t separator_pos = line.find(": ");
        if (separator_pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, separator_pos);
        std::string value = line.substr(separator_pos + 2);
        
        if(key == "Host"){
            int pos = value.find(":");
            request.host = value.substr(0, pos) + "/" + request.host;
        }
        if (key == "Connection") {
            request.connection_close = (value == "close");
        }
    }
    return request;
}

int configurate_address(int port){
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) ERROR("socket error");

    struct sockaddr_in server_address = {}; 
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) ERROR("bind error");
    if (listen(sock_fd, 64) < 0) ERROR("listen error");

    return sock_fd;
}

void try_response_http(int sock_fd, const std::string& catalog) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) ERROR("accept error");

    const std::chrono::milliseconds keep_alive_timeout(500); // 500ms timeout
    auto deadline = std::chrono::steady_clock::now() + keep_alive_timeout;
    bool connection_close = false;

    while(!connection_close){
        auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0)) break;

        struct pollfd fd = {client_fd, POLLIN, 0};
        int timeout = std::chrono::duration_cast<std::chrono::milliseconds>(remaining).count();
        
        int ret = poll(&fd, 1, timeout);
        if (ret < 0) ERROR("poll error");
        if (ret == 0) break;

        uint8_t buffer[65536];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes_read < 0) ERROR("recv error");
        if (bytes_read == 0) break;

        deadline = std::chrono::steady_clock::now() + keep_alive_timeout;

        std::string raw_request(reinterpret_cast<char*>(buffer), bytes_read);
        HttpRequest http_request = parse_http_request(raw_request);
        connection_close = http_request.connection_close;
        
        send_response(client_fd, catalog, http_request);
    }

    // uint8_t buffer[65536];
    // ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    // if (bytes_read < 0) ERROR("recv error");
    // std::string raw_request(reinterpret_cast<char*>(buffer), bytes_read);
    // HttpRequest http_request = parse_http_request(raw_request);

    // send_response(client_fd, catalog, http_request);
    
    close(client_fd);
}
void run(int port, const std::string& catalog){
    int sock_fd = configurate_address(port);
    struct pollfd fds[1];
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;
    while(true){
        int ret = poll(fds, 1, 20);
        if(ret < 0){
            ERROR("poll error");
        }
        else if(ret > 0){
            if (fds[0].revents & POLLIN) {
                try_response_http(sock_fd, catalog);
            }
        }
    }
}

int main(int argc, char* argv[]){
    if(argc != 3)
        ERROR("Wrong arguments number! You should use command './webserver port directory'.\n");
    int port = 0;
    try {
        port = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        ERROR("Port number error");
    }
    if (!directory_exists(argv[2])) ERROR("Directory does not exist\n");

    run(port, std::string(argv[2]));
}
