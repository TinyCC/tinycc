#include <stdio.h>
#include <string.h>

int isid(int c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '_';
}

int isspc(int c)
{
    return c == ' ' || c == '\t';
}

int main(int argc, char **argv)
{
    char l[1000], *p, l2[1000], *q;
    FILE *fp, *op;
    int c, e, f;

    if (argc < 3)
        return 1;

    fp = fopen(argv[1], "rb");
    op = fopen(argv[2], "wb");
    if (!fp || !op) {
        fprintf(stderr, "c2str: file error\n");
        return 1;
    }

    for (;;) {
        p = l;
    append:
        if (fgets(p, sizeof l - (p - l), fp)) {
            p = strchr(p, 0);
            while (p > l && p[-1] == '\n')
                --p;
            *p = 0;
        } else if (p == l)
            break;
        if (p > l && p[-1] == '\\') {
            --p;
            goto append;
        }

        if (l[0] == 0)
            continue;
        p = l, q = l2, f = e = 0;
        while (*p && isspc(*p))
            ++p, ++f;

        if (f < 4) {
            do {
               static const char *sr[] = {
                   "__x86_64__", "TCC_TARGET_X86_64",
                   "_WIN64", "TCC_TARGET_PE",
                   "_WIN32", "TCC_TARGET_PE",         
                   "__arm__", "TCC_TARGET_ARM",
                   "__aarch64__", "TCC_TARGET_ARM64",
                   "__riscv", "TCC_TARGET_RISCV64",
                   "__i386__", "TCC_TARGET_I386", 0 };
                for (f = 0; sr[f]; f += 2) {
                    c = strlen(sr[f]);
                    if (0 == memcmp(p, sr[f], c)) {
                        p += c, ++f;
                        q = strchr(strcpy(q, sr[f]), 0);
                        break;
                    }

                }
                if (sr[f])
                    continue;
            } while (!!(*q++ = *p++));

            fprintf(op, "%s\n", l2);

        } else if (*p == '/') {
            strcpy(q, p);
            fprintf(op, "    %s\n", l2);

        } else {
            f = 0;
            for (;;) {
                c = *p++;
                if (isspc(c)) {
                    if (q == l2 || isspc(q[-1]))
                        continue;
                    if ((f > 2 || e) || (q[-1] != ')' && *p != '(')) {
                        if (!isid(q[-1]) || !isid(*p))
                            continue;
                    }
                }
                if (c == '(')
                    ++e, ++f;
                if (c == ')')
                    --e, ++f;
                if (c == '\\' || c == '\"')
                    *q++ = '\\';
                *q++ = c;
                if (c == 0)
                    break;
            }
            fprintf(op, "    \"%s\\n\"\n", l2);
        }
    }

    fclose(fp);
    fclose(op);
    return 0;
}
