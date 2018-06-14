#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sal.h>

// Constants for the generated C file
const char PREFIX_STRING[] = "#include <stdio.h>\r\n#include <stdlib.h>\r\nint main()\r\n{\r\n\tchar data[300000];\r\n\tchar *p = &data[0];\r\n\tmemset(data, 0, 300000);\r\n";
const char SUFFIX_STRING[] = "\treturn 0;\r\n}\r\n";
const char INC_PTR_STRING[] = "++p;\r\n";
const char DEC_PTR_STRING[] = "--p;\r\n";
const char INC_PTRVAL_STRING[] = "++*p;\r\n";
const char DEC_PTRVAL_STRING[] = "--*p;\r\n";
const char OUTPUT_PTR_BYTE[] = "putchar(*p);\r\n";
const char READ_PTR_BYTE[] = "*p = getchar();\r\n";
const char LOOP_NONZERO[] = "while(*p) {\t\r\n";
const char END_LOOP[] = "}\r\n";

typedef unsigned char byte;

/**
 * Writes tabs to the output file based on the current indentation level.
 * PARAMETERS:
 *  indent_level - The number of tabs to insert
 *  fp - A valid, opened, file pointer opened with "wb" or "w"
 */
void inline writetabs(
	_In_range_(0, INT_MAX) int indent_level,
	_In_ FILE *fp
)
{
	int i;
	for(i = 0; i < indent_level; i++)
	{
		fputc('\t', fp);
	}
}

/**
 * Compiles a Brainfuck program into a C program
 * PARAMETERS:
 *  len - The length of "buffer" in bytes
 *  buffer - The contents of a brainfuck program
 *  outfile - The name of the output file to create
 * RETURNS:
 *  0 if successful, a value from errno.h if something 
 *  fails
 */
_Pre_satisfies_(buffer != NULL && wcslen(outfile) > 0 && len > 0)
_Success_(return == 0)
_Check_return_
errno_t __cdecl compile(
	_In_ size_t len,
	_In_reads_(len) byte *buffer,
	_In_z_ wchar_t *outfile
)
{
	FILE *fp;
	errno_t err;
	size_t i;
	int indent_level = 1;

	err = _wfopen_s(&fp, outfile, L"wb");
	if(err != 0)
	{
		fwprintf_s(stderr, L"Could not create output file, error: %d\n", err);
		return err;
	}

	fwrite(PREFIX_STRING, 1, 126, fp);

	for(i = 0; i < len; i++)
	{
		switch(buffer[i])
		{
			case '>':
				writetabs(indent_level, fp);
				fwrite(INC_PTR_STRING, 1, 6, fp);
				break;
			case '<':
				writetabs(indent_level, fp);
				fwrite(DEC_PTR_STRING, 1, 6, fp);
				break;
			case '+':
				writetabs(indent_level, fp);
				fwrite(INC_PTRVAL_STRING, 1, 7, fp);
				break;
			case '-':
				writetabs(indent_level, fp);
				fwrite(DEC_PTRVAL_STRING, 1, 7, fp);
				break;
			case '.':
				writetabs(indent_level, fp);
				fwrite(OUTPUT_PTR_BYTE, 1, 14, fp);
				break;
			case ',':
				writetabs(indent_level, fp);
				fwrite(READ_PTR_BYTE, 1, 17, fp);
				break;
			case '[':
				writetabs(indent_level, fp);
				indent_level++;
				fwrite(LOOP_NONZERO, 1, 14, fp);
				break;
			case ']':
				indent_level--;
				if(indent_level < 0)
				{
					fwprintf_s(stderr, L"Compilation error: mismatched \"]\" character\n");
					unlink(outfile);
					return ENOEXEC;
				}
				writetabs(indent_level, fp);
				fwrite(END_LOOP, 1, 3, fp);
				break;
				// So that non-brainfuck characters do not disrupt program compilation
			default:
				break;
		}
	}

	fwrite(SUFFIX_STRING, 1, 15, fp);
	fclose(fp);

	return 0;
}

/**
 * Entry point
 */
int __cdecl wmain(
	_In_ int argc,
	_In_reads_(argc) wchar_t *argv[]
)
{
	FILE *fp;
	size_t len;
	byte *buffer = NULL;
	errno_t err;

	if(argc < 3)
	{
		_putws(L"Usage: bfc input-file output-file");
		return 0;
	}

	err = _wfopen_s(&fp, argv[1], L"rb");
	if(err != 0 || fp == NULL)
	{
		_wperror(L"_wfopen_s");
		exit(EXIT_FAILURE);
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	buffer = malloc(len + 1);
	if(buffer == NULL)
	{
		_wperror(L"malloc");
		exit(EXIT_FAILURE);
	}
	fread(buffer, 1, len, fp);
	fclose(fp);

	err = compile(len, buffer, argv[2]);
	if(err != 0)
	{
		wprintf_s(L"Error: %d", err);
	}

	free(buffer);
	buffer = NULL;
	return 0;
}