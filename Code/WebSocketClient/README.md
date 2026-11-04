# Task 3 — WebSocket Client (phần 50%)

## Đã thực hiện

- Lớp `WebSocketClient` quản lý kết nối/ngắt kết nối và phát trạng thái, lỗi.
- Hỗ trợ URL `ws://` và `wss://`.
- Gửi yêu cầu broadcast JSON v1 tương thích với `Code/WebSocketServer/MessageProtocol.h`.
- Nhận sự kiện broadcast, kiểm tra schema và phát sender ID, message ID, nội dung và thời gian; đồng thời cung cấp raw message.
- Có chương trình console mẫu kết nối, gửi tin và in broadcast nhận được.

## Build và chạy

Cần CMake 3.21 trở lên, Qt 6.12 và module WebSockets. Chạy từ thư mục gốc repo (đổi đường dẫn Qt nếu cần):

```powershell
cmake -S Code/WebSocketClient -B build/websocket-client -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/websocket-client --config Release
.\build\websocket-client\udm15_websocket_client.exe ws://127.0.0.1:5002 Xin chao ca lop
```

Cần khởi động WebSocket Server Task 2 trên cổng 5002 trước.

## Phạm vi 50%

Đã có lớp client nền tảng và demo trao đổi broadcast. Tích hợp giao diện, tự kết nối lại và kiểm thử runtime để làm ở phần tiếp theo. Chưa build/chạy xác minh trong môi trường hiện tại vì chưa có Qt/CMake khả dụng.
