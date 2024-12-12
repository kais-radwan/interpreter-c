#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024

typedef struct {
  int code;
  char *tokens;
} ScanResult;

typedef struct {
  int grouped;
  int minnext;
} ParseResult;

char *read_file_contents(const char *filename);
ScanResult scan(const char *content, int log);
char *print_error(int ln, char err[]);
char *print_number(char num[], int log);
ScanResult scan_reserved(int i, const char content[], int log);
char *strupr(char content[]);

void parse(char *content);

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return 1;
    }

    const char *command = argv[1];
    char *file_contents = read_file_contents(argv[2]);

    if (strcmp(command, "tokenize") == 0) {
        fprintf(stderr, "Logs from your program will appear here!\n");

        if (strlen(file_contents) > 0) {
          ScanResult scan_res = scan(file_contents, 1);
          exit(scan_res.code);
        } 
        printf("EOF  null\n");       
        free(file_contents);
    } else if (strcmp(command, "parse") == 0) {
      parse(file_contents);
      return 0;
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

char *read_file_contents(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error reading file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_contents = malloc(file_size + 1);
    if (file_contents == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(file_contents, 1, file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Error reading file contents\n");
        free(file_contents);
        fclose(file);
        return NULL;
    }

    file_contents[file_size] = '\0';
    fclose(file);

    return file_contents;
}

void append_to_buffer(char **buffer, size_t *size, const char *token) {
  size_t token_len = strlen(token);
  size_t new_size = *size + token_len + 2; // +1 for '\n' and +1 for '\0'
  *buffer = realloc(*buffer, new_size);
  strcat(*buffer, token);
  strcat(*buffer, "\n");
  *size = new_size;
}

ScanResult scan(const char *content, int log) {
  int len = strlen(content);
  int line = 1;
  int code = 0;
  int skip = 0;
  int commented = 0;
  int stringed = 0;
  char str[1024];
  int str_i = 0;
  char iden[1024];
  int iden_i = 0;
  char num[1024];
  int num_i = 0;
  int last_num = 0;

  char *tokens_buffer = malloc(1);
  size_t buffer_size = 1;
  tokens_buffer[0] = '\0';

  for (int i = 0; i < len; i++) {
    char c = content[i];
    int last = (i == len - 1);
    int digit = isdigit(c);

    if (skip > 0) {
      skip--;
      continue;
    }

    if (stringed == 0 && iden_i == 0 && commented == 0) {
      ScanResult res = scan_reserved(i, content, log);
      if (res.code > 0) {
        skip = res.code;
        append_to_buffer(&tokens_buffer, &buffer_size, res.tokens);
        continue;
      }
    }

    if ((!digit && c != '.') && num_i > 0) {
      num[num_i] = '\0';
      char *value = print_number(num, log);
      append_to_buffer(&tokens_buffer, &buffer_size, value);
      memset(num, 0, sizeof(num));
      num_i = 0;
    }

    if (stringed == 0 && iden_i == 0 && (digit || (last_num == 1 && c == '.'))) {
      last_num = 1;
    } else {
      last_num = 0;
    }

    if (c == '\n') {
      if (!last) {
        line++;
      }
      commented = 0;
      if (num_i > 0) {
        num[num_i] = '\0';
        char *value = print_number(num, log);
        append_to_buffer(&tokens_buffer, &buffer_size, value);
        memset(num, 0, sizeof(num));
        num_i = 0;
      }
      continue;
    }

    if (commented) continue;

    if (c == '"') {
      if (stringed == 0) {
        stringed = 1;
      } else {
        str[str_i] = '\0';
        char *value = malloc((strlen(str) + 50) * sizeof(char));
        sprintf(value, "STRING %s", str, str);
        append_to_buffer(&tokens_buffer, &buffer_size, value);
        if (log) fprintf(stdout, "STRING \"%s\" %s\n", str, str);
        stringed = 0;
        str_i = 0;
        memset(str, 0, sizeof(str));
      }
      continue;
    }

    if (stringed) {
      str[str_i++] = c;
      continue;
    }

    if (iden_i > 0 && ((!isalpha(c) && c != '_') && !digit)) {
      iden[iden_i] = '\0';
      char value[256];
      sprintf(value, "IDENTIFIER %s null", iden);
      append_to_buffer(&tokens_buffer, &buffer_size, value);
      if (log) fprintf(stdout, "%s\n", value);
      iden_i = 0;
      memset(iden, 0, sizeof(iden));
    }

    if (c == ' ' || c == '\t') continue;

    if (digit && iden_i == 0) {
      num[num_i++] = c;
      continue;
    }

    int valid = 0;
    const char *single_tokens[] = {
      "(", ")", "{", "}", "*", ",", "+", "-", ";", "<=", ">=", "<", ">"
    };
    const char *single_token_names[] = {
      "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE", "STAR", "COMMA",
      "PLUS", "MINUS", "SEMICOLON", "LESS_EQUAL", "GREATER_EQUAL", 
      "LESS", "GREATER"
    };

    for (int t = 0; t < sizeof(single_tokens) / sizeof(single_tokens[0]); t++) {
      int this_one = 0;

      if (c == single_tokens[t][0] && strlen(single_tokens[t]) == 1) {
        this_one = 1;
      } else if (
        c == single_tokens[t][0] && strlen(single_tokens[t]) > 1
        && !last && content[i+1] == single_tokens[t][1]
      ) {
        this_one = 1;
        skip++;
      }

      if (this_one) {
        char value[256];
        sprintf(value, "%s %s null", single_token_names[t], single_tokens[t]);
        append_to_buffer(&tokens_buffer, &buffer_size, value);
        if (log) fprintf(stdout, "%s\n", value);
        valid = 1;
        break;
      }
    }

    if (valid) {
      continue;
    }

    if (c == '/') {
      if  (!last && content[i+1] == '/') {
        commented = 1;
        skip++;
      } else {
        char value[50] = "SLASH / null";
        append_to_buffer(&tokens_buffer, &buffer_size, value);
        if (log) fprintf(stdout, "%s\n", value);
      }

      continue;
    }

    if (c == '=') {
      char value[60];
      if (last == 0 && content[i+1] == '=') {
        strcpy(value, "EQUAL_EQUAL == null");
        skip++;
      } else {
        strcpy(value, "EQUAL = null");
      }

      append_to_buffer(&tokens_buffer, &buffer_size, value);
      if (log) fprintf(stdout, "%s\n", value);
      continue;
    }

    if (c == '!') {
      char value[60];
      if (last == 0 && content[i+1] == '=') {
        strcpy(value, "BANG_EQUAL != null");
        skip++;
      } else {
        strcpy(value, "BANG ! null");
      }

      append_to_buffer(&tokens_buffer, &buffer_size, value);
      if (log) fprintf(stdout, "%s\n", value);
      continue;
    }

    if (c == '!') {
      char value[60];
      if (last == 0 && content[i+1] == '=') {
        strcpy(value, "BANG_EQUAL != null");
        skip++;
      } else {
        strcpy(value, "BANG ! null");
      }

      append_to_buffer(&tokens_buffer, &buffer_size, value);
      if (log) fprintf(stdout, "%s\n", value);
      continue;
    }

    if (c == '.') {
      if (last_num == 1 && last == 0 && isdigit(content[i+1]) != 0) {
        num[num_i] = '.';
        num_i++;
      } else {
        char value[100] = "DOT . null";
        if (log) fprintf(stdout, "%s\n", value);
        append_to_buffer(&tokens_buffer, &buffer_size, value);
      }

      continue;
    }

    if ((iden > 0 && digit) || (isalpha(c) || c == '_')) {
      iden[iden_i++] = c;
      continue;
    }

    char err[256];
    sprintf(err, "Unexpected character: %c", c);
    char *error = print_error(line, err);
    if (log) fprintf(stderr, "%s\n", error);
    code = 65;
  }

  if (stringed) {
    char *error = print_error(line, "Unterminated string.");
    if (log) fprintf(stderr, "%s\n", error);
    code = 65;
  }

  if (num_i > 0) {
    num[num_i] = '\0';
    char *value = print_number(num, log);
    append_to_buffer(&tokens_buffer, &buffer_size, value);
  }

  if (iden_i > 0) {
    iden[iden_i] = '\0';
    char value[256];
    sprintf(value, "IDENTIFIER %s null", iden);
    append_to_buffer(&tokens_buffer, &buffer_size, value);
    if (log) fprintf(stdout, "%s\n", value);
  }

  append_to_buffer(&tokens_buffer, &buffer_size, "EOF\n");
  if (log) fprintf(stdout, "EOF  null\n");

  return (ScanResult){code, tokens_buffer};
}

char *print_error(int ln, char err[]) {
  char *value = malloc((strlen(err) + 50) * sizeof(char));
  sprintf(value, "[line %d] Error: %s\n", ln, err);

  return value;
}

char *print_number(char num[], int log) {
  int num_i = strlen(num);
  float f = atof(num);
  int dot = 0;
  int all_z = 0;
  char *res = malloc((strlen(num) + 40) & sizeof(char));

  for (int i=0; i < num_i; i++) {
    if (num[i] == '.') {
      dot = i;
      continue;
    }

    if (dot && i == num_i-1 && num[i] == '0' && dot != i-1 && all_z) {
      num[i] = '\0';
      continue;
    }

    if (dot && num[i] != '0') {
      all_z = 1;
    } 
  }

  if (dot == 0 || all_z == 0) {
    sprintf(res, "NUMBER %s %0.f.0", num, f);
  } else {
    sprintf(res, "NUMBER %s %s", num, num);
  }

  if (log) {
    fprintf(stdout, "%s\n", res);
  }
  return res;
}

ScanResult scan_reserved(int index, const char content[], int log) {
  char reserved[16][100] = {
    "and", "class", "else", "false", "for", "fun", "if",
    "nil", "or", "print", "return", "super", "this", "true",
    "var", "while"
  };

  int total = strlen(content);
  int skip = 0;
  char token[BUFFER_SIZE];

  for (int i = 0; i < 16; i++) {
    int len = strlen(reserved[i]);
    if (len < 1) {
      continue;
    }
    int h = 0;
    int valid = 1;

    for (int j=index; j < total && j-index < len; j++) {
      if (content[j] != reserved[i][h]) {
        valid = 0;
        break;
      }

      h++;
    }

    if (valid == 1) {
      char *up = strupr(reserved[i]);
      if (log) {
        fprintf(stdout, "%s %s null\n", up, reserved[i]);
      }
      sprintf(token, "%s %s null\n", up, reserved[i]);
      skip = h - 1;
      break;
    }
  }

  return (ScanResult) {skip, token};
}

char *strupr(char content[]) {
  int len = strlen(content);
  char *up = malloc(len * sizeof(char));

  for (int i=0; i < len; i++) {
    up[i] = toupper(content[i]);
  }

  up[len] = '\0';
  return up;
}

char *read_token(char *content) {
  char *space = strchr(content, ' ');
  char *token = malloc(strlen(content) * sizeof(char));

  if (space != NULL) {
    size_t length = space - content;
    strncpy(token, content, length);
    token[length] = '\0';
  }

  return token;
}

void parse_print(char *value, int grouped) {
  if (grouped > 0) {
    fprintf(stdout, value);
  } else {
    fprintf(stdout, "%s\n", value);
  }
}

ParseResult parse_line(char *line, int *grouped, int *subnext) {
  char buffer[BUFFER_SIZE];
  char *token = read_token(line);
  int subnow = 0;

  char base[3][6] = {
    "TRUE", "FALSE", "NIL"
  };

  char base_low[3][6] = {
    "true", "false", "nil"
  };

  for (int i=0; i < sizeof(base) / sizeof(base[0]); i++) {
    if (strcmp(token, base[i]) == 0) {
      parse_print(base_low[i], *grouped);
      break;
    }
  }

  if (strcmp(token, "NUMBER") == 0) {
    char *num = line + strlen(token) + 1;
    num = read_token(num);
    num = line + strlen(token) + strlen(num) + 2;
    parse_print(num, *grouped);
  }

  else if (strcmp(token, "STRING") == 0) {
    char *str = line + strlen(token) + 1;
    parse_print(str, *grouped);
  }

  else if (strcmp(token, "LEFT_PAREN") == 0) {
    fprintf(stdout, "(group ");
    *grouped = *grouped + 1;
    return (ParseResult) {1, 0};
  }

  else if (strcmp(token, "RIGHT_PAREN") == 0) {
    *grouped = *grouped - 1;
    parse_print(")", *grouped);
  }

  else if (strcmp(token, "BANG") == 0) {
    *grouped = *grouped + 1;
    fprintf(stdout, "(! ");
    subnow++;
  }

  else if (strcmp(token, "MINUS") == 0) {
    *grouped = *grouped + 1;
    fprintf(stdout, "(- ");
    subnow = *grouped || 1;
  }

  if (*subnext > 0 && (*subnext == *grouped || *grouped == 1)) {
    parse_print(")", *grouped);
    *subnext = *subnext - 1;
    *grouped = *grouped - 1;
  }

  return (ParseResult) {0, subnow};
}

void parse(char *content) {
  ScanResult scanned = scan(content, 0);
  char buffer[BUFFER_SIZE];
  char *line = scanned.tokens;
  char *newline;
  int grouped = 0;
  int subnext = 0;

  while (line != NULL) {
    newline = strchr(line, '\n');

    if (newline != NULL) {
      *newline = '\0';
    }

    if (strlen(line) > 0) {
      ParseResult parsed = parse_line(line, &grouped, &subnext);
      subnext = subnext + parsed.minnext;
    }

    if (newline != NULL) {
      line = newline + 1;
    } else {
      break;
    }
  }

  while (grouped > 0) {
    grouped--;
    parse_print(")", grouped);
  }
}
