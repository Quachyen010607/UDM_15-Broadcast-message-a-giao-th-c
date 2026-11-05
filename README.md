
# UDM_15 - BROADCAST MESSAGE ĐA GIAO THỨC

## Tiến độ dự án

**Overall Progress: 0%**

`░░░░░░░░░░` 0%

| Hạng mục | Tiến độ | Trạng thái |
|---|---:|---|
| Phân tích yêu cầu | 50% | Đã bắt đầu |
| Thiết kế kiến trúc | 0% | Chưa bắt đầu |
| TCP Server & Client | 0% | Chưa bắt đầu |
| UDP Server & Client | 0% | Chưa bắt đầu |
| WebSocket Server & Client | 30% | Đã bắt đầu |
| Common & Message | 0% | Chưa bắt đầu |
| Giao diện GUI | 0% | Chưa bắt đầu |
| Tích hợp hệ thống | 0% | Chưa bắt đầu |
| Kiểm thử | 0% | Chưa bắt đầu |
| Báo cáo & thuyết trình | 0% | Chưa bắt đầu |

## Thành viên

| STT | MSSV | Họ và tên | Vai trò |
|---:|---|---|---|
| 1 | 060206006877 | Đỗ Ngọc Tiến | TCP Server & Client |
| 2 | 091306007157 | Ngô Anh Thư | UDP Server & Client |
| 3 | 077205009681 | Nguyễn Quốc Toàn | WebSocket Server & Client |
| 4 | 089206007792 | Nguyễn Trần Thiện Huy | Common & Message |
| 5 | 089206003490 | Phạm Khánh Quốc | GUI Server & Client |
| 6 | 075307022774 | Quách Thị Hồng Yến | Broadcast & Integration |

## Giới thiệu

UDM_15 - Broadcast Message đa giao thức là hệ thống truyền và phát
thông điệp từ một Server đến nhiều Client thông qua các giao thức
mạng khác nhau.

Dự án được xây dựng bằng C++ và Qt Framework, hỗ trợ ba giao thức
TCP, UDP và WebSocket.

Mục tiêu:
- Xây dựng hệ thống Client–Server đa giao thức.
- Cho phép Server gửi Broadcast Message đến nhiều Client.
- Quản lý danh sách Client và trạng thái kết nối.
- Hiển thị nội dung và trạng thái gửi nhận message trên giao diện.
- So sánh đặc điểm truyền dữ liệu của các giao thức.

Đối tượng sử dụng:
- Người quản trị Server.
- Các Client tham gia hệ thống.

## Kiến trúc hệ thống

- Mô hình: Client–Server.
- Protocol:
  - TCP (Transmission Control Protocol).
  - UDP (User Datagram Protocol).
  - WebSocket.
- Port mặc định:
  - TCP: 5000.
  - UDP: 5001.
  - WebSocket: 5002.
- Cấu trúc message: JSON.

Hệ thống gồm ba thành phần chính:

1. Server: Quản lý kết nối và điều phối Broadcast Message.
2. Client: Kết nối đến Server thông qua giao thức được lựa chọn.
3. Common: Chứa cấu trúc dữ liệu và xử lý message dùng chung.

## Yêu cầu môi trường

- Hệ điều hành: Windows 10/11 64-bit.
- Ngôn ngữ: C++.
- Framework: Qt 6.12.
- IDE: Qt Creator.
- Compiler: MinGW 64-bit.
- Build system: CMake.
- Dependencies:
  - Qt Core.
  - Qt Network.
  - Qt Widgets.
  - Qt WebSockets.

## Cài đặt

1. Cài đặt Qt Creator và Qt 6.12.
2. Cài đặt bộ compiler MinGW tương thích.
3. Cài đặt CMake.
4. Cài đặt module Qt WebSockets.
5. Clone repository:

```bash
git clone <REPOSITORY_URL>
```

6. Mở file CMakeLists.txt bằng Qt Creator.
7. Chọn Kit Desktop Qt 6.12 MinGW 64-bit.
8. Configure và Build project.

## Hướng dẫn chạy

### Server

1. Mở project Server bằng Qt Creator.
2. Chọn cấu hình Build phù hợp.
3. Build và chạy chương trình.
4. Kiểm tra trạng thái TCP, UDP và WebSocket Server.
5. Chờ Client kết nối.

### Client

1. Mở project Client bằng Qt Creator.
2. Nhập địa chỉ IP Server.
3. Chọn giao thức TCP, UDP hoặc WebSocket.
4. Nhấn Connect để kết nối.
5. Nhận và hiển thị Broadcast Message từ Server.

## Cấu hình

Các thông số mạng có thể được thay đổi tùy theo môi trường triển khai:

| Tham số | Giá trị mặc định |
|---|---|
| Server IP | 127.0.0.1 |
| TCP Port | 5000 |
| UDP Port | 5001 |
| WebSocket Port | 5002 |

Lưu ý:
- Có thể thay đổi IP Server để kết nối qua mạng LAN.
- Đảm bảo các port không bị ứng dụng khác chiếm dụng.
- Kiểm tra Windows Firewall khi kết nối giữa các máy.
- Không lưu password hoặc thông tin nhạy cảm trong repository.

## Chức năng

- [ ] Khởi động và quản lý TCP Server.
- [ ] Khởi động và quản lý UDP Server.
- [ ] Khởi động và quản lý WebSocket Server.
- [ ] Client kết nối và ngắt kết nối Server.
- [ ] Quản lý danh sách Client online.
- [ ] Gửi Broadcast Message đến nhiều Client.
- [ ] Hiển thị nội dung message theo thời gian thực.
- [ ] Hiển thị trạng thái kết nối từng giao thức.
- [ ] Ghi nhận lịch sử gửi và nhận message.
- [ ] Xử lý lỗi kết nối và dữ liệu không hợp lệ.

## Kiểm thử

- Functional test:
  Kiểm tra các chức năng kết nối, gửi, nhận và Broadcast Message.

- Test dữ liệu không hợp lệ:
  Kiểm tra message rỗng, sai định dạng JSON và dữ liệu không hợp lệ.

- Test mất kết nối:
  Kiểm tra khi Client ngắt kết nối hoặc Server gặp sự cố mạng.

- Stress test:
  Kiểm tra khả năng xử lý khi có nhiều Client kết nối đồng thời.

- Performance test:
  Đánh giá thời gian gửi nhận message và độ trễ của từng giao thức.

Bằng chứng kiểm thử lưu tại `Extra/TC/`.

## Demo

- Video: Chưa cập nhật.
- Slide: `PPTX/Thuyet_trinh.pptx`
- Báo cáo: `DOCX/Bao_cao.docx`

## Giới hạn

- Hiệu suất phụ thuộc vào số lượng Client và điều kiện mạng.
- UDP không đảm bảo dữ liệu được truyền đến nơi nhận.
- Chưa hỗ trợ mã hóa nội dung message nếu chưa triển khai thêm.
- Chưa hỗ trợ lưu trữ message lâu dài nếu chưa tích hợp cơ sở dữ liệu.
