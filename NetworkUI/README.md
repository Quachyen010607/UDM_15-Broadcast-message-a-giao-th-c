# NetworkUI — Giao diện Server / Client Qt

Bộ giao diện Qt Widgets dành cho bài tập lớn Lập trình mạng. **Đây là module mới độc lập**, không ghi đè `UDP/`, `Code/WebSocketDemo/` hay `Code/WebSocketServer/` trong repository.

## Chức năng

- **ServerWindow.ui**: mở/dừng Server; hiển thị danh sách client TCP, UDP đã gửi tin, WebSocket; gửi message tới các endpoint đã biết; nhật ký tin nhắn và lỗi.
- **ClientWindow.ui**: nhập IP/cổng Server, chọn TCP / UDP / WebSocket, kết nối/ngắt và nhắn tin JSON.
- Ba widget tái sử dụng: `ClientListWidget`, `MessagePanel`, `ProtocolStatusWidget`.
- **TCP**: kết nối và trao đổi JSON theo từng dòng UTF-8; Server quản lý nhiều client.
- **UDP**: gửi JSON datagram qua IP/cổng; tái sử dụng `UdpService` từ module UDP đã viết, hỗ trợ broadcast khi nhập IP `255.255.255.255` trong phiên dùng UDP. Server ghi nhận peer sau gói UDP đầu tiên rồi trả lời về cổng peer.
- **WebSocket**: sử dụng `QWebSocketServer` / `QWebSocket` nếu có `Qt WebSockets`. CMake tự phát hiện; nếu thiếu thì TCP/UDP vẫn chạy, giao diện đưa thông báo.
- **Kiểm thử**: JSON roundtrip, JSON lỗi, danh sách client, kiểm tra UDP đầu vào bằng Qt Test; kiểm thử tay Server/Client dưới đây.

## Cài đặt trên Windows MSYS2 UCRT64

Mở **MSYS2 UCRT64**:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-websockets
```

Qt Test thuộc bộ Qt6 Base. CMake cần được cài đặt và có trong PATH.

## Build trong VS Code PowerShell

Trong thư mục `NetworkUI`:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\Program Files\CMake\bin;" + $env:Path
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe" `
  -DCMAKE_MAKE_PROGRAM="C:/msys64/ucrt64/bin/mingw32-make.exe" `
  -DCMAKE_PREFIX_PATH="C:/msys64/ucrt64"
cmake --build build
```

Chạy hai terminal:

```powershell
.\build\network_server.exe
```

```powershell
.\build\network_client.exe
```

Nếu thiếu DLL Qt khi khởi động, đảm bảo `C:\msys64\ucrt64\bin` nằm trong PATH của terminal đang chạy. Không trộn bộ Qt MinGW khác với MSYS2 UCRT64.

## Cách dùng — một máy (localhost)

1. Mở `network_server.exe`. Để cổng `45455`, bấm **Khởi động**. TCP/UDP cùng dùng cổng số `45455` vì là các giao thức khác nhau. WebSocket mặc định tự mở tại **45457** (cổng gốc +2) nếu được biên dịch.
2. Mở `network_client.exe`. Với TCP: IP `127.0.0.1`, cổng `45455`, chọn **TCP**, nhấn **Kết nối / Mở cổng**. Gửi `Xin chào từ client`. Server sẽ hiển thị nội dung và 1 client TCP. Gửi đáp lại từ Server.
3. Chọn **Ngắt kết nối** trên Client, chọn **UDP**, IP `127.0.0.1`, cổng `45455`, nhấn **Kết nối / Mở cổng**. Client lắng nghe trên cổng `45456`; gửi một tin đầu tiên để Server ghi nhận peer; sau đó Server có thể gửi đáp lại.
4. WebSocket: ngắt kết nối, chọn **WebSocket**, IP `127.0.0.1`, đổi cổng **45457**, kết nối và trao đổi tin. Nếu chưa cài `Qt WebSockets`, chương trình sẽ hiện thông báo thay vì giả vờ kết nối.
5. Test broadcast: chọn **UDP**, IP `255.255.255.255` thay cho localhost; để kiểm thử thực tế với thiết bị khác phải cùng LAN, không chặn broadcast/UDP trong Firewall. **Test localhost không chứng minh broadcast LAN thực tế**.

### Sử dụng hai máy

Dùng IP IPv4 máy Server, ví dụ `192.168.1.10` (không phải IP minh chứng thật). Mở firewall inbound TCP 45455, UDP 45455, TCP 45457 nếu dùng WebSocket; chỉ áp dụng mạng riêng đáng tin cậy. Server/Client cần cùng LAN. Máy Client phải đặt IP máy Server.

## Qt Designer

Mở `ui/ServerWindow.ui` và `ui/ClientWindow.ui` bằng Qt Designer hoặc Qt Creator. Các vùng có objectName `clientHost`, `messageHost`, `statusHost` là **placeholder** được gắn widget tùy biến bằng C++ khi chạy. Qt Designer chỉ hiển thị layout gốc, không hiển thị nội dung widget động trong chế độ thiết kế. Không đổi `objectName` nếu chưa cập nhật code tương ứng.

## Test

```powershell
ctest --test-dir build --output-on-failure
```

Nếu `Qt Test` không có, target test không được tạo; cài Qt Test rồi cấu hình CMake lại.

## Giới hạn / lưu ý

- Phần UDP độc lập của nhóm vẫn giữ nguyên, `NetworkUI` dùng bản sao `UdpService` có điều chỉnh socket gửi từ cổng đang nhận nhằm hỗ trợ phản hồi.
- Danh sách UDP chỉ hiển thị các peer **đã gửi gói tin** cho Server; UDP không có phiên kết nối thật như TCP.
- Giao diện Server gửi tới **tất cả** client TCP/WebSocket đang kết nối và UDP peer đã ghi nhận, chưa có chọn gửi riêng client.
- Chương trình là bản demo cho LAN, chưa có xác thực, mã hóa hoặc độ tin cậy của UDP, chưa phù hợp triển khai Internet công khai.
- WebSocket là endpoint WS mới và **chưa được tích hợp với module WebSocketDemo/WebSocketServer hiện tại của nhóm** vì chưa có file nguồn của chúng. Có thể thay endpoint này bằng module nhóm sau.
- Mã nguồn được kiểm tra cấu trúc, nhưng chưa thể build/test tự động trong môi trường tạo file do không có Qt. Cần thực hiện build và test trên máy Windows của bạn.
