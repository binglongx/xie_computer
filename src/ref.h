#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>

enum class Opcode : uint8_t
{
    MOV = 0x01,
    LDB = 0x02,
    STB = 0x03,
    LDS = 0x04,
    STS = 0x05,

    ADD = 0x10,
    SUB = 0x11,
    MUL = 0x12,
    DIV = 0x13,
    MOD = 0x14,
    INC = 0x15,
    DEC = 0x16,
    
    AND = 0x20,
    OR_ = 0x21,
    XOR = 0x22,
    NOT = 0x23,
    SHL = 0x24,
    SHR = 0x25,

    CMP = 0x30,
    JPE = 0x31,
    JPL = 0x32,
    JPG = 0x33,
    JMP = 0x34,
    CLL = 0x35,
    RET = 0x36,
    HLT = 0x37,
    
    PSH = 0x40,
    POP = 0x41,
    
    KBD = 0x70,
    DSP = 0x71,
    DPL = 0x72
};

enum class Register : uint8_t
{
    RA = 0x00,
    RB = 0x01,
    RC = 0x02,
    RD = 0x03,
    RE = 0x04,
    RF = 0x05,
    
    SP = 0x10,
    
    PC = 0x11,
    SR = 0x12
};

struct InstructionData
{
    Opcode  opcode;
    int     operandCount;   // 2, 1, 0
    bool    operand2Memory; // if having operand, is operand2 true for load/store/jump/call/dpl instructions: need memory location (i.e., [])
};

inline const std::map<std::string, InstructionData>& getInstructionMap()
{
    static const std::map<std::string, InstructionData> s_instruction_map = {
        //                  // reg, reg/num/mem
        {"MOV", {Opcode::MOV, 2, false}},
        {"LDB", {Opcode::LDB, 2, true }},
        {"STB", {Opcode::STB, 2, true }},
        {"LDS", {Opcode::LDS, 2, true }},
        {"STS", {Opcode::STS, 2, true }},
        
        //                  // reg, reg/num
        {"ADD", {Opcode::ADD, 2, false}},
        {"SUB", {Opcode::SUB, 2, false}},
        {"MUL", {Opcode::MUL, 2, false}},
        {"DIV", {Opcode::DIV, 2, false}},
        {"MOD", {Opcode::MOD, 2, false}},
        {"INC", {Opcode::INC, 1, false}},   // (reg only)
        {"DEC", {Opcode::DEC, 1, false}},   // (reg only)
        
        //                  // reg, reg/num
        {"AND", {Opcode::AND, 2, false}},
        {"OR_", {Opcode::OR_, 2, false}},
        {"XOR", {Opcode::XOR, 2, false}},
        {"NOT", {Opcode::NOT, 1, false}},   // (reg only)
        {"SHL", {Opcode::SHL, 2, false}},
        {"SHR", {Opcode::SHR, 2, false}},

        //                  // reg/mem
        {"CMP", {Opcode::CMP, 2}},   // (reg, reg/num)
        {"JPE", {Opcode::JPE, 1, true }},
        {"JPL", {Opcode::JPL, 1, true }},
        {"JPG", {Opcode::JPG, 1, true }},
        {"JMP", {Opcode::JMP, 1, true }},
        {"CLL", {Opcode::CLL, 1, true }},
        {"RET", {Opcode::RET, 0, true }},
        {"HLT", {Opcode::HLT, 0, false}},

        {"PSH", {Opcode::PSH, 1, false}},   // reg/num
        {"POP", {Opcode::POP, 1, false}},   // (reg only)
        
        {"KBD", {Opcode::KBD, 0, false}},
        {"DSP", {Opcode::DSP, 0, false}},
        {"DPL", {Opcode::DPL, 1, true }}
    };
    return s_instruction_map;
}

inline const std::map<std::string, Register>& getRegisterMap()
{
    static const std::map<std::string, Register> s_register_map = {
        {"RA", Register::RA},
        {"RB", Register::RB},
        {"RC", Register::RC},
        {"RD", Register::RD},
        {"RE", Register::RE},
        {"RF", Register::RF},
        {"SP", Register::SP},
        {"PC", Register::PC},
        {"SR", Register::SR},
    };
    return s_register_map;
}

//===============================================================================================
//===============================================================================================

template<class T>
inline std::string integer_as_hex(T x)
{
    std::ostringstream os;
    os << std::right << std::setfill('0') << std::setw((sizeof(x)*2)) << std::hex;
    if constexpr( sizeof(T)==1 )
        os << uint16_t(uint8_t(x));     // it seems uint8_t is not handled correctly by STL.
    else
        os << x;
    return os.str();
}

//===============================================================================================
//===============================================================================================

inline std::string findInstructionName(Opcode opc)
{
    std::string instruction;
    auto& instruction_map = getInstructionMap();
    for(auto& entry : instruction_map)
    {
        if( entry.second.opcode == opc )
        {
            instruction = entry.first;
            break;
        }
    }
    return instruction;
}

inline std::string findRegisterName(Register reg)
{
    std::string name;
    auto& register_map = getRegisterMap();
    for(auto& entry : register_map)
    {
        if( entry.second == reg )
        {
            name = entry.first;
            break;
        }
    }
    return name;
}

inline std::string findLabel(const std::map<std::string, int>& label_map, int loc)
{
    std::string label;
    for(auto& entry : label_map)
    {
        if( entry.second == loc )
        {
            label = entry.first;
            break;
        }
    }
    return label;
}

inline std::string decodeOperand2(uint16_t operand2, bool flag, bool operand2Memory, const std::map<std::string, int>& label_map = {})
{
    std::string operand2Name;
    if( flag )
    {
        // num/mem/label
        operand2Name = findLabel(label_map, (int16_t)operand2);
        if( operand2Name.empty() )
        {
            operand2Name = "0x";
            operand2Name += integer_as_hex(operand2);
        }
    }
    else
    {
        // reg
        assert( operand2 <= uint16_t(0x7f) );
        Register reg = (Register)(operand2);
        operand2Name = findRegisterName(reg);
        assert( !operand2Name.empty() );
    }
    if( !operand2Name.empty() && operand2Memory )
        operand2Name = std::string("[") + operand2Name + "]";
    return operand2Name;
}

//===============================================================================================
//===============================================================================================

uint32_t assemble_machine_code(uint8_t opcode, bool flag, uint8_t operand1, uint16_t operand2){
    uint32_t bin;
    bin = uint32_t(opcode) << 24;
    bin += uint32_t(flag) << 23;
    bin += uint32_t(operand1) << 16;
    bin += operand2;
    return bin;
}

std::string disasemble_machine_code(int machine_code, const std::map<std::string, int>& label_map = {})
{
    Opcode opcode = static_cast<Opcode>((uint8_t)(machine_code >> 24));
    bool flag = machine_code & (1 << 23);
    uint8_t operand1 = (uint8_t)(machine_code >> 16) & 0x7f;
    uint16_t operand2 = (uint16_t)(machine_code);
    
    // find the instruction
    std::string instructionName = findInstructionName(opcode);
    if( !instructionName.empty() )
    {
        const InstructionData& data = getInstructionMap().find(instructionName)->second;
        if( data.operandCount == 0 )
        {
            return instructionName;
        }
        else if( data.operandCount == 1 )
        {
            std::string operand2Name = decodeOperand2(operand2, flag, data.operand2Memory, label_map);
            if( !operand2Name.empty() )
                return instructionName + " " + operand2Name;
        }
        else if( data.operandCount == 2 )
        {
            std::string operand1Name = findRegisterName((Register)(operand1));
            std::string operand2Name = decodeOperand2(operand2, flag, data.operand2Memory, label_map);
            if( !operand1Name.empty() && !operand2Name.empty() )
                return instructionName + " " + operand1Name + ", " + operand2Name;
        }
        else
        {
            assert(false);
        }
    }
    else
    {
        assert(false);
    }
    return {};
}
