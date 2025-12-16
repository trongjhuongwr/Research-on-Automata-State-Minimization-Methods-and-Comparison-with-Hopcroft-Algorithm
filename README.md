# Research-on-Automata-State-Minimization-Methods-and-Comparison-with-Hopcroft-Algorithm

Dự án này là sản phẩm thuộc học phần **Ngôn ngữ hình thức**, tập trung nghiên cứu, cài đặt và so sánh hiệu năng của ba thuật toán tối thiểu hóa DFA phổ biến. Hệ thống được xây dựng với kiến trúc **Hybrid**: Backend C++ hiệu năng cao và Frontend Python (Streamlit) trực quan.

---

## Thành Viên Nhóm
| STT | Họ và Tên | MSSV | Vai trò |
|:---:|:---|:---|:---|
| 1 | **Nguyễn Trọng Hưởng** | 31231023691 |
| 2 | **Nguyễn Ngọc Thiện** | 31231021200 |
| 3 | **Dương Quang Đông** | 31231020389 |
| 4 | **Nguyễn Minh Nhựt** | 31231022656 |
| 5 | **Trần Viết Gia Huy** | 31231027056 |

**Giảng viên hướng dẫn:** ThS. Hồ Thị Thanh Tuyến

---

## Tính Năng Chính
1.  **Đa Thuật Toán:** Hỗ trợ 3 phương pháp tối thiểu hóa:
    * **Hopcroft Algorithm:** Độ phức tạp $O(N \log N)$.
    * **Moore Algorithm:** Độ phức tạp $O(N^2)$.
    * **Table Filling Algorithm:** Độ phức tạp $O(N^2)$.
2.  **Trực Quan Hóa:** Vẽ đồ thị DFA trước và sau khi tối thiểu hóa.
3.  **So Sánh Hiệu Năng:** Đo thời gian thực thi (ms) của từng thuật toán.
4.  **Tương Tác:** Giao diện Web cho phép upload file JSON và tải về kết quả.

---

## Yêu Cầu Hệ Thống (Prerequisites)
Để chạy được dự án, bạn cần cài đặt:

1.  **C++ Compiler:** MinGW (g++) hoặc Visual Studio Code C++.
2.  **Python 3.8+**
3.  **Graphviz:** Phần mềm vẽ đồ thị.
    * Tải bản cài đặt cho Windows tại: [Graphviz Download](https://graphviz.org/download/)
    * **LƯU Ý QUAN TRỌNG:** Khi cài đặt, bắt buộc tích chọn **"Add Graphviz to the system PATH for all users"**.

---

## Cài Đặt & Biên Dịch

### Bước 1: Cài đặt thư viện Python
Mở Terminal tại thư mục gốc và chạy:
```bash
pip install -r requirements.txt
```
### Bước 2: Biên dịch Mã nguồn C++
Hệ thống cần biên dịch 3 file .cpp thành .exe để Python có thể gọi. Chạy lần lượt các lệnh sau trong Terminal:
```bash
# 1. Biên dịch Hopcroft
g++ src/cpp/hopcroft.cpp -o bin/hopcroft_solver.exe

# 2. Biên dịch Moore
g++ src/cpp/moore.cpp -o bin/moore_solver.exe

# 3. Biên dịch Table Filling
g++ src/cpp/table_filling.cpp -o bin/table_filling_solver.exe

```

### Hướng Dẫn Sử Dụng
Sau khi cài đặt xong, chạy lệnh sau để khởi động Web App:
```bash
streamlit run src/python/app.py
```

Trình duyệt sẽ tự động mở ra. Bạn thực hiện theo các bước:
1. Upload File: Kéo thả file input.json (có sẵn trong thư mục data/ mẫu).
2. Chọn Thuật toán: Chọn 1 trong 3 thuật toán ở thanh bên trái.
3. Run: Nhấn nút "TỐI ƯU HÓA".
4. Kết quả: Xem hình ảnh trực quan và tải file JSON kết quả về.

# Project Structures
```plaintext
DFA-Minimization-Research/
├── .gitignore               
├── README.md                                
├── requirements.txt        
│
├── src/                     
│   ├── algorithms/          
│   │   ├── table_filling
│   │   ├── moore
│   │   ├── hopcroft
│   │   └── utils      
│   │
│   ├── gui/                 
│   │   ├── app.py                    
│   │         
│   └── generate_dfa
│
├── experiments/             
│   ├── raw_data/            
│   │   ├── benchmark_table_filling.csv
│   │   ├── benchmark_moore.csv
│   │   └── benchmark_hopcroft.csv
│   ├── test               
│   │   ├── test_case.py
│   └── evaluation.py   # Script Python để vẽ biểu đồ từ CSV
```
