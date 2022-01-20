#include "./emulator.h"
#include <stdlib.h>

int main()
{

    // if (argc < 2)
    // {
    //     printf("No file provided\n");
    //     exit(1);
    // }

    FILE *fileptr;
    unsigned char *buffer;
    long filelen;

    // fileptr = fopen(argv[1], "rb");  // Open the file in binary mode
    fileptr = fopen("MOV.EXE", "rb");  // Open the file in binary mode
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    buffer = (unsigned char *)malloc(filelen * sizeof(unsigned char)); // Enough memory for the file
    fread(buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file

    // DOSEmulator emulator(buffer, argc > 2 ? true : false);
    DOSEmulator emulator(buffer, true);

    emulator.StartEmulation();
    
}