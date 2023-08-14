//a simple database

//Making a Simple REPL

#define  _GNU_SOURCE    //getline()
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


typedef struct{
    char* buffer;
    size_t buffer_length;   //typedef unsigned long size_t
    ssize_t input_length;   //typedef long ssize_t
} InputBuffer;

InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

//prototypes
void print_prompt();
void read_input(InputBuffer* input_buffer);
void close_input_buffer(InputBuffer* input_buffer);

//---------------------------main---------------------

int main(int argc, char* argv[])
{
    InputBuffer* input_buffer = new_input_buffer();
    
    while(1){
        print_prompt();
        read_input(input_buffer);

        if(strcmp(input_buffer->buffer, ".exit") == 0){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }else{
            printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        }

    }

    return 0;
}

//prints a prompt to the user
void print_prompt() {printf("db > ");}

//read input
void read_input(InputBuffer* input_buffer){
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if(bytes_read <= 0){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    //Ignore the trailing newline character
    input_buffer->input_length = bytes_read -1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

//free the memory allocated
void close_input_buffer(InputBuffer* input_buffer){
    free(input_buffer->buffer);
    free(input_buffer);
}