// a simple database

#define _GNU_SOURCE // getline()
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

typedef enum
{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct
{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;  //数据库中的一行数据，包含 ID、用户名和邮箱

typedef struct
{
    StatementType type; //语句类型
    Row row_to_insert;  //要插入的行数据
} Statement;    //一个 SQL 语句

typedef struct
{
    char *buffer;   //缓冲区指针
    size_t buffer_length; // 缓冲区长度
    ssize_t input_length; // 输入长度
} InputBuffer;  //从用户输入读取的数据


//创建一个新的输入缓冲区，初始化相应的字段
InputBuffer *new_input_buffer()
{
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

// data structure representing the table
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)NULL)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE; // 4+32+255=291

const uint32_t ID_OFFSET = 0;                                  // 0
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;          // 4
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE; // 36

// a table structure that points to pages of rows
const uint32_t PAGE_SIZE = 4096;    //一页大小
#define TABLE_MAX_PAGES 100         //总页数
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE; //每页的行数
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES; //总行数

typedef struct
{
    int file_descriptor;    //文件描述符
    uint32_t file_length;   //文件长度
    void *pages[TABLE_MAX_PAGES];   //页面数组
} Pager;    //对数据库文件的管理

typedef struct
{
    Pager *pager;   //Pager指针
    uint32_t num_rows;  //数据的行数
} Table;    //一个数据库表

void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    // memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    // memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
    strncpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
    strncpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

//根据页号从文件中读取页数据，实现简单的页面缓存
void* get_page(Pager* pager, uint32_t page_num)
{
    if(page_num > TABLE_MAX_PAGES){
        printf("Tried to fetch page number out of bounds. %d > %d\n", 
            page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    //检查指定页面是否已经缓存在内存中
    if(pager->pages[page_num] == NULL){
        //Cache miss. Allocate memory and load from file.
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;

        //如果文件大小不是页面大小的整数倍，
        //说明可能有不满一页的数据，因此需要额外的一个页面
        if(pager->file_length % PAGE_SIZE){
            num_pages += 1;
        }

        if(page_num <= num_pages){
            //将文件指针定位到指定页面的开始位置
            lseek(pager->file_descriptor, page_num*PAGE_SIZE, SEEK_SET);
            //文件中读取一页数据到刚刚分配的内存块
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if(bytes_read == -1){
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        //将读取的页面数据指针存储到 Pager 结构中的页面数组中
        pager->pages[page_num] = page;
    }
    //返回页面数据的指针
    return pager->pages[page_num];
}

// 根据行号计算出相应的页号和字节偏移，以便在内存中找到行数据的位置
void *row_slot(Table *table, uint32_t row_num)
{
    //计算所在页面的页号
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = get_page(table->pager, page_num);  //获取页面数据指针

    uint32_t row_offset = row_num % ROWS_PER_PAGE;  //页面中的行偏移
    uint32_t byte_offset = row_offset * ROW_SIZE;   //页面中的字节偏移
    return page + byte_offset;  //返回一个指向所需行数据的指针
}

// 函数打开数据库文件，初始化数据，返回一个 Pager 结构
Pager *pager_open(const char *filename)
{
    int fd = open(filename,
                  O_RDWR |     /*Read/Write mode*/
                      O_CREAT, /*Create file if it does not exist*/
                  S_IWUSR |    /*User write permission*/
                      S_IRUSR  /*User read permission*/
    );

    if(fd == -1){
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;

    for(uint32_t i = 0; i<TABLE_MAX_PAGES; i++){
        pager->pages[i] = NULL;
    }
    return pager;
}

// 打开数据库并返回一个 Table 结构
Table *db_open(const char *filename)
{
    Pager *pager = pager_open(filename);
    //行数
    uint32_t num_rows = pager->file_length / ROW_SIZE;
    
    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = num_rows;

    return table;
}

//将内存中的页数据写回文件
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size){
    if(pager->pages[page_num] == NULL){
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset = lseek(pager->file_descriptor, page_num*PAGE_SIZE, SEEK_SET);

    if(offset == -1){
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);

    if(bytes_written == -1){
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

//关闭数据库，同时释放内存中的页数据
void db_close(Table* table){
    Pager* pager = table->pager;    //获取指向数据库管理器 Pager 的指针
    uint32_t num_full_pages = table->num_rows/ROWS_PER_PAGE;    //满页的数量

    for(uint32_t i = 0; i<num_full_pages; i++){
        //检查当前页面是否为空
        if(pager->pages[i] == NULL){
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);   //将当前页面数据写回磁盘
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    //处理不满一页的数据
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if(num_additional_rows > 0){
        uint32_t page_num = num_full_pages; //最后一个满页面的页号
        if(pager->pages[page_num] != NULL){
            pager_flush(pager, page_num, num_additional_rows*ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }

    int result = close(pager->file_descriptor);
    if(result == -1){
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }

    for(uint32_t i = 0; i<TABLE_MAX_PAGES; i++){
        void* page = pager->pages[i];
        if(page){
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}

//打印行数据
void print_row(Row *row)
{
    printf("(%u, %s, %s)\n", row->id, row->username, row->email);
}

//---------------------------prototypes---------------------

void print_prompt();
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table);
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);
ExecuteResult execute_statement(Statement *statement, Table *table);

//---------------------------main---------------------

int main(int argc, char* argv[])
{
    if(argc<2){
        printf("Must supply database filename.\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    Table* table = db_open(filename);
    
    InputBuffer *input_buffer = new_input_buffer();

    // Keyword handling
    while (1)
    {
        print_prompt();
        read_input(input_buffer);

        // meta-commands
        if (input_buffer->buffer[0] == '.')
        {
            switch (do_meta_command(input_buffer, table))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized command '%s'\n", input_buffer->buffer);
                continue;
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement))
        {
        case (PREPARE_SUCCESS):
            break;
        case (PREPARE_NEGATIVE_ID):
            printf("ID must be positive.\n");
            continue;
        case (PREPARE_STRING_TOO_LONG):
            printf("String is too long.\n");
            continue;
        case (PREPARE_SYNTAX_ERROR):
            printf("Syntax error. Could not parse statement.\n");
            continue;
        case (PREPARE_UNRECOGNIZED_STATEMENT):
            printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
            continue;
        }

        switch (execute_statement(&statement, table))
        {
        case (EXECUTE_SUCCESS):
            printf("Executed.\n");
            break;
        case (EXECUTE_TABLE_FULL):
            printf("Error: Table full.\n");
            break;
        }
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
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        close_input_buffer(input_buffer);
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// parse the string from insert command
PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement)
{
    statement->type = STATEMENT_INSERT;
    //提取数据
    char *keyword = strtok(input_buffer->buffer, " ");
    char *id_string = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    if (id_string == NULL || username == NULL || email == NULL)
    {
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if (id < 0)
    {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(username) > COLUMN_USERNAME_SIZE)
    {
        return PREPARE_STRING_TOO_LONG;
    }
    if (strlen(email) > COLUMN_EMAIL_SIZE)
    {
        return PREPARE_STRING_TOO_LONG;
    }

    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email, email);

    return PREPARE_SUCCESS;
}

//"SQL Compiler" parse arguments
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement)
{
    //"insert" keyword will be followed by data
    if (strncmp(input_buffer->buffer, "insert", 6) == 0)
    {
        return prepare_insert(input_buffer, statement);
    }

    if (strcmp(input_buffer->buffer, "select") == 0)
    {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);
    void *row_memory = row_slot(table, table->num_rows);
    memset(row_memory, 0, ROW_SIZE);
    serialize_row(row_to_insert, row_memory); 
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
    case (STATEMENT_INSERT):
        return execute_insert(statement, table);
    case (STATEMENT_SELECT):
        return execute_select(statement, table);
    }
}