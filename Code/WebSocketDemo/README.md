# WebSocket — nghiên cứu và prototype

## Mục tiêu
Tìm hiểu WebSocket và cách Server, Client giao tiếp hai chiều trong đề tài Broadcast Message đa giao thức. Thư mục này là prototype riêng; chưa tích hợp vào ứng dụng Qt chính.

## Cơ chế hoạt động
WebSocket chạy trên TCP. Client bắt đầu bằng HTTP Upgrade handshake; nếu Server trả `101 Switching Protocols`, hai bên giữ kết nối TCP mở lâu dài. Sau đó Client và Server đều có thể gửi message bất kỳ lúc nào, không cần chờ một HTTP request mới.

WebSocket truyền text (UTF-8) hoặc binary message, có thể phân mảnh thành nhiều frame. Ping/pong dùng kiểm tra kết nối; close frame đóng phiên có trật tự. Client-to-server frames phải được mask; server-to-client frames không mask. Ứng dụng thường làm việc với message hoàn chỉnh thay vì tự ghép frame.

Luồng trong prototype:
1. Client gọi `QWebSocket::open` đến `ws://127.0.0.1:5002`.
2. Qt hoàn tất handshake; Server nhận `newConnection` và lấy socket bằng `nextPendingConnection`.
3. Client gửi text qua `sendTextMessage`; Server nhận `textMessageReceived`.
4. Server broadcast text đến các socket đang kết nối; Client nhận message bằng `textMessageReceived`.

## API Qt dùng
- `QWebSocketServer::newConnection` và `nextPendingConnection`: nhận client.
- `QWebSocket::textMessageReceived`: đọc message.
- `QWebSocket::sendTextMessage`: gửi message hai chiều.
- `QWebSocket::disconnected`: cập nhật danh sách client.

## Phạm vi prototype
Server nhận nhiều client và relay text tới tất cả client, gồm cả client gửi. Client demo gửi một message rồi in message nhận về. Payload giữ nguyên dạng text vì schema JSON `Message` cần được thống nhất với phần Common. Chưa có GUI, TLS/WSS, xác thực, reconnect, giới hạn tải hoặc kiểm thử tích hợp.

## Build
Yêu cầu Qt 6.12 (Core, WebSockets), CMake 3.21+ và compiler phù hợp với bộ Qt. Điều chỉnh `CMAKE_PREFIX_PATH` theo máy:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.12.0/mingw_64"
cmake --build build
./build/udm15_websocket_server.exe 5002
./build/udm15_websocket_client.exe ws://127.0.0.1:5002 "Xin chao Server"
```

## Tham khảo
- [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455)
- [QWebSocketServer](https://doc.qt.io/qt-6/qwebsocketserver.html)
- [QWebSocket](https://doc.qt.io/qt-6/qwebsocket.html)
