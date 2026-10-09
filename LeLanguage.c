#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "LeParser.c"
#include "DA.h"

//Global consts
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
    le_variable *items;
    int count;
    int capacity;
} le_variables;

typedef struct le_scope{
    struct le_scope *parent;

    le_variables variables;

    int stackBase;
    int nextStackOffset;
    int maxStackOffset;
} le_scope;

//Forward declarations
int leAddVariable(le_scope* scope, const char* name);
int leFindVariable(le_scope* scope, const char* name);
int leResolveAssignment(le_scope *scope, const char *name);

void leGenerateExpression(FILE *out, le_parser *parser, le_scope *scope);

int leAssembleCodeBlock(FILE *out, le_parser *parser, le_scope *scope);

le_scope leCreateScope(le_scope *parent);
void leDestroyScope(le_scope *scope);

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
    le_parser parser = {0};
    leParse(&parser, source);
    printf("parsed %d tokens\n", parser.tokens.count);
    
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
    fprintf(out, "entry _start\n\n"); 

    fprintf(out, "section '.text' code readable executable\n\n"); 
    //runtime stuff
    fprintf(out, "\n");
    fprintf(out, "_start:\n");
    fprintf(out, "    sub rsp, 48h\n\n");

    fprintf(out, "    mov ecx, -11\n");
    fprintf(out, "    call [GetStdHandle]\n");
    fprintf(out, "    mov [stdout_handle], rax\n\n");

    fprintf(out, "    call le_main\n\n");
    
    fprintf(out, "    mov [return_code], eax\n\n");
    
    fprintf(out, "    add eax, '0'\n");
    fprintf(out, "    mov [return_digit], al\n\n");

    fprintf(out, "    mov rcx, [stdout_handle]\n");
    fprintf(out, "    lea rdx, [exit_message]\n");
    fprintf(out, "    mov r8d, exit_message_len\n");
    fprintf(out, "    lea r9, [written]\n");
    fprintf(out, "    mov qword [rsp + 20h], 0\n");
    fprintf(out, "    call [WriteConsoleA]\n");

    fprintf(out, "    mov ecx, [return_code]\n");
    fprintf(out, "    call [ExitProcess]\n\n");

    //program itself
    fprintf(out, "le_main:\n"); 
    fprintf(out, "    sub rsp, 108h\n"); 

    le_scope scope = leCreateScope(NULL);
    scope.variables.count = 0;
    scope.nextStackOffset = 0x20;

    le_token curr;
    le_token next;
    while(leGetNext(&parser, &curr) == 0){    
        printf("-Token[%d], tokenType[%d], name[%s]\n", curr.ID, curr.tokenType, curr.name);   
        switch(curr.tokenType){
            case LE_TOKEN_TYPE_NONE:
                char *varName = curr.name;

                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_EQUALS);
                
                leGenerateExpression(out, &parser, &scope);
                
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_SEMICOLON);

                int offset = leAddVariable(&scope, varName);
                fprintf(out, "    mov dword [rsp + %Xh], eax\n", offset);
            break;

            case LE_TOKEN_TYPE_KEYWORD_RETURN:
                leGenerateExpression(out, &parser, &scope);
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_SEMICOLON);
                fprintf(out, "    add rsp, 108h\n");
                fprintf(out, "    ret\n");
                break;

            case LE_TOKEN_TYPE_KEYWORD_PRINTCHAR:
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_OPARENTHESIS);
                leGenerateExpression(out, &parser, &scope);
                fprintf(out, "    mov [char_buffer], al\n\n");

                fprintf(out, "    mov rcx, [stdout_handle]\n");
                fprintf(out, "    lea rdx, [char_buffer]\n");
                fprintf(out, "    mov r8d, 1\n");
                fprintf(out, "    lea r9, [written]\n");
                fprintf(out, "    mov qword [rsp+20h], 0\n");
                fprintf(out, "    call [WriteConsoleA]\n\n");

                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_CPARENTHESIS);
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_SEMICOLON);

                break;
            
            case LE_TOKEN_TYPE_KEYWORD_REPEAT:
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_OPARENTHESIS);

                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_NUMBER);
                int repeatCount = next.numberValue;

                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_CPARENTHESIS);
                LE_EXPECT_NEXT(&parser, &next, LE_TOKEN_TYPE_OCURLYBRACE);
                
                static int loopID = 0;
                int id = loopID++;

                int counterOffset = scope.nextStackOffset;
                scope.nextStackOffset += 4;
                fprintf(out, "    mov dword [rsp + %Xh], %d\n", counterOffset, repeatCount);
                fprintf(out, "repeat_%d:\n", id);
                fprintf(out, "    cmp dword [rsp + %Xh], 0\n", counterOffset);
                fprintf(out, "    jle repeat_end_%d\n", id);

                int result = leAssembleCodeBlock(out, &parser, &scope);
                if(result != 0){
                    LE_ERROR_EXIT(6, "unclosed repeat block");
                }

                fprintf(out, "    dec dword [rsp + %Xh]\n", counterOffset);
                fprintf(out, "    jmp repeat_%d\n", id);
                fprintf(out, "repeat_end_%d:\n", id);
                break;

            default:
                LE_ERROR_EXIT(3, "unexpected token");
                break;
        }
    }

    fprintf(out, "section '.data' data readable writeable\n\n");

    fprintf(out, "exit_message db 'program exited with return code '\n");
    fprintf(out, "return_digit db '0'\n");
    fprintf(out, "db 13, 10\n");
    fprintf(out, "exit_message_len = $ - exit_message\n\n");

    fprintf(out, "return_code dd 0\n");
    fprintf(out, "stdout_handle dq 0\n");
    fprintf(out, "char_buffer db 0\n");
    fprintf(out, "written dd 0\n\n");

    fprintf(out, "section '.idata' import data readable writeable\n\n");

    fprintf(out, "dd 0, 0, 0, RVA kernel32_name, RVA kernel32_table\n"); 
    fprintf(out, "dd 0, 0, 0, 0, 0\n\n"); 

    fprintf(out, "kernel32_table:\n"); 
    fprintf(out, " ExitProcess dq RVA _ExitProcess\n"); 
    fprintf(out, " GetStdHandle dq RVA _GetStdHandle\n"); 
    fprintf(out, " WriteConsoleA dq RVA _WriteConsoleA\n"); 
    fprintf(out, " dq 0\n\n"); 

    fprintf(out, "kernel32_name db 'kernel32.dll', 0\n\n"); 
    
    fprintf(out, "_ExitProcess:\n"); 
    fprintf(out, " dw 0\n"); 
    fprintf(out, " db 'ExitProcess', 0\n\n"); 
    
    fprintf(out, "_GetStdHandle:\n"); 
    fprintf(out, " dw 0\n"); 
    fprintf(out, " db 'GetStdHandle', 0\n\n"); 

    fprintf(out, "_WriteConsoleA:\n"); 
    fprintf(out, " dw 0\n"); 
    fprintf(out, " db 'WriteConsoleA', 0\n\n"); 

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

int leAssembleCodeBlock(FILE *out, le_parser *parser, le_scope *parent){
    le_scope scope = leCreateScope(parent);

    le_token curr;
    le_token next;

    while(leGetNext(parser, &curr) == 0){
        printf("-Token[%d], tokenType[%d], name[%s]\n", curr.ID, curr.tokenType, curr.name);   
        switch(curr.tokenType){
            case LE_TOKEN_TYPE_CCURLYBRACE:
                leDestroyScope(&scope);
                return 0;
            case LE_TOKEN_TYPE_NONE:
                char varName[MAX_VARIABLE_NAME_LENGTH];
                strcpy(varName, curr.name);

                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_EQUALS);
                
                leGenerateExpression(out, parser, &scope);
                
                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_SEMICOLON);

                int offset = leResolveAssignment(&scope, varName);
                fprintf(out, "    mov dword [rsp + %Xh], eax\n", offset);
            break;

            case LE_TOKEN_TYPE_KEYWORD_RETURN:
                leGenerateExpression(out, parser, &scope);
                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_SEMICOLON);
                fprintf(out, "    add rsp, 108h\n");
                fprintf(out, "    ret\n");
                break;

            case LE_TOKEN_TYPE_KEYWORD_PRINTCHAR:
                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_OPARENTHESIS);
                leGenerateExpression(out, parser, &scope);
                fprintf(out, "    mov [char_buffer], al\n\n");

                fprintf(out, "    mov rcx, [stdout_handle]\n");
                fprintf(out, "    lea rdx, [char_buffer]\n");
                fprintf(out, "    mov r8d, 1\n");
                fprintf(out, "    lea r9, [written]\n");
                fprintf(out, "    mov qword [rsp+20h], 0\n");
                fprintf(out, "    call [WriteConsoleA]\n\n");

                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_CPARENTHESIS);
                LE_EXPECT_NEXT(parser, &next, LE_TOKEN_TYPE_SEMICOLON);

                break;
            
            case LE_TOKEN_TYPE_KEYWORD_REPEAT:
                printf("TODO: repeat");
                break;

            default:
                LE_ERROR_EXIT(3, "unexpected token");
                break;
        }
    }
    leDestroyScope(&scope);
    return 1;
}

le_scope leCreateScope(le_scope *parent) {
    le_scope scope = {0};

    scope.parent = parent;

    if(parent != NULL){
        scope.stackBase = parent->nextStackOffset;
        scope.nextStackOffset = scope.stackBase;
    } else {
        scope.stackBase = 0x20;
        scope.nextStackOffset = 0x20;
    }

    scope.maxStackOffset = scope.nextStackOffset;

    return scope;
}

void leDestroyScope(le_scope *scope){
    if(scope->parent != NULL) {
        if(scope->maxStackOffset > scope->parent->maxStackOffset) {
            scope->parent->maxStackOffset = scope->maxStackOffset;
        }
    }

    free(scope->variables.items);
    scope->variables.items = NULL;
    scope->variables.count = 0;
    scope->variables.capacity = 0;
}

int leAddVariable(le_scope *scope, const char *name){
    for(int i = 0; i < scope->variables.count; i++) {
        if(strcmp(scope->variables.items[i].name, name) == 0) {
            return scope->variables.items[i].stackOffset;
        }
    }

    if(strlen(name) >= MAX_VARIABLE_NAME_LENGTH) {
        LE_ERROR_EXIT(30, "variable name too long");
    }

    le_variable var = {0};
    strcpy(var.name, name);

    var.stackOffset = scope->nextStackOffset;

    da_append(scope->variables, var);

    scope->nextStackOffset += 4;

    if(scope->nextStackOffset > scope->maxStackOffset) {
        scope->maxStackOffset = scope->nextStackOffset;
    }

    return var.stackOffset;
}

int leResolveAssignment(le_scope *scope, const char *name) {
    int offset = leFindVariable(scope, name);

    if(offset >= 0) {
        return offset;
    }

    return leAddVariable(scope, name);
}

int leFindVariable(le_scope* scope, const char* name){
    for(le_scope *current = scope; current != NULL; current = current->parent){
        for(int i = current->variables.count - 1; i >= 0; i--) {
            le_variable *var = &current->variables.items[i];
            if(strcmp(var->name, name) == 0) {
                return var->stackOffset;
            }
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
        fprintf(out, "    mov eax, %d\n", tok.numberValue);
    }
    else if(tok.tokenType == LE_TOKEN_TYPE_NONE){
        int offset = leFindVariable(scope, tok.name);

        if(offset < 0){
            LE_ERROR_EXIT(21, "unknown variable");
        }

        fprintf(out, "    mov eax, dword [rsp + %Xh]\n", offset);
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
        fprintf(out, "    mov ebx, %d\n", tok.numberValue);
    }
    else if(tok.tokenType == LE_TOKEN_TYPE_NONE){
        int offset = leFindVariable(scope, tok.name);

        if(offset < 0){
            LE_ERROR_EXIT(24, "unknown variable");
        }

        fprintf(out, "    mov ebx, dword [rsp + %Xh]\n", offset);
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
            fprintf(out, "    add eax, ebx\n");
        }
        else if(tok.tokenType == LE_TOKEN_TYPE_MINUS){
            leGenerateTermToEbx(out, parser, scope);
            fprintf(out, "    sub eax, ebx\n");
        }
    }
}