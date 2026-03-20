#include <stdio.h>
#include <stdlib.h>
#include "libtcc.h"

void error_callback(void *opaque, const TCCErrorInfo *info)
{
    fprintf(stderr, "file %s\n", info->filename);
    fprintf(stderr, "line %d\n", info->line_num);
    fprintf(stderr, "msg %s\n", info->msg);
}

char *read_file(const char *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fprintf(stderr, "Could not allocate memory for file\n");
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';

    fclose(f);
    return content;
}


int check_syntax(const char *content)
{
    TCCState *s = tcc_new(16 * 1024 * 1024);
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        return -1;
    }

    tcc_set_lib_path(s, ".");
    tcc_set_error_func(s, NULL, error_callback);
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);

    char *json = malloc(2 * 1024 * 1024);  /* 2MB buffer for JSON */
    if (!json) {
        fprintf(stderr, "Could not allocate JSON buffer\n");
        tcc_delete(s);
        return -1;
    }

    TCCBufWriter w = { json, 0, 2 * 1024 * 1024, 0 };
    int result = tcc_compile_string_ex(s, content, &w);
    if (result == 0) {
        if (w.full) {
            fprintf(stderr, "Warning: JSON buffer full, output may be truncated\n");
        }
        //printf("Type Info:\n%s\n\n", json);
    }

    /* Get debug calls */
    char *debug_json = malloc(2 * 1024 * 1024);  /* 2MB buffer for debug calls JSON */
    if (debug_json) {
        TCCBufWriter debug_w = { debug_json, 0, 2 * 1024 * 1024, 0 };
        int debug_result = tcc_get_debug_calls(s, &debug_w);
        if (debug_result == 0 && !debug_w.full) {
            printf("Debug Calls:\n%s\n", debug_json);
        }
        free(debug_json);
    }

    free(json);
    tcc_delete(s);

    return result;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.c>\n", argv[0]);
        return 1;
    }

    char* file_contents = read_file(argv[1]);
    if (!file_contents) {
        return 1;
    }

    for(int i = 0; i<1; i++) {
	    int result = check_syntax(file_contents);

	    if (result == 0) {
		    printf("Syntax OK: %s\n", argv[1]);
	    } else {
		    printf("Syntax errors in: %s\n", argv[1]);
	    }
    }

    free(file_contents);
}
