# Task 3 — WebSocket Client (hoàn thành)

## Chức năng

- `WebSocketClient` quản lý trạng thái kết nối, đóng kết nối và lỗi; hỗ trợ URL `ws://` và `wss://`.
- Client gửi yêu cầu broadcast JSON protocol v1 tương thích với `Code/WebSocketServer/MessageProtocol.h`, nhận sự kiện broadcast và kiểm tra schema trước khi phát dữ liệu.
- Ứng dụng Qt Widgets cho phép nhập URL server, kết nối/ngắt kết nối, gửi broadcast và xem log sự kiện/lỗi.
- Tự kết nối lại có thể bật/tắt trên giao diện. Khi bật, client thử lại theo backoff 1, 2, 4, 8, 16 rồi 30 giây (tối đa 30 giây); ngắt thủ công sẽ dừng việc thử lại.

## Build và chạy

Cần CMake 3.21 trở lên, Qt 6.12 với các module Core, Widgets và WebSockets. Chạy từ thư mục gốc repo (đổi đường dẫn Qt nếu cần):

```powershell
cmake -S Code/WebSocketClient -B build/websocket-client -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/websocket-client --config Release
.\build\websocket-client\udm15_websocket_client.exe
```

Khởi động WebSocket Server Task 2 (mặc định cổng 5002), sau đó kết nối tới `ws://127.0.0.1:5002` trong cửa sổ client.

## Trao đổi message

1. Client mở kết nối WebSocket tới URL đã nhập.
2. Khi gửi, client đóng gói nội dung thành yêu cầu broadcast JSON v1.
3. Server trả sự kiện broadcast; client kiểm tra phiên bản, loại sự kiện và trường nội dung rồi ghi sự kiện vào log.
4. Nếu kết nối rớt ngoài ý muốn, client tự thử kết nối lại theo chu kỳ tăng dần khi tùy chọn tương ứng đang bật.

Đã hoàn thiện phần giao diện và tự kết nối lại của Task 3. Chưa build hoặc chạy kiểm thử runtime trong môi trường này.
