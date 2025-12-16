// Hướng dẫn chạy:
// 1. Đảm bảo có file json.hpp trong cùng thư mục với file moore.cpp (https://github.com/nlohmann/json/releases/latest/download/json.hpp)
// 2. Chạy lệnh biên dịch (ví dụ với g++):
//    g++ moore.cpp -o app -static
//    .\app.exe input.json
// 3. Kết quả sẽ được xuất ra file output.json

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <queue>
#include <iomanip>
#include "json.hpp" // BẮT BUỘC: File json.hpp phải nằm cùng thư mục

using json = nlohmann::json;
using namespace std;

// === PHẦN 1: CẤU TRÚC DFA ===
struct DFA {
    set<int> states;
    set<int> alphabet;
    map<int, map<int, int>> transitions;
    int start_state = -1;
    set<int> final_states;
    
    map<int, string> state_id_to_name;
    map<string, int> state_name_to_id;
    map<int, string> input_id_to_char;
    map<string, int> input_char_to_id;
    int state_counter = 0;
    int input_counter = 0;

    int getStateID(string name) {
        if (state_name_to_id.find(name) == state_name_to_id.end()) {
            state_name_to_id[name] = state_counter;
            state_id_to_name[state_counter] = name;
            states.insert(state_counter);
            state_counter++;
        }
        return state_name_to_id[name];
    }
    int getInputID(string char_str) {
        if (input_char_to_id.find(char_str) == input_char_to_id.end()) {
            input_char_to_id[char_str] = input_counter;
            input_id_to_char[input_counter] = char_str;
            alphabet.insert(input_counter);
            input_counter++;
        }
        return input_char_to_id[char_str];
    }
    void addTransition(string from, string input_char, string to) {
        int u = getStateID(from);
        int v = getStateID(to);
        int c = getInputID(input_char);
        transitions[u][c] = v;
    }
    void setStart(string name) { start_state = getStateID(name); }
    void addFinal(string name) { final_states.insert(getStateID(name)); }
};

DFA loadDFA_JSON(string filename) {
    ifstream f(filename);
    if (!f.is_open()) throw runtime_error("Khong mo duoc file input: " + filename);
    json j; f >> j;
    DFA dfa;
    for (auto& item : j) {
        string s = item["state_name"];
        dfa.getStateID(s);
        if (item.value("is_start", false)) dfa.setStart(s);
        if (item.value("is_end", false)) dfa.addFinal(s);
        if (item.contains("transitions")) {
            for (auto& t : item["transitions"]) 
                dfa.addTransition(s, t["input"], t["target_state"]);
        }
    }
    return dfa;
}

void exportDFA_JSON(const DFA& dfa, string filename) {
    json j_out = json::array();
    for (int u : dfa.states) {
        json j_s;
        j_s["state_name"] = dfa.state_id_to_name.at(u);
        j_s["is_start"] = (u == dfa.start_state);
        j_s["is_end"] = (dfa.final_states.count(u) > 0);
        json j_trans = json::array();
        if (dfa.transitions.count(u)) {
            for (auto const& p : dfa.transitions.at(u)) {
                json t;
                t["input"] = dfa.input_id_to_char.at(p.first);
                t["target_state"] = dfa.state_id_to_name.at(p.second);
                j_trans.push_back(t);
            }
        }
        j_s["transitions"] = j_trans;
        j_out.push_back(j_s);
    }
    ofstream o(filename); o << std::setw(4) << j_out;
}

// === PHẦN 2: LOGIC MOORE ===
class Solver {
private:
    DFA removeUnreachable(const DFA& dfa) {
        DFA clean = dfa; 
        set<int> reachable; queue<int> q;
        if (dfa.start_state == -1) return clean; 
        q.push(dfa.start_state); reachable.insert(dfa.start_state);
        while(!q.empty()) {
            int u = q.front(); q.pop();
            if (dfa.transitions.count(u)) {
                for (auto const& p : dfa.transitions.at(u)) {
                    if (!reachable.count(p.second)) {
                        reachable.insert(p.second); q.push(p.second);
                    }
                }
            }
        }
        clean.states = reachable;
        set<int> new_finals;
        for(int s : reachable) if(dfa.final_states.count(s)) new_finals.insert(s);
        clean.final_states = new_finals;
        return clean;
    }

    // Hàm tái tạo DFA từ phân hoạch (Dùng chung logic tái tạo)
    DFA reconstructDFA(const DFA& oldDFA, const vector<int>& group_id, int num_groups) {
        DFA newDFA;
        newDFA.input_char_to_id = oldDFA.input_char_to_id;
        newDFA.input_id_to_char = oldDFA.input_id_to_char;
        newDFA.alphabet = oldDFA.alphabet;

        // Xây dựng map: group_id -> danh sách các trạng thái cũ
        map<int, vector<int>> groups;
        for(int u : oldDFA.states) {
            groups[group_id[u]].push_back(u);
        }

        map<int, string> new_group_names;

        for(auto const& pair : groups) {
            int g_id = pair.first;
            const vector<int>& members = pair.second;
            
            // Tạo tên: {A,B,C}
            string name = "{";
            vector<string> names;
            bool is_s = false, is_f = false;
            int rep = members[0]; // Đại diện để lấy transition

            for(int u : members) {
                names.push_back(oldDFA.state_id_to_name.at(u));
                if(u == oldDFA.start_state) is_s = true;
                if(oldDFA.final_states.count(u)) is_f = true;
            }
            sort(names.begin(), names.end());
            for(size_t i=0; i<names.size(); ++i) name += (i==0?"":",") + names[i];
            name += "}";

            newDFA.getStateID(name);
            if(is_s) newDFA.setStart(name);
            if(is_f) newDFA.addFinal(name);
            new_group_names[g_id] = name;
        }

        // Tạo transitions
        for(auto const& pair : groups) {
            int g_id = pair.first;
            int rep = pair.second[0];
            string src_name = new_group_names[g_id];

            if(oldDFA.transitions.count(rep)) {
                for(auto const& t : oldDFA.transitions.at(rep)) {
                    int input = t.first;
                    int target_old = t.second;
                    int target_group = group_id[target_old];
                    newDFA.addTransition(src_name, newDFA.input_id_to_char[input], new_group_names[target_group]);
                }
            }
        }
        return newDFA;
    }

public:
    DFA minimize(DFA inputDFA) {
        // 1. Loại bỏ trạng thái thừa
        DFA dfa = removeUnreachable(inputDFA);
        if (dfa.states.empty()) return dfa;

        int n = dfa.state_counter; // Max ID + 1
        vector<int> group(n, -1);

        // 2. Khởi tạo P0: 2 nhóm (Final và Non-Final)
        for(int u : dfa.states) {
            group[u] = dfa.final_states.count(u) ? 1 : 0;
        }

        // 3. Vòng lặp tinh chỉnh (Refinement Loop)
        bool changed = true;
        while(changed) {
            changed = false;
            map<vector<int>, int> signature_to_id;
            int next_group_count = 0;
            vector<int> new_group(n);

            // Duyệt qua tất cả trạng thái để tính chữ ký mới
            for(int u : dfa.states) {
                vector<int> signature;
                signature.push_back(group[u]); // ID nhóm hiện tại
                
                // Thêm ID nhóm của các trạng thái đích
                for(int c : dfa.alphabet) {
                    int target = -1;
                    if(dfa.transitions[u].count(c)) {
                        target = dfa.transitions[u][c];
                    }
                    // Nếu có chuyển đổi thì lấy ID nhóm đích, nếu không thì -1
                    signature.push_back(target != -1 ? group[target] : -1);
                }

                // Gán ID nhóm mới dựa trên chữ ký
                if(signature_to_id.find(signature) == signature_to_id.end()) {
                    signature_to_id[signature] = next_group_count++;
                }
                new_group[u] = signature_to_id[signature];
            }

            // Kiểm tra xem có sự thay đổi nào về phân hoạch không
            // So sánh vector cũ và mới trên tập các trạng thái hợp lệ
            for(int u : dfa.states) {
                if(group[u] != new_group[u]) {
                    // Cần kiểm tra kỹ hơn: liệu số lượng nhóm có tăng lên không?
                    // Hoặc đơn giản là lặp cho đến khi vector không đổi
                }
            }
            
            // Cách đơn giản nhất để check sự thay đổi: so sánh số lượng nhóm
            // Tuy nhiên, để chính xác nhất, ta so sánh toàn bộ vector (hoặc check sự phân tách)
            // Ở đây ta dùng cách gán và check cờ:
            if (new_group != group) {
                group = new_group;
                changed = true;
            }
        }

        // 4. Tái tạo DFA
        return reconstructDFA(dfa, group, 0);
    }
};

// === PHẦN 3: HÀM MAIN ===
int main(int argc, char* argv[]) {
    string inputFile = (argc > 1) ? argv[1] : "input.json";
    string outputFile = (argc > 2) ? argv[2] : "output.json";

    try {
        DFA myDFA = loadDFA_JSON(inputFile);
        Solver solver;
        DFA minDFA = solver.minimize(myDFA);
        exportDFA_JSON(minDFA, outputFile);
        cout << "SUCCESS" << endl;
    } catch (const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }
    return 0;
}