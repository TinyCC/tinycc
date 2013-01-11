
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>

/* Version information */
const char *versionInfo = 
    "bin2c version 1.0 by Fatih Aygun\n"
    "\n"
    "Visit sourceforge.net/projects/bin2c to for new versions.\n"
    "This program is public domain, do whatever you want with it.\n"
;

/* Command line help */
const char *commandLineHelp = 
    "Usage: bin2c [OPTION...] FILE [FILE...]\n"
    "bin2c creates a C soures and headers from binary files.\n"
    "\n"
    "Examples:\n"
    "  bin2c -o foo.h bar.bin           # Create 'foo.h' from the contents of\n"
    "                                     'bar.bin'\n"
    "  bin2c -o foo.h file1 file2       # Create 'foo.h' from the contents of 'file1'\n"
    "                                     and 'file2'\n"
    "  bin2c -d foo.h -o foo.c bar.bin  # Create 'foo.c' and 'foo.h' from the\n"
    "                                     contents of 'bar.bin'\n"
    "\n"
    "Options:\n"
    "  -d, --header <file name>  Name a header file (A header file will not be\n"
    "                            created  unless explicitly named)\n"
    "  -h, --help                Print this command line help and exit immediately\n"
    "  -o, --output <file name>  Name an output file\n"
    "  -m, --macro               Create the size definition as a macro instead of\n"
    "                            a const\n"
    "  -n, --name <symbol name>  Name the symbol to be defined\n"
    "  -v, --version             Print version information and exit immediately\n"
    "\n"
    "Visit sourceforge.net/projects/bin2c to for new versions.\n"
    "This program is public domain, do whatever you want with it.\n"
    ;

/* Output formatting macros */
#define bytesPerLine            16
#define indentString            "    "
#define outputFormatBase        "0x%.2X"
#define outputFormat            outputFormatBase", "
#define outputFormatEOL         outputFormatBase",\n"
#define outputFormatEOF         outputFormatBase"\n"
#define sizeDefinition          "const long int bin2c_%s_size = %d;\n"
#define sizeMacroDefinition     "#define bin2c_%s_size %d\n"
#define typeName                "const unsigned char"

/* Define bool */
#define bool    int
#define false   0
#define true    1

/* Global variables */
FILE *outputFile=0;           /* Current output file stream */
const char *outputFileName=0; /* Current output file name */
FILE *headerFile=0;           /* Current header file */
const char *headerFileName=0; /* Current header file name */
FILE *tmpFuncFile=0;           /* File to write final function that search for files */
const char *tmpFuncFileName=0; /* Current tmpFunc file name */
char *symbolName=0;           /* Current symbol name */
char *headerSymbol=0;         /* Symbol for multiple inclusion protection */
bool createMacro = false;   /* Should we create a define instead of a const? */

/* Error messages */
const char* cantOpen = "Can't open file '%s'";
const char* cantClose = "Can't close file '%s'";
const char* cantRemove = "Can't remove file '%s'";
const char* cantWrite = "Can't write to file '%s'";
const char* noOutputFilesAreNamed = "No output files are named";
const char* noHeaderFilesAreNamed = "No header files are named";
const char* cantMalloc = "Can't allocate memory for '%s'";
const char* cantSeek = "Can't seek in the file '%s'";
const char* cantDetermine = "Can't determine the file size of '%s'";
const char* cantRead = "Can't read from file %s";
const char* unknownOption = "Unknown option '%s'";
const char* tryHelp = "Try 'bin2c --help' for more information.\n";
const char* noSymbolName = "No symbol name is given";
const char* notValidId = "'%s' is not a valid identifier";
const char* symbolNameGiven = "Symbol name is given twice";

/* Print a formatted error message */
static void vprintfError(const char *format, va_list args)
{
    fprintf(stderr, "bin2c: ");
    vfprintf(stderr, format, args);
    if (errno) {
        fprintf(stderr, ": ");
        perror("");
        errno = 0;
    } else {
        fprintf(stderr, "\n");
    }
}

static void printfError(const char *format, ...)
{
    va_list args;
    
    va_start(args, format);
    vprintfError(format, args);
    va_end(args);
}

/* Panic */
static void panic()
{
        /* Close and remove the output file if it's open */
        if (outputFile) {
            if (fclose(outputFile)) {
                printfError(cantClose, outputFileName);
            }
            
            if (remove(outputFileName)) {
                printfError(cantRemove, outputFileName);
            }
        }        
        
        /* Close and remove the header file if it's open */
        if (headerFile) {
            if (fclose(headerFile)) {
                printfError(cantClose, headerFileName);
            }
            
            if (remove(headerFileName)) {
                printfError(cantRemove, headerFileName);
            }
        }        

        /* Close and remove the header file if it's open */
        if (tmpFuncFile) {
            if (fclose(tmpFuncFile)) {
                printfError(cantClose, tmpFuncFileName);
            }
            
            if (remove(tmpFuncFileName)) {
                printfError(cantRemove, tmpFuncFileName);
            }
        }        

        /* Exit with an error code */
        exit(EXIT_FAILURE);
}

/* Check a contidion and panic if it's false */
static void check(bool condition, const char *format, ...)
{
    va_list args;

    if (!condition) {
        va_start(args, format);
        vprintfError(format, args);
        va_end(args);
        panic();        
    }
}

/* Write a formatted string to the output file and check for errors */
static void output(const char *format, ...)
{
    va_list args;

    /* Try to write to the file */
    va_start(args, format);
    vfprintf(outputFile, format, args);
    va_end(args);

    /* Check for errors */
    check(!ferror(outputFile), cantWrite, outputFileName);
}

/* Write a formatted string to the header file and check for errors */
static void header(const char *format, ...)
{
    va_list args;

    /* Try to write to the file */
    va_start(args, format);
    vfprintf(headerFile, format, args);
    va_end(args);

    /* Check for errors */
    check(!ferror(headerFile), cantWrite, headerFileName);
}

/* Write a formatted string to the tmp file and check for errors */
static void tmpFunc(const char *format, ...)
{
    va_list args;

    /* Try to write to the file */
    va_start(args, format);
    vfprintf(tmpFuncFile, format, args);
    va_end(args);

    /* Check for errors */
    check(!ferror(tmpFuncFile), cantWrite, tmpFuncFileName);
}

/* Allocate a valid C identifier from a file name */
static char *fileNameToSymbol(const char *fileName)
{
    char *name;
    int i;
    
    name = malloc(strlen(fileName) + 1);
    check(name != 0, cantMalloc, "the symbol name");

    /* Copy the file name to symbolName */
    strcpy(name, fileName);

    /* Replace every non alphanumerical character with '_' */
    for (i = 0; name[i]; ++i) {
        if (!((name[i] >= 'a' && name[i] <= 'z') || 
                (name[i] >= 'A' && name[i] <= 'Z') || 
                (name[i] >= '0' && name[i] <= '9'))) {
            name[i] = '_';
        }
    }

    /* Replace the first character with '_' if it's a digit */
    if (name[0] >= '0' && name[0] <= '9') {
        name[0] = '_';
    }
    
    return name;
}

 int isdir(const char *  path) 
{
         int bOK = 0;
 #ifndef _MSC_VER
         struct stat buf;
         if (path == NULL) return 0;
         if (stat(path, &buf) == 0 && S_ISDIR(buf.st_mode)) bOK=1;
 #else
         wchar_t *wcpath = to_wide_string((char*)path);
         if (wcpath == NULL) return FALSE;
         bOK = isdirW(wcpath);
         FREE(wcpath);
 #endif
         return bOK;
 }

/* Process an input file */
static int processFile(const char *fileName, long int from, long int to)
{
    bool symbolNameAllocated = false;   /* Did we allocate the symbolName? */
    long int i, j;                      /* Indexes for various purposes */
    FILE *inputFile;                    /* Input file stream */
    int byte;                           /* Byte read from the input */
     long int fsize=0;
	
    if(isdir(fileName)) return 0;
    
    /* Be sure that the output file is open */
    check(outputFile != 0, noOutputFilesAreNamed);

    /* Write the comment */
    output("\n/* Contents of file %s */\n", fileName);
    if (headerFile) {
        header("\n/* Contents of file %s */\n", fileName);
    }

    /* Make up a suitable symbolName if we don't have one */
    if (!symbolName) {
        symbolName = fileNameToSymbol(fileName);
        symbolNameAllocated = true;
    }

    /* Open the input file */
    inputFile = fopen(fileName, "rb");
    check(inputFile != 0, cantOpen, fileName);

    /* If to == -1, set it to the last byte of the file */
    if (to == -1) {
        check(!fseek(inputFile, 0, SEEK_END), cantSeek, fileName);
        to = ftell(inputFile);
        check(to >= 0, cantDetermine, fileName);
        --to;
    }
    
    assert(from <= to - 1);

    /* Position the file to "from" */
    check(!fseek(inputFile, from, SEEK_SET), cantSeek, fileName);
    
    fsize = to - from + 1;

    /* Output the size definition */
    if (headerFile) {
        if (createMacro) {
            header(sizeMacroDefinition, symbolName, fsize);
        } else {
            header("extern "sizeDefinition, symbolName, fsize);
            output(sizeDefinition, symbolName, fsize);
        }
    } else {
        if (createMacro) {
            output(sizeMacroDefinition, symbolName, fsize);
        } else {
            output(sizeDefinition, symbolName, fsize);
        }
    }

    if (tmpFuncFile) {
        tmpFunc("\t{\"/_attach_/%s\", bin2c_%s,  %d},\n", fileName, symbolName, fsize);
    }
    /* Output the definition */
    output("%s bin2c_%s[%d] = {\n", typeName, symbolName, fsize);
    if (headerFile) {
        header("extern %s *bin2c_%s;\n", typeName, symbolName, fsize);
    }

    /* Output the contents */
    j = 1;
    for (i = from; i <= to; ++i) {
        /* Read a byte from the input file */
        byte = fgetc(inputFile);
        check(byte != EOF, cantRead, fileName);
        
        /* Output the indentation if it's the first byte of a line */
        if (j == 1) {
            output(indentString);
        }

        /* Output the actual byte */
        if (i == to) {
            /* Output the last byte */
            output(outputFormatEOF, byte);
        } else if (j == bytesPerLine) {
            /* Output the last byte of a line */
            output(outputFormatEOL, byte);
        } else {
            /* Output a normal byte */
            output(outputFormat, byte);
        }

        /* Next byte */
        ++j;
        if (j > bytesPerLine) {
            j = 1;
        }
    }

    /* Output the end of definition */
    output("};\n");

    /* Free the symbol name if it was allocated by us */
    if (symbolNameAllocated) {
        free(symbolName);
    }

    /* Clear the symbol name */
    symbolName = 0;
    return 1;
}

/* Unknown option error */
static void handleUnknownOption(const char *s)
{
    printfError(unknownOption, s);
    fprintf(stderr, tryHelp);
    panic();
}

/* Open an output file */
static void openOutputFile(const char *fileName)
{
    /* Be sure that a file name is given */
    if (!fileName) {
        printfError(noOutputFilesAreNamed);
        fprintf(stderr, tryHelp);
        panic();
    }
    
    /* Close previous output file if any */
    if (outputFile) {
        check(!fclose(outputFile), cantClose, outputFileName);
    }
    
    /* Open the new output file */
    outputFile = fopen(fileName, "w");
    check(outputFile != 0, cantOpen, outputFileName);
    outputFileName = fileName;

    output("/* Generated by bin2c, do not edit manually */\n");
}

/* Open a header file */
static void openHeaderFile(const char *fileName)
{
    /* Be sure that a file name is given */
    if (!fileName) {
        printfError(noHeaderFilesAreNamed);
        fprintf(stderr, tryHelp);
        panic();
    }
    
    /* Close previous header file if any */
    if (headerFile) {
        header("\n#endif    /* __%s_included */\n", headerSymbol);
        free(headerSymbol);
        headerSymbol = 0;
        check(!fclose(headerFile), cantClose, headerFileName);
    }
    
    /* Open the new header file */
    headerFile = fopen(fileName, "w");
    check(headerFile != 0, cantOpen, headerFileName);
    headerFileName = fileName;
    headerSymbol = fileNameToSymbol(fileName);
    
    header("/* Generated by bin2c, do not edit manually */\n");
    header("#ifndef __%s_included\n", headerSymbol);
    header("#define __%s_included\n", headerSymbol);
}

 typedef void writeFileFunc(const char *format, ...);
 
 void writeAttchmentStruct(writeFileFunc *writeFunc){
	(*writeFunc)("\ntypedef struct {\n");
	(*writeFunc)("\tconst char *file_name;\n");
	(*writeFunc)("\tconst unsigned char *sym_name;\n");
	(*writeFunc)("\tlong size;\n");
	(*writeFunc)("} bin2c_filesAttached_st;\n");
}

/* Open a header file */
static void openFuncFile(const char *fileName)
{
    static char fnBuf[1024];
    /* Be sure that a file name is given */
    if (!fileName) {
        printfError(noHeaderFilesAreNamed);
        fprintf(stderr, tryHelp);
        panic();
    }
    
    /* Close previous header file if any */
    if (tmpFuncFile) {
        tmpFunc("\n};\n");
        check(!fclose(tmpFuncFile), cantClose, tmpFuncFileName);
    }
    
    snprintf(fnBuf, sizeof(fnBuf), "%s.tmp", fileName);
    
    /* Open the new header file */
    tmpFuncFile = fopen(fnBuf, "w");
    check(tmpFuncFile != 0, cantOpen, fnBuf);
    tmpFuncFileName = fnBuf;
    
    tmpFunc("\n/* Generated by bin2c, do not edit manually */\n");
    writeAttchmentStruct(&tmpFunc);
    tmpFunc("const bin2c_filesAttached_st bin2c_filesAttached[] = {\n");
}

/* Set the symbol name for next file */
static void setSymbolName(char *name)
{
    int i;
    
    if (symbolName) {
        printfError(symbolNameGiven);
        fprintf(stderr, tryHelp);
        panic();
    }

    /* Be sure that a symbol name is given */
    if (!name) {
        printfError(noSymbolName);
        fprintf(stderr, tryHelp);
        panic();
    }
    
    /* Check if the symbol is a valid C identifier */
    check((name[0] >= 'a' && name[0] <='z') ||
            (name[0] >= 'A' && name[0] <='Z') ||
            (name[0] == '_'), notValidId, name);
    
    for (i = 1; name[i]; ++i) {
        check((name[i] >= 'a' && name[i] <='z') ||
                (name[i] >= 'A' && name[i] <='Z') ||
                (name[i] >= '0' && name[i] <='9') ||
                (name[i] == '_'), notValidId, name);
    }
    
    /* Set the symbol name */
    symbolName = name;    
 }
 
  void concatenate(const char *dest, const char *src){
   int i,ch;
   FILE *fpdest=fopen(dest,"a");
   FILE *fpsrc=fopen(src,"r");
   while((ch=getc(fpsrc))!=EOF) putc(ch,fpdest);
 }
 
/* Main function */
int main(int argc, char **argv)
{
    int i;              /* Index to process commandline arguments */
    int ipf = 0;   /*total files processed*/
    
    /* Scan for a -h or --help option */
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            /* Print command line arguments help and exit immediately */
            printf("%s", commandLineHelp);
            return EXIT_SUCCESS;
        }
    }

    /* Scan for a -v or --version option */
    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
            /* Print version information and exit immediately */
            printf("%s", versionInfo);
            return EXIT_SUCCESS;
        }
    }

    /* Process command line arguments */
    for (i =  1; i < argc; ++i) {
        /* Check if it's an option or file name */
        if (argv[i][0] == '-') {
            /* Process the option */
            switch (argv[i][1]) {
                case '-':
                    /* Long options */
                    if (!strcmp(argv[i], "--output")) {
                        /* Name an output file */
                        openOutputFile(argv[i+1]);
                        openFuncFile(argv[i+1]);
                        ++i;
                    } else if (!strcmp(argv[i], "--header")) {
                        /* Name a header file */
                        openHeaderFile(argv[i+1]);
                        ++i;
                    } else if (!strcmp(argv[i], "--macro")) {
                        /* Create a macro for the size definition */
                        createMacro = true;
                    } else if (!strcmp(argv[i], "--name")) {
                        /* Name the symbol */
                        setSymbolName(argv[i+1]);
                        ++i;
                    } else {
                        /* Unknown option */
                        handleUnknownOption(argv[i]);
                    }
                    break;
                case 'o':
                    /* Name an output file */
                    openOutputFile(argv[i+1]);
                    openFuncFile(argv[i+1]);
                    ++i;
                    break;
                case 'd':
                    /* Name a header file */
                    openHeaderFile(argv[i+1]);
                    ++i;
                    break;
                case 'm':
                    /* Create a macro for the size definition */
                    createMacro = true;
                    break;
                case 'n':
                    /* Name the symbol */
                    setSymbolName(argv[i+1]);
                    ++i;
                    break;
                default:
                    /* Unknown option */
                    handleUnknownOption(argv[i]);
            }
        } else {
            /* Process the file */
            if(processFile(argv[i], 0, -1)) ipf++;
        }
    }

    /* Close any remaining output files */
    if (headerFile) {
	writeAttchmentStruct(&header);
	header("extern const bin2c_filesAttached_st bin2c_filesAttached[%d];\n", ipf);
	    
        header("\n#endif    /* __%s_included */\n", headerSymbol);
        free(headerSymbol);
        headerSymbol = 0;
        check(!fclose(headerFile), cantClose, headerFileName);
    }

    if (tmpFuncFile) {
	tmpFunc("};\n");
	check(!fclose(tmpFuncFile), cantClose, tmpFuncFileName);
    }

    if (outputFile) {
        check(!fclose(outputFile), cantClose, outputFileName);
    }
    
    if (tmpFuncFile && outputFile) {
	concatenate(outputFileName, tmpFuncFileName);
	remove(tmpFuncFileName);
    }

    return EXIT_SUCCESS;
}
