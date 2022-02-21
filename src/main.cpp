#include "./emulator.h"
#include "bridge.h"

unsigned char *get_buffer(const char *file_name)
{
    FILE *fileptr;
    unsigned char *buffer;
    long filelen;

    fileptr = fopen(file_name, "rb"); // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);      // Jump to the end of the file
    filelen = ftell(fileptr);         // Get the current byte offset in the file
    rewind(fileptr);                  // Jump back to the beginning of the file

    buffer = (unsigned char *)malloc(filelen * sizeof(unsigned char)); // Enough memory for the file
    fread(buffer, filelen, 1, fileptr);                                // Read in the entire file
    fclose(fileptr);                                                   // Close the file

    return buffer;
}

// Main function
int main()
{
    while (true)
    {
        fprintf(stdout, "Which of the following do you wish to run?\n");
        fprintf(stdout, "\t1) HELLOM.EXE: Prints out Hello World\n");
        fprintf(stdout, "\t2) KEY.EXE: Takes in input, if input is a character then capitalize, if period, exit\n");
        fprintf(stdout, "\t3) PONG.EXE: Pong video game\n");
        fprintf(stdout, "\t4) TEST.EXE: Tests moving values around registers and printing as ASCII\n");
        fprintf(stdout, "\t5) Upload your own file\n");

        char user_input = get_char_async();

        unsigned char *buffer;

        if (user_input == '1')
        {
            buffer = get_buffer("./examples/HELLOM.EXE");
        }
        else if (user_input == '2')
        {
            buffer = get_buffer("./examples/KEY.EXE");
        }
        else if (user_input == '3')
        {
            buffer = get_buffer("./examples/PONG.EXE");
        }
        else if (user_input == '4')
        {
            buffer = get_buffer("./examples/TEST.EXE");
        }
        else
        {
            // Get the file and set the buffer to its contents
            buffer = (unsigned char *)get_file_async();
        }

        // Initialize the emulator with the buffer and the debugger set to true
        DOSEmulator emulator(buffer, true);

        // Start the emulator
        emulator.StartEmulation();
    }
}