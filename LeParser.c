#include <stdio.h>

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

#define LE_TOKEN_TYPE_PLUS 96
#define LE_TOKEN_TYPE_MINUS 97

#define LE_TOKEN_TYPE_EQUALS 98
#define LE_TOKEN_TYPE_SEMICOLON 99
//100-inf - keywords
#define LE_TOKEN_TYPE_KEYWORD_RETURN 100

//-keywords
#define LE_KEYWORD_EQUALS "="
#define LE_KEYWORD_PLUS "+"
#define LE_KEYWORD_MINUS "-"

#define LE_KEYWORD_SEMICOLON ";"

#define LE_KEYWORD_RETURN "return"

//Struct declarations
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
void leAddToken(le_token tokens[], int* tokenCount, char tokenName[]);
int isNumber(const char* str);

void leParse(le_parser *parser, FILE *source){
    parser->currentToken = 0;

    int c;
    int tokenCount = 0;
    int length = 0;

    char tokenName[MAX_TOKEN_NAME_LENGTH];

    printf("parsing file\n");
    while((c = fgetc(source)) != EOF)
    {
        if(c != ' ' && c != '\n' && c != '\t' && c != ';'){
            if(length < MAX_TOKEN_NAME_LENGTH - 1){
                tokenName[length] = (char)c;
                length++;
                tokenName[length] = '\0';
            }
        } else{
            if(length > 0){
                leAddToken(parser->tokens, &tokenCount, tokenName);

                length = 0;
                tokenName[0] = '\0';

                if(tokenCount >= MAX_TOKEN_COUNT){
                    break;
                }
            }
            if(c == ';'){
                leAddToken(parser->tokens, &tokenCount, LE_KEYWORD_SEMICOLON);
                if(tokenCount >= MAX_TOKEN_COUNT){
                    break;
                }
            }
        }
    }

    if(length > 0 && tokenCount < MAX_TOKEN_COUNT){
        leAddToken(parser->tokens, &tokenCount, tokenName);
    }

    parser->numTokens = tokenCount;
}

int leGetNext(le_parser *parser, le_token *result){
    if(parser->currentToken < parser->numTokens){
        *result = parser->tokens[parser->currentToken];
        parser->currentToken++;
        printf("g-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
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
        printf("ge-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
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
        printf("p-Token[%d], tokenType[%d], name[%s]\n", result->ID, result->tokenType, result->name);   
        return 0;
    }

    result->ID = LE_TOKEN_ID_EOT;
    result->name[0] = '\0';
    result->tokenType = LE_TOKEN_TYPE_NONE;
    result->numberValue = 0;

    return 1;
}

void leAddToken(le_token tokens[], int* tokenCount, char tokenName[]){
    tokens[*tokenCount].ID = *tokenCount;
    strcpy(tokens[*tokenCount].name, tokenName);

    if(strcmp(tokenName, LE_KEYWORD_RETURN) == 0){
        tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_KEYWORD_RETURN;
    }
    else if(strcmp(tokenName, LE_KEYWORD_SEMICOLON) == 0){
         tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_SEMICOLON;
    }
    else if(strcmp(tokenName, LE_KEYWORD_EQUALS) == 0){
         tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_EQUALS;
    }
    else if(strcmp(tokenName, LE_KEYWORD_PLUS) == 0){
         tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_PLUS;
    }
    else if(strcmp(tokenName, LE_KEYWORD_MINUS) == 0){
         tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_MINUS;
    }
    else if(isNumber(tokenName)){
        tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_NUMBER;
        tokens[*tokenCount].numberValue = atoi(tokenName);
    }
    else{
        tokens[*tokenCount].tokenType = LE_TOKEN_TYPE_NONE;
    }

    (*tokenCount)++;
}

int isNumber(const char* str){
    char* end;
    strtol(str, &end, 10);

    return *end == '\0'; 
}