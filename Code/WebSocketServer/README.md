# Task 2 — WebSocket Server (mốc 50%)

## Đã thực hiện

- WebSocket Server dùng Qt 6 WebSockets, lắng nghe trên IPv4; cổng mặc định là 5002.
- Nhận và quản lý các kết nối client, gán ID tăng dần, lưu địa chỉ peer và thời điểm kết nối.
- Theo dõi sự kiện client kết nối/ngắt kết nối, số client hiện tại, tin nhắn văn bản và lỗi server.
- Giới hạn tối đa 64 client; từ chối kết nối vượt giới hạn.
- Cung cấp lệnh chạy server và tham số cổng tùy chọn.

## Phần còn lại

Phần 50% tiếp theo có thể bổ sung broadcast từ server tới các client, định dạng/giao thức bản tin dùng chung, tích hợp với client và kiểm thử nhiều kết nối. Hiện tại server phát sự kiện tin nhắn nhận được cho ứng dụng nhưng chưa gửi broadcast.

## Build và chạy

Cần cài Qt 6.12 có module WebSockets và CMake 3.21 trở lên. Từ thư mục gốc repository, chạy:

```powershell
cmake -S Code/WebSocketServer -B build/websocket-server -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/websocket-server --config Release
.\build\websocket-server\udm15_websocket_server.exe 5002
```

Có thể bỏ `5002` để dùng cổng mặc định. Điều chỉnh đường dẫn Qt theo bộ Qt đã cài.
