#include "./emulator.h"
#include <stdlib.h>
#include <emscripten/fetch.h>
#include <string.h>


char *file_data;
bool file_ready;

void get_file_success(emscripten_fetch_t *fetch)
{

    int len = 1;

    for (int i = 0; i < fetch->numBytes; i++)
    {
        if (fetch->data[i] == '\x2c') len++;
    }

    file_data = (char*)malloc(sizeof(char)*(len));

    char * converter = (char*)calloc(4, sizeof(char));

    int data_count = 0;
    int converter_count = 0;

    for (int i = 0; i < fetch->numBytes; i++)
    {
        if (fetch->data[i] == '\x2c')
        {
            converter[converter_count] = '\0';
            converter_count = 0;
            file_data[data_count++] = (char)atoi(converter);
        }
        else
        {
            converter[converter_count++] = fetch->data[i];
        }
    }

    converter[converter_count] = '\0';
    file_data[data_count++] = (char)atoi(converter);


    file_ready = true;
    emscripten_fetch_close(fetch);
}

void get_file_fail(emscripten_fetch_t *fetch)
{
    emscripten_fetch_close(fetch);
}


char *get_file_async()
{

    file_ready = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = get_file_success;
    attr.onerror = get_file_fail;
    emscripten_fetch(&attr, "___terminal::open_file");

    while (!file_ready)
    {
        emscripten_sleep(100);
    }

    return file_data;
}

int main()
{

    // if (argc < 2)
    // {
    //     printf("No file provided\n");
    //     exit(1);
    // }

    FILE *fileptr;
    unsigned char *buffer = (unsigned char *)get_file_async();
    long filelen;


    // fileptr = fopen(argv[1], "rb");  // Open the file in binary mode
    // fileptr = fopen("MOV.EXE", "rb");  // Open the file in binary mode
    // fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    // filelen = ftell(fileptr);             // Get the current byte offset in the file
    // rewind(fileptr);                      // Jump back to the beginning of the file

    // buffer = (unsigned char *)malloc(filelen * sizeof(unsigned char)); // Enough memory for the file
    // fread(buffer, filelen, 1, fileptr); // Read in the entire file
    // fclose(fileptr); // Close the file

    // DOSEmulator emulator(buffer, argc > 2 ? true : false);
    DOSEmulator emulator(buffer, true);

    emulator.StartEmulation();
    
}