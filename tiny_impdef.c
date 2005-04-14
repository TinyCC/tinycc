/* -------------------------------------------------------------- */
/*
	"tiny_impdef creates a .def file from a dll"

	"Usage: tiny_impdef [-p] <library.dll> [-o outputfile]"
	"Options:"
	" -p print to stdout"
*/

#include <windows.h>
#include <stdio.h>

/* Offset to PE file signature                              */
#define NTSIGNATURE(a) ((LPVOID)((BYTE *)a                +  \
						((PIMAGE_DOS_HEADER)a)->e_lfanew))

/* MS-OS header identifies the NT PEFile signature dword;
   the PEFILE header exists just after that dword.           */
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
						 ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
							 SIZE_OF_NT_SIGNATURE))

/* PE optional header is immediately after PEFile header.    */
#define OPTHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
						 ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
						   SIZE_OF_NT_SIGNATURE           +  \
						   sizeof (IMAGE_FILE_HEADER)))

/* Section headers are immediately after PE optional header. */
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a               +  \
						 ((PIMAGE_DOS_HEADER)a)->e_lfanew +  \
						   SIZE_OF_NT_SIGNATURE           +  \
						   sizeof (IMAGE_FILE_HEADER)     +  \
						   sizeof (IMAGE_OPTIONAL_HEADER)))


#define SIZE_OF_NT_SIGNATURE 4

/* -------------------------------------------------------------- */

int   WINAPI NumOfSections (
	LPVOID    lpFile)
{
	/* Number of sections is indicated in file header. */
	return (int)
		((PIMAGE_FILE_HEADER)
			PEFHDROFFSET(lpFile))->NumberOfSections;
}


/* -------------------------------------------------------------- */

LPVOID  WINAPI ImageDirectoryOffset (
		LPVOID    lpFile,
		DWORD     dwIMAGE_DIRECTORY)
{
	PIMAGE_OPTIONAL_HEADER   poh;
	PIMAGE_SECTION_HEADER    psh;
	int                      nSections = NumOfSections (lpFile);
	int                      i = 0;
	LPVOID                   VAImageDir;

	/* Retrieve offsets to optional and section headers. */
	poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
	psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile);

	/* Must be 0 thru (NumberOfRvaAndSizes-1). */
	if (dwIMAGE_DIRECTORY >= poh->NumberOfRvaAndSizes)
		return NULL;

	/* Locate image directory's relative virtual address. */
	VAImageDir = (LPVOID)poh->DataDirectory
					   [dwIMAGE_DIRECTORY].VirtualAddress;

	/* Locate section containing image directory. */
	while (i++<nSections)
		{
		if (psh->VirtualAddress <= (DWORD)VAImageDir
		 && psh->VirtualAddress + psh->SizeOfRawData > (DWORD)VAImageDir)
			break;
		psh++;
		}

	if (i > nSections)
		return NULL;

	/* Return image import directory offset. */
	return (LPVOID)(((int)lpFile +
					 (int)VAImageDir - psh->VirtualAddress) +
					(int)psh->PointerToRawData);
}

/* -------------------------------------------------------------- */

BOOL    WINAPI GetSectionHdrByName (
	LPVOID                   lpFile,
	IMAGE_SECTION_HEADER     *sh,
	char                     *szSection)
{
	PIMAGE_SECTION_HEADER    psh;
	int                      nSections = NumOfSections (lpFile);
	int                      i;

	if ((psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) !=
		 NULL)
		{
		/* find the section by name */
		for (i=0; i<nSections; i++)
			{
			if (!strcmp (psh->Name, szSection))
				{
				/* copy data to header */
				memcpy ((LPVOID)sh,
							(LPVOID)psh,
							sizeof (IMAGE_SECTION_HEADER));
				return TRUE;
				}
			else
				psh++;
			}
		}

	return FALSE;
}

/* -------------------------------------------------------------- */

BOOL    WINAPI GetSectionHdrByAddress (
	LPVOID                   lpFile,
	IMAGE_SECTION_HEADER     *sh,
	DWORD                    addr)
{
	PIMAGE_SECTION_HEADER    psh;
	int                      nSections = NumOfSections (lpFile);
	int                      i;

	if ((psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET (lpFile)) !=
		 NULL)
		{
		/* find the section by name */
		for (i=0; i<nSections; i++)
			{
			if (addr >= psh->VirtualAddress && addr < psh->VirtualAddress + psh->SizeOfRawData)
				{
				/* copy data to header */
				memcpy ((LPVOID)sh,
							(LPVOID)psh,
							sizeof (IMAGE_SECTION_HEADER));
				return TRUE;
				}
			else
				psh++;
			}
		}

	return FALSE;
}

/* -------------------------------------------------------------- */

int  WINAPI GetExportFunctionNames (
	LPVOID    lpFile,
	HANDLE    hHeap,
	char      **pszFunctions)
{
	IMAGE_SECTION_HEADER       sh;
	PIMAGE_EXPORT_DIRECTORY    ped;
	int                        *pNames, *pCnt;
	char *pSrc, *pDest;
	int                        i, nCnt;
	DWORD                      VAImageDir;
	PIMAGE_OPTIONAL_HEADER     poh;
	char *pOffset;

	/* Get section header and pointer to data directory
	   for .edata section. */
	if ((ped = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryOffset
			(lpFile, IMAGE_DIRECTORY_ENTRY_EXPORT)) == NULL)
		return 0;

	poh = (PIMAGE_OPTIONAL_HEADER)OPTHDROFFSET (lpFile);
	VAImageDir = poh->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

	if (FALSE == GetSectionHdrByAddress (lpFile, &sh, VAImageDir)) return 0;

	pOffset = (char *)lpFile + (sh.PointerToRawData -  sh.VirtualAddress);

	pNames = (int *)(pOffset + (DWORD)ped->AddressOfNames);

	/* Figure out how much memory to allocate for all strings. */
	nCnt = 1;
	for (i=0, pCnt = pNames; i<(int)ped->NumberOfNames; i++)
	{
	   pSrc = (pOffset + *pCnt++);
	   if (pSrc) nCnt += strlen(pSrc)+1;
	}

	/* Allocate memory off heap for function names. */
	pDest = *pszFunctions = HeapAlloc (hHeap, HEAP_ZERO_MEMORY, nCnt);

	/* Copy all strings to buffer. */
	for (i=0, pCnt = pNames; i<(int)ped->NumberOfNames; i++)
	{
	   pSrc = (pOffset + *pCnt++);
	   if (pSrc) { strcpy(pDest, pSrc); pDest += strlen(pSrc)+1; }
	}
	*pDest = 0;

	return ped->NumberOfNames;
}

/* -------------------------------------------------------------- */

int main(int argc, char **argv)
{

	HANDLE hHeap; HANDLE hFile; HANDLE hMapObject; VOID *pMem;
	int nCnt, ret, argind, std;
	char *pNames;
	char infile[MAX_PATH];
	char buffer[MAX_PATH];
	char outfile[MAX_PATH];
	char libname[80];

	hHeap = NULL;
	hFile = NULL;
	hMapObject = NULL;
	pMem = NULL;
	infile[0] = 0;
	outfile[0] = 0;
	ret = 0;
	std = 0;

	for (argind = 1; argind < argc; ++argind)
	{
		const char *a = argv[argind];
		if ('-' == a[0])
		{
			if (0 == strcmp(a, "-p"))
				std = 1;
			else
			if (0 == strcmp(a, "-o"))
			{
				if (++argind == argc) goto usage;
				strcpy(outfile, argv[argind]);
			}
			else
				goto usage;
		}
		else
		if (0 == infile[0])
			strcpy(infile, a);
		else
			goto usage;
	}

	if (0 == infile[0])
	{
usage:
		fprintf(stderr,
			"tiny_impdef creates a .def file from a dll\n"
			"Usage: tiny_impdef [-p] <library.dll> [-o outputfile]\n"
			"Options:\n"
			" -p print to stdout\n"
			);
error:
		ret = 1;
		goto the_end;
	}

	if (SearchPath(NULL, infile, ".dll", sizeof buffer, buffer, NULL))
		strcpy(infile, buffer);

	if (0 == outfile[0])
	{
		char *p;
		p = strrchr(strcpy(outfile, infile), '\\');
		if (NULL == p)
		p = strrchr(outfile, '/');
		if (p) strcpy(outfile, p+1);

		p = strrchr(outfile, '.');
		if (NULL == p) p = strchr(outfile, 0);
		strcpy(p, ".def");
	}

	hFile=CreateFile(
		infile,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
		);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "file not found: %s\n", infile);
		goto error;
	}

	if (!std) printf("--> %s\n", infile);

	hMapObject = CreateFileMapping(
		hFile,
		NULL,
		PAGE_READONLY,
		0, 0,
		NULL
		);

	if (NULL == hMapObject)
	{
		fprintf(stderr, "could not create file mapping.\n");
		goto error;
	}

	pMem = MapViewOfFile(
		hMapObject,     // object to map view of
		FILE_MAP_READ,  // read access
		0,              // high offset:  map from
		0,              // low offset:   beginning
		0);             // default: map entire file

	if (NULL == pMem)
	{
		fprintf(stderr, "could not map view of file.\n");
		goto error;
	}

	hHeap = GetProcessHeap();
	nCnt = GetExportFunctionNames(pMem, hHeap, &pNames);
	{
		FILE *op; char *p; int n;
		if (!std) printf("<-- %s\n", outfile);

		if (std)
			op = stdout;
		else
			op = fopen(outfile, "wt");

		if (NULL == op)
		{
			fprintf(stderr, "could not create file: %s\n", outfile);
			goto error;
		}

		p = strrchr(infile, '\\');
		if (NULL == p)
		p = strrchr(infile, '/');
		if (NULL == p) p = infile; else ++p;

		fprintf(op, "LIBRARY %s\n\nEXPORTS", p);
		if (std) fprintf(op, " (%d)", nCnt);
		fprintf(op, "\n");
		for (n = 0, p = pNames; n < nCnt; ++n)
		{
			fprintf(op, "%s\n", p);
			while (*p++);
		}
		if (!std) fclose(op);
	}

the_end:
	if (pMem) UnmapViewOfFile(pMem);
	if (hMapObject) CloseHandle(hMapObject);
	if (hFile) CloseHandle(hFile);
	return ret;
}
/* -------------------------------------------------------------- */

