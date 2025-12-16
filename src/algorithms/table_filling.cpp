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
#include <queue>
#include <iomanip>
#include "json.hpp" 

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

// === PHẦN 2: LOGIC TABLE FILLING ===
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
    
    // Hàm tái tạo DFA dùng chung logic tái tạo nhóm
    DFA reconstructDFA(const DFA& oldDFA, const vector<int>& group_id) {
        DFA newDFA;
        newDFA.input_char_to_id = oldDFA.input_char_to_id;
        newDFA.input_id_to_char = oldDFA.input_id_to_char;
        newDFA.alphabet = oldDFA.alphabet;

        map<int, vector<int>> groups;
        for(int u : oldDFA.states) groups[group_id[u]].push_back(u);

        map<int, string> new_group_names;

        for(auto const& pair : groups) {
            int g_id = pair.first;
            string name = "{";
            vector<string> names;
            bool is_s = false, is_f = false;
            for(int u : pair.second) {
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

        for(auto const& pair : groups) {
            int g_id = pair.first;
            int rep = pair.second[0];
            string src_name = new_group_names[g_id];
            if(oldDFA.transitions.count(rep)) {
                for(auto const& t : oldDFA.transitions.at(rep)) {
                    newDFA.addTransition(src_name, newDFA.input_id_to_char[t.first], new_group_names[group_id[t.second]]);
                }
            }
        }
        return newDFA;
    }

public:
    DFA minimize(DFA inputDFA) {
        DFA dfa = removeUnreachable(inputDFA);
        if (dfa.states.empty()) return dfa;

        int n = dfa.state_counter;
        // Bảng đánh dấu: marked[u][v] = true nếu u và v phân biệt
        vector<vector<bool>> marked(n, vector<bool>(n, false));

        // 1. Bước cơ sở: Đánh dấu cặp (Final, Non-Final)
        for(int i : dfa.states) {
            for(int j : dfa.states) {
                if(i < j) {
                    bool f1 = dfa.final_states.count(i);
                    bool f2 = dfa.final_states.count(j);
                    if(f1 != f2) marked[i][j] = true;
                }
            }
        }

        // 2. Vòng lặp: Đánh dấu nếu chuyển đến cặp đã đánh dấu
        bool changed = true;
        while(changed) {
            changed = false;
            for(int i : dfa.states) {
                for(int j : dfa.states) {
                    if(i < j && !marked[i][j]) {
                        for(int c : dfa.alphabet) {
                            int t1 = -1, t2 = -1;
                            if(dfa.transitions[i].count(c)) t1 = dfa.transitions[i][c];
                            if(dfa.transitions[j].count(c)) t2 = dfa.transitions[j][c];
                            
                            // Nếu một cái có cạnh, một cái không -> Phân biệt
                            if((t1 == -1 && t2 != -1) || (t1 != -1 && t2 == -1)) {
                                marked[i][j] = true;
                                changed = true;
                                break;
                            }
                            
                            // Nếu cả hai chuyển đến cặp đã mark
                            if(t1 != -1 && t2 != -1) {
                                int u = min(t1, t2);
                                int v = max(t1, t2);
                                if(u != v && marked[u][v]) {
                                    marked[i][j] = true;
                                    changed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 3. Gom nhóm các trạng thái tương đương (BFS/DFS trên các cặp chưa mark)
        vector<int> group(n, -1);
        int group_count = 0;
        
        for(int i : dfa.states) {
            if(group[i] == -1) {
                group[i] = group_count;
                // Tìm tất cả j tương đương với i
                queue<int> q; q.push(i);
                while(!q.empty()) {
                    int u = q.front(); q.pop();
                    for(int v : dfa.states) {
                        if(group[v] == -1) {
                            int min_uv = min(u, v);
                            int max_uv = max(u, v);
                            if(min_uv == max_uv) continue;
                            
                            // Nếu không bị đánh dấu -> Tương đương
                            if(!marked[min_uv][max_uv]) {
                                group[v] = group_count;
                                q.push(v);
                            }
                        }
                    }
                }
                group_count++;
            }
        }

        return reconstructDFA(dfa, group);
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