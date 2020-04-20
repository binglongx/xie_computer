
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>

#include "parser.h"


int MACHINE_CODE_START = 0x1000; // first machine instruction starts here.

bool parse_naked_reg_or_num(const std::string& str, int& flag, int& operand)
{
    auto reg = upper(str);
    auto& register_map = getRegisterMap();
    auto reg_it = register_map.find(reg);
    if( reg_it != register_map.end() )
    {
        if( reg=="PC" || reg=="SR" )
            return false;   // do not allow literal use of these two registers.
        flag = 0;
        operand = static_cast<int>(reg_it->second);
    }
    else
    {
        if(str.empty())
            return false;
        
        // must be num
        // TODO: check if operand is realy a number literal.
        flag = 1;
        operand = string_to_number(str);
    }
    return true;
}

bool assemble(SourceFile& source, const std::string& binFilePath)
{
    bool ok;
    auto& instruction_map = getInstructionMap();
    auto& register_map = getRegisterMap();

    // first pass: handle labels
    // the line tokens will have label removed.
    std::cout << "Processing labels and comments..." << std::endl;
    std::map<std::string, int> label_map;       // the label instruction_number map
    int global_instruction_line_number = 0;
    ok = for_each_line(source, [&](std::string& filePath, CodeLine& line){
        std::vector<std::string> tokens = tokenize(line.regularized);
        std::string label;
        if( !detect_and_remove_label_for_line(tokens, label) )
        {
            std::cout << "Syntax error: invalid label: " << filePath << ", Line: " << line.number << std::endl
                <<"    " << line.original << std::endl;
            return false;
        }
        if( !label.empty() )
        {
            auto it = label_map.find(label);
            if( it==label_map.end() )
            {
                //label_map[label] = global_instruction_line_number;
                label_map[label] = MACHINE_CODE_START + global_instruction_line_number * 4;
            }
            else
            {
                std::cout << "Syntax error: duplicate label: `" << label << "` : " << filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
        }
        
        if( !tokens.empty() )
        {
            if( instruction_map.find(upper(tokens.front())) != instruction_map.end() )
            {
                // valid instruction
                ++ global_instruction_line_number;
            }
            else
            {
                // currently we only allow non-empty label-removed line to be instruction line
                std::cout << "Syntax error: unrecognized instruction: `" << tokens.front() << "` : " << filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
        }
        line.tokens = tokens;
        return true;
    });
    if( !ok )
        return false;
    
    std::cout << "Labels processed: " << label_map.size() << std::endl;
    if( !label_map.empty() )
    {
        std::cout << "---------------------------------------------" << std::endl;
        for (auto& entry : label_map)
            std::cout << integer_as_hex(entry.second) << " = " << entry.first << ":" << std::endl;
        std::cout << "---------------------------------------------" << std::endl;
    }

    // second pass: handle instructios in line tokens.
    std::cout << "Assembling instructions..." << std::endl;
    std::vector<int> instructions;
    instructions.reserve(global_instruction_line_number+1);
    ok = for_each_line(source, [&](std::string& filePath, CodeLine& line){
        std::vector<std::string>& ops = line.tokens;
        if( ops.empty() )
            return true; // skip to next line

        auto instr = upper(ops[0]);
        auto instr_it = instruction_map.find(instr);
        if( instr_it == instruction_map.end() )
        {
            assert(false);  // no such instruction; but this should be caught at first pass.
            return false;
        }
        
        int opcode = static_cast<int>(instr_it->second.opcode);
        int flag = 0;
        int operand1 = 0;
        int operand2 = 0;
        if( instr_it->second.operandCount == 0 )
        {
            // zero operand instruction
            if( ops.size()!=1 )
            {
                std::cout << "Syntax error: instruction cannot have operands: "<< filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
        }
        else if( instr_it->second.operandCount == 1 )
        {
            // single operand instruction
            if( ops.size()!=2 )
            {
                std::cout << "Syntax error: only one operand allowed for instruction: "<< filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
            if( instr=="INC" || instr=="DEC" || instr=="NOT" || instr=="POP" )
            {
                // naked reg operand only
                auto reg = upper(ops[1]);
                auto reg_it = register_map.find(reg);
                if( reg_it == register_map.end() )
                {
                    std::cout << "Syntax error: invalid register: " << ops[1] << " : " << filePath << ", Line: " << line.number << std::endl
                        <<"    " << line.original << std::endl;
                    return false;
                }
                operand2 = static_cast<int>(reg_it->second);
            }
            else if( instr=="JPE" || instr=="JPL" || instr=="JPG" || instr=="JMP"|| instr=="CLL" )
            {
                // label, [label], or [reg].
                auto operand = ops[1];
                bool is_reg = false;
                if( operand.size()>=2 && operand.front()=='[' && operand.back()==']' )
                {
                    operand.pop_back();
                    operand.erase(0, 1);
                    auto reg = upper(operand);
                    auto reg_it = register_map.find(reg);
                    if( reg_it != register_map.end() )
                    {
                        is_reg = true;
                        operand2 = static_cast<int>(reg_it->second);
                    }
                }
                if( !is_reg )
                {
                    // must be label.
                    auto label_it = label_map.find(operand);
                    if( label_it == label_map.end() )
                    {
                        std::cout << "Syntax error: unrecognized label: " << operand << " : " << filePath << ", Line: " << line.number << std::endl
                            <<"    " << line.original << std::endl;
                        return false;
                    }
                    else
                    {
                        flag = 1;
                        operand2 = label_it->second;
                    }
                }
            }
            else if( instr=="PSH" )
            {
                // naked reg or num
                if( ! parse_naked_reg_or_num(ops[1], flag, operand2) )
                {
                    std::cout << "Syntax error: operand must be register or number: " << filePath << ", Line: " << line.number << std::endl
                        <<"    " << line.original << std::endl;
                    return false;
                }
            }
        }
        else if( instr_it->second.operandCount == 2 )
        {
            // double operand instruction
            //  remove optional comma
            if( ops.size()>=3 && ops[2]=="," )
                ops.erase(ops.begin()+2);
            if( ops.size()>=3 )
            {
                if( ops[1].back()==',' )
                {
                    ops[1].pop_back();
                    if( ops[1].empty() )
                    {
                        std::cout << "Syntax error: instruction needs 2 operands: "<< filePath << ", Line: " << line.number << std::endl
                            <<"    " << line.original << std::endl;
                        return false;
                    }
                }
                if( ops[2].front()==',' )
                {
                    ops[2].erase(0, 1);
                    if( ops[2].empty() )
                        ops.pop_back();
                }
            }
            
            if( ops.size() != 3 )
            {
                std::cout << "Syntax error: instruction needs 2 operands: "<< filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }

            // parse reg1
            auto reg1 = upper(ops[1]);
            auto reg1_it = register_map.find(reg1);
            if( reg1_it == register_map.end() )
            {
                std::cout << "Syntax error: invalid register: " << ops[1] << " : " << filePath << ", Line: " << line.number << std::endl
                    <<"    " << line.original << std::endl;
                return false;
            }
            operand1 = static_cast<int>(reg1_it->second);

            // parse operand2
            if( instr=="LDB" || instr=="STB" || instr=="LDS" || instr=="STS" )
            {
                // operand2: [reg] or [mem]
                auto operand = ops[2];
                if( operand.size()>=2 && operand.front()=='[' && operand.back()==']' )
                {
                    operand.pop_back();
                    operand.erase(0, 1);
                    auto reg = upper(operand);
                    auto reg_it = register_map.find(reg);
                    if( reg_it != register_map.end() )
                    {
                        operand2 = static_cast<int>(reg_it->second);
                    }
                    else
                    {
                        // must be [mem]
                        // TODO: check if operand is realy a number literal.
                        flag = 1;
                        operand2 = string_to_number(operand);
                    }
                }
                else
                {
                    std::cout << "Syntax error: invalid operand2, needing `[` and `]`: " << ops[2] << " : " << filePath << ", Line: " << line.number << std::endl
                        <<"    " << line.original << std::endl;
                    return false;
                }
            }
            else // if( instr=="" ) // all other 2-operand instruction use operand2 as reg/num.
            {
                // operand2: naked reg or num.
                if( ! parse_naked_reg_or_num(ops[2], flag, operand2) )
                {
                    std::cout << "Syntax error: operand must be register or number: " << filePath << ", Line: " << line.number << std::endl
                        <<"    " << line.original << std::endl;
                    return false;
                }
            }
        }
        else
        {
            assert(false);  // should not have instructions with other operand count
        }

        auto code = assemble_machine_code(opcode, flag, operand1, operand2);
        std::cout << integer_as_hex(code) << "  :  ";
        std::cout << "opc=0x"<< integer_as_hex((uint8_t)(opcode)) << "  f=" << flag << "  op1=" << operand1 << "  op2=" << operand2;
        auto disasmbled = disasemble_machine_code(code, label_map);
        if( !disasmbled.empty() )
            std::cout << "\t// " << disasmbled << std::endl;
        else
            std::cout << "\t// " << "!! Invalid machine code" << std::endl;
        instructions.push_back(code);
        return true;
    });
    if( !ok )
        return false;
    std::cout << "Instructions assembled: " << instructions.size() << ",  size = "<< instructions.size()*sizeof(int) <<" bytes" << std::endl;

    std::ofstream s(binFilePath, std::ios::binary);
    if (!s.is_open())
    {
        std::cout << "Failed to open for write: " << binFilePath << std::endl;
        return false;
    }
    else
    {
        for (int i=0; i<instructions.size(); i++)
        {
            int bin = instructions[i];
            s.write(reinterpret_cast<const char*>(&bin), sizeof(bin));
        }
        s.close();
        std::cout << "Binary file written: " << binFilePath << std::endl;
    }
    return true;
}


// usage: xasm [input_xasm_filepath] [output_obj_filepath]
int main(int argc, const char** argv){
    if (argc != 3 && argc != 4 ){
        std::cout << "Usage: " << argv[0] << " <input.xasm> <output.bin> [extra_include_dirs]" << std::endl;
        std::cout << "   extra_include_dirs: use ; to separate multiple directories, e.g: dir_1;dir_2" << std::endl;
        std::cout << "   extra_include_dirs is optional." << std::endl;
        return 1;
    }

    std::string sourceFilePath = argv[1];
    std::string binFilePath = argv[2];

    std::vector<std::string> extra_include_dirs;
    if( argc==4 )
    {
        std::string combined_extra_include_dirs = argv[3];
        size_t pos = 0;
        for(;;)
        {
            auto loc = combined_extra_include_dirs.find(';', pos);
            if( loc != std::string::npos )
            {
                if( loc>pos )
                    extra_include_dirs.push_back( combined_extra_include_dirs.substr(pos, loc-pos) );
                pos = loc + 1;
            }
            else
                break;
        }
        if( pos<combined_extra_include_dirs.size() )
            extra_include_dirs.push_back( combined_extra_include_dirs.substr(pos) );
    }
    if( !extra_include_dirs.empty() )
    {
        std::cout << "Extra include directories: " << extra_include_dirs.size() << std::endl;
        for(auto& include_dir : extra_include_dirs)
            std::cout << "  " << include_dir << std::endl;
    }
    else
        std::cout << "Extra include directories: None" << std::endl;
    
    //auto abs_path = std::filesystem::absolute(p);
    
    auto currentDir = std::filesystem::current_path();
    std::cout << "Current directory: " << currentDir << std::endl;
    
    Loader loader;
    loader.currentDir = currentDir;
    for(auto& dir : extra_include_dirs)
        loader.extraIncludeDirs.push_back(dir);
    
    SourceFile file;
    if( ! loader.load(sourceFilePath, file) )
        return 2;
    
    std::cout << "Source code file loaded: " << sourceFilePath << std::endl;
    if( !assemble(file, binFilePath))
        return 3;

    return 0;
}
