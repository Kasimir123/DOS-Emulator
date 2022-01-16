#include "./emulator.h"
#include <stdbool.h>
#include <termios.h>
#include <cstring>

void PrintHeader(DOS_HEADER *header)
{

    fprintf(stdout, "DOS Header\n");
    fprintf(stdout, "\tSignature:                       \t\t %.*s\n", 2, header->signature);
    fprintf(stdout, "\tBytes in last block:             \t\t%.*hd\n", 3, header->lastsize);
    fprintf(stdout, "\tBlocks in file:                  \t\t%.*hd\n", 3, header->nblocks);
    fprintf(stdout, "\tNumber of relocations:           \t\t%.*hd\n", 3, header->nreloc);
    fprintf(stdout, "\tSize of header in paragraphs:    \t\t%.*hd\n", 3, header->hdrsize);
    fprintf(stdout, "\tMinimum extra paragraphs needed: \t\t%.*hd\n", 3, header->minalloc);
    fprintf(stdout, "\tMaximum extra paragraphs needed: \t\t%.*hd\n", 2, header->maxalloc);
    fprintf(stdout, "\tInitial (relative) SS value:     \t\t%.*hd\n", 3, header->ss);
    fprintf(stdout, "\tInitial SP value:                \t\t%.*hd\n", 3, header->sp);
    fprintf(stdout, "\tChecksum:                        \t\t%.*hd\n", 3, header->checksum);
    fprintf(stdout, "\tInitial IP value:                \t\t%.*hd\n", 3, header->ip);
    fprintf(stdout, "\tInitial (relative) CS value:     \t\t%.*hd\n", 3, header->cs);
    fprintf(stdout, "\tFile address of relocation table:\t\t%.*hd\n", 3, header->relocpos);
    fprintf(stdout, "\tOverlay number:                  \t\t%.*hd\n", 3, header->noverlay);
}

int DOSEmulator::CalculateStartAddress()
{
    return (512 * header->nblocks) - (16 * header->hdrsize);
}

void DOSEmulator::PrintStack()
{
    fprintf(stdout, "Stack:\n");

    fprintf(stdout, "\tRegisters:\n");
    fprintf(stdout, "\t\tAX: %02x %02x", registers[AX][0], registers[AX][1]);
    fprintf(stdout, "\t\tCX: %02x %02x", registers[CX][0], registers[CX][1]);
    fprintf(stdout, "\t\tDX: %02x %02x", registers[DX][0], registers[DX][1]);
    fprintf(stdout, "\t\tBX: %02x %02x\n", registers[BX][0], registers[BX][1]);
    fprintf(stdout, "\t\tSP: %02x %02x", registers[SP][0], registers[SP][1]);
    fprintf(stdout, "\t\tBP: %02x %02x", registers[BP][0], registers[BP][1]);
    fprintf(stdout, "\t\tSI: %02x %02x", registers[SI][0], registers[SI][1]);
    fprintf(stdout, "\t\tDI: %02x %02x\n", registers[DI][0], registers[DI][1]);

    fprintf(stdout, "\tSpecial Registers:\n");
    fprintf(stdout, "\t\tES: %02x %02x", special_registers[ES][0], special_registers[ES][1]);
    fprintf(stdout, "\t\tCS: %02x %02x", special_registers[CS][0], special_registers[CS][1]);
    fprintf(stdout, "\t\tSS: %02x %02x\n", special_registers[SS][0], special_registers[SS][1]);
    fprintf(stdout, "\t\tDS: %02x %02x", special_registers[DS][0], special_registers[DS][1]);
    fprintf(stdout, "\t\tFS: %02x %02x", special_registers[FS][0], special_registers[FS][1]);
    fprintf(stdout, "\t\tGS: %02x %02x\n", special_registers[GS][0], special_registers[GS][1]);

    fprintf(stdout, "\tFlags:\n");
    fprintf(stdout, "\t\tCF: %d",   flags[CF] ? 1 : 0);
    fprintf(stdout, "\t\tPF: %d",   flags[PF] ? 1 : 0);
    fprintf(stdout, "\t\tAF: %d",   flags[AF] ? 1 : 0);
    fprintf(stdout, "\t\tZF: %d\n", flags[ZF] ? 1 : 0);
    fprintf(stdout, "\t\tSF: %d",   flags[SF] ? 1 : 0);
    fprintf(stdout, "\t\tIF: %d",   flags[IF] ? 1 : 0);
    fprintf(stdout, "\t\tDF: %d",   flags[DF] ? 1 : 0);
    fprintf(stdout, "\t\tOF: %d\n", flags[OF] ? 1 : 0);
}

void DOSEmulator::ClearRegisters()
{
    for (int i = 0; i < 8; i++)
    {
        registers[i][0] = 0;
        registers[i][1] = 0;
    }

    for (int i = 0; i < 6; i++)
    {
        special_registers[i][0] = 0;
        special_registers[i][1] = 0;
    }
}

void DOSEmulator::ClearFlags()
{
    for (int i = 0; i < 8; i++)
    {
        flags[i] = false;
    }
}

short DOSEmulator::GetRegister(char op)
{
    return ((op >> 3) & 0x7);
}

short DOSEmulator::GetModRegister(char op)
{
    return (op & 0x7);
}

char DOSEmulator::GetModValue(char op)
{
    return ((op >> 6) & 0x3);
}

unsigned char *DOSEmulator::GetDataStart(short reg = DS)
{
    short ds_val = ((special_registers[reg][0] & 0xFF) << 8) + (special_registers[reg][1] & 0xFF);

    return opcodes + (ds_val * 16);
}

short DOSEmulator::GetModMemVal(char op)
{
    short val = 0;
    switch (GetModValue(op))
    {
    case 0x3:
    {
        val = registers[GetModRegister(op)][1] + (registers[GetModRegister(op)][0] << 8);
        break;
    }
    default:
        fprintf(stdout, "Not yet Implemented mod men val\n");
        break;
    }
    return val;
}

void DOSEmulator::SetModMemVal(short val, char op)
{
    switch (GetModValue(op))
    {
    case 0x3:
    {
        registers[GetModRegister(op)][0] = (val >> 8) & 0xFF;
        registers[GetModRegister(op)][1] = val & 0xFF;
        break;
    }
    default:
        fprintf(stdout, "Not yet Implemented\n");
        break;
    }
}

short DOSEmulator::GetModMemVal8(char op)
{
    char val = 0;
    switch (GetModValue(op))
    {
    case 0x2:
    {
        short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
        switch (GetModRegister(op))
        {
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = GetDataStart()[offset + bx_val];
            break;
        }
        default:
            fprintf(stdout, "Not yet Implemented: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x3:
    {
        val = registers[GetModRegister(op) % 4][GetModRegister(op) <= 3];
        break;
    }
    default:
        fprintf(stdout, "Not yet Implemented mod men val 8: %d\n", GetModValue(op));
        break;
    }
    return val;
}

char getCh()
{

    struct termios old_kbd_mode;    /* orig keyboard settings   */
    struct termios new_kbd_mode;

    tcgetattr (0, &old_kbd_mode);

    memcpy (&new_kbd_mode, &old_kbd_mode, sizeof(struct termios));

    new_kbd_mode.c_lflag &= ~(ICANON | ECHO);  /* new kbd flags */
    new_kbd_mode.c_cc[VTIME] = 0;
    new_kbd_mode.c_cc[VMIN] = 1;

    tcsetattr (0, TCSANOW, &new_kbd_mode);

    char ret = fgetc(stdin);

    /* reset original keyboard  */
    tcsetattr (0, TCSANOW, &old_kbd_mode);

    return ret;

}

void DOSEmulator::PerformInterrupt(char val)
{
    switch (registers[AX][AH])
    {
    case WRITE_CHAR_STDOUT:
    {
        fprintf(stdout, "%c", registers[DX][DL]);
        registers[AX][AL] = registers[DX][DL];
        break;
    }
    case READ_CHAR_STDIN_NOECHO:
    {
        registers[AX][AL] = getCh();
        break;
    }
    case WRITE_STR_STDOUT:
    {
        short dx_val = ((registers[DX][DH] << 8) & 0xFF) + (registers[DX][DL] & 0xFF);
        while (GetDataStart()[dx_val] != '$')
            fprintf(stdout, "%c", GetDataStart()[dx_val++]);

        registers[AX][AL] = 0x24;
        break;
    }
    case EXIT_PROGRAM:
    {
        fprintf(stdout, "\nExit with code: %d\n", registers[AX][AL]);
        run = false;
        break;
    }
    default:
        fprintf(stdout, "Not yet Implemented: %d\n", registers[AX][AH]);
        break;
    }
}

bool DOSEmulator::CheckIfCarry(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        case SUBTRACTION:
        {
            // If the first bit is 0 and then becomes 1 we had to carry
            if (!(val2 & 0x8) && ((val2 - val1) & 0x8) == 8)
                val = true;
            break;
        }
        default:
            fprintf(stdout, "Check carry not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfParity(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        default:
            fprintf(stdout, "Check parity not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfAuxiliary(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        default:
            fprintf(stdout, "Check auxiliary not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfZero(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        case SUBTRACTION:
        {
            if (!(val2 - val1))
                val = true;
            break;
        }
        default:
            fprintf(stdout, "Check zero not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfSign(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        case SUBTRACTION:
        {
            if ((val1 - val2) < 0)
                val = true;
            break;
        }
        default:
            fprintf(stdout, "Check sign not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfInterrupt(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        default:
            fprintf(stdout, "Check interrupt not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfDirection(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        default:
            fprintf(stdout, "Check direction not implemented\n");
            break;
    }
    return val;
}

bool DOSEmulator::CheckIfOverflow(char val1, char val2, char operation)
{
    bool val = false;
    switch (operation)
    {
        case SUBTRACTION:
        {
            if ((val1 - val2) > val1)
                val = true;
            break;
        }
        default:
            fprintf(stdout, "Check overflow not implemented\n");
            break;
    }
    return val;
}

void DOSEmulator::UpdateFlags(char val1, char val2, char operation)
{
    flags[CF] = CheckIfCarry(val1, val2, operation);
    // flags[PF] = CheckIfParity(val1, val2, operation);
    // flags[AF] = CheckIfAuxiliary(val1, val2, operation);
    flags[ZF] = CheckIfZero(val1, val2, operation);
    flags[SF] = CheckIfSign(val1, val2, operation);
    // flags[IF] = CheckIfInterrupt(val1, val2, operation);
    // flags[DF] = CheckIfDirection(val1, val2, operation);
    flags[OF] = CheckIfOverflow(val1, val2, operation);

}

void DOSEmulator::RunCode()
{

    int instr_count = 0;

    ip = 0;
    opcodes = data + startAddress;
    unsigned char op = opcodes[ip++];
    run = true;

    ClearRegisters();
    ClearFlags();

    while (run)
    {
        printf("%02x\n", op);
        switch (op)
        {
        case 0x0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xb:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x10:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x11:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x12:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x13:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x14:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x15:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x16:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x17:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x18:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x19:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x1f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x20:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x21:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x22:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x23:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x24:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x25:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x26:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x27:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x28:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x29:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2c:
        {
            op = opcodes[ip++];
            UpdateFlags(registers[AX][AL], op, SUBTRACTION);

            registers[AX][AL] -= (char)op;
            break;
        }
        case 0x2d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x2f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x30:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x31:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x32:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x33:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x34:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x35:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x36:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x37:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x38:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x39:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3c:
        {
            op = opcodes[ip++];

            UpdateFlags(registers[AX][AL], op, SUBTRACTION);

            break;
        }
        case 0x3d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x3f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x40:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x41:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x42:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x43:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x44:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x45:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x46:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x47:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x48:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x49:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x4f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x50:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x51:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x52:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x53:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x54:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x55:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x56:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x57:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x58:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x59:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x5f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x60:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x61:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x62:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x63:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x64:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x65:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x66:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x67:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x68:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x69:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x6f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x70:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x71:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x72:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x73:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x74:
        {
            op = opcodes[ip++];
            if (flags[ZF])
                ip += (char)op;
            break;
        }
        case 0x75:
        {
            op = opcodes[ip++];
            if (!flags[ZF])
                ip += (char)op;
            break;
        }
        case 0x76:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x77:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x78:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x79:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7c:
        {
            op = opcodes[ip++];
            if (flags[SF] != flags[OF])
                ip += (char)op;
            break;
        }
        case 0x7d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x7f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x80:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x81:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x82:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x83:
        {
            op = opcodes[ip++];

            switch (GetRegister(op))
            {
            case AND:
            {
                short val = GetModMemVal(op) & opcodes[ip++];
                SetModMemVal(val, op);
                break;
            }
            default:

                printf("Not Yet Implemented: %d\n", op);
                break;
            }

            break;
        }
        case 0x84:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x85:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x86:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x87:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x88:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x89:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x8a:
        {
            op = opcodes[ip++];

            registers[GetRegister(op) % 4][GetRegister(op) <= 3] = GetModMemVal8(op);

            break;
        }
        case 0x8b:
        {
            op = opcodes[ip++];

            short val = GetModMemVal(op);
            registers[GetRegister(op)][1] = val & 0xFF;
            registers[GetRegister(op)][0] = (val >> 8) & 0xFF;

            break;
        }
        case 0x8c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x8d:
        {
            op = opcodes[ip++];
            short val = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
            
            registers[GetRegister(op)][1] = val & 0xFF;
            registers[GetRegister(op)][0] = (val >> 8) & 0xFF;
            break;
        }
        case 0x8e:
        {
            op = opcodes[ip++];

            short val = GetModMemVal(op);
            special_registers[GetRegister(op)][1] = val & 0xFF;
            special_registers[GetRegister(op)][0] = (val >> 8) & 0xFF;

            break;
        }
        case 0x8f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x90:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x91:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x92:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x93:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x94:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x95:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x96:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x97:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x98:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x99:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9a:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9b:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9c:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9d:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9e:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0x9f:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa1:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa2:
        {
            short val = opcodes[ip++] + (opcodes[ip++] << 8);
            GetDataStart()[val] = registers[AX][AL];
            break;
        }
        case 0xa3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xa9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xaa:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xab:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xac:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xad:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xae:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xaf:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
        {
            short reg = (op & 0xF);
            registers[reg % 4][reg <= 3] = opcodes[ip++];
            break;
        }
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
        {
            short reg = (op & 0xF) - 0x8;
            registers[reg][1] = opcodes[ip++];
            registers[reg][0] = opcodes[ip++];
            break;
        }
        case 0xc0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc1:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc2:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xc9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xca:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xcb:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xcc:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xcd:
        {
            char val = opcodes[ip++];
            PerformInterrupt(val);
            break;
        }
        case 0xce:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xcf:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd1:
        {
            op = opcodes[ip++];

            switch (GetRegister(op))
            {
            case SHR:
            {
                short val = GetModMemVal(op) >> 1;
                SetModMemVal(val, op);
            }
            }

            break;
        }
        case 0xd2:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xd9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xda:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xdb:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xdc:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xdd:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xde:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xdf:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe1:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe2:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xe9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xea:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xeb:
        {
            op = opcodes[ip++];
            ip += (char)op;
            break;
        }
        case 0xec:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xed:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xee:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xef:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf0:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf1:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf2:
        {
            op = opcodes[ip++];
            switch(op)
            {
                case SCASB:
                    {
                        unsigned char * start = GetDataStart(ES) + (registers[DI][0] << 8) + registers[DI][1];
                        
                        flags[ZF] = false;
                        
                        while(*start != '$')
                        {
                            if (*start == registers[AX][AL])
                            {
                                flags[ZF] = true;
                                break;
                            }
                            start++;
                        }
                        break;
                    }
                default:
                    printf("0xf2 component not implemented");
                    break;
            }
            break;
        }
        case 0xf3:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf4:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf5:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf6:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf7:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf8:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xf9:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xfa:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xfb:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xfc:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xfd:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xfe:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        case 0xff:
        {
            printf("Not Yet Implemented: %2x\n", op);
            break;
        }
        default:
            run = false;
            break;
        }
        if (instr_count++ >= 2)
            run = false;
        PrintStack();
        op = opcodes[ip++];
    }
}

void DOSEmulator::StartEmulation()
{
    header = (DOS_HEADER *)data;

    PrintHeader(header);

    startAddress = CalculateStartAddress();

    fprintf(stdout, "Runtime: %d\n", startAddress);

    RunCode();
}