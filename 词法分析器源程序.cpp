#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TOKEN_LENGTH 1024

// 枚举定义标记类型
typedef enum {
    KEYWORD = 0,
    IDENTIFIER,
    OPERATOR,
    DELIMITER,
    CHARCON,
    STRING,
    NUMBER,
    ERROR,
    TOKEN_TYPE_COUNT
} TokenType;

// 枚举定义字符类型
typedef enum {
    CHAR_LETTER = 0,
    CHAR_DIGIT,
    CHAR_SINGLE_QUOTE,
    CHAR_DOUBLE_QUOTE,
    CHAR_OTHER
} CharType;

// 关键字列表
const char* keywords[] = {
    "char", "double", "enum", "float", "int", "long",
    "short", "signed", "struct", "union", "unsigned", "void",
    "for", "do", "while", "break", "continue", "if", "else",
    "goto", "switch", "case", "default", "return", "auto",
    "extern", "register", "static", "const", "sizeof", "typedef",
    "volatile"
};
const size_t keyword_count = sizeof(keywords) / sizeof(keywords[0]);

// 词法分析器状态结构体
typedef struct {
    FILE* file;
    int line_number;
    long long token_counts[TOKEN_TYPE_COUNT];
    char* lexeme;
    int lexeme_length;
    int lexeme_capacity;
} LexerState;

// 函数声明
int read_char(LexerState* state);
void unread_char(LexerState* state, int ch);
CharType classify_char(int ch);
void append_char(LexerState* state, int ch);
void reset_lexeme(LexerState* state);
void output_token(LexerState* state, TokenType type, const char* value);
void process_word(LexerState* state, int ch);
void process_number(LexerState* state, int ch);
void process_string(LexerState* state);
void process_char_const(LexerState* state, int ch);
void process_operator_or_delimiter(LexerState* state, int ch);
int process_fraction_part(LexerState* state);
int process_exponent_part(LexerState* state);
void process_string_or_char(LexerState* state, const char* prefix);
int is_valid_integer_suffix(const char* suffix);
int is_valid_float_suffix(const char* suffix);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "用法: %s <源文件名>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // 初始化词法分析器状态
    LexerState state;
    state.file = fopen(argv[1], "r");
    if (!state.file) {
        perror("文件打开失败");
        return EXIT_FAILURE;
    }
    state.line_number = 1;
    memset(state.token_counts, 0, sizeof(state.token_counts));
    state.lexeme_capacity = MAX_TOKEN_LENGTH;
    state.lexeme = (char*)malloc(state.lexeme_capacity);
    state.lexeme_length = 0;

    int ch;
    while ((ch = read_char(&state)) != EOF) {
        if (ch == '\n') {
            state.line_number++;
        }
        if (isspace(ch)) {
            continue;
        }

        CharType char_type = classify_char(ch);
        switch (char_type) {
        case CHAR_LETTER:
            process_word(&state, ch);
            break;
        case CHAR_DIGIT:
            process_number(&state, ch);
            break;
        case CHAR_SINGLE_QUOTE:
            process_char_const(&state, ch);
            break;
        case CHAR_DOUBLE_QUOTE:
            process_string(&state);
            break;
        case CHAR_OTHER:
            process_operator_or_delimiter(&state, ch);
            break;
        }
    }

    fclose(state.file);

    // 输出总行数
    printf("%d\n", state.line_number);

    // 输出各标记类型的计数
    for (int i = 0; i < TOKEN_TYPE_COUNT - 1; ++i) {
        printf("%lld", state.token_counts[i]);
        if (i == NUMBER) {
            putchar('\n');
        }
        else {
            putchar(' ');
        }
    }
    printf("%lld", state.token_counts[ERROR]); // 输出结束后不再输出换行符

    free(state.lexeme);
    return EXIT_SUCCESS;
}

// 从文件读取下一个字符
int read_char(LexerState* state) {
    return fgetc(state->file);
}

// 将字符放回输入流
void unread_char(LexerState* state, int ch) {
    if (ch != EOF) {
        ungetc(ch, state->file);
    }
}

// 分类字符类型
CharType classify_char(int ch) {
    if (isalpha(ch) || ch == '_') {
        return CHAR_LETTER;
    }
    else if (isdigit(ch)) {
        return CHAR_DIGIT;
    }
    else if (ch == '\'') {
        return CHAR_SINGLE_QUOTE;
    }
    else if (ch == '"') {
        return CHAR_DOUBLE_QUOTE;
    }
    else {
        return CHAR_OTHER;
    }
}

// 将字符添加到词素缓冲区
void append_char(LexerState* state, int ch) {
    if (state->lexeme_length >= state->lexeme_capacity - 1) {
        state->lexeme_capacity *= 2;
        state->lexeme = (char*)realloc(state->lexeme, state->lexeme_capacity);
    }
    state->lexeme[state->lexeme_length++] = (char)ch;
    state->lexeme[state->lexeme_length] = '\0';
}

// 重置词素缓冲区
void reset_lexeme(LexerState* state) {
    state->lexeme_length = 0;
    state->lexeme[0] = '\0';
}

// 输出标记，按照 v0 的格式
void output_token(LexerState* state, TokenType type, const char* value) {
    const char* type_names[] = {
        "KEYWORD", "IDENTIFIER", "OPERATOR", "DELIMITER",
        "CHARCON", "STRING", "NUMBER", "ERROR"
    };
    printf("%d <%s,%s>\n", state->line_number, type_names[type], value);
    state->token_counts[type]++;
    reset_lexeme(state);
}

// 处理标识符或关键字，处理字符串和字符常量的前缀
void process_word(LexerState* state, int ch) {
    append_char(state, ch);
    int next_ch;

    // 检查前缀
    if (ch == 'u') {
        next_ch = read_char(state);
        if (next_ch == '8') {
            append_char(state, next_ch);
            int peek_ch = read_char(state);
            if (peek_ch == '"' || peek_ch == '\'') {
                append_char(state, peek_ch);
                process_string_or_char(state, state->lexeme);
                return;
            }
            else {
                unread_char(state, peek_ch);
            }
        }
        else if (next_ch == '"' || next_ch == '\'') {
            append_char(state, next_ch);
            process_string_or_char(state, state->lexeme);
            return;
        }
        else {
            unread_char(state, next_ch);
        }
    }
    else if (ch == 'U' || ch == 'L') {
        next_ch = read_char(state);
        if (next_ch == '"' || next_ch == '\'') {
            append_char(state, next_ch);
            process_string_or_char(state, state->lexeme);
            return;
        }
        else {
            unread_char(state, next_ch);
        }
    }

    // 继续读取标识符
    while ((next_ch = read_char(state)), isalnum(next_ch) || next_ch == '_') {
        append_char(state, next_ch);
    }
    unread_char(state, next_ch);

    // 循环对比检查是否为关键字
    int is_keyword = 0;
    for (size_t i = 0; i < keyword_count; ++i) {
        if (strcmp(state->lexeme, keywords[i]) == 0) {
            is_keyword = 1;
            break;
        }
    }

    output_token(state, is_keyword ? KEYWORD : IDENTIFIER, state->lexeme);
}

// 处理带前缀的字符串或字符常量
void process_string_or_char(LexerState* state, const char* prefix) {
    int ch = state->lexeme[state->lexeme_length - 1]; // 已经读取了引号
    int is_string = (ch == '"');
    int is_valid = 1;

    while ((ch = read_char(state)) != EOF && ch != '\n') {
        append_char(state, ch);
        if (ch == '\\') {
            ch = read_char(state);
            if (ch == EOF || ch == '\n') {
                is_valid = 0;
                break;
            }
            append_char(state, ch);
        }
        else if ((is_string && ch == '"') || (!is_string && ch == '\'')) {
            // 结束引号
            break;
        }
    }

    if ((is_string && ch != '"') || (!is_string && ch != '\'')) {
        is_valid = 0;
    }

    if (is_valid) {
        output_token(state, is_string ? STRING : CHARCON, state->lexeme);
    }
    else {
        output_token(state, ERROR, state->lexeme);
        if (ch == '\n') {
            // 读取到换行符后再增加行号
            state->line_number++;
        }
    }
}

// 处理数字，包括整数、浮点数、十六进制、八进制等
void process_number(LexerState* state, int ch) {
    append_char(state, ch);
    int next_ch;
    int is_valid = 1;
    int is_float = 0;

    if (ch == '.') {
        // 处理以 . 开头的浮点数
        is_float = 1;
        is_valid = process_fraction_part(state);
    }
    else if (ch == '0') {
        next_ch = read_char(state);
        if (next_ch == 'x' || next_ch == 'X') {
            // 处理十六进制数
            append_char(state, next_ch);
            while ((next_ch = read_char(state)), isxdigit(next_ch)) {
                append_char(state, next_ch);
            }
            if (isalpha(next_ch)) {
                is_valid = 0;
                while (isalnum(next_ch)) {
                    append_char(state, next_ch);
                    next_ch = read_char(state);
                }
            }
            unread_char(state, next_ch);
        }
        else if (isdigit(next_ch) && next_ch != '8' && next_ch != '9') {
            // 处理八进制数
            append_char(state, next_ch);
            while ((next_ch = read_char(state)), isdigit(next_ch)) {
                if (next_ch >= '0' && next_ch <= '7') {
                    append_char(state, next_ch);
                }
                else {
                    is_valid = 0;
                    append_char(state, next_ch);
                }
            }
            unread_char(state, next_ch);
        }
        else if (next_ch == '.') {
            // 处理浮点数，如 0.5
            append_char(state, next_ch);
            is_float = 1;
            is_valid = process_fraction_part(state);
        }
        else if (next_ch == 'e' || next_ch == 'E') {
            // 处理科学计数法，如 0e10
            append_char(state, next_ch);
            is_float = 1;
            is_valid = process_exponent_part(state);
        }
        else {
            unread_char(state, next_ch);
        }
    }
    else {
        // 处理十进制数或浮点数
        while ((next_ch = read_char(state)), isdigit(next_ch)) {
            append_char(state, next_ch);
        }
        if (next_ch == '.') {
            append_char(state, next_ch);
            is_float = 1;
            is_valid = process_fraction_part(state);
        }
        else if (next_ch == 'e' || next_ch == 'E') {
            append_char(state, next_ch);
            is_float = 1;
            is_valid = process_exponent_part(state);
        }
        else {
            unread_char(state, next_ch);
        }
    }

    // 处理数字后缀
    if (is_valid) {
        char suffix[4] = { 0 }; // 最大后缀长度为3
        int suffix_len = 0;
        int suffix_ch;

        while (suffix_len < 3) {
            suffix_ch = read_char(state);
            if (isalpha(suffix_ch)) {
                append_char(state, suffix_ch);
                suffix[suffix_len++] = (char)suffix_ch;
            }
            else {
                unread_char(state, suffix_ch);
                break;
            }
        }
        suffix[suffix_len] = '\0';

        int valid_suffix = 0;
        if (is_float) {
            // 检查浮点数合法后缀
            valid_suffix = is_valid_float_suffix(suffix);
        }
        else {
            // 检查整数合法后缀
            valid_suffix = is_valid_integer_suffix(suffix);
        }

        if (!valid_suffix) {
            is_valid = 0;
        }
    }

    if (is_valid) {
        output_token(state, NUMBER, state->lexeme);
    }
    else {
        output_token(state, ERROR, state->lexeme);
    }
}

// 处理小数部分
int process_fraction_part(LexerState* state) {
    int next_ch;
    int has_digits = 0;

    while ((next_ch = read_char(state)), isdigit(next_ch)) {
        has_digits = 1;
        append_char(state, next_ch);
    }
    if (next_ch == 'e' || next_ch == 'E') {
        append_char(state, next_ch);
        if (!process_exponent_part(state)) {
            return 0;
        }
    }
    else {
        unread_char(state, next_ch);
    }
    return has_digits;
}

// 处理科学计数法的指数部分
int process_exponent_part(LexerState* state) {
    int next_ch = read_char(state);
    if (next_ch == '+' || next_ch == '-') {
        append_char(state, next_ch);
        next_ch = read_char(state);
    }
    if (!isdigit(next_ch)) {
        unread_char(state, next_ch);
        return 0;
    }
    while (isdigit(next_ch)) {
        append_char(state, next_ch);
        next_ch = read_char(state);
    }
    unread_char(state, next_ch);
    return 1;
}

// 检查是否为有效的整数后缀
int is_valid_integer_suffix(const char* suffix) {
    const char* valid_suffixes[] = {
        "u", "U", "l", "L",
        "ul", "uL", "Ul", "UL",
        "lu", "lU", "Lu", "LU",
        "ll", "LL",
        "ull", "uLL", "Ull", "ULL",
        "llu", "llU", "LLu", "LLU"
    };
    size_t num_suffixes = sizeof(valid_suffixes) / sizeof(valid_suffixes[0]);

    for (size_t i = 0; i < num_suffixes; ++i) {
        if (strcmp(suffix, valid_suffixes[i]) == 0) {
            return 1;
        }
    }
    return suffix[0] == '\0'; // 空后缀也是合法的
}

// 检查是否为有效的浮点数后缀
int is_valid_float_suffix(const char* suffix) {
    const char* valid_suffixes[] = { "f", "F", "l", "L" };
    size_t num_suffixes = sizeof(valid_suffixes) / sizeof(valid_suffixes[0]);

    for (size_t i = 0; i < num_suffixes; ++i) {
        if (strcmp(suffix, valid_suffixes[i]) == 0) {
            return 1;
        }
    }
    return suffix[0] == '\0'; // 空后缀也是合法的
}

// 处理字符串字面量
void process_string(LexerState* state) {
    append_char(state, '"');
    int ch;
    int is_valid = 1;

    while ((ch = read_char(state)) != EOF && ch != '\n') {
        append_char(state, ch);
        if (ch == '\\') {
            ch = read_char(state);
            if (ch == EOF || ch == '\n') {
                is_valid = 0;
                break;
            }
            append_char(state, ch);
        }
        else if (ch == '"') {
            break;
        }
    }

    if (ch != '"') {
        is_valid = 0;
    }

    if (is_valid) {
        output_token(state, STRING, state->lexeme);
    }
    else {
        output_token(state, ERROR, state->lexeme);
        if (ch == '\n') {
            // 读取到换行符后再增加行号
            state->line_number++;
        }
    }
}

// 处理字符常量，允许单引号内有多个字符
void process_char_const(LexerState* state, int ch) {
    append_char(state, ch);
    int is_valid = 1;

    while ((ch = read_char(state)) != EOF && ch != '\n') {
        append_char(state, ch);
        if (ch == '\\') {
            ch = read_char(state);
            if (ch == EOF || ch == '\n') {
                is_valid = 0;
                break;
            }
            append_char(state, ch);
        }
        else if (ch == '\'') {
            break;
        }
    }

    if (ch != '\'') {
        is_valid = 0;
    }

    if (is_valid) {
        output_token(state, CHARCON, state->lexeme);
    }
    else {
        output_token(state, ERROR, state->lexeme);
        if (ch == '\n') {
            // 读取到换行符后再增加行号
            state->line_number++;
        }
    }
}

// 处理运算符和分隔符
void process_operator_or_delimiter(LexerState* state, int ch) {
    int next_ch = read_char(state);

    if (ch == '.' && isdigit(next_ch)) {
        // 处理浮点数，如 .5
        unread_char(state, next_ch);
        process_number(state, ch);
        return;
    }

    append_char(state, ch);

    // 分隔符处理
    if (strchr(";,:?[](){}", ch)) {
        output_token(state, DELIMITER, state->lexeme);
        unread_char(state, next_ch);
        return;
    }

    // 处理多字符运算符
    if (ch == '+' && (next_ch == '+' || next_ch == '=')) {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if (ch == '-' && (next_ch == '-' || next_ch == '=' || next_ch == '>')) {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if (ch == '*' && next_ch == '=') {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if (ch == '/' && next_ch == '=') {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if ((ch == '%' || ch == '^' || ch == '&' || ch == '|') && next_ch == '=') {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if ((ch == '<' || ch == '>') && (next_ch == '=' || next_ch == ch)) {
        append_char(state, next_ch);
        if (next_ch == ch) {
            // 可能是 <<= 或 >>= 等
            int third_ch = read_char(state);
            if (third_ch == '=') {
                append_char(state, third_ch);
            }
            else {
                unread_char(state, third_ch);
            }
        }
        output_token(state, OPERATOR, state->lexeme);
    }
    else if ((ch == '=' || ch == '!') && next_ch == '=') {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if ((ch == '&' && next_ch == '&') || (ch == '|' && next_ch == '|')) {
        append_char(state, next_ch);
        output_token(state, OPERATOR, state->lexeme);
    }
    else if (ch == '/' && (next_ch == '/' || next_ch == '*')) {
        // 处理注释
        if (next_ch == '/') {
            // 单行注释
            while ((ch = read_char(state)) != EOF && ch != '\n');
            if (ch == '\n') {
                state->line_number++;
            }
            reset_lexeme(state);
        }
        else {
            // 多行注释
            int prev_ch = 0;
            while ((ch = read_char(state)) != EOF) {
                if (ch == '\n') {
                    state->line_number++;
                }
                if (prev_ch == '*' && ch == '/') {
                    break;
                }
                prev_ch = ch;
            }
            reset_lexeme(state);
        }
    }
    else if (ispunct(ch) && strchr("~!@#$%^&*-+=|\\:;\"'<>./?", ch)) {
        // 单字符运算符或分隔符
        if (ch == '@') {
            // 处理非法字符
            output_token(state, ERROR, state->lexeme);
        }
        else {
            output_token(state, OPERATOR, state->lexeme);
        }
        unread_char(state, next_ch);
    }
    else {
        // 处理未识别的字符
        output_token(state, ERROR, state->lexeme);
        unread_char(state, next_ch);
    }
}