# Nghiên cứu WebSocket — Task 1

## Mục tiêu và kết quả
Tìm hiểu giao thức WebSocket và cơ chế giao tiếp hai chiều giữa Server và Client cho đề tài Broadcast Message đa giao thức. Thư mục này gồm phần nghiên cứu và prototype C++/Qt độc lập; chưa tích hợp vào GUI, BroadcastManager hoặc schema Message của ứng dụng chính.

## WebSocket so với HTTP
HTTP thường theo mô hình Client gửi request rồi Server trả response. WebSocket dùng HTTP để thiết lập kết nối ban đầu, sau đó giữ một TCP connection mở lâu dài. Từ lúc handshake thành công, Client và Server đều có thể chủ động gửi message bất kỳ lúc nào (full-duplex), không phải chờ request mới. Cơ chế này phù hợp với chat, thông báo và broadcast theo sự kiện; payload nghiệp vụ vẫn do ứng dụng tự định nghĩa.

## 1. Handshake HTTP Upgrade
Client mở TCP connection rồi gửi HTTP/1.1 Upgrade request. Các trường thường gặp gồm Upgrade: websocket, Connection: Upgrade, Sec-WebSocket-Key và Sec-WebSocket-Version: 13; trình duyệt còn gửi Origin. Sec-WebSocket-Key là nonce 16 byte được Base64 hóa.

```http
GET /broadcast HTTP/1.1
Host: example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
Origin: https://example.com
```

Server chấp nhận bằng 101 Switching Protocols. Sec-WebSocket-Accept được tính theo RFC 6455: nối key với GUID 258EAFA5-E914-47DA-95CA-C5AB0DC85B11, tính SHA-1 rồi Base64 hóa. Giá trị này xác nhận handshake, không xác thực người dùng và không mã hóa dữ liệu. Nếu request hoặc policy không hợp lệ, server từ chối và WebSocket session không được mở.

ws:// chạy không mã hóa; wss:// chạy trên TLS. Sau phản hồi 101, hai bên dùng WebSocket frames trên cùng TCP connection. Trong Qt, QWebSocket::open() khởi tạo phía Client; QWebSocketServer::listen() và nextPendingConnection() nhận phía Server. Thư viện tự xử lý handshake và framing.

## 2. Giao tiếp hai chiều
```text
Client                                      Server
  | ---- TCP + HTTP Upgrade --------------> |
  | <--- HTTP 101 Switching Protocols ----- |
  |                                         |
  | ---- text/binary message -------------> | nhận message
  | <--- broadcast hoặc thông báo -------- | gửi bất kỳ lúc nào
  | ---- message khác bất kỳ lúc nào ----> |
  |                                         |
  | ---- Close frame ---------------------> |
  | <--- Close frame ---------------------- |
```
Full-duplex mô tả khả năng hai hướng gửi độc lập trên cùng kết nối; trong chương trình event-driven, signal/callback được phát khi socket kết nối, nhận message, lỗi hoặc ngắt. Luồng demo: server phát newConnection, lấy socket bằng nextPendingConnection; client gửi bằng sendTextMessage; server nhận textMessageReceived rồi broadcast tới các socket đang kết nối; client nhận lại qua textMessageReceived.

## 3. Message, frame và control frame
Ứng dụng gửi WebSocket message; message được vận chuyển bằng một hoặc nhiều frame. Header frame chứa FIN, opcode, độ dài payload, mask bit và có thể có RSV bits cho extension.
- Opcode 0x1: text (UTF-8); 0x2: binary; 0x0: continuation khi message bị phân mảnh.
- Opcode 0x8: close; 0x9: ping; 0xA: pong.
- Client-to-server frames phải được mask bằng khóa 4 byte; server-to-client frames không mask. Masking không phải mã hóa.
- Control frames không được phân mảnh, payload tối đa 125 byte và có thể xen giữa các fragment.
Qt ghép fragment thành message hoàn chỉnh trước khi phát textMessageReceived hoặc binaryMessageReceived; ứng dụng thông thường không phải tự parse frame.

## 4. Vòng đời và lỗi kết nối
1. Connecting: tạo TCP connection và thực hiện Upgrade handshake.
2. Open: hai phía trao đổi message và ping/pong.
3. Closing: một phía gửi Close frame, phía kia phản hồi rồi đóng transport.
4. Closed: socket đã ngắt; Server bỏ socket khỏi danh sách client và giải phóng tài nguyên.
Ping/pong hỗ trợ kiểm tra peer còn phản hồi nhưng ứng dụng cần timeout phù hợp. WebSocket không tự reconnect khi mất mạng; nếu cần, ứng dụng tự thử lại với exponential backoff, jitter và giới hạn số lần thử.

## 5. API Qt 6 trong prototype
| API/signal | Vai trò |
| --- | --- |
| QWebSocketServer::listen() | Lắng nghe địa chỉ và port |
| newConnection / nextPendingConnection() | Nhận socket Client mới |
| QWebSocket::open() / connected | Mở kết nối và báo handshake thành công |
| textMessageReceived | Nhận text message hoàn chỉnh |
| sendTextMessage() | Gửi text message cho peer |
| errorOccurred / disconnected | Xử lý lỗi và cập nhật vòng đời socket |
| QWebSocketServer::close() | Ngừng lắng nghe kết nối mới |

## 6. Build và cách quan sát nhiều Client
Yêu cầu Qt 6.12 (Core, WebSockets), CMake 3.21+ và compiler tương thích với bộ Qt. Từ thư mục gốc repository, thay đường dẫn Qt phù hợp với máy:
```powershell
cmake -S Code/WebSocketDemo -B build/websocket -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64
cmake --build build/websocket --config Release
.\build\websocket\udm15_websocket_server.exe 5002
```
Mở hai terminal khác nhau để chạy hai Client:
```powershell
.\build\websocket\udm15_websocket_client.exe ws://127.0.0.1:5002 "Xin chao client 1"
.\build\websocket\udm15_websocket_client.exe ws://127.0.0.1:5002 "Xin chao client 2"
```
Quan sát log Server để thấy peer kết nối, message đến và ngắt kết nối. Mỗi Client nhận broadcast. Client demo gửi một message rồi thoát khi nhận message đầu tiên; đây là minh họa luồng cơ bản, chưa phải ứng dụng chat tương tác liên tục.

## 7. Bảo mật và giới hạn
- Dùng wss:// với TLS hợp lệ khi traffic đi qua mạng; ws:// chỉ dùng cho môi trường local/thử nghiệm.
- Kiểm tra Origin phù hợp với loại Client; Origin không thay thế xác thực và phân quyền.
- Giới hạn kích thước message, số Client, tốc độ gửi và thời gian chờ để tránh cạn tài nguyên.
- Chỉ broadcast tới người nhận được phép; xử lý backpressure khi Client nhận chậm.
- Subprotocol dùng để thương lượng định dạng/phiên bản ứng dụng khi cần.
- Prototype hiện chỉ relay text; chưa có TLS, xác thực, reconnect, backpressure, subprotocol, JSON schema, GUI hoặc tích hợp BroadcastManager.

## Kết luận
WebSocket dùng HTTP Upgrade để mở một kết nối TCP lâu dài, rồi cho phép Client và Server gửi message hai chiều theo sự kiện. Frame là cách vận chuyển ở tầng giao thức; ứng dụng tự xác định payload và cần thiết kế bảo mật, giới hạn tải, phân quyền và phục hồi kết nối. Prototype cho thấy luồng nhiều Client → Server → broadcast.

## Tài liệu tham khảo
- [RFC 6455 — The WebSocket Protocol](https://www.rfc-editor.org/rfc/rfc6455)
- [Qt 6.12 — QWebSocketServer](https://doc.qt.io/qt-6/qwebsocketserver.html)
- [Qt 6.12 — QWebSocket](https://doc.qt.io/qt-6/qwebsocket.html)
- [Qt WebSocket Echo Server example](https://doc.qt.io/qt-6/echoserver.html)
