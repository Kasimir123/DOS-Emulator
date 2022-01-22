#include "./emulator.h"
#include <stdbool.h>
#include <termios.h>
#include <cstring>
#include <vector>
#include <ostream>
#include <iostream>
#include <emscripten/fetch.h>
#include <string.h>
#include "unistd.h"
#include <sys/time.h>

char *read_data;
bool ready;

void read_success(emscripten_fetch_t *fetch)
{
    if (read_data)
        free(read_data);

    read_data = (char *)malloc(sizeof(char) * (fetch->numBytes + 1));

    memcpy(read_data, fetch->data, fetch->numBytes);

    read_data[fetch->numBytes - 1] = '\n';
    read_data[fetch->numBytes] = '\0';

    ready = true;
    emscripten_fetch_close(fetch);
}

void read_fail(emscripten_fetch_t *fetch)
{
    emscripten_fetch_close(fetch);
}

char *read_async()
{

    ready = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = read_success;
    attr.onerror = read_fail;
    emscripten_fetch(&attr, "___terminal::read");

    while (!ready)
    {
        emscripten_sleep(100);
    }

    return read_data;
}

char get_char_async()
{
    ready = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = read_success;
    attr.onerror = read_fail;
    emscripten_fetch(&attr, "___terminal::get_char");

    while (!ready)
    {
        emscripten_sleep(100);
    }

    return read_data[0];
}

void send_ping_and_char_async(char *command, char c)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    std::string url("___terminal::");

    url.append(command);

    url.append("::");

    url.append(1, c);

    emscripten_fetch(&attr, url.c_str());
}

void send_ping_async(char *command)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    std::string url("___terminal::");

    url.append(command);

    emscripten_fetch(&attr, url.c_str());
}

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

    fprintf(stdout, "Relocations\n");
    for (int i = 0; i < header->nreloc; i++)
    {
        RELOCATION *rel = (RELOCATION *)((char *)header + header->relocpos + (i * 4));
        fprintf(stdout, "\tRelocation:\n");
        fprintf(stdout, "\t\tOffset:        %04hd\n", rel->offset);
        fprintf(stdout, "\t\tSegment Value: %04hd\n", rel->segment_value);
    }
}

int DOSEmulator::CalculateStartAddress()
{
    RELOCATION *start_rel = (RELOCATION *)((char *)header + header->relocpos);
    return (16 * header->hdrsize) + (start_rel->segment_value * 16);
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
    fprintf(stdout, "\t\tCF: %d", flags[CF] ? 1 : 0);
    fprintf(stdout, "\t\tPF: %d", flags[PF] ? 1 : 0);
    fprintf(stdout, "\t\tAF: %d", flags[AF] ? 1 : 0);
    fprintf(stdout, "\t\tZF: %d\n", flags[ZF] ? 1 : 0);
    fprintf(stdout, "\t\tSF: %d", flags[SF] ? 1 : 0);
    fprintf(stdout, "\t\tIF: %d", flags[IF] ? 1 : 0);
    fprintf(stdout, "\t\tDF: %d", flags[DF] ? 1 : 0);
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

void DOSEmulator::SetRegistersFromHeader()
{
    ClearRegisters();

    short ss = header->ss;
    short sp = header->sp;
    short cs = header->cs;

    special_registers[SS][0] = (ss >> 8) & 0xFF;
    special_registers[SS][1] = ss & 0xFF;

    registers[SP][0] = (sp >> 8) & 0xFF;
    registers[SP][1] = sp & 0xFF;

    special_registers[CS][0] = (cs >> 8) & 0xFF;
    special_registers[CS][1] = cs & 0xFF;

    ip = header->ip;
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

    return data + (header->hdrsize * 16) + (ds_val * 16);
}

short DOSEmulator::GetModMemVal(char op, bool commit_changes)
{
    short val = 0;

    int start_ip = ip;
    switch (GetModValue(op))
    {
    case 0x0:
    {
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[bx_val + si_val] << 8) + GetDataStart()[bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[bx_val + di_val] << 8) + GetDataStart()[bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[bp_val + si_val] << 8) + GetDataStart()[bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[bp_val + di_val] << 8) + GetDataStart()[bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[si_val] << 8) + GetDataStart()[si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[di_val] << 8) + GetDataStart()[di_val];
            break;
        }
        case 0x6:
        {
            short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
            val = (GetDataStart()[offset] << 8) + GetDataStart()[offset];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = (GetDataStart()[bx_val] << 8) + GetDataStart()[bx_val];
            break;
        }

        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x1:
    {
        short offset = opcodes[ip++];
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + bx_val + si_val] << 8) + GetDataStart()[offset + bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + bx_val + di_val] << 8) + GetDataStart()[offset + bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val + si_val] << 8) + GetDataStart()[offset + bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val + di_val] << 8) + GetDataStart()[offset + bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + si_val] << 8) + GetDataStart()[offset + si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + di_val] << 8) + GetDataStart()[offset + di_val];
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val] << 8) + GetDataStart()[offset + bp_val];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = (GetDataStart()[offset + bx_val] << 8) + GetDataStart()[offset + bx_val];
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x2:
    {
        short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + bx_val + si_val] << 8) + GetDataStart()[offset + bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + bx_val + di_val] << 8) + GetDataStart()[offset + bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val + si_val] << 8) + GetDataStart()[offset + bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val + di_val] << 8) + GetDataStart()[offset + bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = (GetDataStart()[offset + si_val] << 8) + GetDataStart()[offset + si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = (GetDataStart()[offset + di_val] << 8) + GetDataStart()[offset + di_val];
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            val = (GetDataStart()[offset + bp_val] << 8) + GetDataStart()[offset + bp_val];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = (GetDataStart()[offset + bx_val] << 8) + GetDataStart()[offset + bx_val + 1];
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x3:
    {
        val = registers[GetModRegister(op) % 4][1] + (registers[GetModRegister(op) % 4][0] << 8);
        break;
    }
    default:
            fprintf(stdout, "Error: %d\n", GetModValue(op));
        break;
    }
    if (!commit_changes)
        ip = start_ip;

    return val;
}

void DOSEmulator::SetModMemVal(short val, char op, bool commit_changes)
{

    int start_ip = ip;
    switch (GetModValue(op))
    {
    case 0x0:
    {
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[bx_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[bx_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[bx_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[bx_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[bp_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[bp_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[bp_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[bp_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[si_val] = (val >> 8) & 0xFF;
            GetDataStart()[si_val + 1] = val & 0xFF;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[di_val] = (val >> 8) & 0xFF;
            GetDataStart()[di_val + 1] = val & 0xFF;
            break;
        }
        case 0x6:
        {
            short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset] = (val >> 8) & 0xFF;
            GetDataStart()[offset + 1] = val & 0xFF;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[bx_val] = (val >> 8) & 0xFF;
            GetDataStart()[bx_val + 1] = val & 0xFF;
            break;
        }

        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x1:
    {
        short offset = opcodes[ip++];
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + 1] = val & 0xFF;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + 1] = val & 0xFF;
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x2:
    {
        short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + si_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + si_val + 1] = val & 0xFF;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + di_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + di_val + 1] = val & 0xFF;
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bp_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bp_val + 1] = val & 0xFF;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = (opcodes[ip++] << 8) + opcodes[ip++];
            GetDataStart()[offset + bx_val] = (val >> 8) & 0xFF;
            GetDataStart()[offset + bx_val + 1] = val & 0xFF;
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x3:
    {
        registers[GetModRegister(op) % 4][0] = (val >> 8) & 0xFF;
        registers[GetModRegister(op)% 4][1] = val & 0xFF;
        break;
    }
    default:
            fprintf(stdout, "Error: %d\n", GetModValue(op));
        break;
    }
    if (!commit_changes)
        ip = start_ip;
}

char DOSEmulator::GetModMemVal8(char op, bool commit_changes)
{
    char val = 0;

    int start_ip = ip;
    switch (GetModValue(op))
    {
    case 0x0:
    {
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[di_val];
            break;
        }
        case 0x6:
        {
            short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
            val = GetDataStart()[offset];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = GetDataStart()[bx_val];
            break;
        }

        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x1:
    {
        short offset = opcodes[ip++];
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + di_val];
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            val = GetDataStart()[offset + bp_val];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = GetDataStart()[offset + bx_val];
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x2:
    {
        short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + bx_val + si_val];
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + bx_val + di_val];
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + bp_val + si_val];
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + bp_val + di_val];
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            val = GetDataStart()[offset + si_val];
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            val = GetDataStart()[offset + di_val];
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            val = GetDataStart()[offset + bp_val];
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            val = GetDataStart()[offset + bx_val];
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
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
            fprintf(stdout, "Error: %d\n", GetModValue(op));
        break;
    }
    if (!commit_changes)
        ip = start_ip;

    return val;
}

void DOSEmulator::SetModMemVal8(char val, char op, bool commit_changes)
{
    int start_ip = ip;
    switch (GetModValue(op))
    {
    case 0x0:
    {
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[bx_val + si_val] = val;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[bx_val + di_val] = val;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[bp_val + si_val] = val;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[bp_val + di_val] = val;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[si_val] = val;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[di_val] = val;
            break;
        }
        case 0x6:
        {
            short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset] = val;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[bx_val] = val;
            break;
        }

        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x1:
    {
        short offset = opcodes[ip++];
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val + si_val] = val;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val + di_val] = val;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val + si_val] = val;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val + di_val] = val;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + si_val] = val;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + di_val] = val;
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val] = val;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val] = val;
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x2:
    {
        short offset = (opcodes[ip++] & 0xFF) + ((opcodes[ip++] & 0xFF) << 8);
        switch (GetModRegister(op))
        {
        case 0x0:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val + si_val] = val;
            break;
        }
        case 0x1:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val + di_val] = val;
            break;
        }
        case 0x2:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val + si_val] = val;
            break;
        }
        case 0x3:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val + di_val] = val;
            break;
        }
        case 0x4:
        {
            short si_val = ((registers[SI][0] << 8) & 0xFF) + (registers[SI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + si_val] = val;
            break;
        }
        case 0x5:
        {
            short di_val = ((registers[DI][0] << 8) & 0xFF) + (registers[DI][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + di_val] = val;
            break;
        }
        case 0x6:
        {
            short bp_val = ((registers[BP][0] << 8) & 0xFF) + (registers[BP][1] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bp_val] = val;
            break;
        }
        case 0x7:
        {
            short bx_val = ((registers[BX][BH] << 8) & 0xFF) + (registers[BX][BL] & 0xFF);
            if (val == NULL)
                val = opcodes[ip++];
            GetDataStart()[offset + bx_val] = val;
            break;
        }
        default:
            fprintf(stdout, "Error: %d\n", GetModRegister(op));
            break;
        }
        break;
    }
    case 0x3:
    {
        registers[GetModRegister(op) % 4][GetModRegister(op) <= 3] = val;
        break;
    }
    default:
            fprintf(stdout, "Error: %d\n", GetModValue(op));
        break;
    }
    if (!commit_changes)
        ip = start_ip;
}

void DOSEmulator::PerformInterrupt(char val)
{
    switch (val)
    {
    case 0x10:
    {
        switch (registers[AX][AH])
        {
        case 0x0:
        {
            if (registers[AX][AL] == 0x13)
            {
                send_ping_async("activate_video_mode");
                video_mode = true;
            }
            break;
        }
        case 0x2:
        {
            vCursor->row = registers[DX][DH];
            vCursor->column = registers[DX][DL];
            vCursor->page_number = registers[BX][BH];

            break;
        }
        // This could potentially be slightly wrong
        case 0xb:
        {
            send_ping_and_char_async("set_background_color", registers[BX][BL]);
            break;
        }
        default:
            fprintf(stdout, "Not yet Implemented: %02x\n", registers[AX][AH]);
            break;
        }
        break;
    }
    case 0x16:
    {
        switch (registers[AX][AH])
        {
        case 0x0:
        {
            // AH needs to be set to BIOS scan code
            registers[AX][AL] = get_char_async();
            break;
        }
        default:
            fprintf(stdout, "Not yet Implemented: %02x\n", registers[AX][AH]);
            break;
        }
        break;
    }
    case 0x21:
    {
        switch (registers[AX][AH])
        {
        case WRITE_CHAR_STDOUT:
        {
            send_ping_and_char_async("write", registers[DX][DL]);
            registers[AX][AL] = registers[DX][DL];
            break;
        }
        case READ_CHAR_STDIN_NOECHO:
        {
            registers[AX][AL] = get_char_async();
            break;
        }
        case WRITE_STR_STDOUT:
        {
            if (video_mode)
            {
                std::string send("write::");
                short dx_val = ((registers[DX][DH] << 8) & 0xFF) + (registers[DX][DL] & 0xFF);
                while (GetDataStart()[dx_val] != '$')
                    send.append(1, GetDataStart()[dx_val++]);

                send.append("::");
                send.append(std::to_string(vCursor->column));
                send.append("::");
                send.append(std::to_string(vCursor->row));

                send_ping_async((char *)send.c_str());
            }
            else
            {
                short dx_val = ((registers[DX][DH] << 8) & 0xFF) + (registers[DX][DL] & 0xFF);
                while (GetDataStart()[dx_val] != '$')
                    send_ping_and_char_async("write", GetDataStart()[dx_val++]);

                registers[AX][AL] = 0x24;
            }
            break;
        }
        case GET_SYSTEM_TIME:
        {
            struct timeval time_now;
            gettimeofday(&time_now, NULL);
            struct tm *time_str_tm = gmtime(&time_now.tv_sec);

            registers[CX][CH] = (time_str_tm->tm_hour) & 0xFF;
            registers[CX][CL] = (time_str_tm->tm_min) & 0xFF;
            registers[DX][DH] = (time_str_tm->tm_sec) & 0xFF;
            registers[DX][DL] = (time_now.tv_usec / 100) & 0xFF;

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
        break;
    }
    default:
        fprintf(stdout, "Interrupt type %02x not yet implemented\n", val);
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

std::vector<char *> GetTokens(char *line)
{
    int i = 0;
    int last = 0;
    std::vector<char *> tokens;

    while (line[i] != '\0')
    {
        if (line[i] == ' ' || line[i] == '\n')
        {
            line[i] = '\0';
            char *str = line + last;
            tokens.push_back(str);
            last += i + 1;
        }
        i++;
    }

    return tokens;
}

void DOSEmulator::DebugMenu()
{
    fprintf(stdout, "Total Instructions executed: %d\n", instr_executed);

    char *data_from_stdin = read_async();

    while (strcmp(data_from_stdin, "n") & strcmp(data_from_stdin, "next"))
    {
        std::vector<char *> commands = GetTokens(data_from_stdin);

        char *command = commands[0];

        if (!(strcmp(command, "h") & strcmp(command, "help")))
        {
            fprintf(stdout, "Commands:\n");
            fprintf(stdout, "\tnext (n):   Step to next instruction\n");
            fprintf(stdout, "\tstatus (s): Prints status of registers and flags\n");
            fprintf(stdout, "\tstep (st) <#>: Runs for # amount of instructions\n");
            fprintf(stdout, "\tprint (p): Prints current address and opcode\n");
            fprintf(stdout, "\tpm <#>: Prints memory at region (begin with 0x to display hex)\n");
            fprintf(stdout, "\tc: Continue program execution\n");
            fprintf(stdout, "\thelp (h):   Get a list of commands\n");
        }
        else if (!(strcmp(command, "s") & strcmp(command, "status")))
        {
            PrintStack();
        }
        else if (!(strcmp(command, "st") & strcmp(command, "step")))
        {
            step = instr_executed + atoi(commands[1]);
            debug = false;
            break;
        }
        else if (!(strcmp(command, "p") & strcmp(command, "print")))
        {
            fprintf(stdout, "Current Address: %04x\tCurrent opcode: %02x\n", ip - 1 + startAddress, opcodes[ip - 1]);
        }
        else if (!strcmp(command, "pm"))
        {
            int start;

            if (commands[1][0] == '0' && commands[1][1] == 'x')
            {
                start = (int)strtol(commands[1], NULL, 16);
            }
            else
            {
                start = atoi(commands[1]);
            }
            fprintf(stdout, "Memory from %04x:\n", start);

            for (int i = 0; i < 5; i++)
            {
                for (int j = 0; j < 16; j++)
                {
                    fprintf(stdout, "%02x ", data[start + j + (i * 16)]);
                }
                fprintf(stdout, "\n");
            }
        }
        else if (!(strcmp(command, "c") & strcmp(command, "cont")))
        {
            debug = false;
            break;
        }
        else
        {
            printf("Command %s not found, type h or help for list of commands\n", data_from_stdin);
        }

        data_from_stdin = read_async();
    }
}

void DOSEmulator::Push(short val)
{
    short sp_offset = (registers[SP][0] << 8) + registers[SP][1];

    data[--sp_offset + (header->hdrsize * 16)] = val & 0xFF;
    data[--sp_offset + (header->hdrsize * 16)] = (val >> 8) & 0xFF;

    registers[SP][0] = (sp_offset >> 8) & 0xFF;
    registers[SP][1] = sp_offset & 0xFF;
}

void DOSEmulator::Push8(char val)
{
    short sp_offset = (registers[SP][0] << 8) + registers[SP][1];

    data[--sp_offset + (header->hdrsize * 16)] = val;

    registers[SP][0] = (sp_offset >> 8) & 0xFF;
    registers[SP][1] = sp_offset & 0xFF;
}

short DOSEmulator::Pop()
{
    short sp_offset = (registers[SP][0] << 8) + registers[SP][1];

    short val = ((data[sp_offset++ + (header->hdrsize * 16)] << 8) & 0xFF) +
                data[sp_offset++ + (header->hdrsize * 16)];

    registers[SP][0] = (sp_offset >> 8) & 0xFF;
    registers[SP][1] = sp_offset & 0xFF;

    return val;
}

char DOSEmulator::Pop8()
{
    short sp_offset = (registers[SP][0] << 8) + registers[SP][1];

    char val = data[sp_offset++ + (header->hdrsize * 16)];

    registers[SP][0] = (sp_offset >> 8) & 0xFF;
    registers[SP][1] = sp_offset & 0xFF;

    return val;
}

void DOSEmulator::RunCode()
{

    int instr_count = 0;

    instr_executed = 0;
    opcodes = data + startAddress;
    run = true;

    ClearFlags();

    SetRegistersFromHeader();

    unsigned char op = opcodes[ip++];

    while (run)
    {

        if (debug)
            DebugMenu();

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
            Push((special_registers[DS][0] << 8) + special_registers[DS][1]);
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
            op = opcodes[ip++];

            short val1 = (registers[GetRegister(op)][0] << 8) + registers[GetRegister(op)][1];
            short val2 = GetModMemVal(op, true);

            UpdateFlags(val1, val2, SUBTRACTION);

            short result = val1 - val2;

            registers[GetRegister(op)][0] = (result >> 8) & 0xFF;
            registers[GetRegister(op)][1] = result & 0xFF;

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
            op = opcodes[ip++];

            UpdateFlags(registers[GetRegister(op) % 4][GetRegister(op) <= 3], GetModMemVal8(op, true), SUBTRACTION);
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
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        {
            Push((registers[op - 0x50][0] << 8) + registers[op - 0x50][1]);
            break;
        }
        case 0x58:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
        {
            short val = Pop();
            registers[op - 0x58][0] = (val >> 8) & 0xFF;
            registers[op - 0x58][1] = val & 0xFF;
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
            op = opcodes[ip++];
            switch (GetRegister(op))
            {
            case CMP:
            {
                UpdateFlags(GetModMemVal8(op, true), opcodes[ip++], SUBTRACTION);
                break;
            }
            default:
                printf("Not Yet Implemented 0x80: %2x\n", op);
                break;
            }
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
                short val = GetModMemVal(op, true) & opcodes[ip++];
                SetModMemVal(val, op, true);
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
            op = opcodes[ip++];
            SetModMemVal8(registers[GetRegister(op) % 4][GetRegister(op) <= 3], op, true);
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

            registers[GetRegister(op) % 4][GetRegister(op) <= 3] = GetModMemVal8(op, true);

            break;
        }
        case 0x8b:
        {
            op = opcodes[ip++];

            short val = GetModMemVal(op, true);
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

            short val = GetModMemVal(op, true);
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
            short offset = opcodes[ip++] + (opcodes[ip++] << 8);

            registers[AX][AH] = GetDataStart()[offset + 1];
            registers[AX][AL] = GetDataStart()[offset];
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
            ip = call_stack[--csp];
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
            op = opcodes[ip++];
            SetModMemVal8(NULL, op, true);
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
                short val = GetModMemVal(op, true) >> 1;
                SetModMemVal(val, op, true);
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
            short rel = opcodes[ip++] + (opcodes[ip++] << 8);
            call_stack[csp++] = ip;
            ip += rel;
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
            switch (op)
            {
            case SCASB:
            {
                unsigned char *start = GetDataStart(ES) + (registers[DI][0] << 8) + registers[DI][1];

                flags[ZF] = false;

                while (*start != '$')
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
        // if (instr_count++ >= 2)
        //     run = false;

        instr_executed++;

        if (instr_executed == step)
            debug = true;

        op = opcodes[ip++];
    }
}

void DOSEmulator::StartEmulation()
{
    header = (DOS_HEADER *)data;

    if (header->signature[0] != 'M' || header->signature[1] != 'Z')
    {
        fprintf(stdout, "Not a DOS file\n");
        exit(1);
    }

    PrintHeader(header);

    startAddress = CalculateStartAddress();

    fprintf(stdout, "Runtime: %04x\n", startAddress);

    RunCode();
}