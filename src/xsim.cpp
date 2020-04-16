#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

using namespace std;

void print_hex_short(short x)
{
    cout << setfill('0') << setw(4) << right << hex << x;
}

void print_hex_int(int x)
{
    cout << setfill('0') << setw(8) << right << hex << x;
}

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
        cout<<"RA="; print_hex_short(RA); cout<<" ";
        cout<<"RB="; print_hex_short(RB); cout<<" ";
        cout<<"RC="; print_hex_short(RC); cout<<" ";
        cout<<"RD="; print_hex_short(RD); cout<<" ";
        cout<<"RE="; print_hex_short(RE); cout<<" ";
        cout<<"RF="; print_hex_short(RF); cout<<" ";
        cout<<"  SP="; print_hex_short(SP); cout<<" ";
        cout<<"SR="; print_hex_short(SR); cout<<" ";
        cout<<"PC="; print_hex_short(PC); cout<<" ";
    }

    short* getRegister(int operand)
    {
        switch(operand)
        {
            case 0: return &RA;
            case 1: return &RB;
            case 2: return &RC;
            case 3: return &RD;
            case 4: return &RE;
            case 5: return &RF;
            case 11: return &SP;
            default: /*cout << "Error: unrecognized operand for register: " << operand << endl;*/ return nullptr;
        }
    }
};




enum class Opcode
{
    MOV = 0x01,
    LDS = 0x02,
    STS = 0x03,
    ADD = 0x04,
    SUB = 0x05,
    MUL = 0x06,
    DIV = 0x07,
    MOD = 0x08,
    INC = 0x09,
    DEC = 0x0A,
    AND = 0x0B,
    OR_ = 0x0C,
    XOR = 0x0D,
    CMP = 0x0E,
    JPE = 0x0F,
    JPL = 0x10,
    JMP = 0x11,
    CLL = 0x12,
    RET = 0x13,
    HLT = 0x14,
    PSH = 0x15,
    POP = 0x16,
    NOT = 0x17,
    LDB = 0x18,
    STB = 0x19,
    SHL = 0x1A,
    SHR = 0x1B
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

        case Opcode::LDS:
            *reg1 = *ram.access_short(num);  // perform load from [reg] to reg
            break;
            
        case Opcode::STS:
            *ram.access_short(num) = *reg1;  // store to mem
            break;

        case Opcode::LDB:
            *reg1 = *ram.access_byte(num);  // perform load from [reg] to reg
            break;
            
        case Opcode::STB:
            *ram.access_byte(num) = (char)*reg1;  // store to mem
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
            if ((regs.SR & 0x1) != 0)
                regs.PC = num;
            else
                regs.PC += 4;
            return false;   // control flow instruction
        case Opcode::JPL:
            if ((regs.SR & 0x2) != 0)
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
            cout << regs.SP << " " << regs.PC << endl;
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
        cout << "Instruction @"; print_hex_int(pc); cout<< " "; print_hex_int(ram.fetch_instruction(pc)); cout << endl;
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
        cout << " Instruction @"; print_hex_int(regs.PC); cout<< " "; print_hex_int(ram.fetch_instruction(regs.PC)); cout << endl;
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
