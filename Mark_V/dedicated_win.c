#ifndef CORE_SDL
#include "environment.h"
#ifdef PLATFORM_WINDOWS // Has to be here, set by a header

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2009-2014 Baker and others

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// dedicated_win.c

#include "quakedef.h"
#include "winquake.h"
#include "dedicated_win.h"

HANDLE	heventDone;
HANDLE	hfileBuffer;
HANDLE	heventChildSend;
HANDLE	heventParentSend;
HANDLE	hStdout;
HANDLE	hStdin;


void InitConProc (HANDLE hFile, HANDLE heventParent, HANDLE heventChild)
{
	DWORD	dwID;

// ignore if we don't have all the events.
	if (!hFile || !heventParent || !heventChild)
		return;

	hfileBuffer = hFile;
	heventParentSend = heventParent;
	heventChildSend = heventChild;

// so we'll know when to go away.
	heventDone = CreateEvent (NULL, FALSE, FALSE, NULL);

	if (!heventDone)
	{
		Con_SafePrintLinef ("Couldn't create heventDone");
		return;
	}

	if (!CreateThread (NULL,
					   0,
					   (LPTHREAD_START_ROUTINE) RequestProc,
					   0,
					   0,
					   &dwID))
	{
		CloseHandle (heventDone);
		Con_SafePrintLinef ("Couldn't create QHOST thread");
		return;
	}

// save off the input/output handles.
	hStdout = GetStdHandle (STD_OUTPUT_HANDLE);
	hStdin = GetStdHandle (STD_INPUT_HANDLE);

// force 80 character width, at least 25 character height
	SetConsoleCXCY (hStdout, 80, 25);
}


void DeinitConProc (void)
{
	if (heventDone)
		SetEvent (heventDone);
}


DWORD RequestProc (DWORD dwNichts)
{
	int		*pBuffer;
	DWORD	dwRet;
	HANDLE	heventWait[2];
	int		iBeginLine, iEndLine;

	heventWait[0] = heventParentSend;
	heventWait[1] = heventDone;

	while (1)
	{
		dwRet = WaitForMultipleObjects (2, heventWait, FALSE, INFINITE);

	// heventDone fired, so we're exiting.
		if (dwRet == WAIT_OBJECT_0 + 1)
			break;

		pBuffer = (int *) GetMappedBuffer (hfileBuffer);

	// hfileBuffer is invalid.  Just leave.
		if (!pBuffer)
		{
			Con_SafePrintLinef ("Invalid hfileBuffer");
			break;
		}

		switch (pBuffer[0])
		{
			case CCOM_WRITE_TEXT:
			// Param1 : Text
				pBuffer[0] = WriteText ((LPCTSTR) (pBuffer + 1));
				break;

			case CCOM_GET_TEXT:
			// Param1 : Begin line
			// Param2 : End line
				iBeginLine = pBuffer[1];
				iEndLine = pBuffer[2];
				pBuffer[0] = ReadText ((LPTSTR) (pBuffer + 1), iBeginLine, iEndLine);
				break;

			case CCOM_GET_SCR_LINES:
			// No params
				pBuffer[0] = GetScreenBufferLines (&pBuffer[1]);
				break;

			case CCOM_SET_SCR_LINES:
			// Param1 : Number of lines
				pBuffer[0] = SetScreenBufferLines (pBuffer[1]);
				break;
		}

		ReleaseMappedBuffer (pBuffer);
		SetEvent (heventChildSend);
	}

	return 0;
}


LPVOID GetMappedBuffer (HANDLE hfileBuffer)
{
	LPVOID pBuffer;

	pBuffer = MapViewOfFile (hfileBuffer, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	return pBuffer;
}


void ReleaseMappedBuffer (LPVOID pBuffer)
{
	UnmapViewOfFile (pBuffer);
}


BOOL GetScreenBufferLines (int *piLines)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;
	BOOL						bRet;

	bRet = GetConsoleScreenBufferInfo (hStdout, &info);

	if (bRet)
		*piLines = info.dwSize.Y;

	return bRet;
}


BOOL SetScreenBufferLines (int iLines)
{

	return SetConsoleCXCY (hStdout, 80, iLines);
}


BOOL ReadText (LPTSTR pszText, int iBeginLine, int iEndLine)
{
	COORD	coord;
	DWORD	dwRead;
	BOOL	bRet;

	coord.X = 0;
	coord.Y = iBeginLine;

	bRet = ReadConsoleOutputCharacter(
		hStdout,
		pszText,
		80 * (iEndLine - iBeginLine + 1),
		coord,
		&dwRead);

	// Make sure it's null terminated.
	if (bRet)
		pszText[dwRead] = '\0';

	return bRet;
}


BOOL WriteText (LPCTSTR szText)
{
	DWORD			dwWritten;
	INPUT_RECORD	rec;
	char			upper, *sz;

	sz = (LPTSTR) szText;

	while (*sz)
	{
	// 13 is the code for a carriage return (\n) instead of 10.
		if (*sz == 10)
			*sz = 13;

		upper = toupper(*sz);

		rec.EventType = KEY_EVENT;
		rec.Event.KeyEvent.bKeyDown = TRUE;
		rec.Event.KeyEvent.wRepeatCount = 1;
		rec.Event.KeyEvent.wVirtualKeyCode = upper;
		rec.Event.KeyEvent.wVirtualScanCode = CharToCode (*sz);
		rec.Event.KeyEvent.uChar.AsciiChar = *sz;
		rec.Event.KeyEvent.uChar.UnicodeChar = *sz;
		rec.Event.KeyEvent.dwControlKeyState = isupper(*sz) ? 0x80 : 0x0;

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		rec.Event.KeyEvent.bKeyDown = FALSE;

		WriteConsoleInput(
			hStdin,
			&rec,
			1,
			&dwWritten);

		sz++;
	}

	return TRUE;
}


int CharToCode (char c)
{
	char upper;

	upper = toupper(c);

	switch (c)
	{
		case 13:
			return 28;

		default:
			break;
	}

	if (isalpha(c))
		return (30 + upper - ASCII_A_65);

	if (isdigit(c))
		return (1 + upper - 47); // Baker: Change to 2 + upper - ASCII_ZERO_CHAR_48?

	return c;
}


BOOL SetConsoleCXCY(HANDLE hStdout, int cx, int cy)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;
	COORD						coordMax;

	coordMax = GetLargestConsoleWindowSize(hStdout);

	if (cy > coordMax.Y)
		cy = coordMax.Y;

	if (cx > coordMax.X)
		cx = coordMax.X;

	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;

// height
    info.srWindow.Left = 0;
    info.srWindow.Right = info.dwSize.X - 1;
    info.srWindow.Top = 0;
    info.srWindow.Bottom = cy - 1;

	if (cy < info.dwSize.Y)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;

		info.dwSize.Y = cy;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
    }
    else if (cy > info.dwSize.Y)
    {
		info.dwSize.Y = cy;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;

		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
    }

	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;

// width
	info.srWindow.Left = 0;
	info.srWindow.Right = cx - 1;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = info.dwSize.Y - 1;

	if (cx < info.dwSize.X)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;

		info.dwSize.X = cx;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
	}
	else if (cx > info.dwSize.X)
	{
		info.dwSize.X = cx;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;

		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
	}

	return TRUE;
}

void Dedicated_Init (void)
{
	int t;

	if (!AllocConsole ())
	{
		System_Error ("Couldn't create dedicated server console");
	}

	sysplat.hinput = GetStdHandle (STD_INPUT_HANDLE);
	sysplat.houtput = GetStdHandle (STD_OUTPUT_HANDLE);

// give QHOST a chance to hook into the console
	if ((t = COM_CheckParm ("-HFILE")) > 0)
	{
		if (t < com_argc)
			sysplat.hFile = (HANDLE)atoi (com_argv[t+1]);
	}

	if ((t = COM_CheckParm ("-HPARENT")) > 0)
	{
		if (t < com_argc)
			sysplat.heventParent = (HANDLE)atoi (com_argv[t+1]);
	}

	if ((t = COM_CheckParm ("-HCHILD")) > 0)
	{
		if (t < com_argc)
			sysplat.heventChild = (HANDLE)atoi (com_argv[t+1]);
	}

	InitConProc (sysplat.hFile, sysplat.heventParent, sysplat.heventChild);
}


const char *Dedicated_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD	numread, numevents, dummy;

	if (!isDedicated)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (sysplat.hinput, &numevents))
			System_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(sysplat.hinput, recs, 1, &numread))
			System_Error ("Error reading console input");

		if (numread != 1)
			System_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(sysplat.houtput, "\r\n", 2, &dummy, NULL);

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						else if (sysplat.sc_return_on_enter)
						{
						// special case to allow exiting from the error handler on Enter
							text[0] = '\r';
							len = 0;
							return text;
						}

						break;

					case '\b':
						WriteFile(sysplat.houtput, "\b \b", 3, &dummy, NULL);
						if (len)
						{
							len--;
						}
						break;

					default:
						if (ch >= ' ')
						{
							WriteFile(sysplat.houtput, &ch, 1, &dummy, NULL);
							text[len] = ch;
							len = (len + 1) & 0xff;
						}

						break;

				}
			}
		}
	}

	return NULL;
}

void Dedicated_Local_Print (const char *text)
{
	DWORD		dummy;
	WriteFile (sysplat.houtput, text, strlen (text), &dummy, NULL);
}

#define CONSOLE_ERROR_TIMEOUT	60.0	// # of seconds to wait on System_Error running
										//  dedicated before exiting

void ConProc_Error (const char *text)
{
	char		text2[SYSTEM_STRING_SIZE_1024];
	char		*text3 = "Press Enter to exit\n";
	char		*text4 = "***********************************\n";
	char		*text5 = "\n";
	double		starttime;
	DWORD		dummy;

	c_snprintf1 (text2, "ERROR: %s\n", text);
	WriteFile (sysplat.houtput, text5, strlen (text5), &dummy, NULL);
	WriteFile (sysplat.houtput, text4, strlen (text4), &dummy, NULL);
	WriteFile (sysplat.houtput, text2, strlen (text2), &dummy, NULL);
	WriteFile (sysplat.houtput, text3, strlen (text3), &dummy, NULL);
	WriteFile (sysplat.houtput, text4, strlen (text4), &dummy, NULL);

	starttime = System_DoubleTime ();
	sysplat.sc_return_on_enter = true;	// so Enter will get us out of here

	while (!Dedicated_ConsoleInput () &&
			((System_DoubleTime () - starttime) < CONSOLE_ERROR_TIMEOUT))
	{
	}
}



#endif // PLATFORM_WINDOWS

#endif // !CORE_SDL