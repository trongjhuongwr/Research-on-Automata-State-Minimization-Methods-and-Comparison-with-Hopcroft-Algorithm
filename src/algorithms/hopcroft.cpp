// Hướng dẫn chạy:
// 1. Đảm bảo có file json.hpp trong cùng thư mục với file hopcroft.cpp (https://github.com/nlohmann/json/releases/latest/download/json.hpp)
// 2. Chạy lệnh biên dịch (ví dụ với g++):
//    g++ hopcroft.cpp -o app -static
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
#include "json.hpp" // File json.hpp phải nằm cùng thư mục

using json = nlohmann::json;
using namespace std;

// === PHẦN 1: CẤU TRÚC DFA ===
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

// Hàm đọc/ghi JSON
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

// === PHẦN 2: LOGIC HOPCROFT ===
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

public:
    DFA minimize(DFA inputDFA) {
        DFA dfa = removeUnreachable(inputDFA);
        if (dfa.states.empty()) return dfa;

        set<set<int>> P;
        set<int> non_finals;
        for(int s : dfa.states) if(!dfa.final_states.count(s)) non_finals.insert(s);
        if(!dfa.final_states.empty()) P.insert(dfa.final_states);
        if(!non_finals.empty()) P.insert(non_finals);
        
        set<set<int>> W = P;
        map<int, map<int, set<int>>> inv;
        for(int u : dfa.states)
            for(auto const& p : dfa.transitions[u]) inv[p.second][p.first].insert(u);

        while(!W.empty()) {
            set<int> A = *W.begin(); W.erase(W.begin());
            for(int c : dfa.alphabet) {
                set<int> X;
                for(int u : A) for(int pre : inv[u][c]) X.insert(pre);
                if(X.empty()) continue;

                vector<set<int>> rem;
                vector<pair<set<int>, set<int>>> add;
                for(const set<int>& Y : P) {
                    set<int> i, d;
                    set_intersection(Y.begin(), Y.end(), X.begin(), X.end(), inserter(i, i.begin()));
                    set_difference(Y.begin(), Y.end(), X.begin(), X.end(), inserter(d, d.begin()));
                    if(!i.empty() && !d.empty()) {
                        rem.push_back(Y); add.push_back({i, d});
                        if(W.count(Y)) { W.erase(Y); W.insert(i); W.insert(d); }
                        else { if(i.size() <= d.size()) W.insert(i); else W.insert(d); }
                    }
                }
                for(auto& r : rem) P.erase(r);
                for(auto& a : add) { P.insert(a.first); P.insert(a.second); }
            }
        }
        
        // Tái tạo DFA
        DFA minDFA;
        minDFA.input_char_to_id = dfa.input_char_to_id;
        minDFA.input_id_to_char = dfa.input_id_to_char;
        minDFA.alphabet = dfa.alphabet;
        map<int, string> mapping;
        map<string, int> rep_map;
        for(const set<int>& g : P) {
            string name = "{"; 
            int rep = *g.begin();
            bool is_s = false, is_f = false;
            vector<string> nlist;
            for(int s : g) {
                nlist.push_back(dfa.state_id_to_name.at(s));
                if(s == dfa.start_state) is_s = true;
                if(dfa.final_states.count(s)) is_f = true;
            }
            sort(nlist.begin(), nlist.end());
            for(size_t i=0; i<nlist.size(); ++i) name += (i==0?"":",") + nlist[i];
            name += "}";
            minDFA.getStateID(name);
            if(is_s) minDFA.setStart(name);
            if(is_f) minDFA.addFinal(name);
            rep_map[name] = rep;
            for(int s : g) mapping[s] = name;
        }
        for(auto const& pair : rep_map) {
            if(dfa.transitions.count(pair.second)) {
                for(auto const& t : dfa.transitions.at(pair.second)) {
                    minDFA.addTransition(pair.first, minDFA.input_id_to_char[t.first], mapping[t.second]);
                }
            }
        }
        return minDFA;
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