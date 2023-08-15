// a simple database

#define _GNU_SOURCE // getline()
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct
{
    char *buffer;
    size_t buffer_length; // typedef unsigned long size_t
    ssize_t input_length; // typedef long ssize_t
} InputBuffer;

InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

typedef struct
{
    StatementType type;
} Statement;

// prototypes
void print_prompt();
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);

MetaCommandResult do_meta_command(InputBuffer *input_buffer);
PrepareResult prepare_statment(InputBuffer *input_buffer, Statement *statement);
void execute_statement(Statement *statement);

//---------------------------main---------------------

int main(int argc, char *argv[])
{
    InputBuffer *input_buffer = new_input_buffer();

    // Keyword handling
    while (1)
    {
        print_prompt();
        read_input(input_buffer);

        //meta-commands
        if (input_buffer->buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized command '%s'\n", input_buffer->buffer);
                continue;
            }
        }

        Statement statement;
        switch (prepare_statment(input_buffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        }

        //pass the prepared statement to execute_statement
        execute_statement(&statement);
        printf("Executed.\n");
    }

    return 0;
}



//---------------------------function---------------------

// prints a prompt to the user
void print_prompt() { printf("db > "); }

// read input
void read_input(InputBuffer *input_buffer)
{
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0)
    {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // Ignore the trailing newline character
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

// free the memory allocated
void close_input_buffer(InputBuffer *input_buffer)
{
    free(input_buffer->buffer);
    free(input_buffer);
}

// a wrapper for existing functionality, leaves room for more commands
MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

//"SQL Compiler"
PrepareResult prepare_statment(InputBuffer *input_buffer, Statement *statement)
{
    //"insert" keyword will be followed by data
    if (strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (strcmp(input_buffer->buffer, "select") == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement *statement)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        printf("This is where we would do an insert.\n");
        break;
    case (STATEMENT_SELECT):
        printf("This is where we would do a select.\n");
        break;
    }
}