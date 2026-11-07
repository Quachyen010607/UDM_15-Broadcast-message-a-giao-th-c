# Kịch bản kiểm thử giao diện (manual QA)

| ID | Bước kiểm thử | Kết quả mong đợi | Thực tế |
|---|---|---|---|
| UI-01 | Mở Server/Client | Hai cửa sổ hiển thị đúng vùng; không che nút | Chưa chạy |
| UI-02 | Server chọn 45455, Khởi động | TCP/UDP hiển thị trạng thái hoạt động | Chưa chạy |
| UI-03 | Client TCP tới 127.0.0.1:45455 | Có trạng thái kết nối; Server thấy client | Chưa chạy |
| UI-04 | Client TCP gửi JSON tin nhắn | Server nhận, ghi thời gian và nội dung | Chưa chạy |
| UI-05 | Server gửi phản hồi | Client nhận nội dung phản hồi | Chưa chạy |
| UI-06 | Ngắt TCP | Server xóa client khỏi danh sách | Chưa chạy |
| UI-07 | Client UDP 127.0.0.1:45455 gửi | Server nhận, peer xuất hiện | Chưa chạy |
| UI-08 | Server gửi UDP trả lời | Client nhận trên cổng 45456 | Chưa chạy |
| UI-09 | WebSocket qua cổng 45457 | Kết nối, gửi/nhận nếu Qt WebSockets đã cài | Chưa chạy |
| UI-10 | Không có Qt WebSockets | Có thông báo dễ hiểu thay vì ứng dụng crash | Chưa chạy |
| UI-11 | Sai IP hoặc Server tắt | Có lỗi kết nối | Chưa chạy |
| UI-12 | Gửi tin rỗng | Không gửi | Chưa chạy |
| UI-13 | Đóng Server đang có Client | Không treo/crash; Client nhận sự kiện disconnect | Chưa chạy |
| UI-14 | Thay đổi kích thước cửa sổ | Layout giãn hợp lý, nội dung đọc được | Chưa chạy |
| UI-15 | Hai máy cùng Wi-Fi, broadcast UDP | Bên nhận có log; firewall cho phép | Chưa chạy |

**Chưa đánh dấu PASS** vì chưa thực thi chương trình trên máy có Qt tại đây. Sau khi chạy thử, cập nhật cột 'Thực tế' và chụp ảnh minh chứng.
