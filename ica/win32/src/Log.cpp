// Log.cpp: implementation of the Log class.
//
//////////////////////////////////////////////////////////////////////

#include "stdhdrs.h"
#include "Log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#if (defined(_UNICODE) || defined(_MBCS))
#error Cannot compile with multibyte/wide character support
#endif

const int Log::ToDebug   =  1;
const int Log::ToFile    =  2;
const int Log::ToConsole =  4;

const static int LINE_BUFFER_SIZE = 1024;

Log::Log(int mode, int level, char *filename, bool append)
{
	m_lastLogTime = 0;
	m_filename = NULL;
	m_append = false;
    hlogfile = NULL;
    m_todebug = false;
    m_toconsole = false;
    m_tofile = false;

    SetFile(filename, append);
    SetMode(mode);
	SetLevel(level);

	// If the compiler returns full path names in __FILE__,
	// remember the path prefix, to remove it from the log messages.
	char *path = __FILE__;
	char *ptr = strrchr(path, '\\');
	if (ptr != NULL) {
		m_prefix_len = ptr + 1 - path;
		m_prefix = (char *)malloc(m_prefix_len + 1);
		memcpy(m_prefix, path, m_prefix_len);
		m_prefix[m_prefix_len] = '\0';
	}
}

void Log::SetMode(int mode) {
    
	m_mode = mode;

    if (mode & ToDebug)
        m_todebug = true;
    else
        m_todebug = false;

    if (mode & ToFile)  {
		if (!m_tofile)
			OpenFile();
	} else {
		CloseFile();
        m_tofile = false;
    }
    
    if (mode & ToConsole) {
        if (!m_toconsole)
            AllocConsole();
        m_toconsole = true;
    } else {
        m_toconsole = false;
    }
}

int Log::GetMode() {
	return m_mode;
}

void Log::SetLevel(int level) {
    m_level = level;
}

int Log::GetLevel() {
	return m_level;
}

void Log::SetStyle(int style) {
	m_style = style;
}

int Log::GetStyle() {
	return m_style;
}

void Log::SetFile(const char *filename, bool append)
{
	CloseFile();
	if (m_filename != NULL)
		free(m_filename);
	m_filename = strdup(filename);
	m_append = append;
	if (m_tofile)
		OpenFile();
}

void Log::OpenFile()
{
	// Is there a file-name?
	if (m_filename == NULL)
	{
        m_todebug = true;
        m_tofile = false;
        Print(0, "Error opening log file\n");
		return;
	}

    m_tofile  = true;
	m_lastLogTime = 0;

	// If there's an existing log and we're not appending then move it
	if (!m_append)
	{
		// Build the backup filename
		char *backupfilename = new char[strlen(m_filename)+5];
		if (backupfilename)
		{
			strcpy(backupfilename, m_filename);
			strcat(backupfilename, ".bak");
			// Attempt the move and replace any existing backup
			// Note that failure is silent - where would we log a message to? ;)
			DeleteFile(backupfilename);
			MoveFile(m_filename, backupfilename);
			delete [] backupfilename;
		}
	}

    // If filename is NULL or invalid we should throw an exception here
    hlogfile = CreateFile(m_filename,
						  GENERIC_WRITE,
						  FILE_SHARE_READ | FILE_SHARE_WRITE,
						  NULL,
						  OPEN_ALWAYS,
						  FILE_ATTRIBUTE_NORMAL,
						  NULL);

    if (hlogfile == INVALID_HANDLE_VALUE) {
        // We should throw an exception here
        m_todebug = true;
        m_tofile = false;
        Print(0, "Error opening log file %s\n", m_filename);
    }
    if (m_append) {
        SetFilePointer(hlogfile, 0, NULL, FILE_END);
    } else {
        SetEndOfFile(hlogfile);
    }
}

// if a log file is open, close it now.
void Log::CloseFile() {
    if (hlogfile != NULL) {
        CloseHandle(hlogfile);
        hlogfile = NULL;
    }
}

inline void Log::ReallyPrintLine(char *line) 
{
    if (m_todebug) OutputDebugString(line);
    if (m_toconsole) {
        DWORD byteswritten;
        WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), line, strlen(line), &byteswritten, NULL);
    }
    if (m_tofile && (hlogfile != NULL)) {
        DWORD byteswritten;
        WriteFile(hlogfile, line, strlen(line), &byteswritten, NULL); 
    }
}

void Log::ReallyPrint(char *format, va_list ap) 
{
	// Write current time to the log if necessary
	time_t current = time(NULL);
	if (current != m_lastLogTime) {
		m_lastLogTime = current;

		char time_str[32];
		strncpy(time_str, ctime(&m_lastLogTime), 24);
		if (m_style & TIME_INLINE) {
			strcpy(&time_str[24], " - ");
		} else {
			strcpy(&time_str[24], "\r\n");
		}
		ReallyPrintLine(time_str);
	}

	// Exclude path prefix from the format string if needed
	char *format_ptr = format;
	if (m_prefix != NULL && strlen(format) > m_prefix_len + 4) {
#ifndef _DEBUG
		if (memcmp(format, m_prefix, m_prefix_len) == 0)
			format_ptr = format + m_prefix_len;
#else
		if (_strnicmp(format, m_prefix, m_prefix_len) == 0)
			format_ptr = format + m_prefix_len;
#endif
	}

	// Prepare the complete log message
	char line[LINE_BUFFER_SIZE];
	_vsnprintf(line, LINE_BUFFER_SIZE - 2, format_ptr, ap);
	line[LINE_BUFFER_SIZE - 2] = '\0';
	int len = strlen(line);
	if (len > 0 && len <= LINE_BUFFER_SIZE - 2 && line[len - 1] == '\n') {
		// Replace trailing '\n' with MS-DOS style end-of-line.
		line[len-1] = '\r';
		line[len] =   '\n';
		line[len+1] = '\0';
	}
	if (m_style & (NO_FILE_NAMES | NO_TAB_SEPARATOR)) {
		char *ptr = strchr(line, '\t');
		if (ptr != NULL) {
			// Print without file names if desired
			if (m_style & NO_FILE_NAMES) {
				ReallyPrintLine(ptr + 1);
				return;
			} else if (m_style & NO_TAB_SEPARATOR) {
				*ptr = ' ';
			}
		}
	}
	ReallyPrintLine(line);
}

Log::~Log()
{
	if (m_filename != NULL)
		free(m_filename);
	if (m_prefix != NULL)
		free(m_prefix);
    CloseFile();
}
