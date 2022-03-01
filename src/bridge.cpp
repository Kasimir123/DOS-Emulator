#include "bridge.h"
#include <stdlib.h>
#include <string>

// Data for the file we read in
char *file_data;

// Bool for whether or not we have read the file
bool file_ready;

// Function that gets run if we were able to get the file
void get_file_success(emscripten_fetch_t *fetch)
{

    // File gets sent in with the format:
    // 77,88,99
    // So we will have one number that doesn't have the comma so start with 1
    int len = 1;

    // Loop through the data and see how many ','s we have
    for (int i = 0; i < fetch->numBytes; i++)
    {
        if (fetch->data[i] == '\x2c')
            len++;
    }

    // malloc enough space for all the bytes in the file
    file_data = (char *)malloc(sizeof(char) * (len));

    // Create an empty array of 4 chars, will use this to convert 
    // the numbers back into chars
    char *converter = (char *)calloc(4, sizeof(char));

    // Initialize the two counts
    int data_count = 0;
    int converter_count = 0;

    // This loop splits on ',' and converts numbers to char
    for (int i = 0; i < fetch->numBytes; i++)
    {
        // If the current character is a ','
        if (fetch->data[i] == '\x2c')
        {
            // Set the converter value to null terminator
            converter[converter_count] = '\0';

            // reset converter count
            converter_count = 0;

            // Add the char to the file_data
            file_data[data_count++] = (char)atoi(converter);
        }
        else
        {
            // Otherwise we add the value to our converter
            converter[converter_count++] = fetch->data[i];
        }
    }

    // Get the last data value
    converter[converter_count] = '\0';
    file_data[data_count++] = (char)atoi(converter);

    // Set file_ready to true so our program can continue
    file_ready = true;

    // Close fetch
    emscripten_fetch_close(fetch);
}

// Function that gets run if we were unable to get the file
void get_file_fail(emscripten_fetch_t *fetch)
{
    // Close fetch
    emscripten_fetch_close(fetch);
}

// Try to get the file from the frontend
char *get_file_async()
{
    // Set file_ready to false so we can block later
    file_ready = false;

    // Declare and initialize our attribute
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    // Set the request method to get
    strcpy(attr.requestMethod, "GET");

    // Set attribute
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    // Set success and error functions
    attr.onsuccess = get_file_success;
    attr.onerror = get_file_fail;

    // Fetch the data using the open_file method
    emscripten_fetch(&attr, "___emulator::open_file");

    // While our data isn't ready do our non-blocking block
    while (!file_ready)
    {
        // Non blocking block for 100 ms
        emscripten_sleep(100);
    }

    // Return file data
    return file_data;
}


// char pointer for the data we get from the frontend
char *read_data;

// boolean to see if we have gotten the data
bool ready;

// Function that gets called if we successfully received data
void read_success(emscripten_fetch_t *fetch)
{
    if (read_data)
        free(read_data);

    read_data = (char *)malloc(sizeof(char) * (fetch->numBytes + 1));

    memcpy(read_data, fetch->data, fetch->numBytes);

    // append a newline and string terminator, this is for command parsing
    read_data[fetch->numBytes - 1] = '\n';
    read_data[fetch->numBytes] = '\0';

    ready = true;
    emscripten_fetch_close(fetch);
}

// function that gets called on failure
void read_fail(emscripten_fetch_t *fetch)
{
    emscripten_fetch_close(fetch);
}

// read data from the frontend
char *read_async()
{

    ready = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = read_success;
    attr.onerror = read_fail;
    emscripten_fetch(&attr, "___emulator::read");

    while (!ready)
    {
        emscripten_sleep(50);
    }

    return read_data;
}

// get a character from the frontend
char get_char_async(bool wait)
{
    ready = false;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = read_success;
    attr.onerror = read_fail;
    if (wait)
    {
        emscripten_fetch(&attr, "___emulator::get_char_now");
    }
    else
    {
        emscripten_fetch(&attr, "___emulator::get_char");
    }

    while (!ready)
    {
        emscripten_sleep(50);
    }

    return read_data[0];
}

// send a message and a character to the frontend
void send_ping_and_char_async(char *command, char c)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    std::string url("___emulator::");

    url.append(command);

    url.append("::");

    url.append(1, c);

    emscripten_fetch(&attr, url.c_str());
}

// send a message to the frontend
void send_ping_async(char *command)
{
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    std::string url("___emulator::");

    url.append(command);

    emscripten_fetch(&attr, url.c_str());
}