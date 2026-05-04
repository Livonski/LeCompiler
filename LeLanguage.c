#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "LeParser.c"

//Global consts
#define MAX_VARIABLE_COUNT 32
#define MAX_VARIABLE_NAME_LENGTH 32

//Useful macro
#define LE_ERROR_EXIT(errorCode, msg) \
    do { \
        fprintf(stderr, "Error(%d): %s\nFile: %s\nLine: %d\n", errorCode, msg, __FILE__, __LINE__); \
        exit(errorCode); \
    } while(0);


#define LE_EXPECT_NEXT(parserPtr, resultPtr, type) \
    do { \
        if(leGetAndExpectNext((parserPtr), (resultPtr), (type)) != 0){ \
            LE_ERROR_EXIT(4, "expected token not found"); \
        } \
    } while(0)

//Struct declarations
typedef struct {
    char name[MAX_VARIABLE_NAME_LENGTH];
    int stackOffset;
} le_variable;

typedef struct {
    le_variable variables[MAX_VARIABLE_COUNT];
    int variableCount;
    int nextStackOffset;
} le_scope;

//Forward declarations
int leAddVariable(le_scope* scope, const char* name);
int leFindVariable(le_scope* scope, const char* name);

void leGenerateExpression(FILE *out, le_parser *parser, le_scope *scope);

int main(int argc, char** argv){

    if (argc < 2) LE_ERROR_EXIT(1, "Usage: LeLanguage.exe <file>");

    const char* filePath = argv[1];
    const char* input = argv[1];

    char base[256];
    char asmFile[256];
    char exeFile[256];

    strcpy(base, input);

    char* dot = strrchr(base, '.');
    if(dot != NULL){
        *dot = '\0';
    }
    
    snprintf(asmFile, sizeof(asmFile), "%s.asm", base);
    snprintf(exeFile, sizeof(exeFile), "%s.exe", base);

    FILE *source = fopen(filePath, "r");

    if(source == NULL) LE_ERROR_EXIT(2, "Error while opening file");

    //File parsing
    le_parser parser;
    leParse(&parser, source);
    printf("parsed %d tokens\n", parser.numTokens);
    
    fclose(source);

    //assembly generation
    //for now we only expect that program will contain return [number]
    //any other code will be treated as error
    printf("generating assembly\n");
    FILE* out = fopen(asmFile, "w");

    if (out == NULL)
    {
        LE_ERROR_EXIT(5, "cannot create asm file");   
    }

    fprintf(out, "format PE64 console\n"); 
    fprintf(out, "entry main\n\n"); 

    fprintf(out, "section '.text' code readable executable\n\n"); 

    fprintf(out, "main:\n"); 

    le_scope scope;
    scope.variableCount = 0;
    scope.nextStackOffset = 0x20;

    le_token curr;
    le_token next;
    while(leGetNext(&parser, &curr) == 0){    
        //printf("-Token[%d], tokenType[%d], name[%s]\n", curr.ID, curr.tokenType, curr.name);   
        switch(curr.tokenType){
            case LE_TOKEN_TYPE_NONE:
                char *varName = curr.name;

                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_EQUALS);
                
                leGenerateExpression(out, &parser, &scope);
                
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_SEMICOLON);

                int offset = leAddVariable(&scope, varName);
                fprintf(out, " mov dword [rsp + %Xh], eax\n", offset);
            break;

            case LE_TOKEN_TYPE_KEYWORD_RETURN:
                leGenerateExpression(out, &parser, &scope);
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_SEMICOLON);
                fprintf(out, " mov ecx, eax\n");
                fprintf(out, " call [ExitProcess]\n");
                break;

            default:
                LE_ERROR_EXIT(3, "unexpected token");
                break;
        }
    }

    fprintf(out, "section '.idata' import data readable writeable\n\n");

    fprintf(out, "dd 0, 0, 0, RVA kernel32_name, RVA kernel32_table\n"); 
    fprintf(out, "dd 0, 0, 0, 0, 0\n\n"); 

    fprintf(out, "kernel32_table:\n"); 
    fprintf(out, " ExitProcess dq RVA _ExitProcess\n"); 
    fprintf(out, " dq 0\n\n"); 

    fprintf(out, "kernel32_name db 'kernel32.dll', 0\n\n"); 
    
    fprintf(out, "_ExitProcess:\n"); 
    fprintf(out, " dw 0\n"); 
    fprintf(out, " db 'ExitProcess', 0\n"); 
    
    fclose(out);

    printf("Generated %s\n", asmFile);
    
    //building final executable
    char fasmCall[512];
    snprintf(fasmCall, sizeof(fasmCall), "fasm %s %s", asmFile, exeFile);

    int result = system(fasmCall); 
    if (result != 0) { printf("Error: FASM failed with code %d\n", result); return 1; } 
    printf("Generated %s\n", exeFile); 
    return 0;
}

int leAddVariable(le_scope *scope, const char *name){
    if(scope->variableCount >= MAX_VARIABLE_COUNT){
        LE_ERROR_EXIT(10, "too many variables");
    }

    if(leFindVariable(scope, name) != -1){
        LE_ERROR_EXIT(11, "two wariables with same name");
    }

    int offset = scope->nextStackOffset;

    strcpy(scope->variables[scope->variableCount].name, name);
    scope->variables[scope->variableCount].stackOffset = offset;

    scope->variableCount++;
    scope->nextStackOffset +=4;

    return offset;
}

int leFindVariable(le_scope* scope, const char* name){
    for(int i = 0; i < scope->variableCount; i++){
        if(strcmp(scope->variables[i].name, name) == 0){
            return scope->variables[i].stackOffset;
        }
    }

    return -1;
}

void leGenerateTerm(FILE *out, le_parser *parser, le_scope *scope){
    le_token tok;

    if(leGetNext(parser, &tok) != 0){
        LE_ERROR_EXIT(20, "expected term");
    }

    if(tok.tokenType == LE_TOKEN_TYPE_NUMBER){
        fprintf(out, " mov eax, %d\n", tok.numberValue);
    }
    else if(tok.tokenType == LE_TOKEN_TYPE_NONE){
        int offset = leFindVariable(scope, tok.name);

        if(offset < 0){
            LE_ERROR_EXIT(21, "unknown variable");
        }

        fprintf(out, " mov eax, dword [rsp + %Xh]\n", offset);
    }
    else{
        LE_ERROR_EXIT(22, "expected number or variable");
    }
}

void leGenerateTermToEbx(FILE* out, le_parser* parser, le_scope* scope){
    le_token tok;

    if(leGetNext(parser, &tok) != 0){
        LE_ERROR_EXIT(23, "expected term");
    }

    if(tok.tokenType == LE_TOKEN_TYPE_NUMBER){
        fprintf(out, " mov ebx, %d\n", tok.numberValue);
    }
    else if(tok.tokenType == LE_TOKEN_TYPE_NONE){
        int offset = leFindVariable(scope, tok.name);

        if(offset < 0){
            LE_ERROR_EXIT(24, "unknown variable");
        }

        fprintf(out, " mov ebx, dword [rsp + %Xh]\n", offset);
    }
    else{
        LE_ERROR_EXIT(25, "expected number or variable");
    }
}

void leGenerateExpression(FILE *out, le_parser *parser, le_scope *scope){
    le_token tok;

    leGenerateTerm(out, parser, scope);

    while(lePeekNext(parser, &tok) == 0){
        if(tok.tokenType != LE_TOKEN_TYPE_PLUS &&
           tok.tokenType != LE_TOKEN_TYPE_MINUS){
            break;
        }

        leGetNext(parser, &tok);

        if(tok.tokenType == LE_TOKEN_TYPE_PLUS){
            leGenerateTermToEbx(out, parser, scope);
            fprintf(out, " add eax, ebx\n");
        }
        else if(tok.tokenType == LE_TOKEN_TYPE_MINUS){
            leGenerateTermToEbx(out, parser, scope);
            fprintf(out, " sub eax, ebx\n");
        }
    }
}