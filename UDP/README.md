# Thành viên 4 — Module giao thức UDP (Qt/C++)

## 1. Mục tiêu

- Xây dựng UDP bằng `QUdpSocket` (`src/UdpService.h`, `src/UdpService.cpp`).
- Gửi broadcast tới các client trong LAN.
- Nhận và kiểm tra JSON, kiểm tra cổng, xử lý lỗi dữ liệu.
- Kiểm thử từ một đến nhiều máy trong mạng LAN.

Đây là module **thêm mới** trong `Code/UDP/`. Không ghi đè `Code/WebSocketDemo/` hay `Code/WebSocketServer/`.

## 2. Cấu trúc

```text
Code/UDP/
├── CMakeLists.txt            # build demo + test với Qt 5/6
├── UDP.pro                   # mở trực tiếp bằng Qt Creator (qmake)
├── src/
│   ├── UdpService.h          # API/signal cho tích hợp
│   └── UdpService.cpp        # QUdpSocket, kiểm tra JSON
├── demo/
│   └── main.cpp              # app Qt Widgets thử gửi/nhận
├── tests/
│   └── test_udp.cpp          # test Qt Test (tùy chọn)
└── README.md
```

## 3. Yêu cầu

- Qt 5.15 hoặc Qt 6.x (Core, Network, Widgets); Qt Test nếu chạy unit test.
- C++17, CMake >= 3.16, bộ biên dịch phù hợp với bản Qt cài đặt.
- VS Code với CMake Tools hoặc Qt Creator. VS Code không tự cài Qt.

## 4. Build trên Windows/VS Code

Mở Terminal **trong thư mục `Code/UDP`**, chạy:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Nếu CMake không tìm được Qt, chỉ định đường dẫn Qt (ví dụ, thay bằng đường dẫn thật):

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build --config Debug
```

File `.exe` nằm trong `build/` hoặc `build/Debug/` tùy generator; với môi trường Qt trên Windows có thể cần thêm thư mục `bin` của Qt vào `PATH` hoặc chạy `windeployqt` để chép DLL.

Trong Qt Creator: mở `UDP.pro` → chọn kit Qt phù hợp → Run.

## 5. Quy trình demo

1. Kết nối hai hoặc nhiều máy cùng mạng LAN, kiểm tra IP bằng `ipconfig` (Windows).
2. Mở `udp_demo` trên mỗi máy. Mặc định module tự mở cổng nhận **45454**.
3. Giữ cổng đích `45454`; dùng IP `255.255.255.255` để broadcast. Một số LAN không chuyển tiếp limited broadcast; thử địa chỉ broadcast của subnet, ví dụ `192.168.1.255` khi subnet là `192.168.1.0/24`.
4. Từ máy A nhập tin nhắn rồi chọn **Gửi UDP**. Kiểm tra log trên máy B, C.
5. Có thể thay IP đích bằng IP máy B để thử gửi unicast riêng lẻ.
6. Nếu không nhận: cho phép inbound UDP 45454 trong Windows Defender Firewall (mạng Private), kiểm tra Wi-Fi AP/client isolation và hai máy có cùng LAN.

**Không dùng IP ví dụ làm IP cố định**; dùng địa chỉ theo mạng của bạn.

## 6. Định dạng JSON thống nhất (đề xuất cho nhóm)

```json
{
  "type": "udp_message",
  "id": "fbd62d35-a747-41ae-bb4a-55230fe55de2",
  "message": "Xin chao LAN",
  "timestamp": "2026-11-05T10:00:00.000Z"
}
```

- `type`: bắt buộc bằng `udp_message` khi nhận.
- `message`: bắt buộc là chuỗi JSON khi nhận.
- `id`: UUID để nhận diện gói; tự sinh khi gọi `sendMessage`.
- `timestamp`: thời điểm gửi theo UTC, tự sinh khi gọi `sendMessage`.
- Giới hạn dữ liệu JSON gửi và nhận: **1200 byte** để giảm nguy cơ phân mảnh UDP.
- Datagram UDP không bảo đảm gói tới nơi, đúng thứ tự hoặc không trùng lặp. Demo không có ACK/retry.

## 7. API tích hợp dự án Qt chung

Trong CMake project tổng, thêm (điều chỉnh đường dẫn nếu cần):

```cmake
add_subdirectory(Code/UDP)
target_link_libraries(TEN_TARGET_APP_CHINH PRIVATE udp_service)
```

`TEN_TARGET_APP_CHINH` là tên target đang dùng trong project tổng. Nếu chỉ cần thư viện, có thể tách thư viện `udp_service` trong `Code/UDP/CMakeLists.txt` mà không đưa demo vào app chính.

Trong source Qt của app chính:

```cpp
#include "UdpService.h"

// Trong constructor QObject/QWidget của app:
auto *udp = new UdpService(this);
connect(udp, &UdpService::messageReceived, this,
        [this](const QJsonObject &obj, const QString &ip, quint16 port) {
            qDebug() << "Nhan UDP:" << ip << port << obj;
        });
udp->startListening(45454);
udp->sendMessage("Xin chao", 45454);
```

Đảm bảo target app chính liên kết Qt Network. Phải thống nhất schema JSON và port với các thành viên khác.

## 8. Kiểm thử tự động

Nếu đã cài Qt Test:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Các test: sai cổng, tin nhắn rỗng, gửi/nhận loopback trên `127.0.0.1`, JSON không hợp lệ, sai schema, payload quá lớn. **Test nhiều máy cần chạy thủ công** vì phụ thuộc LAN/firewall.

## 9. Bảng kiểm thử thủ công

| Mã | Thao tác | Kết quả mong đợi |
|---|---|---|
| UDP-01 | Mở app trên 2 máy, A broadcast tới cổng 45454 | Máy B nhận JSON và hiển thị message |
| UDP-02 | Mở app trên 3 máy, A broadcast | B và C đều có log nhận (nếu mạng cho phép) |
| UDP-03 | A gửi tới IPv4 của B | B nhận tin nhắn |
| UDP-04 | Để nội dung trống rồi gửi | Báo lỗi, không gửi |
| UDP-05 | Nhập cổng 0 hoặc 65536 | Không chấp nhận |
| UDP-06 | Nhập địa chỉ IP không hợp lệ | Báo lỗi |
| UDP-07 | Gửi chuỗi không phải JSON tới cổng đang nghe | Báo gói lỗi, app không crash |
| UDP-08 | Dừng nhận trên B, gửi từ A | B không hiển thị gói mới |
| UDP-09 | Gửi JSON lớn >1200 byte | Module báo lỗi và từ chối gửi |
| UDP-10 | Bật/tắt firewall nhận UDP 45454 | Ghi lại kết quả có/không nhận theo cấu hình mạng |

Ghi lại ảnh màn hình từng thiết bị, ngày giờ, địa chỉ IP (có thể ẩn thông tin nhạy cảm), kết quả Pass/Fail cho báo cáo của nhóm.

## 10. Giới hạn, lưu ý

- Broadcast chỉ dùng trong LAN/subnet; router thường không chuyển tiếp sang subnet khác.
- `255.255.255.255` có thể bị chặn bởi OS/router. Có thể sử dụng broadcast theo subnet.
- UDP không có xác nhận gửi đến người nhận; “đã gửi” chỉ phản ánh lệnh ghi datagram thành công trên máy gửi.
- Các máy chạy cùng cổng UDP trên **các thiết bị khác nhau** không xung đột. Nhiều process cùng bind một cổng trên **cùng máy** có hành vi khác nhau theo OS.
- Cần thống nhất giao diện và giao thức JSON với nhóm trước khi tích hợp.
