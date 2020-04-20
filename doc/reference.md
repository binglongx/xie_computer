Registers: 16-bit
=============================
```
RA: Register A	: RA-RF: general purpose registers
RB: Register B
RC: Register C
RD: Register D
RE: Register E
RF: Register F
PC: Program Counter: memory location to load next instruction
SP: Stack Pointer:   top of stack
SR: Status Register: 
(z) Zero bit:     Z=1 if CMP results in equal; Z=0 otherwise
(n) Negative bit: N=1 if CMP results in less;  N=0 otherwise
           +-------------+-+-+
Fields:    |             |N|Z|
           +-------------+-+-+
Bit index: |             |1|0|
           +-------------+-+-+
```

Assembly Instructions
=============================

// move instructions
```c++
MOV     reg,  num       // reg  <- num              // MOV RB, 42
MOV     reg1, reg2      // reg1 <- reg2             // MOV RA, RB
LDB     reg,  [mem]     // reg_lowbyte <- mem       // LDB RC, [0]  ;load byte and write 
LDB     reg,  [reg]     // reg_lowbyte <- [reg]     // LDB RC, [RD] ; into lower RC byte
STB     reg,  [mem]     // reg_lowbyte -> mem       // STB RD, [0]  ;store RD lower byte 
STB     reg,  [reg]     // reg_lowbyte -> [reg]     // STB RD, [RC] ; to mem location 
LDS     reg,  [mem]     // reg  <- mem              // LDS RC, [0]
LDS     reg,  [reg]     // reg  <- [reg]            // LDS RC, [RD]
STS     reg,  [mem]     // reg  -> mem              // STS RC, [0]
STS     reg,  [reg]     // reg  -> [reg]            // STS RD, [RC]
```

// arithmetic instructions
```c++
ADD     reg1, reg2      // reg1 <- reg1 + reg2      // ADD RA, RB
ADD     reg,  num       // reg  <- reg  + num       // ADD RA, 10
SUB     reg1, reg2      // reg1 <- reg1 - reg2      // SUB RC, RD
SUB     reg,  num       // reg  <- reg  - num       // SUB RC, 7
MUL     reg1, reg2      // reg1 <- reg1 * reg2      // MUL RB, RC
MUL     reg,  num       // reg  <- reg  * num       // MUL RB, 2
DIV     reg1, reg2      // reg1 <- reg1 / reg2      // DIV RD, RA
DIV     reg,  num       // reg  <- reg  / num       // DIV RD, 3
MOD     reg1, reg2      // reg1 <- reg1 % reg2      // MOD RA, RB
MOD     reg,  num       // reg  <- reg  % num       // MOD RA, 5
INC     reg             // reg  <- reg  + 1         // INC RC
DEC     reg             // reg  <- reg  - 1         // DEC RC
```

// logical instructions
```c++
AND     reg1, reg2      // reg1 <- reg1 & reg2      // AND RA, RB
AND     reg,  num       // reg  <- reg  & num       // AND RA, 0
OR_     reg1, reg2      // reg1 <- reg1 | reg2      // OR_ RA, RB
OR_     reg,  num       // reg  <- reg  | num       // OR_ RA, 1
XOR     reg1, reg2      // reg1 <- reg1 ^ reg2      // XOR RA, RA // RA==0
XOR     reg,  num       // reg  <- reg  ^ num       // XOR RA, 3
NOT     reg             // reg  <- ~reg             // NOT RA
SHL     reg,  num       // reg  <- reg  << num      // SHL RA, 1
SHL     reg1, reg2      // reg1 <- reg1 << reg2     // SHL RA, RB
SHR     reg,  num       // reg  <- reg  >> num      // SHR RA, 1
SHR     reg1, reg2      // reg1 <- reg1 >> reg2     // SHR RA, RB
```

// program flow
```c++
CMP     reg1, reg2      // SR: Z=1 if reg1 = reg2 (or num), Z=0 otherwise;
CMP     reg1, num       //     N=1 if reg1 < reg2 (or num), N=0 otherwise
JPE     [lab]           // jump to label if SR Z=1 (N=0)
JPE     [reg]           // jump to [reg] if SR Z=1 (N=0)
JPL     [lab]           // jump to label if SR N=1 (Z=0)
JPL     [reg]           // jump to [reg] if SR N=1 (Z=0)
JPG     [lab]           // jump to label if SR Z=0 && N=0
JPG     [reg]           // jump to [reg] if SR Z=0 && N=0
JMP     [lab]           // jump to label
JMP     [reg]           // jump to [reg]
CLL     [lab]           // PSH PC, JMP [lab]
CLL     [reg]           // PSH PC, JMP [reg]
RET                     // POP PC
HLT                     // halt everything
```

```c++
PSH     reg             // SP decreases by 2; store reg into [SP].
PSH     num             // SP decreases by 2; store num into [SP].
POP     reg             // load [SP] to reg; SP increases by 2.
```

// IO
```
KBD                     // wait for user enter a line. will get xstring at input area
DSP                     // update screen with display area contents
```

Machine Code Definition
=============================
// overall structure 32-bit
```
           +----------+-+---------+-----------------+
Fields:    |  opcode  |f| operand1|    operand2     |
           +----------+-+---------+-----------------+
Bit index: |    8     |1|    7    |       16        |
           +----------+-+---------+-----------------+
```
f = flag of operand2 type, 0==reg; 1==num/mem/label

Instruction with 0 operands:
```
RET                     // operand1 and operand2 are encoded as 0s.
```

Instruction with 1 operand:
```
INC     operand2        // the single operand is encoded in operand2 field; operand1 encoded as 0
```

Instruction with 2 operands:
```
MOV     operand1, operand2
```

// opcode table 8-bit (hex)
```c++
MOV 01
LDB 02
STB 03
LDS 04
STS 05

ADD 10
SUB 11
MUL 12
DIV 13
MOD 14
INC 15
DEC 16

AND 20
OR_ 21
XOR 22
NOT 23
SHL 24
SHR 25

CMP 30
JPE 31
JPL 32
JPG 33
JMP 34
CLL 35
RET 36
HLT 37

PSH 40
POP 41

```
// register table 8-bit (hex)
```c++
RA 00
RB 01
RC 02
RD 03
RE 04
RF 05
PC 	// no code needed: PC never appears in machine code directly
SP 11
SR 12
```
Machine Code Examples
Examples: 
```c++
MOV RA, RB       01 00 00 01
JMP [255]        11 80 00 FF
JPL [900]        10 80 03 84
STS RC, [RD]     18 02 00 03
```

Memory Layout
=============================
```
0x0000: data start here
0x1000: instructions start here
0x2000: stack bottom is here. stack grows backwards to lower locations.
```

Input/Output
=============================
0x3000: 
Display area in memory. This is an 80x25 characters display. The first row of characters. In display should be Stored in 0x3000-0x304F, second row 0x3050-0x309F. Total memory block: 0x3000-0x37CF

0x4000: Input area in memory. The first short tells you how many chars(N) are available in this area from the keyboard. Starting from 0x4002 there is an N amount of chars, without terminator null char.

Calling Convention
=============================
When calling a function, the first parameter is passed to the function in RC. The second one will go to RD, and so on. If there are more than four parameters, they will be passed in the stack, and will be pushed in descending order. (eg. the function has seven parameters, which means the first four are in RC-RF, and then the last three are pushed in descending order: seventh, sixth, fifth.)