//
//  main.cpp
//  xasm
//
//  Created by Colin Xie on 4/11/20.
//  Copyright © 2020 Colin Xie. All rights reserved.
//
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

int MACHINE_CODE_START = 0x1000; // first machine instruction starts here.

map<string, int> reg_map = {{"RA", 0x0}, {"RB", 0x1}, {"RC", 0x2}, {"RD", 0x3}, {"RE", 0x4}, {"RF", 0x5}, {"SP", 0x10}, {"PC", 0x11}, {"SR", 0x12}};
map<string, int> instr_map = {{"MOV", 0x1}, {"LDS", 0x2}, {"STS", 0x3}, {"ADD", 0x4}, {"SUB", 0x5}, {"MUL", 0x6}, {"DIV", 0x7}, {"MOD", 0x8}, {"INC", 0x9}, {"DEC", 0xA},
{"AND", 0xB}, {"OR_", 0xC}, {"XOR", 0xD}, {"CMP", 0xE}, {"JPE", 0xF}, {"JPL", 0x10}, {"JMP", 0x11}, {"CLL", 0x12}, {"RET", 0x13}, {"HLT", 0x14}, {"PSH", 0x15}, {"POP", 0x16},
    {"NOT", 0x17}, {"LDB", 0x18}, {"STB", 0x19}, {"SHL", 0x1A}, {"SHR", 0x1B}};
map<string, int> mem_map;
vector<string> regs = {"RA", "RB", "RC", "RD", "RE", "RF", "PC", "SP", "SR"};

int assemble_machine_code(int opcode, int flag, int operand1, int operand2){
    int bin;
    bin = opcode << 24;
    bin += flag << 23;
    bin += operand1 << 16;
    bin += operand2;
    return bin;
}

vector<string> tokenize(string str){
    vector<string> tokens;
    istringstream stream{str};
    string token;
    while (stream >> token)
        tokens.push_back(token);
    return tokens;
}

void upper(string& str){
    for (char& letter : str)
        letter = toupper(letter);
}

// str could be "1288", "0x3000", "0X3000", "0b00110011", "0B00110011", "'a'".
int string_to_number(string str)
{
    int val = 0;
    if( str.size()>2 )
    {
        string prefix = str.substr(0, 2);
        upper(prefix);
        if( prefix=="0X" )
        {
            string proper = str.substr(2);
            std::istringstream ss(proper);
            ss >> std::hex >> val;
            return val;
        }
        else if( prefix=="0B" )
        {
            string proper = str.substr(2);
            int term = 1;
            for (int i=0; i<proper.length(); i++)
            {
                int ind = (int)(proper.size()-1-i);
                char bit = proper[ind];
                if( bit=='1' )
                    val += term;
                else if( bit != '0' )
                {
                    // there is something wrong
                }
                term *= 2;
            }
            return val;
        }
        else if(( str.front() == '\'' && str.back() == '\'') || ( str.front() == '"' && str.back() == '"'))
        {
            std::string proper = str.substr(1, str.size()-2);
            for(int i=0; i<proper.size(); ++i)
            {
                int ind = int(proper.size()-1-i);
                unsigned int ch = (unsigned char)(proper[ind]);
                val |= (ch << (8*ind));
            }
            return val;
        }
    }
    val = stoi(str);
    return val;
}

// if the tokens have comments, they will be removed.
void remove_comments(vector<string>& tokens)
{
    for(auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        auto& token = *it;
        size_t loc = token.find("//");
        if( string::npos != loc )
        {
            if(loc == 0)
            {
                // that whole token is thrown away
                tokens.erase(it, tokens.end());
            }
            else
            {
                // some part of this toke is still good.
                token.resize(loc);
                tokens.erase(it+1, tokens.end());
            }
            return;
        }
    }
}

// input tokens must have stripped comments
// return non-empty label string excluding ':' if this is a label line
string detect_label_line(vector<string> tokens)
{
    if (tokens.size() == 1)
    {
        if( tokens.front().back()==':' )
        {
            string label = tokens.front();
            label.pop_back();
            return label;
        }
    }
    else if( tokens.size() == 2 )
    {
        if (tokens[1] == ":")
            return tokens.front();
    }
    return {};
}

// usage: xasm [input_xasm_filepath] [output_obj_filepath]
int main(int argc, const char** argv){
    if (argc != 3){
        cout << "Too many or too little arguments";
        return 1;
    }
    int lines=0;
    vector<int> instructions;
    string source = argv[1];
    ifstream s1(source);
    if (!s1.is_open()) {
        cout << "failed to open " << source << '\n';
    } else {

        // first pass: handle labels (comments always handled)
        int skipped_line_count = 0;
        while (true){
            string line;
            if (!getline(s1, line))
                break;
            upper(line);
            vector<string> tokens = tokenize(line);
            remove_comments(tokens);

            // remove empty line or its equivalent, i.e., whole line comments for calculating label location.
            if( tokens.empty() )
            {
                skipped_line_count ++;
            }
            else if( string label = detect_label_line(tokens); !label.empty() )
            {
                skipped_line_count ++;
                mem_map[label] = MACHINE_CODE_START + (lines+1-skipped_line_count) * 4;
            }
            lines++;
        }

        cout << "Found labels:\n";
        cout << "-------------------------------\n";
        for (auto& entry : mem_map)
            cout << hex << entry.second << " = " << entry.first << ":" << endl;
        cout << "-------------------------------\n";

        s1.clear();
        s1.seekg(0);
        while (true){
            // second pass: handle instructios
            string line;
            vector<string> ops;
            string substring="";
            if( !getline(s1, line) )
                break;
            cout << "Read: " << line << endl;
            upper(line);
            ops = tokenize(line);
            remove_comments(ops);
            if (ops.size() == 0)
                continue; // skip to next line

            if( !detect_label_line(ops).empty() )
                continue; // label line.

            // erase commas between operands
            for (int i=0; i<ops.size(); i++){
                if (ops[i].back() == ',')
                    ops[i].pop_back();
            }

            auto instruction_name = ops[0];
            auto instr_it = instr_map.find(instruction_name);
            if( instr_it == instr_map.end() )
            {
                cout << "Error: unrecognzed instruction: " << instruction_name << std::endl;
                return -1;
            }

            int flag = 0;
            int opcode= instr_it->second;
            int opers[2] = {0, 0};

            //int operands;
            for (int i=1; i<ops.size(); i++){
                bool isreg = false;
                for (int j=0; j<regs.size(); j++){
                    if (ops[i] == regs[j])
                        isreg = true;
                    else if (ops[i].substr(1, ops[i].length()-2) == regs[j]){
                        ops[i].pop_back();
                        ops[i].erase(ops[i].begin());
                        isreg = true;
                    }
                }
                if (isreg && ops[0] != "INC" && ops[0] != "DEC" && ops[0] != "NOT" && ops[0] != "PSH" && ops[0] != "POP")
                    opers[i-1] = reg_map[ops[i]];
                else if (isreg)
                    opers[1] = reg_map[ops[i]];
                else if (ops[i].front() == '[' && ops[i].back() == ']'){
                    flag = 1;
                    ops[i].pop_back();
                    ops[i].erase(ops[i].begin());
                    if (mem_map.find(ops[i]) != mem_map.end())
                        opers[1] = mem_map[ops[i]];
                    else {
                        cout << ops[i] << endl;
                        opers[1] = string_to_number(ops[i]);
                    }
                }else if (ops[0] == "MOV" || ops[0] == "ADD" || ops[0] == "SUB" || ops[0] == "MUL" || ops[0] == "DIV" || ops[0] == "MOD" || ops[0] == "AND" || ops[0] == "OR_" || ops[0] == "XOR" || ops[0] == "CMP" || ops[0] == "JPE" || ops[0] == "JPL" || ops[0] == "JMP" || ops[0] == "CLL"){
                    flag = 1;
                    
                    opers[1] = string_to_number(ops[i]);
                }
            }
            cout << opcode << " " << opers[0] << " " << opers[1] << "\n";
            instructions.push_back(assemble_machine_code(opcode, flag, opers[0], opers[1]));
            cout << hex << instructions.back() << endl;
        }
    }
    string xasm = argv[2];
    ofstream s(xasm, ios::binary);
    if (!s.is_open()) {
        cout << "failed to open " << xasm << '\n';
    } else {
        for (int i=0; i<instructions.size(); i++){
            int bin = instructions[i];
            s.write(reinterpret_cast<const char*>(&bin), sizeof(bin));
        }
        s.close();
    }
    return 0;
}
