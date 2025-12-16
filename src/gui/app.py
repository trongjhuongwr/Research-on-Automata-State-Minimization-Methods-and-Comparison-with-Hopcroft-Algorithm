import streamlit as st
import json
import subprocess
import os
import graphviz
import time

# --- 1. CẤU HÌNH HỆ THỐNG ---
# Tự động định vị đường dẫn tuyệt đối (tránh lỗi file not found)
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(CURRENT_DIR))
BIN_DIR = os.path.join(PROJECT_ROOT, "bin")
DATA_DIR = os.path.join(PROJECT_ROOT, "data")

# Đảm bảo thư mục data tồn tại
os.makedirs(DATA_DIR, exist_ok=True)

# Bản đồ ánh xạ: Tên hiển thị -> Tên file EXE
ALGO_MAP = {
    "Hopcroft Algorithm (O(N log N))": "hopcroft_solver.exe",
    "Moore Algorithm (O(N^2))": "moore_solver.exe",
    "Table Filling Algorithm (O(N^2))": "table_filling_solver.exe"
}

st.set_page_config(
    page_title="Automata Minimizer", 
    layout="wide", 
    page_icon="🤖",
    initial_sidebar_state="expanded"
)

# --- 2. HÀM VẼ ĐỒ THỊ DFA ---
def draw_dfa(dfa_data):
    """Chuyển đổi JSON DFA sang hình ảnh Graphviz"""
    if not dfa_data: return None
    
    # Tạo đồ thị có hướng (Digraph)
    dot = graphviz.Digraph()
    dot.attr(rankdir='LR') # Vẽ từ Trái sang Phải
    dot.attr('node', shape='circle')
    
    # Vẽ các Trạng thái (Nodes)
    for state in dfa_data:
        name = state['state_name']
        
        # Kiểu dáng node: 2 vòng tròn nếu là Final State
        shape = 'doublecircle' if state['is_end'] else 'circle'
        color = 'black'
        style = 'filled' if state['is_start'] else ''
        fillcolor = '#e1f5fe' if state['is_start'] else 'white' # Màu xanh nhạt cho Start
        
        # Mũi tên trỏ vào Start State
        if state['is_start']:
            dot.node('start_pointer', '', shape='none', width='0')
            dot.edge('start_pointer', name)
            
        dot.node(name, shape=shape, style=style, fillcolor=fillcolor, color=color)
        
        # Vẽ các Chuyển đổi (Edges)
        # Gom nhóm các input cùng đích đến (ví dụ: 0,1 -> B)
        transitions = {}
        for t in state.get('transitions', []):
            target = t['target_state']
            inp = t['input']
            if target not in transitions: transitions[target] = []
            transitions[target].append(inp)
            
        for target, inputs in transitions.items():
            label = ",".join(sorted(inputs))
            dot.edge(name, target, label=label)
            
    return dot

# --- 3. HÀM GỌI C++ BACKEND ---
def run_solver(exe_name, input_data):
    """Quy trình: Ghi Input -> Gọi EXE -> Đọc Output"""
    input_path = os.path.join(DATA_DIR, "temp_input.json")
    output_path = os.path.join(DATA_DIR, "temp_output.json")
    exe_path = os.path.join(BIN_DIR, exe_name)
    
    # B1: Ghi dữ liệu input ra file
    try:
        with open(input_path, "w") as f:
            json.dump(input_data, f)
    except Exception as e:
        return False, f"Lỗi ghi file Input: {str(e)}", 0

    # B2: Kiểm tra file EXE
    if not os.path.exists(exe_path):
        return False, f"LỖI: Không tìm thấy file '{exe_name}' trong thư mục bin/.\nHãy biên dịch C++ trước!", 0

    # B3: Gọi subprocess chạy file EXE
    try:
        start_time = time.time()
        # Lệnh tương đương: ./solver.exe input.json output.json
        process = subprocess.run(
            [exe_path, input_path, output_path],
            capture_output=True, 
            text=True
        )
        end_time = time.time()
        runtime_ms = (end_time - start_time) * 1000 # Đổi sang miliseconds
        
        # Kiểm tra mã lỗi trả về từ C++
        if process.returncode != 0:
            return False, f"C++ Runtime Error:\n{process.stderr}", 0
            
    except Exception as e:
        return False, f"Lỗi khi gọi file EXE: {str(e)}", 0

    # B4: Đọc file Output
    if os.path.exists(output_path):
        try:
            with open(output_path, "r") as f:
                output_data = json.load(f)
            return True, output_data, runtime_ms
        except Exception as e:
            return False, f"Lỗi đọc file Output JSON: {str(e)}", 0
    else:
        return False, "C++ chạy xong nhưng không sinh ra file output.json", 0

# --- 4. GIAO DIỆN CHÍNH (STREAMLIT UI) ---
st.title("🔬 Nghiên cứu Tối thiểu hóa Automata")
st.markdown("Hệ thống so sánh hiệu năng giữa **Hopcroft**, **Moore** và **Table Filling**.")
st.markdown("---")

# Sidebar: Cấu hình
with st.sidebar:
    st.header("1. Nhập Dữ liệu")
    uploaded_file = st.file_uploader("Upload file JSON DFA", type=["json"])
    
    st.header("2. Chọn Thuật toán")
    algo_option = st.radio("Phương pháp:", list(ALGO_MAP.keys()))
    
    st.markdown("---")
    btn_run = st.button("🚀 TỐI ƯU HÓA", type="primary", use_container_width=True)

# Layout chính: 2 Cột
col_input, col_output = st.columns(2)

input_data = None

# XỬ LÝ CỘT TRÁI (INPUT)
with col_input:
    st.subheader("📥 DFA Ban đầu")
    if uploaded_file:
        try:
            input_data = json.load(uploaded_file)
            st.info(f"Số trạng thái: **{len(input_data)}**")
            
            # Vẽ hình
            graph = draw_dfa(input_data)
            st.graphviz_chart(graph)
            
            with st.expander("Xem chi tiết JSON Input"):
                st.json(input_data)
        except Exception as e:
            st.error("File JSON không hợp lệ!")
    else:
        st.warning("Vui lòng upload file input.json để bắt đầu.")

# XỬ LÝ CỘT PHẢI (OUTPUT - Khi nhấn nút)
with col_output:
    st.subheader("📤 DFA Tối thiểu")
    
    if btn_run and input_data:
        exe_file = ALGO_MAP[algo_option]
        
        with st.spinner("Đang xử lý tại Backend C++..."):
            # Gọi hàm xử lý
            success, result_data, runtime = run_solver(exe_file, input_data)
            
        if success:
            # Hiển thị Metrics (Chỉ số)
            n_old = len(input_data)
            n_new = len(result_data)
            reduced = n_old - n_new
            
            m1, m2, m3 = st.columns(3)
            m1.metric("Trạng thái mới", f"{n_new}", delta=f"-{reduced} removed")
            m2.metric("Thời gian chạy", f"{runtime:.2f} ms")
            m3.metric("Thuật toán", algo_option.split(" ")[0])
            
            # Vẽ hình kết quả
            st.success("Tối ưu hóa thành công!")
            st.graphviz_chart(draw_dfa(result_data))
            
            # Nút tải về
            out_json = json.dumps(result_data, indent=4)
            st.download_button(
                label="Tải kết quả (JSON)",
                data=out_json,
                file_name=f"minimized_{algo_option.split()[0]}.json",
                mime="application/json"
            )
        else:
            st.error("Có lỗi xảy ra!")
            st.code(result_data)
            
    elif btn_run and not input_data:
        st.error("Bạn chưa upload file Input!")