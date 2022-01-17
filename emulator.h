#include "./structs.h"

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AH 0
#define AL 1
#define CH 0
#define CL 1
#define DH 0
#define DL 1
#define BH 0
#define BL 1

#define ES 0
#define CS 1
#define SS 2
#define DS 3
#define FS 4
#define GS 5

#define CF 0
#define PF 1
#define AF 2
#define ZF 3
#define SF 4
#define IF 5
#define DF 6
#define OF 7

#define SHR 5

#define AND 4

#define SCASB 0xae

#define ADDITION 0
#define SUBTRACTION 1

#define WRITE_CHAR_STDOUT 0x2
#define READ_CHAR_STDIN_NOECHO 0x8
#define WRITE_STR_STDOUT 0x9
#define EXIT_PROGRAM 0x4C

class DOSEmulator
{
public:
    DOSEmulator(unsigned char * program_data, bool start_debug = false)
    {
        data = program_data;
        debug = start_debug;
    }

    void StartEmulation();
private:
    unsigned char * data;
    int startAddress;
    DOS_HEADER *header;
    unsigned char registers[8][2];
    unsigned char special_registers[6][2];
    bool flags[8];
    unsigned char * opcodes;
    int ip = 0;
    int instr_executed = 0;
    int step = 0;
    bool run = true;
    bool debug;

    void RunCode();
    int CalculateStartAddress();
    void PrintStack();
    void ClearRegisters();
    void ClearFlags();
    short GetRegister(char op);
    short GetModRegister(char op);
    char GetModValue(char op);
    unsigned char * GetDataStart(short reg);
    short GetModMemVal(char op);
    void SetModMemVal(short val, char op);
    short GetModMemVal8(char op);
    void SetModMemVal8(char val, char op);
    void PerformInterrupt(char val);
    bool CheckIfCarry(char val1, char val2, char operation);
    bool CheckIfParity(char val1, char val2, char operation);
    bool CheckIfAuxiliary(char val1, char val2, char operation);
    bool CheckIfZero(char val1, char val2, char operation);
    bool CheckIfSign(char val1, char val2, char operation);
    bool CheckIfInterrupt(char val1, char val2, char operation);
    bool CheckIfDirection(char val1, char val2, char operation);
    bool CheckIfOverflow(char val1, char val2, char operation);
    void UpdateFlags(char val1, char val2, char operation);
    void DebugMenu();
};

