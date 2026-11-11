# Task 2 — WebSocket Server (hoàn thành)


## Chức năng


- Server Qt 6 WebSockets lắng nghe trên IPv4, cổng mặc định 5002.
- Quản lý kết nối client, ID tăng dần, peer address, thời điểm kết nối và giới hạn 64 client.
- Nhận yêu cầu broadcast JSON v1, kiểm tra phiên bản, loại bản tin và payload; từ chối yêu cầu sai định dạng.
- Phát bản tin hợp lệ đến tất cả client đang kết nối, gồm cả người gửi. Mỗi sự kiện có `messageId`, `senderId` và `sentAt`.
- Có client Qt mẫu để gửi một tin nhắn và nhận sự kiện broadcast.


## Task 4 — Xử lý vòng đời kết nối (đã triển khai)


Phần triển khai này gia cố quản lý client ở Server:


- Theo dõi và phát sự kiện khi trạng thái socket client thay đổi, đồng thời ghi nhận lỗi gắn với đúng client ID.
- Gỡ client khỏi danh sách đúng một lần khi socket ngắt kết nối hoặc bị hủy ngoài dự kiến.
- Khi Server đóng, đóng các socket đang quản lý, phát sự kiện ngắt kết nối và cập nhật số lượng client về 0 ngay.
- Có bộ kiểm thử tích hợp QtTest cho nhiều client, ngắt kết nối lặp lại, giới hạn kết nối và đóng Server.


## Task 5 — Kiểm thử WebSocket (hoàn thành 100%)


Đã bổ sung kiểm thử tích hợp cho các luồng sau:


- Ba client kết nối đồng thời; một client gửi JSON và cả ba client nhận broadcast cùng `messageId`.
- Server từ chối JSON sai định dạng nhưng giữ kết nối để client tiếp tục gửi tin hợp lệ.
- Client ngắt đột ngột; Server dọn danh sách và cho phép client kết nối lại.
- Client kết nối tới cổng không có Server; lỗi được báo mà không tạo client giả trong danh sách.


Đã chạy workflow GitHub Actions với Qt 6.12.0: cấu hình CMake và build thành công; CTest hoàn tất, tất cả test đều đạt.


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


## Chạy kiểm thử vòng đời Client


Từ thư mục gốc repository:


```powershell
cmake -S Code/WebSocketServer -B build/websocket-server -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DBUILD_TESTING=ON
cmake --build build/websocket-server --config Release
ctest --test-dir build/websocket-server -C Release --output-on-failure
```


Các kiểm thử dùng cổng loopback ngẫu nhiên, không cần chạy Server riêng.


## Trạng thái xác minh


GitHub Actions đã xác nhận build và toàn bộ kiểm thử Task 4/5 thành công trên Qt 6.12.0. TLS/WSS và xác thực client chưa nằm trong phạm vi triển khai hiện tại.
