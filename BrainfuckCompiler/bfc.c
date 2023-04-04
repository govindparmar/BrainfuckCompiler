#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>
#include <sal.h>

// Constants for the generated C file
const char PREFIX_STRING[] = "#include <stdio.h>\r\n#include <stdlib.h>\r\nint main()\r\n{\r\n\tchar data[300000];\r\n\tchar *p = &data[0];\r\n\tmemset(data, 0, 300000);\r\n";
const char SUFFIX_STRING[] = "\treturn 0;\r\n}\r\n";
const char CHG_PTR_STRING[] = "p+=%d;\r\n";
const char INC_PTR_STRING[] = "++p;\r\n";
const char DEC_PTR_STRING[] = "--p;\r\n";
const char CHG_PTRVAL_STRING[] = "*p+=%d;\r\n";
const char INC_PTRVAL_STRING[] = "++*p;\r\n";
const char DEC_PTRVAL_STRING[] = "--*p;\r\n";
const char OUTPUT_PTR_BYTE[] = "putchar(*p);\r\n";
const char READ_PTR_BYTE[] = "*p = getchar();\r\n";
const char LOOP_NONZERO[] = "while(*p) {\t\r\n";
const char END_LOOP[] = "}\r\n";

typedef unsigned char byte;

/**
 * Returns the total change in the value of the pointer address
 * from a string of '>' and '<' characters in a brainfuck program.
 * PARAMETERS:
 *  pos - Pointer to the current index into the program text
 *  len - The length, in bytes, of the buffer containing program text
 *  buffer - The buffer containing the program text
 * RETURNS:
 *  The net change in the pointer address after the < and > string.
 */
_Pre_satisfies_(*pos >= 0 && len > 0 && buffer != NULL)
_Check_return_
int __cdecl getpointerdelta(
	_Inout_ size_t *pos,
	_In_ size_t len,
	_In_reads_(len) byte *buffer
)
{
	int delta = 0;
	while (*pos <= len)
	{
		switch (buffer[*pos])
		{
		case '>':
			delta++;
			break;
		case '<':
			delta--;
			break;
		default:
			// So that the current character that breaks the <> chain is not skipped
			(*pos)--;
			return delta;
		}
		(*pos)++;
	}
	return delta;
}

/**
 * Returns the total change in the value of the pointed-to cell
 * from a string of '+' and '-' characters in a brainfuck program.
 * PARAMETERS:
 *  pos - Pointer to the current index into the program text
 *  len - The length, in bytes, of the buffer containing program text
 *  buffer - The buffer containing the program text
 * RETURNS:
 *  The net change in the pointed-to value after the + and - string.
 */
_Pre_satisfies_(*pos >= 0 && len > 0 && buffer != NULL)
_Check_return_
int __cdecl getvaluedelta(
	_Inout_ size_t *pos,
	_In_ size_t len,
	_In_reads_(len) byte *buffer
)
{
	int delta = 0;
	while (*pos <= len)
	{
		switch (buffer[*pos])
		{
		case '+':
			delta++;
			break;
		case '-':
			delta--;
			break;
		default:
			// So the current character is processed by the appropriate handler rather than skipped
			(*pos)--;
			return delta;
		}
		(*pos)++;
	}
	return delta;
}

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
	if(NULL == fp || err != 0)
	{
		fwprintf_s(stderr, L"Could not create output file, error: %d\n", err);
		return err;
	}

	fwrite(PREFIX_STRING, 1, 126, fp);

	for(i = 0; i < len; i++)
	{
		int delta;
		switch(buffer[i])
		{		
		case '>':
		case '<':
			writetabs(indent_level, fp);
			delta = getpointerdelta(&i, len, buffer);
			switch (delta)
			{
			case 1:
				fwrite(INC_PTR_STRING, 1, 6, fp);
				break;
			case -1:
				fwrite(DEC_PTR_STRING, 1, 6, fp);
				break;
			default:
				fprintf(fp, CHG_PTR_STRING, delta);
				break;
			}
			break;
		case '+':
		case '-':
				writetabs(indent_level, fp);
				delta = getvaluedelta(&i, len, buffer);
				switch (delta)
				{
				case 1:
					fwrite(INC_PTRVAL_STRING, 1, 7, fp);
					break;
				case -1:
					fwrite(DEC_PTRVAL_STRING, 1, 7, fp);
					break;
				default:
					fprintf(fp, CHG_PTRVAL_STRING, delta);
					break;
				}

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
	if(NULL == fp || err != 0)
	{
		_wperror(L"_wfopen_s");
		exit(err);
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	rewind(fp);

	buffer = malloc(len + 1);
	if(NULL == buffer)
	{
		_wperror(L"malloc");
		exit(ENOMEM);
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

