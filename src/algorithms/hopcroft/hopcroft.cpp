// Hướng dẫn chạy:
// 1. Đảm bảo có file json.hpp trong cùng thư mục với file hopcroft.cpp (https://github.com/nlohmann/json/releases/latest/download/json.hpp)
// 2. Chạy lệnh biên dịch (ví dụ với g++):
//    g++ hopcroft.cpp -o app -static
//    .\app.exe input.json
// 3. Kết quả sẽ được xuất ra file output_hopcroft.json

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <queue>
#include <iomanip>
#include "json.hpp" // BẮT BUỘC: Phải có file json.hpp cùng thư mục

using json = nlohmann::json;
using namespace std;

// === CẤU TRÚC DFA ===
struct DFA {
    set<int> states;
    set<int> alphabet; 
    map<int, map<int, int>> transitions; 
    int start_state = -1;
    set<int> final_states;
    
    // Mapping tên <-> ID
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

// === ĐỌC FILE JSON ===
DFA loadDFA_JSON(string filename) {
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "LOI CHI MANG: Khong tim thay file '" << filename << "'!" << endl;
        cerr << "Vui long tao file input.json cung thu muc voi file .exe" << endl;
        exit(1);
    }
    json j;
    f >> j;
    
    DFA dfa;
    for (auto& item : j) {
        string s_name = item["state_name"];
        dfa.getStateID(s_name);
        if (item["is_start"]) dfa.setStart(s_name);
        if (item["is_end"]) dfa.addFinal(s_name);
        
        for (auto& trans : item["transitions"]) {
            string inp = trans["input"];
            string target = trans["target_state"];
            dfa.addTransition(s_name, inp, target);
        }
    }
    return dfa;
}

// === XUẤT FILE JSON ===
void exportDFA_JSON(const DFA& dfa, string filename) {
    json j_out = json::array();
    for (int u : dfa.states) {
        json j_state;
        string u_name = dfa.state_id_to_name.at(u);
        j_state["state_name"] = u_name;
        j_state["is_start"] = (u == dfa.start_state);
        j_state["is_end"] = (dfa.final_states.count(u) > 0);
        
        json j_trans_list = json::array();
        if (dfa.transitions.count(u)) {
            // Thay thế vòng lặp structured binding để tương thích C++11
            for (auto const& pair : dfa.transitions.at(u)) {
                int c = pair.first;
                int v = pair.second;
                json j_trans;
                j_trans["input"] = dfa.input_id_to_char.at(c);
                j_trans["target_state"] = dfa.state_id_to_name.at(v);
                j_trans_list.push_back(j_trans);
            }
        }
        j_state["transitions"] = j_trans_list;
        j_out.push_back(j_state);
    }
    ofstream o(filename);
    o << std::setw(4) << j_out << endl;
}

// === THUẬT TOÁN HOPCROFT ===
class HopcroftMinimizer {
private:
    DFA removeUnreachable(const DFA& dfa) {
        DFA clean = dfa; 
        set<int> reachable; queue<int> q;
        if (dfa.start_state == -1) return clean; 
        
        q.push(dfa.start_state); reachable.insert(dfa.start_state);
        while(!q.empty()) {
            int u = q.front(); q.pop();
            if (dfa.transitions.count(u)) {
                for (auto const& pair : dfa.transitions.at(u)) {
                    int v = pair.second;
                    if (reachable.find(v) == reachable.end()) {
                        reachable.insert(v); q.push(v);
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

public:
    DFA minimize(DFA inputDFA) {
        DFA dfa = removeUnreachable(inputDFA);
        if (dfa.states.empty()) return dfa;

        set<set<int>> P;
        set<int> non_finals;
        for(int s : dfa.states) 
            if(!dfa.final_states.count(s)) non_finals.insert(s);
            
        if(!dfa.final_states.empty()) P.insert(dfa.final_states);
        if(!non_finals.empty()) P.insert(non_finals);
        
        set<set<int>> W = P;

        map<int, map<int, set<int>>> inv_trans;
        for(int u : dfa.states)
            for(auto const& pair : dfa.transitions[u])
                inv_trans[pair.second][pair.first].insert(u);

        while(!W.empty()) {
            set<int> A = *W.begin(); W.erase(W.begin());
            for(int c : dfa.alphabet) {
                set<int> X;
                for(int a_node : A) 
                    for(int pre : inv_trans[a_node][c]) X.insert(pre);
                
                if(X.empty()) continue;

                vector<set<int>> to_remove;
                vector<pair<set<int>, set<int>>> to_add;

                for(const set<int>& Y : P) {
                    set<int> inter, diff;
                    set_intersection(Y.begin(), Y.end(), X.begin(), X.end(), inserter(inter, inter.begin()));
                    set_difference(Y.begin(), Y.end(), X.begin(), X.end(), inserter(diff, diff.begin()));

                    if(!inter.empty() && !diff.empty()) {
                        to_remove.push_back(Y);
                        to_add.push_back({inter, diff});
                        if(W.count(Y)) {
                            W.erase(Y); W.insert(inter); W.insert(diff);
                        } else {
                            if(inter.size() <= diff.size()) W.insert(inter);
                            else W.insert(diff);
                        }
                    }
                }
                for(auto& rem : to_remove) P.erase(rem);
                for(auto& add : to_add) { P.insert(add.first); P.insert(add.second); }
            }
        }

        // Tái tạo DFA
        DFA minDFA;
        minDFA.input_char_to_id = dfa.input_char_to_id;
        minDFA.input_id_to_char = dfa.input_id_to_char;
        minDFA.alphabet = dfa.alphabet;

        map<int, string> old_to_new_name;
        map<string, int> new_name_to_rep;

        for(const set<int>& group : P) {
            string name = "{";
            bool f = true;
            int rep = *group.begin();
            bool is_s = false, is_f = false;
            
            vector<string> sub_names;
            for(int s : group) {
                sub_names.push_back(dfa.state_id_to_name[s]);
                if(s == dfa.start_state) is_s = true;
                if(dfa.final_states.count(s)) is_f = true;
            }
            sort(sub_names.begin(), sub_names.end());
            
            for(const string& n : sub_names) {
                if(!f) name += ",";
                name += n;
                f = false;
            }
            name += "}";

            minDFA.getStateID(name);
            if(is_s) minDFA.setStart(name);
            if(is_f) minDFA.addFinal(name);
            
            new_name_to_rep[name] = rep;
            for(int s : group) old_to_new_name[s] = name;
        }

        for(auto const& pair : new_name_to_rep) {
            string new_name = pair.first;
            int rep = pair.second;
            if(dfa.transitions.count(rep)) {
                for(auto const& trans_pair : dfa.transitions.at(rep)) {
                    int c = trans_pair.first;
                    int target_old = trans_pair.second;
                    string target_new = old_to_new_name[target_old];
                    minDFA.addTransition(new_name, minDFA.input_id_to_char[c], target_new);
                }
            }
        }
        return minDFA;
    }
};

int main(int argc, char* argv[]) {
    // Cho phép nhập tên file từ dòng lệnh, mặc định là input.json
    string inputFile = (argc > 1) ? argv[1] : "input.json";
    string outputFile = "output_hopcroft.json";

    cout << "1. Doc file: " << inputFile << "..." << endl;
    
    // Kiểm tra file tồn tại bằng cách mở thử
    ifstream check(inputFile);
    if (!check.good()) {
        cout << "LOI: Khong tim thay file " << inputFile << endl;
        cout << "Hay tao file nay truoc khi chay chuong trinh!" << endl;
        return 1;
    }
    check.close();

    try {
        DFA myDFA = loadDFA_JSON(inputFile);
        cout << "   -> Da doc thanh cong DFA voi " << myDFA.states.size() << " trang thai." << endl;

        cout << "2. Chay thuat toan Hopcroft..." << endl;
        HopcroftMinimizer solver;
        DFA minDFA = solver.minimize(myDFA);

        cout << "3. Ghi ket qua ra: " << outputFile << "..." << endl;
        exportDFA_JSON(minDFA, outputFile);

        cout << "THANH CONG! Ket qua:" << endl;
        cout << "- File input: " << inputFile << endl;
        cout << "- File output: " << outputFile << endl;
        cout << "- Giam tu " << myDFA.states.size() << " trang thai xuong " << minDFA.states.size() << "." << endl;

    } catch (const std::exception& e) {
        cerr << "LOI KHONG MONG MUON: " << e.what() << endl;
        return 1;
    }

    return 0;
}