#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#include "ref.h"

using namespace std;


using Instruction = uint32_t;
using BYTE = char;
int MACHINE_CODE_START = 0x1000; // first machine instruction starts here.
int RAM_SIZE = 0x5000;

struct RAM
{
    RAM(size_t size) : ram(size) {}

    // access
    BYTE* access_byte(int loc)
    {
        return ram.data() + loc;
    }

    short* access_short(int loc)
    {
        BYTE* p = ram.data() + loc;
        return reinterpret_cast<short*>(p);
    }

    int* access_int(int loc)
    {
        BYTE* p = ram.data() + loc;
        return reinterpret_cast<int*>(p);
    }

    Instruction fetch_instruction(int PC)
    {
        return * access_int(PC);
    }

private:
    vector<BYTE> ram;
};

struct RegisterFile
{
    short RA, RB, RC, RD, RE, RF;
    short PC;
    short SP;
    short SR;

    void print()
    {
        cout<<"RA=" << integer_as_hex(RA) <<" ";
        cout<<"RB=" << integer_as_hex(RB) <<" ";
        cout<<"RC=" << integer_as_hex(RC) <<" ";
        cout<<"RD=" << integer_as_hex(RD) <<" ";
        cout<<"RE=" << integer_as_hex(RE) <<" ";
        cout<<"RF=" << integer_as_hex(RF) <<" ";
        cout<<"  SP=" << integer_as_hex(SP) <<" ";
        cout<<"SR=" << integer_as_hex(SR) <<" ";
        cout<<"PC=" << integer_as_hex(PC) <<" ";
    }

    short* getRegister(int operand)
    {
        Register reg = (Register)(operand);
        switch(reg)
        {
            case Register::RA: return &RA;
            case Register::RB: return &RB;
            case Register::RC: return &RC;
            case Register::RD: return &RD;
            case Register::RE: return &RE;
            case Register::RF: return &RF;
            case Register::SP: return &SP;
            default: /*cout << "Error: unrecognized operand for register: " << operand << endl;*/ return nullptr;
        }
    }
};

bool run_instruction(Instruction instruction, RegisterFile& regs, RAM& ram)
{
    int opcode = instruction >> 24;
    int flag = (instruction >> 23) & 0x0001;
    int operand1 = (instruction >> 16) & 0x7F;
    int operand2 = instruction & 0xffff;

    Opcode opc = (Opcode)opcode;
    short num = operand2;
    short* reg1 = regs.getRegister(operand1);
    short* reg2 = nullptr;
    if( flag==0 )
    {
        reg2 = regs.getRegister(operand2);
        num = *reg2;
    }
    short cmp_result;
    switch(opc)
    {
        case Opcode::MOV:
            *reg1 = num;  // perform move
            break;
        case Opcode::LDB:
            *reg1 = *ram.access_byte(num);  // perform load from [reg] to reg
            break;
        case Opcode::STB:
            *ram.access_byte(num) = (char)*reg1;  // store to mem
            break;
        case Opcode::LDS:
            *reg1 = *ram.access_short(num);  // perform load from [reg] to reg
            break;
        case Opcode::STS:
            *ram.access_short(num) = *reg1;  // store to mem
            break;
            
        case Opcode::ADD:
            *reg1 += num;
            break;
        case Opcode::SUB:
            *reg1 -= num;
            break;
        case Opcode::MUL:
            *reg1 *= num;
            break;
        case Opcode::DIV:
            *reg1 /= num;
            break;
        case Opcode::MOD:
            *reg1 %= num;
            break;
        case Opcode::INC:
            (*reg2) ++;
            break;
        case Opcode::DEC:
            (*reg2) --;
            break;
            
        case Opcode::AND:
            *reg1 &= num;
            break;
        case Opcode::OR_:
            *reg1 |= num;
            break;
        case Opcode::XOR:
            *reg1 ^= num;
            break;
        case Opcode::NOT:
            (*reg2) = ~(*reg2);
            break;
        case Opcode::SHL:
            *reg1 <<= num;
            break;
        case Opcode::SHR:
            *reg1 >>= num;
            break;

        case Opcode::CMP:
            if (*reg1 < num)
                cmp_result = 0x02;
            else if (*reg1 == num)
                cmp_result = 0x01;
            else
                cmp_result = 0x00;
            regs.SR = cmp_result;
            break;
        
        case Opcode::JPE:
            if ((regs.SR & 0x03) != 0x01)
                regs.PC = num;
            else
                regs.PC += 4;
            return false;   // control flow instruction
        case Opcode::JPL:
            if ((regs.SR & 0x03) == 0x02)
                regs.PC = num;
            else
                regs.PC += 4;
            return false;   // control flow instruction
        case Opcode::JPG:
            if ((regs.SR & 0x03) == 0x00)
                regs.PC = num;
            else
                regs.PC += 4;
            return false;   // control flow instruction
        case Opcode::JMP:
            regs.PC = num;
            return false;   // control flow instruction
            
        case Opcode::CLL:
            regs.SP -= 2;
            *ram.access_short(regs.SP) = regs.PC+4;
            regs.PC = num;
            return false;   // control flow instruction
        case Opcode::RET:
            regs.PC = *ram.access_short(regs.SP);
            //cout << regs.SP << " " << regs.PC << endl;
            regs.SP += 2;
            return false;   // control flow instruction
        case Opcode::HLT:
            return true;
            
        case Opcode::PSH:
            regs.SP -= 2;
            if (flag == 0)
                *ram.access_short(regs.SP) = *reg2;
            else if (flag == 1){
                num = operand2;
                *ram.access_short(regs.SP) = num;
            }
            break;
        case Opcode::POP:
            *reg2 = *ram.access_short(regs.SP);
            regs.SP += 2;
            break;
            
        case Opcode::KBD:
            break;
        case Opcode::DSP:
            break;
    }
    regs.PC += 4;
    return false;
}

int main(int argc, const char** argv)
{
    if( argc != 2)
    {
        cout << "Usage: " << argv[0] << " [xasm_binary_filepath]" << endl;
        return -1;
    }

    string filepath = argv[1];

    ifstream f(filepath, std::ios::binary);
    if( !f.is_open() )
    {
        cout << "Error: cannot open " << filepath << endl;
        return -2;
    } 

    RAM ram(RAM_SIZE);
    RegisterFile regs;
    // initialize display
    const BYTE fill_display = ' ';
    for(int i=0; i<25*80; ++i)
       *ram.access_byte(0x3000 + i) = fill_display;
    
    f.seekg(0, std::ios_base::end);
    size_t fileLength = f.tellg();
    f.clear();
    f.seekg(0, std::ios_base::beg);
    if( fileLength > RAM_SIZE-MACHINE_CODE_START )
    {
        cout << "Error: File length exceeds simulator ram limit\n";
        return -3;
    }
    f.read(ram.access_byte(MACHINE_CODE_START), fileLength);
    f.close();
    cout << "Bin file read size: " << fileLength << endl;
    for(short index = 0; index<fileLength; index += 4)
    {
        short pc = MACHINE_CODE_START + index;
        cout << " Instruction @" << integer_as_hex(pc) << " " << integer_as_hex(ram.fetch_instruction(pc)) << endl;
    }
    /*
    *ram.access_byte(0x3000) = '0';
    *ram.access_byte(0x3001) = 'a';
    *ram.access_byte(0x3002) = 'b';
    *ram.access_byte(0x3003) = 'c';
    *ram.access_byte(0x3004) = 'd';
    *ram.access_byte(0x3005) = 'e';
    */
    *ram.access_short(0x4000) = 1;
    *ram.access_byte(0x4002) = '0';
    *ram.access_byte(0x4003) = '9';
    *ram.access_byte(0x4004) = '9';
    *ram.access_byte(0x4005) = '9';
    *ram.access_byte(0x4006) = '9';
    // boot our XIE computer
    regs.PC = MACHINE_CODE_START;
    regs.SP = 0x2000;

    while(true)
    {
        int32_t instruction = ram.fetch_instruction(regs.PC);
        regs.print(); cout<<endl;
        cout << " Instruction @" << integer_as_hex(regs.PC) << " " << integer_as_hex(instruction) << "  // " << disasemble_machine_code(instruction) << endl;
        bool halt = run_instruction(instruction, regs, ram);
        if (halt)
            break;
    }

    for(int i=0; i<5; ++i)
        cout << dec << *ram.access_short(i*2) << " ";
    cout << endl;

    // show display
    cout << endl;
    cout << "This is our display: ------------------>>>";
    cout << endl;
    for(int i=0; i<25; ++i)
    {
        for(int j=0; j<80; ++j)
        {
            cout << *ram.access_byte(0x3000 + i*80 + j);
        }
        cout << endl;
    }
    cout << "<<<---------------------------------------";
    cout << endl;


    return 0;
}
