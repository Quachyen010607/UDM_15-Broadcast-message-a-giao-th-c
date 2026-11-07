#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;

    // 1. Tạo UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    std::string message = "Xin chào từ UDP Client!";
    
    // 2. Gửi tin nhắn tới Server
    sendto(sockfd, message.c_str(), message.length(), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    std::cout << "[UDP Client] Đã gửi: " << message << std::endl;

    // 3. Nhận phản hồi từ Server
    socklen_t addr_len = sizeof(server_addr);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
    if (n > 0) {
        buffer[n] = '\0';
        std::cout << "[UDP Client] Phản hồi từ Server: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}