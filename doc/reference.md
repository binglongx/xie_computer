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
(z) Zero bit:     1 if CMP results in equal; 0 otherwise
(n) Negative bit: 1 if CMP results in less;  0 otherwise
           +-------------+-+-+
Fields:    |             |n|z|
           +-------------+-+-+
Bit index: |             |1|0|
           +-------------+-+-+
```

Assembly Instructions
=============================
// move instructions
```c++
MOV	reg, num // reg <- num			// MOV RB, 42
MOV	reg1, reg2	// reg1 <- reg2			// MOV RA, RB
LDS	reg, mem	// reg <- mem			// LDS RC, [0]
LDS	reg, [reg]	// reg <- [reg]		      // LDS RC, [RD]
STS	reg, mem	// reg -> mem			// STS RC, [0]
STS	reg, [reg]	// reg -> [reg]			// STS RD, [RC]
LDB	reg, mem	// reg_low8bits <- mem	// LDB RC, [0]  ;load byte and write 
LDB	reg, [reg]	// reg_low8bits <- [reg]// LDB RC, [RD] ; into lower RC byte
STB	reg, mem	// reg_low8bits -> mem	// STB RD, [0]  ;STSre lower byte 
STB	reg, [reg]	// reg_low8bits -> [reg]// STB RD, [RC] ; to mem location 
```

// arithmetic instructions
```c++
ADD	reg1, reg2	// reg1 <- reg1+ reg2		// ADD RA, RB
ADD	reg, num	// reg <- reg + num		// ADD RA, 10
SUB	reg1, reg2	// reg1 <- reg1 - reg2		// SUB RC, RD
SUB	reg, num	// reg <- reg - num		// SUB RC, 7
MUL	reg1, reg2	// reg1 <- reg1 * reg2		// MUL RB, RC
MUL	reg, num	// reg <- reg * num		// MUL RB, 2
DIV	reg1, reg2	// reg1 <- reg1 / reg2		// DIV RD, RA
DIV	reg, num	// reg <- reg / num		// DIV RD, 3
MOD	reg1, reg2 	// reg1 <- reg1 % reg2		// MOD RA, RB
MOD	reg, num 	// reg <- reg % num		// MOD RA, 5
INC	reg		// reg <- reg + 1			// INC RC
DEC	reg		// reg <- reg - 1			// DEC RC
```

// logical instructions
```c++
AND	reg1, reg2	// reg1 <- reg1 & reg2		// AND RA, RB
AND	reg, num	// reg <- reg & num		// AND RA, 0
OR_	reg1, reg2	// reg1 <- reg1 | reg2		// OR_ RA, RB
OR_	reg, num	// reg <- reg | num		// OR_ RA, 1
XOR	reg1, reg2	// reg1 <- reg1 ^ reg2		// XOR RA, RA // RA==0
XOR	reg, num	// reg <- reg ^ num		// XOR RA, 3
NOT	reg		// reg  <- ~reg			// NOT RA
SHL   reg, num	// reg  <- reg << num 		// SHL RA, 1
SHL 	reg1, reg2  // reg1 <- reg1 << reg2 	// SHL RA, RB
SHR   reg, num	// reg  <- reg >> num 		// SHR RA, 1
SHR 	reg1, reg2  // reg1 <- reg1 >> reg2 	// SHR RA, RB
```

// program flow
```c++
CMP	reg1, reg2	
CMP 	reg, num
// SR bits set: Zero bit true if reg1==reg2 (reg==num); 
//              Negative bit true if reg1<reg2 (reg<num)
JPE	mem		// jump to [mem] if SR Zero bit true
JPE	reg		// jump to [reg] if SR Zero bit true
JPL	mem		// jump to [mem] if SR Negative bit true
JPL	reg		// jump to [reg] if SR Negative bit true
JMP	mem		// jump to [mem]
JMP	reg		// jump to [reg]
CLL	mem		// PSH PC, JMP [mem]
CLL	reg		// PSH PC, JMP [reg]
RET			// POP PC
HLT			// halt everything
```

```c++
PSH	reg		// SP decreases by 2; Store reg into [SP].
PSH 	num 		// SP decreases by 2; Store num into [SP].
POP	reg		// load [SP] to reg; SP increases by 2.
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
f = flag of operand2 type, 0==reg; 1==num

// opcode table 8-bit (hex)
```c++
MOV 01
LDS 02
STS 03
ADD 04
SUB 05
MUL 06
DIV 07
MOD 08
INC 09
DEC 0A
AND 0B
OR_ 0C
XOR 0D
CMP 0E
JPE 0F
JPL 10
JMP 11
CLL 12
RET 13
HLT 14
PSH 15
POP 16
NOT 17
LDB 18
STB 19
SHL 1A
SHR 1B
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
