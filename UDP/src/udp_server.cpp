#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int server_fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 1. Tạo UDP Socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 2. Gán địa chỉ (Bind)
    if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    std::cout << "[UDP Server] Đang lắng nghe trên cổng " << PORT << "...\n";

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        // 3. Nhận dữ liệu từ Client
        int n = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << "[UDP Server] Nhận tin nhắn: " << buffer << std::endl;

            // 4. Phản hồi lại Client
            std::string ack_msg = "Server đã nhận: " + std::string(buffer);
            sendto(server_fd, ack_msg.c_str(), ack_msg.length(), 0, (struct sockaddr *)&client_addr, addr_len);
        }
    }

    close(server_fd);
    return 0;
}