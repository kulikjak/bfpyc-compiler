#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "Python.h"
#include "opcode.h"

#include "compiler.h"


FILE *inputFile;

bool initInput(const char* filename, u32 *mtime, u32 *source_size) {
    struct stat statbuf;

    inputFile = fopen(filename, "rt");
    if (!inputFile) {
        fprintf(stderr, "Cannot open given file: %s.\n", strerror(errno));
        return false;
    }

    if(stat(filename, &statbuf) == -1) {
        fprintf(stderr, "stat() failed: %s.\n", strerror(errno));
        return false;
    }

    *mtime = (u32)statbuf.st_mtime;
    *source_size = (u32)statbuf.st_size;

    return true;
}

u32 getChar() {
    static u8 line[INPUT_BUFFER_SIZE];
    static u8 *linePointer = line;

    if (!*linePointer) {
        if (!fgets((char*)line, INPUT_BUFFER_SIZE, inputFile)) {
            fclose(inputFile);
            return EOF;
        }
        linePointer = line;
    }
    return *linePointer++;
}


typedef enum {
  INC, DEC, PRINTC, SCANC, FWD, BWD, BOPEN, BCLOSE
} Instruction;

// Array of user readable instruction names.
const char *InstructionTable[8] = {
  "INC", "DEC", "PRINTC", "SCANC", "FWD", "BWD", "BOPEN", "BCLOSE"
};


bool nextInstruction(u32* line, u8* instr) {
    u32 character;
    while(1) {
        character = getChar();
        if (character == EOF) {
            return false;
        }

        switch((u8)character) {
            case '+':
                *instr = INC;
                return true;
            case '-':
                *instr = DEC;
                return true;
            case '.':
                *instr = PRINTC;
                return true;
            case ',':
                *instr = SCANC;
                return true;
            case '>':
                *instr = FWD;
                return true;
            case '<':
                *instr = BWD;
                return true;
            case '[':
                *instr = BOPEN;
                return true;
            case ']':
                *instr = BCLOSE;
                return true;
            case '\n':
                /* remember information about line numbers */
                *line += 1;
                break;
            /* happy to fall through */
        }
    }
    _UNREACHABLE;
}


const u8 symbol_table_size = 10;
enum symb_table {
    SYMB_P, SYMB_A,
    SYMB_SYS, SYMB_STDIN, SYMB_STDOUT, SYMB_READ, SYMB_WRITE,
    SYMB_CHR, SYMB_ORD, SYMB_APPEND
};

char *symb_values[10] = {
    "p", "a", "sys", "stdin", "stdout", "read", "write",
    "chr", "ord", "append"
};

#define _NOARG 0


typedef struct {
    char* buffer;
    u32 curr;
    u32 size;
} compilation_block;


compilation_block* new_block() {
    compilation_block *block = (compilation_block*)malloc(sizeof(compilation_block));
    block->curr = 0;
    block->size = 512;
    block->buffer = (char*)malloc(block->size);
    return block;
}

void block_free(compilation_block* block) {
    free(block->buffer);
    free(block);
}

void block_append(compilation_block* block, compilation_block* next) {
    while (block->size < block->curr + next->curr) {
        block->size += 512;
        block->buffer = (char*)realloc(block->buffer, block->size);
    }
    memcpy(block->buffer + block->curr, next->buffer, next->curr);
    block->curr += next->curr;
}


void write_bytecode(compilation_block* block, i32 len, ...) {
    /* reserve enough space in the bytecode block */
    while (block->size < block->curr + len) {
        block->size += 512;
        block->buffer = (char*)realloc(block->buffer, block->size);
    }

    /* copy all instructions into the block */
    va_list args;
    va_start(args, len);

    for (i32 i = 0; i < len; i++) {
        block->buffer[block->curr++] = (u8)va_arg(args, int);
    }
    va_end(args);
}

void fwrite_bytecode(FILE* fp, i32 len, ...) {
    va_list args;
    va_start(args, len);

    for (i32 i = 0; i < len; i++) {
        fputc((u8)va_arg(args, int), fp);
    }
    va_end(args);
}


compilation_block* compile_block(int parent_counter) {
    u32 line = 1;
    u8 instr;

    compilation_block *block = new_block();

    if (parent_counter == 0) {
        /* Output all the initialization code here */

        write_bytecode(block, 40,
          /* import sys library for IO operations. */
          LOAD_CONST,    0,
          LOAD_CONST,    3,  /* None */
          IMPORT_NAME,   SYMB_SYS,
          STORE_NAME,    SYMB_SYS,

          /* pointer is first initialized to size of the main array
           * and used to initialize it to zeroes below. */
          LOAD_CONST,    2,  /* BF_ARRAY_SIZE */
          STORE_NAME,    SYMB_P,

          /* create the main array */
          BUILD_LIST,    0,
          STORE_NAME,    SYMB_A,

          /* loop until pointer is zero */
          LOAD_NAME,          SYMB_P,
          POP_JUMP_IF_FALSE,  40,

          /* append zero */
          LOAD_NAME,     SYMB_A,
          LOAD_METHOD,   SYMB_APPEND,
          LOAD_CONST,    0,
          CALL_METHOD,   1,
          POP_TOP,       _NOARG,

          /* decrement the pointer and jump back */
          LOAD_NAME,         SYMB_P,
          LOAD_CONST,        1,
          INPLACE_SUBTRACT,  _NOARG,
          STORE_NAME,        SYMB_P,
          JUMP_ABSOLUTE,     16
        );

        /* pointer now points to the first element of the list */
    }

    while(nextInstruction(&line, &instr)) {
        switch(instr) {
            case INC:
            case DEC:
                write_bytecode(block, 16,
                  LOAD_NAME,     SYMB_A,
                  LOAD_NAME,     SYMB_P,
                  DUP_TOP_TWO,   _NOARG,
                  BINARY_SUBSCR, _NOARG,
                  LOAD_CONST,    1,
                  (instr == INC) ? INPLACE_ADD : INPLACE_SUBTRACT, _NOARG,
                  ROT_THREE,     _NOARG,
                  STORE_SUBSCR,  _NOARG
                );
                break;

            case FWD:
            case BWD:
                write_bytecode(block, 8,
                  LOAD_NAME,     SYMB_P,
                  LOAD_CONST,    1,
                  (instr == FWD) ? INPLACE_ADD : INPLACE_SUBTRACT, _NOARG,
                  STORE_NAME,    SYMB_P
                );
                break;

            case PRINTC:
                write_bytecode(block, 20,
                  LOAD_NAME,     SYMB_SYS,
                  LOAD_ATTR,     SYMB_STDOUT,
                  LOAD_METHOD,   SYMB_WRITE,
                  LOAD_NAME,     SYMB_CHR,
                  LOAD_NAME,     SYMB_A,
                  LOAD_NAME,     SYMB_P,
                  BINARY_SUBSCR, _NOARG,
                  CALL_FUNCTION, 1,
                  CALL_METHOD,   1,
                  POP_TOP,       _NOARG
                );
                break;
            case SCANC:
                write_bytecode(block, 20,
                  LOAD_NAME,     SYMB_ORD,
                  LOAD_NAME,     SYMB_SYS,
                  LOAD_ATTR,     SYMB_STDIN,
                  LOAD_METHOD,   SYMB_READ,
                  LOAD_CONST,    1,
                  CALL_METHOD,   1 ,
                  CALL_FUNCTION, 1,
                  LOAD_NAME,     SYMB_A,
                  LOAD_NAME,     SYMB_P,
                  STORE_SUBSCR,  _NOARG
                );
                break;

            case BOPEN:
                write_bytecode(block, 6,
                  LOAD_NAME,     SYMB_A,
                  LOAD_NAME,     SYMB_P,
                  BINARY_SUBSCR, _NOARG
                );

                /* the position of jump (and size of its arguments) are
                 * determined by the size of the following nested block */
                compilation_block *nested = compile_block(parent_counter + block->curr + 8);

                u32 targetpos = parent_counter + block->curr + 8 + nested->curr;
                write_bytecode(block, 8,
                  EXTENDED_ARG,      (targetpos >> 24) & 0xff,
                  EXTENDED_ARG,      (targetpos >> 16) & 0xff,
                  EXTENDED_ARG,      (targetpos >> 8) & 0xff,
                  POP_JUMP_IF_FALSE, (targetpos) & 0xff
                );

                /* now merge both blocks together */
                block_append(block, nested);
                block_free(nested);
                break;

            case BCLOSE:
                /* if we encountered BCLOSE when not in a nested block, this is an error */
                if (parent_counter == 0) {
                    fprintf(stderr, "Unexpected ']' encountered on line %d\n", line);
                    exit(EXIT_FAILURE);
                }

                write_bytecode(block, 14,
                  LOAD_NAME,     SYMB_A,
                  LOAD_NAME,     SYMB_P,
                  BINARY_SUBSCR, _NOARG,

                  /* jump at the beginning of this block */
                  EXTENDED_ARG,     (parent_counter >> 24) & 0xff,
                  EXTENDED_ARG,     (parent_counter >> 16) & 0xff,
                  EXTENDED_ARG,     (parent_counter >> 8) & 0xff,
                  POP_JUMP_IF_TRUE, (parent_counter) & 0xff
                );
                return block;

            default:
                _UNREACHABLE;
        }
    }
    /* we never moved out of the nested block */
    if (parent_counter != 0) {
        fprintf(stderr, "Missing matching ']'\n");
        exit(EXIT_FAILURE);
    } else {
        /* Output all the closing bytecode */
        write_bytecode(block, 4,
          LOAD_CONST,    3,  /* None */
          RETURN_VALUE,  _NOARG
        );
    }
    return block;
}


#define _le(val)           \
    ((val)       & 0xff),  \
    ((val >> 8)  & 0xff),  \
    ((val >> 16) & 0xff),  \
    ((val >> 24) & 0xff)

void write_consts_section(FILE *fp) {
    /* considering the simplicity of input programs,
     * constants are always the same */

    fwrite_bytecode(fp, 18,
      TYPE_SMALL_TUPLE, 4,
      TYPE_INT, _le(0),
      TYPE_INT, _le(1),
      TYPE_INT, _le(BF_ARRAY_SIZE),
      TYPE_NONE
    );
}

void write_names_section(FILE *fp) {
    fputc(TYPE_SMALL_TUPLE, fp);
    fputc(symbol_table_size, fp);

    for (u8 i = 0; i < symbol_table_size; i++) {
        fputc(TYPE_SHORT_ASCII_INTERNED, fp);
        fputc((u8)strlen(symb_values[i]), fp);
        fputs(symb_values[i], fp);
    }
}

void write_code_section(compilation_block* bytecode, FILE *fp) {
    fwrite_bytecode(fp, 5,
      TYPE_STRING, _le(bytecode->curr) /* code size */
    );
    fwrite(bytecode->buffer, bytecode->curr, 1, fp);
}


FILE* get_output_file(const char* sourceName, const char* outputName) {

    FILE* output_file;
    char newName[1024];

    if (outputName != NULL) {
        int len = strlen(outputName);
        /* simple check for buffer overflow */
        if (len >= 1008) {
            fprintf(stderr, "Output filename is too long.\n");
            return NULL;
        }
        strcpy(newName, outputName);

        /* don't add anything if given name already ends with the .pyc extension */
        if (strcmp(outputName + len - 4, ".pyc")) {
            sprintf(newName + len, ".cpython-39.pyc");
        }
    } else {

        int len = strlen(sourceName);
        /* simple check for buffer overflow */
        if (len >= 1008) {
            fprintf(stderr, "Input filename is too long.\n");
            return NULL;
        }

        strcpy(newName, sourceName);

        if (!strcmp(sourceName + len - 3, ".bf")) {  /* standard '.bf' extension */
            sprintf(newName + len - 2, "cpython-39.pyc");
        } else if (!strcmp(sourceName + len - 2, ".b")) {  /* shorter '.b' extension */
            sprintf(newName + len - 1, "cpython-39.pyc");
        } else {  /* different or no extension -> just append the new one */
            sprintf(newName + len, ".cpython-39.pyc");
        }
    }

    output_file = fopen(newName, "w+");
    if (!output_file) {
        fprintf(stderr, "Cannot open output file '%s': %s.\n", newName, strerror(errno));
        return NULL;
    }
    return output_file;
}

bool compile_source(const char* sourceName, const char* outputName) {

    u32 mtime, source_size;

    if(!initInput(sourceName, &mtime, &source_size)) {
        return false;
    }
    compilation_block* bytecode = compile_block(0);

    FILE* fp = get_output_file(sourceName, outputName);
    if (!fp) {
        return false;
    }

    /* pyc header */
    u32 magic = MAGIC_NUMBER + (0x0a0d << 16);
    u32 flags = 0;

    /* first write pyc header */
    fwrite_bytecode(fp, 16,
      _le(magic), _le(flags),
      _le(mtime), _le(source_size)
    );

    /* this file has no arguments of any kind */
    u32 co_argcount = 0;
    u32 co_posonlyargcount = 0;
    u32 co_kwonlyargcount = 0;
    /* there are no locals ever */
    u32 co_nlocals = 0;
    /* must be enough to hold everything pushed onto the stack */
    u32 co_stacksize = 32;
    /* flags are defined and described here:
     * https://github.com/python/cpython/blob/3.9/Include/cpython/code.h */
    u32 co_flags = 0x40;

    fwrite_bytecode(fp, 25,
      TYPE_CODE,
      _le(co_argcount),
      _le(co_posonlyargcount),
      _le(co_kwonlyargcount),
      _le(co_nlocals),
      _le(co_stacksize),
      _le(co_flags)
    );

    write_code_section(bytecode, fp);
    block_free(bytecode);

    write_consts_section(fp);
    write_names_section(fp);

    fwrite_bytecode(fp, 6,
      /* varnames - tuple of strings (local variable names) */
      TYPE_SMALL_TUPLE, 0,

      /* freevars - tuple of strings (free variable names) */
      TYPE_SMALL_TUPLE, 0,

      /* cellvars - tuple of strings (cell variable names) */
      TYPE_SMALL_TUPLE, 0
    );

    /* source filename */
    fputc(TYPE_SHORT_ASCII, fp);
    fputc((u8)strlen(sourceName), fp);
    fputs(sourceName, fp);

    /* module name */
    fputc(TYPE_SHORT_ASCII_INTERNED, fp);
    fputc((u8)strlen("<module>"), fp);
    fputs("<module>", fp);

    fwrite_bytecode(fp, 9,
      /* first source line number */
      _le(1),

      /* lnotab - mapping between bytecode and original source lines. This
       * is not implemented as there can be almost no possible errors
       * in the resulting bytecode. */
      TYPE_STRING, _le(0)
    );
    fclose(fp);
    return true;
}

int main(int argc, char *const argv[]) {

    int c;
    char* outputName = NULL;
    extern char *optarg;
    extern int optind, optopt;

    while ((c = getopt(argc, argv, ":o:")) != -1) {
        switch(c) {
        case 'o':
            outputName = optarg;
            break;
        case ':':
            fprintf(stderr, "Argument -%c is missing a value.\n", optopt);
            return EXIT_FAILURE;
        case '?':
            fprintf(stderr, "Unknown argument %c.\n", optopt);
            return EXIT_FAILURE;
        } 
    }

    if (optind == argc) {
        fprintf(stderr, "No input files given.\n");
        return EXIT_FAILURE;
    }

    if (!compile_source(argv[optind], outputName)) {
        fprintf(stderr, "Compilation failed.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
