#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

struct buffer {
    char *chars;
    ssize_t len;
    ssize_t cap;
    size_t indent;
};

struct buffer title = { NULL };

void buffer_add(struct buffer *buf, char *str, ssize_t size) {
    if (size == -1) size = strlen(str);

    assert(buf->cap != -1);

    size_t current = buf->len;
    buf->len += size;
    if ((buf->len + 4) >= buf->cap) {
        while ((buf->len + 4) >= buf->cap) {
            buf->cap = (buf->cap + 8) * 2;
        }
        buf->chars = realloc(buf->chars, buf->cap);
        memset(&buf->chars[buf->len], 0, buf->cap - buf->len);
    }
    memcpy(&buf->chars[current], str, size);
}

void buffer_deinit(struct buffer *buf) {
    free(buf->chars);
}

void buffer_concat(struct buffer *dest, struct buffer *addl) {
    if (addl->len != 0) {
        buffer_add(dest, addl->chars, addl->len);
    }
}

void buffer_slice(struct buffer *source, ssize_t start, ssize_t len, struct buffer *dest) {
    dest->chars = source->chars + start;
    dest->len = len < 0 ? (source->len - start) : len;
    dest->cap = -1;
    dest->indent = source->indent;
}

void buffer_indent(struct buffer *into, ssize_t offset) {
    ssize_t amt = into->indent;
    if (offset < 0) {
        amt += offset;
    }
    into->indent += offset;
    for (ssize_t i = 0; i < amt; i++) {
        buffer_add(into, "  ", -1);
    }
}

struct inline_ctx {
    ssize_t index;
    bool prev_was_space;

    bool prev_was_open;

    bool only_text;
};

void escape_char(struct buffer *into, char ch) {
    if (ch == '&') {
        buffer_add(into, "&amp;", -1);
        return;
    }
    if (ch == '<') {
        buffer_add(into, "&lt;", -1);
        return;
    }
    if (ch == '>') {
        buffer_add(into, "&gt;", -1);
        return;
    }
    if (ch == '"') {
        buffer_add(into, "&quot;", -1);
        return;
    }
    buffer_add(into, &ch, 1);
}

bool escape_until(struct buffer *from, struct buffer *into, struct inline_ctx *ctx, char stop_at) {
    ctx->prev_was_space = false;
    ctx->prev_was_open = false;

    bool finished = false;

    ssize_t i;
    for (i = ctx->index; i < from->len; i++) {
        if (from->chars[i] == '\\') {
            buffer_add(into, from->chars + i + 1, 1);
            i++;
            continue;
        }
        if (from->chars[i] == stop_at) {
            finished = true;
            break;
        }
        buffer_add(into, from->chars + i, 1);
    }
    ctx->index = i;

    return finished;
}

bool format_inline(struct buffer *from, struct buffer *into, struct inline_ctx *ctx, char stop_at) {
    for (; ctx->index < from->len; ctx->index++) {
        ssize_t i = ctx->index;
        if (from->chars[i] == '\\') {
            buffer_add(into, from->chars + i + 1, 1);
            ctx->index++;
            continue;
        }
        char special = 0;
        if (!ctx->prev_was_space ||
            !(
                from->chars[i + 1] == ' ' ||
                from->chars[i + 1] == '\n' ||
                (i + 1) >= from->len
            )
        ) {
            special = from->chars[i];
        }

        if (special != 0 && special == stop_at) {
            ctx->index++;
            return true;
        }

        if (special == '[') {
            ctx->index++;
            struct buffer text_into = { NULL };
            if (format_inline(from, &text_into, ctx, ']')) {
                struct buffer link_into = { NULL };
                if (from->chars[ctx->index] == '(') {
                    ctx->index++;
                    escape_until(from, &link_into, ctx, ')');
                }

                if (ctx->only_text) {
                    buffer_concat(into, &text_into);
                } else {
                    buffer_add(into, "<a class=\"link\" href=\"", -1);
                    buffer_concat(into, &link_into);
                    buffer_add(into, "\">", -1);
                    buffer_concat(into, &text_into);
                    buffer_add(into, "</a>", -1);
                }

                buffer_deinit(&text_into);
                buffer_deinit(&link_into);
                continue;
            }

            buffer_add(into, "[", -1);
            buffer_concat(into, &text_into);

            buffer_deinit(&text_into);
            continue;
        }

        if (special == '/') {
            ctx->index++;
            struct buffer new_into = { NULL };
            if (format_inline(from, &new_into, ctx, '/')) {
                if (!ctx->only_text) buffer_add(into, "<em>", -1);
                buffer_concat(into, &new_into);
                if (!ctx->only_text) buffer_add(into, "</em>", -1);
                ctx->index--;
            } else {
                buffer_add(into, "/", -1);
                buffer_concat(into, &new_into);
            }
            buffer_deinit(&new_into);
            continue;
        }

        if (special == '*') {
            ctx->index++;
            struct buffer new_into = { NULL };
            if (format_inline(from, &new_into, ctx, '*')) {
                if (!ctx->only_text) buffer_add(into, "<strong>", -1);
                buffer_concat(into, &new_into);
                if (!ctx->only_text) buffer_add(into, "</strong>", -1);
                ctx->index--;
            } else {
                buffer_add(into, "*", -1);
                buffer_concat(into, &new_into);
            }
            buffer_deinit(&new_into);
            continue;
        }

        if (from->chars[i] == '`') {
            ctx->index++;
            struct buffer new_into = { NULL };
            if (escape_until(from, &new_into, ctx, '`')) {
                if (!ctx->only_text) buffer_add(into, "<code>", -1);
                buffer_concat(into, &new_into);
                if (!ctx->only_text) buffer_add(into, "</code>", -1);
            } else {
                buffer_add(into, "`", -1);
                buffer_concat(into, &new_into);
            }
            buffer_deinit(&new_into);
            continue;
        }

        if (from->chars[i] == ' ' || from->chars[i] == '\n' || from->chars[i] == '\t') {
            if (!ctx->prev_was_space) {
                buffer_add(into, " ", -1);
            }
            ctx->prev_was_space = true;
            continue;
        }

        if ((i > 0 && from->chars[i - 1] == '-') &&
            ((i - 1) > from->len || (from->chars[i + 1] != ' ' && from->chars[i + 1] != '\n'))) {
            ctx->prev_was_open = true;
        }

        if (from->chars[i] == '"') {
            if (ctx->prev_was_open || ctx->prev_was_space) {
                buffer_add(into, "“", -1);
                ctx->prev_was_open = true;
            } else {
                buffer_add(into, "”", -1);
            }
        } else if (from->chars[i] == '\'') {
            if (ctx->prev_was_open || ctx->prev_was_space) {
                buffer_add(into, "‘", -1);
                ctx->prev_was_open = true;
            } else {
                buffer_add(into, "’", -1);
            }
        } else {
            ctx->prev_was_open = from->chars[i] == '(' || from->chars[i] == '{' || from->chars[i] == '<';
            escape_char(into, from->chars[i]);
        }

        ctx->prev_was_space = false;
    }
    return false;
}

void format_paragraph(struct buffer *from, struct buffer *into) {
    if (from->len > 2 && from->chars[0] == '!' && from->chars[1] == '[') {
        do {
            struct buffer caption = { NULL };
            struct inline_ctx ctx = {
                .index = 2,
                .prev_was_space = true,
            };

            if (!format_inline(from, &caption, &ctx, ']')) {
                buffer_deinit(&caption);
                break;
            }

            if (ctx.index >= from->len || from->chars[ctx.index] != '(') {
                break;
            }

            ctx.index++;

            struct buffer url = { NULL };
            if (!escape_until(from, &url, &ctx, ')')) {
                buffer_deinit(&caption);
                buffer_deinit(&url);
                break;
            }
            if (ctx.index < from->len - 1) {
                buffer_deinit(&caption);
                buffer_deinit(&url);
                break;
            }

            buffer_indent(into, 1);
            buffer_add(into, "<figure>\n", -1);

            buffer_indent(into, 0);
            buffer_add(into, "<a href=\"", -1);
            buffer_concat(into, &url);
            buffer_add(into, "\">", -1);
            buffer_add(into, "<img src=\"", -1);
            buffer_concat(into, &url);
            buffer_add(into, "\"/></a>", -1);

            buffer_indent(into, 0);
            buffer_add(into, "<figcaption>", -1);
            buffer_concat(into, &caption);
            buffer_add(into, "</figcaption>", -1);

            buffer_indent(into, -1);
            buffer_add(into, "</figure>\n", -1);

            buffer_deinit(&caption);
            buffer_deinit(&url);
            return;
        } while(0);
    }

    char *start = "<p>", *end = "</p>";

    struct inline_ctx ctx = {
        .index = 0,
        .prev_was_space = true,
    };

    bool link_to_self = false;
    if (!strncmp(from->chars, "# ", 2)) {
        ctx.index = 2;
        link_to_self = true;
        start = "<h2>";
        end = "</h2>";
    }
    if (!strncmp(from->chars, "## ", 3)) {
        ctx.index = 3;
        link_to_self = true;
        start = "<h3>";
        end = "</h3>";
    }
    if (!strncmp(from->chars, "### ", 4)) {
        ctx.index = 4;
        link_to_self = true;
        start = "<h4>";
        end = "</h4>";
    }

    buffer_indent(into, 0);
    buffer_add(into, start, -1);
    if (link_to_self) {
        struct buffer id = { NULL };

        struct inline_ctx new_ctx = ctx;
        new_ctx.only_text = true;
        format_inline(from, &id, &new_ctx, 0);

        char buf[128];
        size_t buf_len = 0;
        bool was_hyphen = true;
        for (ssize_t i = 0; i < id.len && buf_len < 127; i++) {
            char ch = id.chars[i];
            if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
                buf[buf_len++] = ch;
                was_hyphen = false;
                continue;
            }
            if (ch >= 'A' && ch <= 'Z') {
                buf[buf_len++] = ch + 'a' - 'A';
                was_hyphen = false;
                continue;
            }
            if (!was_hyphen) {
                was_hyphen = true;
                buf[buf_len++] = '-';
            }
        }
        if (was_hyphen && buf_len > 0) {
            buf[buf_len - 1] = '\0';
        } else {
            buf[buf_len] = '\0';
        }

        buffer_add(into, "<a class=\"heading__link\" id=\"", -1);
        buffer_add(into, buf, -1);
        buffer_add(into, "\" href=\"#", -1);
        buffer_add(into, buf, -1);
        buffer_add(into, "\">", -1);

        buffer_deinit(&id);
    }
    format_inline(from, into, &ctx, 0);
    if (link_to_self) {
        buffer_add(into, "</a>", -1);
    }
    buffer_add(into, end, -1);
    buffer_add(into, "\n", -1);
}

void format_main(struct buffer *from, struct buffer *into);

struct highlight_word {
    char *fwdre;
    char *style;
    char *next_mode; // NULL to stay in current mode, "!pop" to go to previous mode in the stack, or any other string to push that mode onto the stack.
};

struct highlight_mode {
    char *name;
    struct highlight_word *words;
};

struct highlight_language {
    struct highlight_mode *modes;
};

struct highlight_state {
    char *stack[32];
    int stack_len;
};

struct highlight_language lang_c = {
    .modes = (struct highlight_mode[]) {
        {
            .name = "default",
            .words = (struct highlight_word[]) {
                { "if", "keyword", NULL },
                { "else", "keyword", NULL },
                { "while", "keyword", NULL },
                { "for", "keyword", NULL },
                { "do", "keyword", NULL},
                { "in", "keyword", NULL},
                { "extern", "keyword", NULL},
                { "inline", "keyword", NULL},
                { "static", "keyword", NULL},
                { "struct", "keyword", NULL},
                { "union", "keyword", NULL},

                // :)
                { "const", "keyword", NULL},
                { "let", "keyword", NULL},
                { "var", "keyword", NULL},

                { "true", "keyword", NULL},
                { "false", "keyword", NULL},

                { "void", "type", NULL },
                { "short", "type", NULL },
                { "int", "type", NULL },
                { "long", "type", NULL },
                { "unsigned", "type", NULL },
                { "signed", "type", NULL },
                { "float", "type", NULL },
                { "double", "type", NULL },
                { "?uint+[0-9]_t", "type", NULL },
                { "?[ui]vec+[0-9][^a-zA-Z0-9_]", "type", NULL },

                { "\"+[^\"]\"", "string", NULL },
                { "//", "comment", "comment" },
                { "/*", "comment", "comment_multiline" },

                { "[a-z_]*[a-zA-Z0-9_]", NULL, NULL },
                { "[A-Z]*[a-zA-Z0-9_]", "type", NULL },
                { "[0-9]*[0-9a-fA-FxXeEoObB_]", "number", NULL },
                { "#[a-zA-Z_]*[a-zA-Z0-9_]", "keyword", NULL },
                { 0 },
            },
        },
        {
            .name = "string",
            .words = (struct highlight_word[]) {
                { "\\.", "string", NULL },
                { "\"", "string", "!pop" },
                { ".", "string", NULL },
                NULL, NULL,
            },
        },
        {
            .name = "comment",
            .words = (struct highlight_word[]) {
                { "+[^\n]", "comment", NULL },
                { "\n", "comment", "!pop" },
                { 0 },
            },
        },
        {
            .name = "comment_multiline",
            .words = (struct highlight_word[]) {
                { ".", "comment", NULL },
                { "*/", "comment", "!pop" },
                { 0 },
            },
        },
        {
            .name = NULL
        },
    },
};

struct highlight_language lang_acre = {
    .modes = (struct highlight_mode[]) {
        {
            .name = "default",
            .words = (struct highlight_word[]) {
                { "if", "keyword", NULL },
                { "else", "keyword", NULL },
                { "while", "keyword", NULL },
                { "for", "keyword", NULL },
                { "each", "keyword", NULL },
                { "func", "keyword", NULL},
                { "return", "keyword", NULL},
                { "break", "keyword", NULL},
                { "continue", "keyword", NULL},
                { "var", "keyword", NULL },
                { "def", "keyword", NULL },
                { "extern", "keyword", NULL},
                { "struct", "keyword", NULL},
                { "union", "keyword", NULL},
                { "and", "keyword", NULL},
                { "or", "keyword", NULL},
                { "not", "keyword", NULL},
                { "await", "keyword", NULL},
                { "signal", "keyword", NULL},

                { "void", "type", NULL },
                { "opaque", "type", NULL },
                { "u8", "type", NULL },
                { "u16", "type", NULL },
                { "u32", "type", NULL },
                { "u64", "type", NULL },
                { "usize", "type", NULL },
                { "i8", "type", NULL },
                { "i16", "type", NULL },
                { "i32", "type", NULL },
                { "i64", "type", NULL },
                { "isize", "type", NULL },
                { "f32", "type", NULL },
                { "f64", "type", NULL },

                { "false", "constant", NULL },
                { "true", "constant", NULL },
                { "null", "constant", NULL },

                { "\"+[^\"]\"", "string", NULL },
                { "\"", "string", "string" },
                { "//", "comment", "comment" },
                { "/*", "comment", "comment_multiline" },

                { "[a-zA-Z_]*[a-zA-Z0-9_]", NULL, NULL },
                { "[0-9]*[0-9a-fA-FxXeEoObB_]", "number", NULL },
                { "#[a-zA-Z_]*[a-zA-Z0-9_]", "keyword", NULL },
                { "@[a-zA-Z_]*[a-zA-Z0-9_]", "keyword", NULL },
                { "# *[^\n]", "comment", NULL },
                { 0 },
            },
        },
        {
            .name = "string",
            .words = (struct highlight_word[]) {
                { "\\.", "string", NULL },
                { "\"", "string", "!pop" },
                { ".", "string", NULL },
                NULL, NULL,
            },
        },
        {
            .name = "comment",
            .words = (struct highlight_word[]) {
                { "+[^\n]", "comment", NULL },
                { "\n", "comment", "!pop" },
                { 0 },
            },
        },
        {
            .name = "comment_multiline",
            .words = (struct highlight_word[]) {
                { ".", "comment", NULL },
                { "/*", "comment", "comment_multiline" },
                { "*/", "comment", "!pop" },
                { 0 },
            },
        },
        {
            .name = NULL
        },
    },
};

size_t fwdre_match(char *re, ssize_t re_size, char *str, size_t length) {
    if (re_size == -1) {
        re_size = strlen(re);
    }
    char *last_groupish = NULL;
    ssize_t last_groupish_len = 0;
    for (size_t i = 0, j = 0; i < length; i++) {
        if (j == re_size || re[j] == ')') {
            return i;
        }
        char repeat = '\0';
        if (re[j] == '*' || re[j] == '+' || re[j] == '?') {
            repeat = re[j];
            j++;
        }
        if (re[j] == '(') {
            int times = 0;
            while (true) {
                size_t match_amount = fwdre_match(&re[j + 1], -1, &str[i], length - i);
                if (match_amount == 0) {
                    break;
                }
                i += match_amount;
                times++;
                if (repeat == '\0' || repeat == '?') {
                    break;
                }
            }
            if (times == 0 && repeat != '*' && repeat != '?') {
                return 0; // no match
            }
        } else if (re[j] == '.') {
            continue;
        } else if (re[j] == '[') {
            j++;

            bool invert = false;
            if (re[j] == '^') {
                invert = true;
                j++;
            }

            size_t start = j;
            if (re[j] == ']') {
                j++;
            }
            if (re[j] == '[') {
                j++;
            }

            while (j < re_size && re[j] != ']') {
                j++;
            }
            size_t end = j;
            j++;

            int times = 0;
            while (true) {
                bool match = false;
                for (size_t k = start; k < end; k++) {
                    if ((k + 2) < end && re[k + 1] == '-') {
                        if (str[i] >= re[k] && str[i] <= re[k + 2]) {
                            match = true;
                            break;
                        }
                    } else {
                        if (str[i] == re[k]) {
                            match = true;
                            break;
                        }
                    }
                }
                if (match == invert) {
                    break;
                }

                times++;
                i++;
                if (i >= length) {
                    break;
                }
                if (repeat == '\0' || repeat == '?') {
                    break;
                }
            }
            i--;

            if (times == 0 && repeat != '*' && repeat != '?') {
                return 0; // no match
            }
        } else {
            int times = 0;
            while (i < length && str[i] == re[j]) {
                i++;
                times++;
                if (repeat == '\0' || repeat == '?') {
                    break;
                }
            }
            j++;
            i--;
            if (times == 0 && repeat != '*' && repeat != '?') {
                return 0; // no match
            }
        }
    }

    return 0;
}

void highlight_code(struct buffer *from, struct buffer *into, struct highlight_language *language) {
    struct highlight_state state = { 0 };
    buffer_indent(into, 0);
    buffer_add(into, "<pre class=\"codeblock\">", -1);
    char *current_style = NULL;
    for (size_t i = 0; i < from->len; i++) {
        size_t line_start = i;
        while (i < from->len && from->chars[i] != '\n') {
            i++;
        }
        size_t line_end = i + 1;
        size_t start = line_start;
        struct highlight_mode *current_mode = &language->modes[0];
        while (start < line_end) {
            size_t max_len = 0;
            struct highlight_word *word = NULL;
            for (size_t i = 0; current_mode->words[i].fwdre != NULL; i++) {
                size_t len = fwdre_match(current_mode->words[i].fwdre, -1, &from->chars[start], line_end - start);
                if (len > 0 && len > max_len) {
                    max_len = len;
                    word = &current_mode->words[i];
                }
            }
            char *new_style = NULL;
            if (word != NULL) {
                new_style = word->style;
            }

            if (new_style != current_style) {
                if (current_style != NULL) {
                    buffer_add(into, "</span>", -1);
                }
                if (new_style != NULL) {
                    buffer_add(into, "<span class=\"hl__", -1);
                    buffer_add(into, new_style, -1);
                    buffer_add(into, "\">", -1);
                }
                current_style = new_style;
            }
            if (max_len == 0) {
                escape_char(into, from->chars[start]);
                start += 1;
                continue;
            }

            for (size_t i = 0; i < max_len; i++) {
                escape_char(into, from->chars[start + i]);
            }
            start += max_len;
            char *mode = word->next_mode;
            if (mode != NULL) {
                if (!strcmp(mode, "!pop")) {
                    mode = state.stack[--state.stack_len];
                } else {
                    state.stack[state.stack_len++] = mode;
                }
                current_mode = NULL;
                for (size_t i = 0; language->modes[i].name != NULL; i++) {
                    if (!strcmp(language->modes[i].name, mode)) {
                        current_mode = &language->modes[i];
                        break;
                    }
                }
                if (current_mode == NULL) {
                    printf("couldn't find mode %s\n", mode);
                    current_mode = &language->modes[0];
                    state.stack_len = 0;
                }
            }
        }
        if (current_style != NULL) {
            buffer_add(into, "</span>", -1);
        }
        current_style = NULL;
        if ((i + 1) < from->len && from->chars[i + 1] != '\n') {
            i += from->indent;
        }
    }
    buffer_add(into, "</pre>", -1);
}

void format_code_block(struct buffer *from, struct buffer *into, char *language) {
    ssize_t i = 0;

    // skip the `code:<language>` bit
    while (i < from->len && from->chars[i] != '\n') {
        i++;
    }

    if (language != NULL) {
        struct highlight_language *lang = NULL;
        if (!strcmp(language, "acre")) lang = &lang_acre;
        if (!strcmp(language, "c")) lang = &lang_c;

        if (lang != NULL) {
            struct buffer from_stripped;
            buffer_slice(from, i, -1, &from_stripped);

            highlight_code(&from_stripped, into, lang);
            return;
        }
    }

    buffer_indent(into, 0);
    buffer_add(into, "<pre class=\"codeblock\">", -1);
    for (; i < from->len; i++) {
        if (from->chars[i] == '\n') {
            buffer_add(into, "\n", -1);

            if (from->chars[i + 1] != '\n') {
                i += from->indent;
            }
            continue;
        }
        escape_char(into, from->chars[i]);
    }

    buffer_add(into, "</pre>\n", -1);
}

void format_block(struct buffer *from, struct buffer *into) {
    ssize_t nl = 1;
    if (from->chars[1] == ' ' && from->chars[0] == '>') {
        for (ssize_t i = 0; i < from->len; i++) {
            if (from->chars[i] == '\n') {
                nl = i;
                break;
            }
        }
    }
    char block_name[64];
    {
        size_t len = 0;
        for (size_t i = 2; i < nl; i++) {
            block_name[len++] = from->chars[i];
        }
        block_name[len] = '\0';
    }
    if (!strcmp(block_name, "code")) {
        format_code_block(from, into, NULL);
        return;
    }
    if (!memcmp(block_name, "code:", 5)) {
        format_code_block(from, into, &block_name[5]);
        return;
    }
    if (!strcmp(block_name, "title")) {
        struct inline_ctx ctx = {
            .index = 0,
            .prev_was_space = true,
            .only_text = true,
        };
        struct buffer new_from;
        ssize_t next_nl = nl + 1;
        while (from->chars[next_nl] != '\n' && next_nl < from->len) {
            next_nl++;
        }
        buffer_slice(from, nl, next_nl - nl, &new_from);
        format_inline(&new_from, &title, &ctx, 0);
    }
    char *end;
    {
        buffer_indent(into, 1);
        if (from->chars[0] == '-' || from->chars[0] == '+') {
            // the actual <ul> or <ol> is handled by format_main
            buffer_add(into, "<li>\n", -1);
            end = "</li>\n";
        } else if (!strcmp(block_name, "header")) {
            buffer_add(into, "<header>\n", -1);
            end = "</header>\n";
        } else {
            if (nl > 1) {
                buffer_add(into, "<div class=\"block block--", -1);
                buffer_add(into, from->chars + 2, nl - 2);
                buffer_add(into, "\">\n", -1);
                end = "</div>\n";
            } else {
                buffer_add(into, "<blockquote>\n", -1);
                end = "</blockquote>\n";
            }
        }
    }
    struct buffer inner = { NULL };
    buffer_slice(from, nl, -1, &inner);
    from = &inner;
    format_main(from, into);
    buffer_indent(into, -1);
    buffer_add(into, end, -1);
}

bool is_block_begin(struct buffer *buf, ssize_t offs) {
    if (buf->chars[offs + 1] == '\n' || buf->chars[offs + 1] == ' ') {
        if (strchr("-+>", buf->chars[offs]) != NULL) {
            return true;
        }
    }
    return false;
}

void format_main(struct buffer *from, struct buffer *into) {
    char *current_list = NULL;
    for (ssize_t i = 0; i < from->len; i++) {
        if (is_block_begin(from, i)) {
            if (from->chars[i] == '-') {
                if (current_list == NULL || strcmp(current_list, "</ul>\n") != 0) {
                    if (current_list != NULL) {
                        buffer_indent(into, -1);
                        buffer_add(into, current_list, -1);
                    }
                    current_list = "</ul>\n";
                    buffer_indent(into, 1);
                    buffer_add(into, "<ul>\n", -1);
                }
            }
            if (from->chars[i] == '+') {
                if (current_list == NULL || strcmp(current_list, "</ol>\n") != 0) {
                    if (current_list != NULL) {
                        buffer_indent(into, -1);
                        buffer_add(into, current_list, -1);
                    }
                    current_list = "</ol>\n";
                    buffer_indent(into, 1);
                    buffer_add(into, "<ol>\n", -1);
                }
            }
            ssize_t new_indent = from->indent + 2;
            ssize_t end = from->len;
            for (ssize_t j = i; j < from->len; j++) {
                if (from->chars[j] == '\n') {
                    bool is_indented = true;
                    if (from->chars[j + 1] != '\n') {
                        for (ssize_t k = 0; k < new_indent; k++) {
                            if (from->chars[j + k + 1] != ' ') {
                                is_indented = false;
                                break;
                            }
                        }
                    }
                    if (!is_indented) {
                        end = j;
                        break;
                    }
                }
            }
            struct buffer new_buffer;
            buffer_slice(from, i, end - i, &new_buffer);
            new_buffer.indent = new_indent;
            format_block(&new_buffer, into);

            i = end - 1;
            continue;
        }
        if (from->chars[i] == ' ' || from->chars[i] == '\n') {
            continue;
        }
        if (current_list != NULL) {
            buffer_indent(into, -1);
            buffer_add(into, current_list, -1);
            current_list = NULL;
        }
        ssize_t end = from->len;
        ssize_t slice_end = end;
        for (ssize_t j = i; j < from->len; j++) {
            slice_end = j + 1;
            if (from->chars[j] == '\n') {
                slice_end = j;
                if (from->chars[j + 1] == '\n') {
                    end = j;
                    break;
                }
                j += from->indent + 1;
                if (is_block_begin(from, j)) {
                    end = j;
                    break;
                }
                j -= 1;
            }
        }
        struct buffer slice;
        buffer_slice(from, i, slice_end - i, &slice);
        format_paragraph(&slice, into);
        i = end - 1;
    }
    if (current_list != NULL) {
        buffer_indent(into, -1);
        buffer_add(into, current_list, -1);
        current_list = NULL;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <FILE>\n", argv[0]);
        return 1;
    }

    char *template_file = getenv("TEMPLATE");
    if (template_file == NULL) template_file = "template.html";
    FILE *tmp = fopen(template_file, "rb");
    struct buffer template_txt = { NULL };
    if (tmp == NULL) {
        fprintf(stderr, "couldn't read file %s\n", template_file);
        buffer_add(&template_txt, "$CONTENT", -1);
    } else {
        while (true) {
            char chunk[1024];
            size_t size = fread(chunk, 1, 1024, tmp);
            if (size == 0) break;
            buffer_add(&template_txt, chunk, size);
        }
        fclose(tmp);
    }

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) {
        perror("fopen");
        fprintf(stderr, "couldn't open file %s\n", argv[1]);
        return 1;
    }
    struct buffer source = { NULL };
    while (true) {
        char chunk[1024];
        size_t size = fread(chunk, 1, 1024, f);
        if (size == 0) break;
        buffer_add(&source, chunk, size);
    }
    fclose(f);

    struct buffer out = { NULL };
    format_main(&source, &out);

    buffer_add(&out, "\0", 1);

    for (ssize_t i = 0; i < template_txt.len; i++) {
        if (template_txt.chars[i] == '$') {
            char name[64];
            size_t name_len = 0;
            for (size_t j = 0; j < 64; j++) {
                char ch = template_txt.chars[i + j + 1];
                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_') {
                    name[name_len++] = ch;
                } else {
                    break;
                }
            }
            name[name_len] = '\0';
            i += name_len;

            if (!strcmp(name, "CONTENT")) {
                printf("%s", out.chars);
            } else if (!strcmp(name, "TITLE")) {
                printf("%.*s", (int) title.len, title.chars);
            } else if (getenv(name)) {
                printf("%s", getenv(name));
            } else {
                printf("(unknown variable %s)", name);
            }
            continue;
        }
        printf("%c", template_txt.chars[i]);
    }

    buffer_deinit(&source);
    buffer_deinit(&out);
    buffer_deinit(&template_txt);

    return 0;
}
