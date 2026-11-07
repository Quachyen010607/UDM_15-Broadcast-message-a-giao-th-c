# Phân tích yêu cầu và thiết kế giao diện

## Server

- Thanh trên: tiêu đề, cổng dịch vụ, khởi động, dừng.
- Cột trái: `ClientListWidget` danh sách endpoint TCP, UDP đã gửi, WebSocket.
- Cột giữa: `MessagePanel` nhật ký, gửi tin, timestamp.
- Cột phải: `ProtocolStatusWidget` ba dòng trạng thái TCP / UDP / WebSocket.
- Lỗi từ bind/socket được ghi trong nhật ký; lỗi bắt đầu TCP hiển thị dialog.

## Client

- Thanh trên: IP máy Server, cổng đích, chọn TCP/UDP/WebSocket, kết nối/ngắt.
- Vùng chính: `MessagePanel` trao đổi tin; `ProtocolStatusWidget` theo dõi giao thức.
- Cổng UDP Client = cổng Server+1 (mặc định 45456) để nhận reply; cổng WebSocket = TCP Server+2 (mặc định 45457).

## JSON chuẩn dùng chung

```json
{
  "type": "udp_message",
  "message": "Xin chao",
  "protocol": "TCP",
  "sender": "Client",
  "id": "uuid-string",
  "timestamp": "2026-11-06T00:00:00.000Z"
}
```

`type=udp_message` được giữ để tương thích UdpService hiện tại. TCP gửi từng dòng JSON (newline-delimited); UDP gửi datagram JSON; WS gửi JSON text frame. Khi tích hợp cuối cùng, nhóm nên thống nhất lại tên trường `type` áp dụng cho mọi giao thức.

## Bàn giao

- File `.ui` mở trực tiếp bằng Qt Designer.
- `ServerWindow.*`, `ClientWindow.*` quản lý UI / luồng socket.
- `Widgets.*` cung cấp 3 widget tái sử dụng.
- `Protocol.h` quản lý định dạng JSON.
- `UdpService.*` tái sử dụng module UDP hiện tại.
- `tests/` và `docs/` là test plan và tài liệu.
