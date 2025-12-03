# Research-on-Automata-State-Minimization-Methods-and-a-Comparison-with-Hopcroft-s-Algorithm
# Project Structures
DFA-Minimization-Research/
├── .gitignore               
├── README.md                
├── LICENSE                  
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
│   │   └── static/          
│   │
│   └── generator/           
│       └── generate_dfa
│
├── tests/                   
│   ├── correctness/         # Test tính đúng đắn (DFA nhỏ)
│   │   ├── test_case_01.json
│   │   └── expected_01.json
│   └── stress/              # Test hiệu năng (DFA lớn)
│       └── large_dfa_1000.json
│
├── experiments/             
│   ├── raw_data/            
│   │   ├── benchmark_table_filling.csv
│   │   ├── benchmark_moore.csv
│   │   └── benchmark_hopcroft.csv
│   ├── plots/               
│   │   ├── time_comparison.png
│   │   └── memory_usage.png
│   └── analysis_script.py   # Script Python để vẽ biểu đồ từ CSV
