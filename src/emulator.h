#include "./structs.h"
#include <vector>

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
#define CMP 7

#define SCASB 0xae

#define NEG 3

#define INC 0
#define DEC 1

#define ADDITION 0
#define SUBTRACTION 1
#define XOR 2

#define WRITE_CHAR_STDOUT 0x2
#define READ_CHAR_STDIN_NOECHO 0x8
#define WRITE_STR_STDOUT 0x9
#define GET_SYSTEM_TIME 0x2C
#define EXIT_PROGRAM 0x4C

class Cursor 
{
    public:
    char row;
    char column;
    char page_number;
};

class DOSEmulator
{
public:
    DOSEmulator(unsigned char * program_data, bool start_debug = false)
    {
        data = program_data;
        debug = start_debug;
        vCursor = new Cursor;
    }

    void StartEmulation();
private:
    unsigned char * data;
    int startAddress;
    DOS_HEADER *header;
    unsigned char registers[8][2];
    unsigned char special_registers[6][2];
    int call_stack[256];
    int csp = 0;
    bool flags[8];
    unsigned char * opcodes;
    int ip = 0;
    long long instr_executed = 0;
    int step = 0;
    bool run = true;
    bool debug;
    Cursor * vCursor;
    bool video_mode = false;
    std::vector<int> breakpoints;

    void RunCode();
    int CalculateStartAddress();
    void PrintStack();
    void ClearRegisters();
    void ClearFlags();
    void SetRegistersFromHeader();
    short GetRegister(char op);
    short GetModRegister(char op);
    char GetModValue(char op);
    unsigned char * GetDataStart(short reg);
    short GetModMemVal(char op, bool commit_changes);
    void SetModMemVal(short val, char op, bool commit_changes);
    char GetModMemVal8(char op, bool commit_changes);
    void SetModMemVal8(char val, char op, bool commit_changes);
    void PerformInterrupt(char val);
    bool CheckIfCarry(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfParity(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfAuxiliary(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfZero(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfSign(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfInterrupt(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfDirection(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfOverflow(unsigned short val1, unsigned short val2, char operation);
    void UpdateFlags(unsigned short val1, unsigned short val2, char operation);
    bool CheckIfCarry8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfParity8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfAuxiliary8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfZero8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfSign8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfInterrupt8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfDirection8(unsigned char val1, unsigned char val2, char operation);
    bool CheckIfOverflow8(unsigned char val1, unsigned char val2, char operation);
    void UpdateFlags8(unsigned char val1, unsigned char val2, char operation);
    void Push(short val);
    void Push8(char val);
    short Pop();
    char Pop8();
    void DebugMenu();
    bool CheckIfBreakpoint(char op);
};

    
    

    