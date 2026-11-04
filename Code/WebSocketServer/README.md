# Task 2 — WebSocket Server (hoàn thành)

## Chức năng

- Server Qt 6 WebSockets lắng nghe trên IPv4, cổng mặc định 5002.
- Quản lý kết nối client, ID tăng dần, peer address, thời điểm kết nối và giới hạn 64 client.
- Nhận yêu cầu broadcast JSON v1, kiểm tra phiên bản, loại bản tin và payload; từ chối yêu cầu sai định dạng.
- Phát bản tin hợp lệ đến tất cả client đang kết nối, gồm cả người gửi. Mỗi sự kiện có `messageId`, `senderId` và `sentAt`.
- Có client Qt mẫu để gửi một tin nhắn và nhận sự kiện broadcast.

## Giao thức JSON v1

Client gửi:

```json
{
  "version": 1,
  "type": "broadcast",
  "payload": { "text": "Xin chao ca lop" }
}
```

Server phát sự kiện với cùng `version`, `type` và `payload`, đồng thời bổ sung `messageId`, `senderId` và thời gian UTC `sentAt`.

## Build và chạy

Cần CMake 3.21 trở lên và Qt 6.12 có module WebSockets. Từ thư mục gốc repository (điều chỉnh đường dẫn Qt cho máy của bạn):

```powershell
cmake -S Code/WebSocketServer -B build/websocket-server -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/websocket-server --config Release
.\build\websocket-server\udm15_websocket_server.exe 5002
```

Mở terminal khác để gửi một tin nhắn bằng client mẫu:

```powershell
.\build\websocket-server\udm15_websocket_client.exe ws://127.0.0.1:5002 Xin chao ca lop
```

## Trạng thái xác minh

Đã hoàn tất phần triển khai Task 2. Chưa xác minh build/chạy thực tế trong môi trường này vì chưa có Qt/CMake khả dụng. TLS/WSS và xác thực client chưa nằm trong phạm vi triển khai hiện tại.
