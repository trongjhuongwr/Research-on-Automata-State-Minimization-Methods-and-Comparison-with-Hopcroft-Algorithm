#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <iomanip> 
#include "json.hpp"
using json = nlohmann::json;


class DFA {
public:
    int N; 
    int Z; 
    int start_state_id = -1; 
    std::set<int> F;
    std::vector<std::vector<int>> delta; 
    
    std::map<int, std::string> id_to_state;
    std::map<int, std::string> id_to_input;

private:
    std::map<std::string, int> state_to_id;
    std::map<std::string, int> input_to_id;

public:
    //đọc file -> DFA
    DFA(const std::string& filename) {
        std::ifstream f(filename);
        if (!f.is_open()) throw std::runtime_error("cant open input file.");

        json data;
        f >> data;

        int state_cnt = 0;
        int input_cnt = 0;

        // đọc trạng thái
        for (const auto& item : data) {
            std::string s_name = item["state_name"];
            if (state_to_id.find(s_name) == state_to_id.end()) {
                state_to_id[s_name] = state_cnt;
                id_to_state[state_cnt] = s_name;
                state_cnt++;
            }

            //đọc hàm chuyển
            for (const auto& t : item["transitions"]) {
                std::string inp = t["input"];
                if (input_to_id.find(inp) == input_to_id.end()) {
                    input_to_id[inp] = input_cnt;
                    id_to_input[input_cnt] = inp;
                    input_cnt++;
                }
            }
        }

        N = state_cnt;
        Z = input_cnt;
        delta.assign(N, std::vector<int>(Z, -1));

        // đọc input -> 0,1 (a,b) gì đó map nó thành id nội bộ
        for (const auto& item : data) {
            std::string u_str = item["state_name"];
            int u = state_to_id[u_str];

            if (item.contains("is_start") && item["is_start"].get<bool>()) {
                start_state_id = u;
            }
            if (item["is_end"].get<bool>()) {
                F.insert(u);
            }

            for (const auto& t : item["transitions"]) {
                int inp = input_to_id[t["input"]];
                int v = state_to_id[t["target_state"]];
                delta[u][inp] = v;
            }
        }
        
        // nếu không có trạng thái bắt đầu thì vô nghĩa
        if (start_state_id == -1) throw std::runtime_error("Erro: cant find start state.");
    }

    //kiểm tra 2 tập trạng thái có tương đương nhau không
    bool isEquivalent(int i, int j, const std::vector<int>& partition) const {
        for (int z = 0; z < Z; z++) {
            if (partition[delta[i][z]] != partition[delta[j][z]]) return false;
        }
        return true;
    }

    //thuật toán moore minimize
    std::vector<int> moore_minimize() const {
        //khởi tạo phân hạch ban đầu theo trạng thái chấp nhận (nhóm có trạng thái kết thúc) và không chấp nhận (còn lại)
        std::vector<int> partition_0(N);
        int accept_id = 1, reject_id = 0;
        for (int i = 0; i < N; i++) partition_0[i] = (F.count(i)) ? accept_id : reject_id;

        //bắt đầu phân hạch 
        while (true) {
            // tạo phân hạch mới
            std::vector<int> partition(N, -1);
            for (int i = 0; i < N; ++i) {
                if (partition[i] != -1) continue;
                partition[i] = i; 
                for (int j = i + 1; j < N; ++j) {
                    if (partition[j] != -1) continue;
                    if (partition_0[i] == partition_0[j] && isEquivalent(i, j, partition_0)) {
                        partition[j] = i;
                    }
                }
            }
            if (partition_0 == partition) break;
            partition_0 = partition;
        }
        return partition_0;
    }

    // xuất kết quả
    void exportJSON(const std::vector<int>& partition, const std::string& outFilename) {
        // gom nhóm theo phân hạch đã làm 
        std::map<int, std::vector<int>> groups;
        for(int i = 0; i < N; i++) {
            groups[partition[i]].push_back(i);
        }

        // map tên nhóm  vd: ID 0 -> tên "{A,B}"
        std::map<int, std::string> group_names;
        for(auto const& [gid, members] : groups) {
            std::string name = "{";
            for(size_t k = 0; k < members.size(); k++) {
                name += id_to_state[members[k]];
                if (k < members.size() - 1) name += ",";
            }
            name += "}";
            group_names[gid] = name;
        }

        // build kết quả
        json output = json::array();

        for(auto const& [gid, members] : groups) {
            json state_obj;
            
            // tên nhóm đã tạo
            state_obj["state_name"] = group_names[gid];

            // is_start và is_end
            bool is_grp_start = false;
            bool is_grp_end = false;
            for(int m : members) {
                if (m == start_state_id) is_grp_start = true;
                if (F.count(m)) is_grp_end = true;
            }
            state_obj["is_start"] = is_grp_start;
            state_obj["is_end"] = is_grp_end;

            int rep = members[0];
            std::vector<json> trans_list;

            // duyệt qua tất cả các input đã biết 
            for (int z = 0; z < Z; z++) {
                // Đích đến cũ
                int old_target = delta[rep][z];
                // Đích đến mới (Group ID của đích đến cũ)
                int new_target_gid = partition[old_target];
                
                json t;
                t["input"] = id_to_input[z];
                t["target_state"] = group_names[new_target_gid]; 
                trans_list.push_back(t);
            }
            
            state_obj["transitions"] = trans_list;
            output.push_back(state_obj);
        }

        std::ofstream o(outFilename);
        if (o.is_open()) {
            o << std::setw(4) << output << std::endl; 
            std::cout << "Export success output file: " << outFilename << std::endl;
        } else {
            std::cerr << "Fail export output file: " << outFilename << std::endl;
        }
    }
};

int main() {
    try {
        DFA myDfa("input.json");
        std::vector<int> result = myDfa.moore_minimize();
        
        // xuất file JSON
        myDfa.exportJSON(result, "output_moore.json");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}