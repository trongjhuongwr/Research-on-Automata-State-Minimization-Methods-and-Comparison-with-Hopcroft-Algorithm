// Hướng dẫn biên dịch và chạy:
// 1. Đảm bảo có file json.hpp trong cùng thư mục.
// 2. Lệnh biên dịch (ví dụ g++): g++ table_filling.cpp -o app -std=c++17
// 3. Chạy: ./app input.json

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <iomanip>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

struct DFA {
    int numStates = 0;
    set<string> alphabet;
    
    // Mapping tên <-> ID
    map<string, int> name_to_id;
    map<int, string> id_to_name;
    
    // Bảng chuyển trạng thái: transitions[u][input_char] = v
    vector<map<string, int>> transitions;
    
    int start_state = -1;
    vector<bool> is_final;

    int getStateID(const string& name) {
        if (name_to_id.find(name) == name_to_id.end()) {
            name_to_id[name] = numStates;
            id_to_name[numStates] = name;
            transitions.push_back({});
            is_final.push_back(false);
            numStates++;
        }
        return name_to_id[name];
    }
};

// === HÀM ĐỌC JSON ===
DFA loadDFA_JSON(const string& filename) {
    ifstream f(filename);
    if (!f.is_open()) {
        throw runtime_error("Khong the mo file " + filename);
    }
    json j;
    f >> j;

    DFA dfa;
    // Tất cả trạng thái trước để có ID
    for (const auto& item : j) {
        dfa.getStateID(item["state_name"]);
    }

    // Nạp dữ liệu
    for (const auto& item : j) {
        string u_name = item["state_name"];
        int u = dfa.getStateID(u_name);

        if (item.value("is_start", false)) dfa.start_state = u;
        if (item.value("is_end", false)) dfa.is_final[u] = true;

        for (const auto& trans : item["transitions"]) {
            string inp = trans["input"];
            string v_name = trans["target_state"];
            int v = dfa.getStateID(v_name);
            
            dfa.transitions[u][inp] = v;
            dfa.alphabet.insert(inp);
        }
    }
    return dfa;
}

// === THUẬT TOÁN TABLE FILLING ===
class TableFillingMinimizer {
public:
    DFA minimize(const DFA& dfa) {
        int n = dfa.numStates;
        if (n == 0) return dfa;

        // Bảng đánh dấu: table[i][j] = true nếu cặp (i, j) phân biệt được
        // Chỉ xét i < j
        vector<vector<bool>> marked(n, vector<bool>(n, false));

        // Đánh dấu các cặp (Final, Non-Final)
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (dfa.is_final[i] != dfa.is_final[j]) {
                    marked[i][j] = true;
                }
            }
        }

        // Lặp đánh dấu
        bool changed = true;
        while (changed) {
            changed = false;
            for (int i = 0; i < n; ++i) {
                for (int j = i + 1; j < n; ++j) {
                    if (!marked[i][j]) {
                        // Kiểm tra tất cả đầu vào
                        for (const string& symbol : dfa.alphabet) {
                            if (dfa.transitions[i].count(symbol) && dfa.transitions[j].count(symbol)) {
                                int u = dfa.transitions[i].at(symbol);
                                int v = dfa.transitions[j].at(symbol);
                                
                                if (u == v) continue;
                                
                                int smaller = min(u, v);
                                int larger = max(u, v);
                                
                                if (marked[smaller][larger]) {
                                    marked[i][j] = true;
                                    changed = true;
                                    break;
                                }
                            } else if (dfa.transitions[i].count(symbol) != dfa.transitions[j].count(symbol)) {
                                // Một bên có đường đi, một bên không -> phân biệt được
                                marked[i][j] = true;
                                changed = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Gom nhóm trạng thái tương đương
        vector<int> component(n, -1);
        int numComponents = 0;
        
        // Map từ ID nhóm -> danh sách tên trạng thái cũ
        map<int, vector<string>> group_members;

        for (int i = 0; i < n; ++i) {
            if (component[i] == -1) {
                component[i] = numComponents;
                group_members[numComponents].push_back(dfa.id_to_name.at(i));
                
                // Tìm tất cả j tương đương với i (tức là !marked[i][j])
                for (int j = i + 1; j < n; ++j) {
                    if (!marked[i][j]) {
                        component[j] = numComponents;
                        group_members[numComponents].push_back(dfa.id_to_name.at(j));
                    }
                }
                numComponents++;
            }
        }

        // Xây dựng DFA tối thiểu
        DFA minDFA;
        
        // Tạo tên mới cho các nhóm: ví dụ "{A,B}"
        vector<string> new_names(numComponents);
        for(auto& [gid, members] : group_members) {
            sort(members.begin(), members.end());
            string name = "{";
            for(size_t k = 0; k < members.size(); ++k) {
                name += members[k];
                if(k < members.size()-1) name += ",";
            }
            name += "}";
            new_names[gid] = name;
            minDFA.getStateID(name); // Đăng ký trạng thái mới
        }

        minDFA.alphabet = dfa.alphabet;

        // Thiết lập chuyển đổi cho DFA mới
        for (int gid = 0; gid < numComponents; ++gid) {
            // Lấy đại diện đầu tiên của nhóm
            string rep_name = group_members[gid][0];
            int rep_id = dfa.name_to_id.at(rep_name);
            int new_u_id = minDFA.getStateID(new_names[gid]);

            // Start state
            if (component[dfa.start_state] == gid) {
                minDFA.start_state = new_u_id;
            }

            // Final state
            if (dfa.is_final[rep_id]) {
                minDFA.is_final[new_u_id] = true;
            }

            // Transitions
            for (const string& symbol : dfa.alphabet) {
                if (dfa.transitions[rep_id].count(symbol)) {
                    int old_dest = dfa.transitions[rep_id].at(symbol);
                    int new_dest_gid = component[old_dest];
                    string new_dest_name = new_names[new_dest_gid];
                    int new_v_id = minDFA.getStateID(new_dest_name);
                    
                    minDFA.transitions[new_u_id][symbol] = new_v_id;
                }
            }
        }

        return minDFA;
    }
};

// === HÀM XUẤT JSON ===
void exportDFA_JSON(const DFA& dfa, const string& filename) {
    json j_arr = json::array();
    
    for (int i = 0; i < dfa.numStates; ++i) {
        json j_state;
        j_state["state_name"] = dfa.id_to_name.at(i);
        j_state["is_start"] = (i == dfa.start_state);
        j_state["is_end"] = dfa.is_final[i];
        
        json j_trans_list = json::array();
        for (auto const& [inp, target_id] : dfa.transitions[i]) {
            json t;
            t["input"] = inp;
            t["target_state"] = dfa.id_to_name.at(target_id);
            j_trans_list.push_back(t);
        }
        j_state["transitions"] = j_trans_list;
        j_arr.push_back(j_state);
    }

    ofstream o(filename);
    o << setw(4) << j_arr << endl;
    cout << "Da xuat ket qua ra file: " << filename << endl;
}

int main(int argc, char* argv[]) {
    string inputFile = (argc > 1) ? argv[1] : "input.json";
    string outputFile = "output_table_filling.json";

    try {
        cout << "[Table Filling] Dang doc file: " << inputFile << "..." << endl;
        DFA dfa = loadDFA_JSON(inputFile);
        cout << " -> So trang thai ban dau: " << dfa.numStates << endl;

        TableFillingMinimizer solver;
        DFA minDFA = solver.minimize(dfa);

        cout << " -> So trang thai sau khi toi thieu: " << minDFA.numStates << endl;
        exportDFA_JSON(minDFA, outputFile);
        
    } catch (const exception& e) {
        cerr << "LOI: " << e.what() << endl;
        return 1;
    }

    return 0;
}