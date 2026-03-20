#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtcc.h"

TCCBufWriter buf_writer = { NULL, 0, 0, 0 };
TCCBufWriter debug_buf_writer = { NULL, 0, 0, 0 };

typedef struct {
    char *filename;
    int line_num;
    int is_warning;
    char *msg;
} ErrorEntry;

static ErrorEntry error_list[16];
static int error_count = 0;

void reset_errors() {
    for (int i = 0; i < error_count; i++) {
        free(error_list[i].filename);
        free(error_list[i].msg);
    }
    error_count = 0;
}

void error_callback(void *opaque, const TCCErrorInfo *info) {
    if (error_count >= 16) return;

    ErrorEntry *entry = &error_list[error_count];
    entry->filename = info->filename ? strdup(info->filename) : NULL;
    entry->line_num = info->line_num;
    entry->is_warning = info->is_warning;
    entry->msg = strdup(info->msg);
    error_count++;
}

TCCState* init_tcc() {
    reset_errors();
    TCCState *s = tcc_new(16*1024*1024);
    tcc_set_lib_path(s, ".");
    tcc_add_sysinclude_path(s, ".");
    tcc_set_error_func(s, NULL, error_callback);
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    tcc_set_options(s, "-Wall");
    tcc_set_options(s, "-Werror");
    return s;
}
int check_syntax(const char *source, int with_type_info) {

    TCCState *s = init_tcc();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        return -1;
    }

    int result;

    if (with_type_info) {
        if (buf_writer.buf == NULL) {
             buf_writer.buf = malloc(1 * 1024 * 1024);
             buf_writer.size = 1 * 1024 * 1024;
        }
        do {
            buf_writer.full = 0;
            buf_writer.pos = 0;
            result = tcc_compile_string_ex(s, source, &buf_writer);
            if (buf_writer.full) {
                buf_writer.size *= 2;
                buf_writer.buf = realloc(buf_writer.buf, buf_writer.size);
                tcc_delete(s);
                s = init_tcc();
            }
        } while (buf_writer.full);

        /* Get debug calls */
        if (debug_buf_writer.buf == NULL) {
            debug_buf_writer.buf = malloc(1 * 1024 * 1024);
            debug_buf_writer.size = 1 * 1024 * 1024;
        }
        debug_buf_writer.full = 0;
        debug_buf_writer.pos = 0;
        tcc_get_debug_calls(s, &debug_buf_writer);
    } else {
        result = tcc_compile_string_ex(s, source, NULL);
    }

    tcc_delete(s);
    return result;
}

const char* get_type_info_buffer() {
    return buf_writer.buf;
}

int get_type_info_length() {
    return buf_writer.pos;
}

const char* get_debug_calls_buffer() {
    return debug_buf_writer.buf;
}

int get_debug_calls_length() {
    return debug_buf_writer.pos;
}

int get_error_count() {
    return error_count;
}

const ErrorEntry* get_error(int index) {
    if (index >= 0 && index < error_count) {
        return &error_list[index];
    }
    return NULL;
}

const char* get_error_filename(int index) {
    if (index >= 0 && index < error_count) {
        return error_list[index].filename;
    }
    return NULL;
}

int get_error_line_num(int index) {
    if (index >= 0 && index < error_count) {
        return error_list[index].line_num;
    }
    return 0;
}

int get_error_is_warning(int index) {
    if (index >= 0 && index < error_count) {
        return error_list[index].is_warning;
    }
    return 0;
}

const char* get_error_msg(int index) {
    if (index >= 0 && index < error_count) {
        return error_list[index].msg;
    }
    return NULL;
}
