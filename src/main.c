#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *read_file_contents(const char *filename);
int scan_paren(const char *content);
void print_error(int ln, char err[]);
void print_number(const char num[]);

int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "tokenize") == 0) {
        fprintf(stderr, "Logs from your program will appear here!\n");
        
        char *file_contents = read_file_contents(argv[2]);

        if (strlen(file_contents) > 0) {
          int code = scan_paren(file_contents);
          exit(code);
        } 
        printf("EOF  null\n");       
        free(file_contents);
    } else {
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

int scan_paren(const char *content) {
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

  for (int i=0; i < len; i++) {
    char c = content[i];
    int last = 0;
    int digit = isdigit(c);

    if (i == len-1) {
      last = 1;
    }

    if ((!digit && c != '.') && num_i > 0) {
      num[num_i] = '\0';
      print_number(num);
      memset(num, 0, strlen(num));
      num_i = 0;
    }

    if (stringed == 0 && (digit || (last_num == 1 && c == '.'))) {  
      last_num = 1;
    } else {
      last_num = 0;
    }

    if (c == '\n') {
      line++;
      commented = 0;
      if (num_i > 0) {
        num[num_i] = '\0';
        print_number(num);
        memset(num, 0, strlen(num));
        num_i = 0;
      }
      continue;
    }

    if (skip > 0) {
      skip--;
      continue;
    }

    if (commented != 0) {
      continue;
    }

    if (c == '"') {
      if (stringed == 0) {
        stringed = 1;
      } else {
        str[str_i] = '\0';
        stringed = 0;
        str_i = 0;

        char log[1224];
        sprintf(log, "STRING \"%s\" %s", str, str);
        fprintf(stdout, "%s\n", log);
        memset(str, 0, strlen(str));
      }

      continue;
    }

    if (stringed != 0) {
      str[str_i] = c;
      str_i++;
      continue;
    }

    if  (c == ' ' || c == '\t') {
      if (iden_i > 0) {
        iden[iden_i] = '\0';
        fprintf(stdout, "IDENTIFIER %s null\n", iden);
        iden_i = 0;
        memset(iden, 0, strlen(iden));
      }
      continue;
    }

    if (digit) {
      num[num_i] = c;
      num_i++;
      continue;
    }

    if (c == '(') {
      fprintf(stdout, "LEFT_PAREN ( null\n");
      continue;
    }

    if (c == ')') {
      fprintf(stdout, "RIGHT_PAREN ) null\n");
      continue;
    }

    if (c == '{') {
      fprintf(stdout, "LEFT_BRACE { null\n");
      continue;
    }

    if (c == '}') {
      fprintf(stdout, "RIGHT_BRACE } null\n");
      continue;
    }

    if (c == '*') {
      fprintf(stdout, "STAR * null\n");
      continue;
    }

    if (c == '.') {
      if (last_num == 1 && last == 0 && isdigit(content[i+1]) != 0) {
        num[num_i] = '.';
        num_i++;
      } else {
        fprintf(stdout, "DOT . null\n");
      }

      continue;
    }

    if (c == ',') {
      fprintf(stdout, "COMMA , null\n");
      continue;
    }

    if (c == '+') {
      fprintf(stdout, "PLUS + null\n");
      continue;
    }

    if (c == '-') {
      fprintf(stdout, "MINUS - null\n");
      continue;
    }

    if (c == ';') {
      fprintf(stdout, "SEMICOLON ; null\n");
      continue;
    }

    if (c == '=') {
      if (last == 0 && content[i+1] == '=') {
        fprintf(stdout, "EQUAL_EQUAL == null\n");
        skip++;
      } else {
        fprintf(stdout, "EQUAL = null\n");
      }

      continue;
    }

    if (c == '!') {
      if  (last == 0 && content[i+1] == '=') {
        fprintf(stdout, "BANG_EQUAL != null\n");
        skip++;
      } else {
        fprintf(stdout, "BANG ! null\n");
      }

      continue;
    }

    if (c == '<') {
      if (last == 0 && content[i+1] == '=') {
        fprintf(stdout, "LESS_EQUAL <= null\n");
        skip++;
      } else {
        fprintf(stdout, "LESS < null\n");
      }

      continue;
    }

    if (c == '>') {
      if (last == 0 && content[i+1] == '=') {
        fprintf(stdout, "GREATER_EQUAL >= null\n");
        skip++;
      } else {
        fprintf(stdout, "GREATER > null\n");
      }

      continue;
    }

    if (c == '/') {
      if  (last == 0 && content[i+1] == '/') {
        commented = 1;
      } else {
        fprintf(stdout, "SLASH / null\n");
      }

      continue;
    }

    if (isalpha(c)) {
      iden[iden_i] = c;
      iden_i++;
      continue;
    }

    char err[256];
    sprintf(err, "Unexpected character: %c", content[i]);
    print_error(line, err);
    code = 65;
  }

  if (stringed != 0) {
    print_error(line, "Unterminated string.");
    code = 65;
  }

  if (num_i > 0) {
    num[num_i] = '\0';
    print_number(num);
  }

  if (iden_i > 0) {
    iden[iden_i] = '\0';
    fprintf(stdout, "IDENTIFIER %s null\n", iden);
  }

  fprintf(stdout, "EOF  null\n");
  return code;
}

void print_error(int ln, char err[]) {
  fprintf(stderr, "[line %d] Error: %s\n", ln, err);
}

void print_number(const char num[]) {
  int num_i = strlen(num);
  float f = atof(num);
  int dot = 0;
  int all_z = 0;

  for (int i=0; i < num_i; i++) {
    if (num[i] == '.') {
      dot = 1;
      continue;
    }

    if (dot == 1 && num[i] != '0') {
      all_z = 1;
    } 
  }

  if (dot == 0 || all_z == 0) {
    fprintf(stdout, "NUMBER %s %0.f.0\n", num, f);
  } else {
    fprintf(stdout, "NUMBER %s %s\n", num, num);
  }
}
