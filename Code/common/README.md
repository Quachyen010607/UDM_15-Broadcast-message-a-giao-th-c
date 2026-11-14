# Tài liệu kỹ thuật Common Message JSON v1

## Mục đích

`Code/common` định nghĩa một hợp đồng JSON dùng chung để biểu diễn yêu cầu broadcast và sự kiện broadcast. Mục tiêu là để TCP, UDP và WebSocket mang cùng một nội dung logic; mỗi giao thức chỉ khác cách đóng khung dữ liệu trên đường truyền.

Các thành phần header-only:

- `Message.h`: cấu trúc dữ liệu, kiểm tra bất biến và chuyển đổi JSON.
- `MessageParser.h`: giải mã JSON đầu vào và kiểm tra schema.
- `MessageSerializer.h`: xác thực rồi tuần tự hóa JSON; `deserialize` chuyển tiếp sang parser.

## Schema JSON

Yêu cầu từ Client có các trường bắt buộc:

| Trường | Kiểu | Quy tắc |
| --- | --- | --- |
| `version` | số nguyên | Phải bằng `1`. |
| `type` | chuỗi | Phải là `broadcast`. |
| `payload` | object | Phải có trường `text`. |
| `payload.text` | chuỗi | Không được rỗng sau khi trim khoảng trắng. |

Ví dụ request:

```json
{
  "version": 1,
  "type": "broadcast",
  "payload": { "text": "Xin chào cả lớp" }
}
```

Sự kiện do Server phát giữ nguyên ba trường trên và thêm đủ bộ metadata:

| Trường | Kiểu | Quy tắc |
| --- | --- | --- |
| `messageId` | chuỗi UUID | Định danh duy nhất của sự kiện. |
| `senderId` | chuỗi | ID Client gửi tin. |
| `sentAt` | chuỗi thời gian | ISO 8601, chuẩn hóa UTC; có thể có phần mili giây. |

Ví dụ event:

```json
{
  "version": 1,
  "type": "broadcast",
  "payload": { "text": "Xin chào cả lớp" },
  "messageId": "f5fcbaf0-045c-4daa-9ec6-9d9f10a17f1b",
  "senderId": "client-1",
  "sentAt": "2026-11-13T01:30:00.000Z"
}
```

Metadata sự kiện là một nhóm: hoặc cả ba trường cùng vắng mặt (request), hoặc cả ba cùng có và đúng kiểu (event). Parser bỏ qua trường JSON chưa biết để cho phép mở rộng tương thích; các trường bắt buộc và metadata đã biết vẫn phải hợp lệ.

## Quy tắc dữ liệu

- Chỉ hỗ trợ protocol version 1 và loại bản tin `broadcast`.
- Nội dung chỉ được kiểm tra không rỗng sau trim; parser giữ nguyên giá trị chuỗi gốc để không tự ý sửa nội dung người dùng.
- `messageId` phải phân tích được thành UUID.
- Thời gian được đọc theo ISO 8601 và xuất ra UTC với `Qt::ISODateWithMs`.
- Serializer từ chối `Message` không hợp lệ và trả lỗi qua tham số `QString *error`; parser trả `std::nullopt` khi JSON hoặc schema sai.
- JSON đầu ra được rút gọn bằng `QJsonDocument::Compact`.

## Đóng khung theo giao thức

JSON là payload dùng chung. Quy tắc ranh giới message do transport quyết định:

| Giao thức | Đóng khung | Quy tắc nhận |
| --- | --- | --- |
| TCP | Mỗi JSON kết thúc bằng ký tự xuống dòng `\n`. | TCP là byte stream, cần ghép dữ liệu đến và chỉ parse sau khi nhận đủ một dòng. |
| UDP | Một JSON trong mỗi datagram. | Mỗi datagram được parse độc lập; giới hạn hiện tại của `UDP/src/UdpService` là 1200 byte. |
| WebSocket | Một JSON trong mỗi text frame. | Parse toàn bộ text frame; request Client không được gửi metadata dành riêng cho event. |

Không thêm `\n` vào payload UDP hoặc WebSocket. Dấu xuống dòng thuộc framing TCP, không thuộc schema JSON.

## Tính thống nhất và phạm vi triển khai

- `UDP/src/UdpService` dùng `MessageSerializer` để gửi và `deserialize` để xác thực datagram nhận vào.
- `Code/WebSocketServer/MessageProtocol.h` giữ các helper tương thích, nhưng mã hóa/giải mã thông qua `Code/common`.
- Bộ kiểm thử `Code/WebSocketServer/MessageContractTests.cpp` dùng cùng request JSON để kiểm tra parse dạng TCP line, nhận datagram UDP và request WebSocket; kiểm tra cả version, type và text.
- Các triển khai TCP cũ nằm ngoài adapter Common cần gọi `MessageParser`/`MessageSerializer` để dùng cùng schema. Việc thêm newline chỉ giải quyết framing; nó không tự chuyển schema cũ sang JSON v1.

## Cách dùng trong C++

```cpp
#include "Message.h"
#include "MessageParser.h"
#include "MessageSerializer.h"

const auto request = udm15::Message::broadcastRequest(QStringLiteral("Xin chào"));
QString error;
const auto wire = udm15::MessageSerializer::serialize(request, &error);
if (!wire) {
    // Xử lý error; không gửi message không hợp lệ.
}

const auto parsed = udm15::MessageParser::parse(*wire, &error);
if (!parsed) {
    // Từ chối JSON không hợp lệ.
}
```

## Chạy kiểm thử hợp đồng

Từ thư mục gốc repository, với Qt 6.12 và CMake 3.21 trở lên:

```powershell
cmake -S Code/WebSocketServer -B build/websocket-server -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -DBUILD_TESTING=ON
cmake --build build/websocket-server --config Release
ctest --test-dir build/websocket-server -C Release --output-on-failure
```

Test target `udm15_message_contract_tests` kiểm tra schema request, parse/serialize, dữ liệu sai, TCP line, UDP loopback và WebSocket loopback.
