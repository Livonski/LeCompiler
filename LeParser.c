#include <stdio.h>
#include <stdbool.h>

//Global consts
#define MAX_TOKEN_COUNT 64
#define MAX_TOKEN_NAME_LENGTH 32

//Token ID-s
//0-inf - valid token
//-1 signifies that there is no more tokens to parse
#define LE_TOKEN_ID_EOT -1
//-2 signifies that expected token not found
#define LE_TOKEN_ID_NOT_FOUND -2

//0 - unindentified token, probably variable name
#define LE_TOKEN_TYPE_NONE 0
//1-99 - numbers and number related things
#define LE_TOKEN_TYPE_NUMBER 1

#define LE_TOKEN_TYPE_OPARENTHESIS 94
#define LE_TOKEN_TYPE_CPARENTHESIS 95

#define LE_TOKEN_TYPE_PLUS 96
#define LE_TOKEN_TYPE_MINUS 97

#define LE_TOKEN_TYPE_EQUALS 98
#define LE_TOKEN_TYPE_SEMICOLON 99
//100-inf - keywords
#define LE_TOKEN_TYPE_KEYWORD_RETURN 100
#define LE_TOKEN_TYPE_KEYWORD_PRINTCHAR 101

//Struct declarations
typedef struct{
    int type;
    char name[MAX_TOKEN_NAME_LENGTH];
    bool separator;
}le_keyword;

typedef struct{
    int ID;
    char name[MAX_TOKEN_NAME_LENGTH];
    int tokenType;
    
    int numberValue;
}le_token;

typedef struct{
    le_token tokens[MAX_TOKEN_COUNT];

    int numTokens;
    int currentToken;
}le_parser;

//Forward declarations
void leAddToken(le_token tokens[], le_keyword keywords[], int* tokenCount, char tokenName[]);
int isNumber(const char* str);
int isSeparator(const char* separators, char c);

void leParse(le_parser *parser, FILE *source){
    parser->currentToken = 0;

    le_keyword keywords[] = {
        [0] = {.type = LE_TOKEN_TYPE_PLUS, .name = "+"},
        [1] = {.type = LE_TOKEN_TYPE_MINUS, .name = "-"},
        [2] = {.type = LE_TOKEN_TYPE_EQUALS, .name = "="},
        [3] = {.type = LE_TOKEN_TYPE_CPARENTHESIS, .name = ")"},
        [4] = {.type = LE_TOKEN_TYPE_OPARENTHESIS, .name = "("},
        [5] = {.type = LE_TOKEN_TYPE_SEMICOLON, .name = ";"},
        [6] = {.type = LE_TOKEN_TYPE_KEYWORD_RETURN, .name = "return"},
        [7] = {.type = LE_TOKEN_TYPE_KEYWORD_PRINTCHAR, .name = "printChar"},
    };

    char separators[] = { '+', '-', '=', '(', ')', ';'};
    
    int c;
    int tokenCount = 0;
    int length = 0;

    char tokenName[MAX_TOKEN_NAME_LENGTH];

    printf("parsing file\n");
    while((c = fgetc(source)) != EOF)
    {
        int separator = isSeparator(separators, c);

        if((c == ' ' || c == '\n' || c == '\t') && length > 0){
            leAddToken(parser->tokens, keywords, &tokenCount, tokenName);

            length = 0;
            tokenName[0] = '\0';

            if(tokenCount >= MAX_TOKEN_COUNT){
                break;
            }
        }

        if(!separator && c != ' ' && c != '\n' && c != '\t'){
            if(length < MAX_TOKEN_NAME_LENGTH - 1){
                tokenName[length] = (char)c;
                length++;
                tokenName[length] = '\0';
            }
        }
        
        if(separator){
            leAddToken(parser->tokens, keywords, &tokenCount, tokenName);

            length = 0;
            tokenName[0] = '\0';

            if(tokenCount >= MAX_TOKEN_COUNT){
                break;
            }

            tokenName[length] = (char)c;
            length++;
            tokenName[length] = '\0';

            leAddToken(parser->tokens, keywords, &tokenCount, tokenName);
            if(tokenCount >= MAX_TOKEN_COUNT){
                break;
            }
            length = 0;
            tokenName[0] = '\0';
        }
    }

    if(length > 0 && tokenCount < MAX_TOKEN_COUNT){
        leAddToken(parser->tokens, keywords, &tokenCount, tokenName);
    }

    parser->numTokens = tokenCount;
}

int leGetNext(le_parser *parser, le_token *result){
    if(parser->currentToken < parser->numTokens){
        *result = parser->tokens[parser->currentToken];
        parser->currentToken++;
        //printf("g-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
        return 0;
    }

    le_token emptyToken;
    emptyToken.ID = LE_TOKEN_ID_EOT;
    emptyToken.name[0] = '\0';
    emptyToken.tokenType = LE_TOKEN_TYPE_NONE;
    emptyToken.numberValue = 0;

    *result = emptyToken;
    return 1;
}

int leGetAndExpectNext(le_parser *parser, le_token *result, int excpected){
    le_token token;
    leGetNext(parser, &token);

    if(token.tokenType == excpected && token.ID != -1){
        *result = token;
        //printf("ge-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
        return 0;
    }

    le_token emptyToken;
    emptyToken.ID = LE_TOKEN_ID_NOT_FOUND;
    emptyToken.name[0] = '\0';
    emptyToken.tokenType = LE_TOKEN_TYPE_NONE;
    emptyToken.numberValue = 0;

    *result = emptyToken;
    return 1;
}

int lePeekNext(le_parser *parser, le_token *result){
    if(parser->currentToken < parser->numTokens){
        *result = parser->tokens[parser->currentToken];
        //printf("p-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
        return 0;
    }

    result->ID = LE_TOKEN_ID_EOT;
    result->name[0] = '\0';
    result->tokenType = LE_TOKEN_TYPE_NONE;
    result->numberValue = 0;

    return 1;
}

void leAddToken(le_token tokens[], le_keyword keywords[], int* tokenCount, char tokenName[]){
    if(tokenName[0] == '\0'){
        return;
    }

    tokens[*tokenCount].ID = *tokenCount;
    strcpy(tokens[*tokenCount].name, tokenName);
    tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_NONE;
    tokens[*tokenCount].numberValue = 0;

    for(int i = 0; i < 8; i++){
        if(strcmp(tokenName, keywords[i].name) == 0){
            tokens[*tokenCount].tokenType = keywords[i].type;
            (*tokenCount)++;
            return;
        }
    }

    if(isNumber(tokenName)){
        tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_NUMBER;
        tokens[*tokenCount].numberValue = atoi(tokenName);
    }
    (*tokenCount)++;
}

int isNumber(const char* str){
    if(str[0] == '\0'){
        return 0;
    }

    char* end;
    strtol(str, &end, 10);

    return *end == '\0'; 
}

int isSeparator(const char* separators, char c){
    for(int i = 0; i < sizeof(separators) / sizeof(separators[0]); i++){
        if (separators[i] == c) return 1;
    }
    return 0;
}