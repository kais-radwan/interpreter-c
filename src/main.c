#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_contents(const char *filename);
int scan_paren(const char *content);
void print_error(int ln, char err[]);

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

  for (int i=0; i < len; i++) {
    char c = content[i];
    int last = 0;

    if (skip != 0) {
      skip--;
      continue;
    }

    if (i == len-1) {
      last = 1;
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
      fprintf(stdout, "DOT . null\n");
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

    if (c == '=') {
      if (last == 0 && content[i+1] == '=') {
        fprintf(stdout, "EQUAL_EQUAL == null\n");
        skip++;
      } else {
        fprintf(stdout, "EQUAL = null\n");
      }

      continue;
    }

    if (c == ';') {
      fprintf(stdout, "SEMICOLON ; null\n");
      continue;
    }

    if (c == '\n') {
      line++;
      continue;
    }

    char err[256];
    sprintf(err, "Unexpected character: %c", content[i]);
    print_error(line, err);
    code = 65;
  }

  fprintf(stdout, "EOF  null\n");
  return code;
}

void print_error(int ln, char err[]) {
  fprintf(stderr, "[line %d] Error: %s\n", ln, err);
}
