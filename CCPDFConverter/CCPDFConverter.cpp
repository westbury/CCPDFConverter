/**
@file
@brief Main functions file for CCPDFConverter/XL2PDFConverter application
*/

/*
* CC PDF Converter: Windows PDF Printer with Creative Commons license support
* Excel to PDF Converter: Excel PDF printing addin, keeping hyperlinks AND Creative Commons license support
* Copyright (C) 2007-2010 Guy Hachlili <hguy@cogniview.com>, Cogniview LTD.
* 
* This file is part of CC PDF Converter / Excel to PDF Converter
* 
* CC PDF Converter and Excel to PDF Converter are free software;
* you can redistribute them and/or modify them under the terms of the 
* GNU General Public License as published by the Free Software Foundation;
* either version 2 of the License, or (at your option) any later version.
* 
* CC PDF Converter and Excel to PDF Converter are is distributed in the hope 
* that they will be useful, but WITHOUT ANY WARRANTY; without even the implied 
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>. * 
*/

#include "stdafx.h"

#include "iapi.h"
#include <shellapi.h>
#include <errno.h>
#include <stdio.h>
#include "Helpers.h"
#include <io.h>
#include "resource.h"
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>

#include "Userenv.h"
#pragma comment(lib, "userenv.lib")
#include "crtdbg.h"

#ifdef CC_PDF_CONVERTER
#define PRODUCT_NAME	"CC PDF Converter"
#elif EXCEL_TO_PDF
#define PRODUCT_NAME	"Excel to PDF Converter"
#else
#error "One of the printer types must be defined"
#endif

#ifdef _DEBUG
/// Debugging file pointer
FILE* pSave = NULL;
#endif
/// Input file pointer
FILE* fileInput;
/// Initial input buffer
char cBuffer[MAX_PATH * 2 + 1];
/// Length of data in initial buffer
int nBuffer = 0;
/// Current location in the initial buffer
int nInBuffer = 0;
/// Size of error string buffer
#define MAX_ERR		1023
/// Error string buffer
char cErr[MAX_ERR + 1];

// File to write
TCHAR docName[MAX_PATH];
// Path to root directory
TCHAR path[MAX_PATH];
// Path to selected first level directory
TCHAR path1[MAX_PATH];
// Path to selected second level directory
TCHAR path2[MAX_PATH];
// File to write
TCHAR fullFileName[MAX_PATH];
// TRUE of user pressed OK, not sure if we really need this.
boolean okPressed;

#ifdef _DEBUG


#include <iostream>
#include <map>
#include <string>

namespace configuration
  {

  //---------------------------------------------------------------------------
  // The configuration::data is a simple map string (key, value) pairs.
  // The file is stored as a simple listing of those pairs, one per line.
  // The key is separated from the value by an equal sign '='.
  // Commentary begins with the first non-space character on the line a hash or
  // semi-colon ('#' or ';').
  //
  // Example:
  //   # This is an example
  //   source.directory = C:\Documents and Settings\Jennifer\My Documents\
  //   file.types = *.jpg;*.gif;*.png;*.pix;*.tif;*.bmp
  //
  // Notice that the configuration file format does not permit values to span
  // more than one line, commentary at the end of a line, or [section]s.
  //   
  struct data: std::map <std::string, std::string>
    {
    // Here is a little convenience method...
    bool iskey( const std::string& s ) const
      {
      return count( s ) != 0;
      }
    };

  //---------------------------------------------------------------------------
  // The extraction operator reads configuration::data until EOF.
  // Invalid data is ignored.
  //
  std::istream& operator >> ( std::istream& ins, data& d )
    {
    std::string s, key, value;

    // For each (key, value) pair in the file
    while (std::getline( ins, s ))
      {
      std::string::size_type begin = s.find_first_not_of( " \f\t\v" );

      // Skip blank lines
      if (begin == std::string::npos) continue;

      // Skip commentary
      if (std::string( "#;" ).find( s[ begin ] ) != std::string::npos) continue;

      // Extract the key value
      std::string::size_type end = s.find( '=', begin );
      key = s.substr( begin, end - begin );

      // (No leading or trailing whitespace allowed)
      key.erase( key.find_last_not_of( " \f\t\v" ) + 1 );

      // No blank keys allowed
      if (key.empty()) continue;

      // Extract the value (no leading or trailing whitespace allowed)
      begin = s.find_first_not_of( " \f\n\r\t\v", end + 1 );
	  if (begin == 0xFFFFFFFF) {
		  value = "";
	  } else {
		end   = s.find_last_not_of(  " \f\n\r\t\v" ) + 1;
		value = s.substr( begin, end - begin );
	  }
      // Insert the properly extracted (key, value) pair into the map
      d[ key ] = value;
      }

    return ins;
    }


//---------------------------------------------------------------------------
  // The insertion operator writes all configuration::data to stream.
  //
  std::ostream& operator << ( std::ostream& outs, const data& d )
    {
    data::const_iterator iter;
    for (iter = d.begin(); iter != d.end(); iter++)
      outs << iter->first << " = " << iter->second << '\n';
    return outs;
    }
}

	// Read-only properties
  	  configuration::data myconfigdata;

	  // Writable properties
  	  configuration::data myconfigdata2;


/**
@brief This function outputs an error via OutputDebugStringn
@param pBefore Text to add before the error
@param buf Error description
@param len Size of error description
*/
static void WriteOutput(const char* pBefore, const char* buf, int len)
{
	// Calculate and create a large enough buffer for the error descriptionn
	int n = len + strlen(pBefore);
	char* pStr = new char[n + 1];
	// Fill it
	sprintf_s(pStr, n + 1, "%s%.*s", pBefore, len, buf);
	// Send it
	::OutputDebugString(pStr);
	// Cleanup
	delete [] pStr;
}
#endif

//////////////////////////////////////////////////////////////////////////
/**
@param pPath Path to test
@return true if a folder (not file) exists in the path
*/
bool ExistsAsFolder(LPCTSTR pPath)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!::GetFileAttributesEx(pPath, GetFileExInfoStandard, &data))
		return false;
	if (data.dwFileAttributes & (FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_TEMPORARY))
		return false;

	return ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}
//////////////////////////////////////////////////////////////////////////

#define TEMP_FILENAME "ccprint_"
#define TEMP_EXTENSION "pdf"

void CleanTempFiles ()
{
	char sTempFolder[MAX_PATH];
	GetTempPath(MAX_PATH, sTempFolder);

	char sFileToFind[MAX_PATH];
	sprintf_s (sFileToFind, "%s%s*.%s", sTempFolder, TEMP_FILENAME, TEMP_EXTENSION);

	struct _finddata64i32_t finddata;
	intptr_t hFind = _findfirst (sFileToFind, &finddata);
	int ret = (int)hFind;
	char sFullFilename[MAX_PATH];

	while (ret != -1) {
		sprintf_s (sFullFilename, "%s%s", sTempFolder, finddata.name);
		DeleteFile (sFullFilename);						// Deliberately ignore the return value
		ret = _findnext (hFind, &finddata);				// Find the next file
	}

	_findclose (hFind);
}


//////////////////////////////////////////////////////////////////////////

/**
@brief Callback function used by GhostScript to retrieve more data from the input buffer; stops at newlines
@param instance Pointer to the GhostScript instance (not used)
@param buf Buffer to fill with data
@param len Length of requested data
@return Size of retrieved data (in bytes), 0 when there's no more data
*/
static int GSDLLCALL my_in(void *instance, char *buf, int len)
{
	// Initialize variables
	int ch;
	int count = 0;
	char* pStart = buf;
	// Read until we reached the wanted size...
	while (count < len) 
	{
		// Is there still data in the initial buffer?
		if (nBuffer > nInBuffer)
			// Yes, read from there
			ch = cBuffer[nInBuffer++];
		else
			// No, get more data
			ch = fgetc(fileInput);
		if (ch == EOF)
			// That's it
			return 0;
		// Put the character in the buffer and increate the countn
		*buf++ = ch;
		count++;
		if (ch == '\n')
			// Stop on newlines
			break;
	}
#ifdef _DEBUG
	// Leave a trace of the data (debug mode)
	WriteOutput("", pStart, count);
	if (pSave != NULL)
	{
		// Also save the data into the save file (debug mode)
		fwrite(pStart, 1, count, pSave);
	}
#endif
	// That's it
	return count;
}

/**
@brief Callback function used by GhostScript to output notes and warnings
@param instance Pointer to the GhostScript instance (not used)
@param str String to output
@param len Length of output
@return Count of characters written
*/
static int GSDLLCALL my_out(void *instance, const char *str, int len)
{
#ifdef _DEBUG
	// Write to stdout (debug mode)
	fwrite(str, 1, len, stdout);
	fflush(stdout);
	// Trace also (debug mode)
	WriteOutput("OUT: ", str, len);
#endif

	// That's it
	return len;
}

/**
@brief Callback function used by GhostScript to output errors
@param instance Pointer to the GhostScript instance (not used)
@param str Error string
@param len Length of string
@return Count of characters written
*/
static int GSDLLCALL my_err(void *instance, const char *str, int len)
{
#ifdef _DEBUG
	// Write to stderr (debug mode)
	fwrite(str, 1, len, stderr);
	fflush(stderr);
	// Trace too (debug mode)
	WriteOutput("ERR: ", str, len);
#endif
	// Keep the error in cErr for later handling
	int nAdd = min(len, (int)(MAX_ERR - strlen(cErr)));
	strncat_s(cErr, str, MAX_ERR);
	// OK
	return len;
}

/**
Reads all the data from the input (so no error will be raised if application
ends without sending the data to ghostscript)
*/
void CleanInput()
{
	char cBuffer[1024];
	while (fread(cBuffer, 1, 1024, fileInput) > 0)
		;
}

/**
@brief This function will center the window on the screen
@param hWnd The window to center
*/
void CenterWindow(HWND hWnd)
{
	// get coordinates of the window relative to the screen
	RECT rcWnd;
	::GetWindowRect(hWnd, &rcWnd);
	RECT rcCenter, rcArea;
	// center within screen coordinates
	::SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
	rcCenter = rcArea;

	int WndWidth = rcWnd.right - rcWnd.left;
	int WndHeight = rcWnd.bottom - rcWnd.top;

	// find dialog's upper left based on rcCenter
	int xLeft = (rcCenter.left + rcCenter.right) / 2 - WndWidth / 2;
	int yTop = (rcCenter.top + rcCenter.bottom) / 2 - WndHeight / 2;

	// if the dialog is outside the screen, move it inside
	if (xLeft < rcArea.left)
		xLeft = rcArea.left;
	else if(xLeft + WndWidth > rcArea.right)
		xLeft = rcArea.right - WndWidth;

	if(yTop < rcArea.top)
		yTop = rcArea.top;
	else if(yTop + WndHeight > rcArea.bottom)
		yTop = rcArea.bottom - WndHeight;

	// map screen coordinates to child coordinates
	::SetWindowPos(hWnd, NULL, xLeft, yTop, -1, -1, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
@param hDlg Handle of the dialog
@param uMsg ID of the message
@param wParam First message paramenter
@param lParam Second message paramenter
@return TRUE if the message was handled, FALSE otherwise
*/
UINT_PTR CALLBACK SaveDlgCallback(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Which message?
	switch (uMsg)
	{
	case WM_NOTIFY:
		// Notification
		{
			LPNMHDR pNotify = (LPNMHDR)lParam;
			if (pNotify->code == CDN_INITDONE)
			{
				// Initial display: center and bring to top
				HWND hParent = ::GetParent(hDlg);
				::BringWindowToTop(hParent);
				SetForegroundWindow(hParent);
				CenterWindow(hParent);
			}
		}
		break;
	}
	return FALSE;
}

/// Command line options used by GhostScript
// Option at index 5 is replaced with actual file before use.
const char* ARGS[] =
{
	"PS2PDF",
	"-dNOPAUSE",
	"-dBATCH",
	"-dSAFER",
	"-sDEVICE=pdfwrite",
	"-sOutputFile=c:\\test.pdf",
	"-I.\\",
	"-c",
	".setpdfwrite",
	"-"
};

void combine(TCHAR* destination, const TCHAR* pathPart1, const TCHAR* pathPart2)
{
	if(pathPart1 == NULL && pathPart2 == NULL) {
		strcpy(destination, "");;
	}
	else if(pathPart2 == NULL || strlen(pathPart2) == 0) {
		strcpy(destination, pathPart1);
	}
	else if(pathPart1 == NULL || strlen(pathPart1) == 0) {
		strcpy(destination, pathPart2);
	} 
	else {
		char directory_separator[] = "/";
#ifdef WIN32
		directory_separator[0] = '\\';
#endif
		const char *last_char = pathPart1;
		while(*last_char != '\0')
			last_char++;        
		int append_directory_separator = 0;
		if(strcmp(last_char, directory_separator) != 0) {
			append_directory_separator = 1;
		}
		strcpy(destination, pathPart1);
		if(append_directory_separator)
			strcat(destination, directory_separator);
		strcat(destination, pathPart2);
	}
}

void FillChildDirectories(HWND LIST, TCHAR * directoryPath) {
	TCHAR filePattern[MAX_PATH] = { 0 };
	WIN32_FIND_DATA FindFileData;

	combine(filePattern, directoryPath, "*");

	HANDLE hFind = FindFirstFile(filePattern, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf ("FindFirstFile failed (%d)\n", GetLastError());
		return; //????
	} 
	else 
	{
		// do this later  FindClose(hFind);
	}

	int Index = 0;
	BOOL result;
	do {

		if (_tcscmp(FindFileData.cFileName, _T("..")) != 0 
			&& _tcscmp(FindFileData.cFileName, _T(".")) != 0 
			&& FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{

			SendMessage(LIST, LB_INSERTSTRING, Index++, (LPARAM)FindFileData.cFileName);
		}
		result = FindNextFile(hFind, &FindFileData);
	} while (result);

	// Select if only one
	if (Index == 1) {

	}
}

boolean GetSelectedText(HWND control, TCHAR * text) {
	// Fill the secondary list box with directories
	// in the selected first-level directory.

	// get the number of items in the box.
	int count = SendMessage(control, LB_GETCOUNT, 0, 0);

	int iSelected = -1;

	// go through the items and find the first selected one
	for (int i = 0; i < count; i++)
	{
		// check if this item is selected or not..
		if (SendMessage(control, LB_GETSEL, i, 0) > 0)
		{
			// yes, we only want the first selected so break.
			iSelected = i;
			break;
		}
	}

	if (iSelected != -1) {
		//	 int length = SendMessage(control, LB_GETTEXTLEN, (WPARAM)iSelected, 0);
		//      TCHAR * text = new TCHAR[length + 1];

		SendMessage(control, LB_GETTEXT, (WPARAM)iSelected , (LPARAM)text);
		return true;
	} else {
		return false;
	}
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_LIST1:
			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				HWND control = GetDlgItem(hDlg, IDC_LIST1);
				TCHAR * text = new TCHAR[MAX_PATH + 1];
				boolean isAnythingSelected = GetSelectedText(control, text);

				// get the text of the selected item
				if (isAnythingSelected) {
					// Get list of directories in selected directory.

					combine(path1, path, text);

					HWND List2 = GetDlgItem(hDlg, IDC_LIST2);
					FillChildDirectories(List2, path1);

					// Save selected level 1 directory as 'last used'
					myconfigdata2["recent1"] = text;

				}

				return (INT_PTR)TRUE; 
			}
			break; 

		case IDC_LIST2:
			if(HIWORD(wParam) == LBN_SELCHANGE)
			{
				HWND control = GetDlgItem(hDlg, IDC_LIST2);
				TCHAR * text = new TCHAR[MAX_PATH + 1];
				boolean isAnythingSelected = GetSelectedText(control, text);

				// get the text of the selected item
				if (isAnythingSelected) {
					combine(path2, path1, text);

					// Save selected level 2 directory as 'last used'
					myconfigdata2["recent2"] = text;

					HWND OK = GetDlgItem(hDlg, IDOK);
					EnableWindow(OK, true);
				} else {
					HWND OK = GetDlgItem(hDlg, IDOK);
					EnableWindow(OK, false);
				}
				return (INT_PTR)TRUE; 
			}
			break;

		case IDOK:
			{
					TCHAR * text = new TCHAR[MAX_PATH];
					HWND docNameControl = GetDlgItem(hDlg, IDC_EDIT_DOC_NAME);
					GetWindowText(docNameControl, text, MAX_PATH);
					combine(fullFileName, path2, text);
					strcat(fullFileName, ".pdf");
					okPressed = TRUE;
			}

			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		

		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			return TRUE;
		}
		break;
		
	case WM_CLOSE:
		DestroyWindow(hDlg);
		return TRUE;
		
	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

/*
void GetUserHomeDir(TCHAR* szHomeDirBuf)
{
	HANDLE hToken = 0;
	boolean result = OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken );
	_ASSERT(result);

	DWORD BufSize = MAX_PATH;
	result = GetUserProfileDirectory( hToken, szHomeDirBuf, &BufSize );

	boolean result = GetAllUsersProfileDirectory( szHomeDirBuf, &BufSize );
	_ASSERT(result);

	CloseHandle( hToken );
}
*/

void GetPublicDocsDir(TCHAR* szHomeDirBuf)
{
	DWORD BufSize = MAX_PATH;
    HRESULT result = SHGetFolderPath(NULL, CSIDL_COMMON_DOCUMENTS, NULL, SHGFP_TYPE_CURRENT, szHomeDirBuf);

	char * x = strstr(szHomeDirBuf, "\\Documents");
	*x = 0;

    if (result != S_OK)
        std::cout << "Error: " << result << "\n";
    else
        std::cout << "Path: " << szHomeDirBuf << "\n";
}

/**
@brief Main function
@param hInstance Handle to the current instance
@param hPrevInstance Handle to the previous running instance (not used)
@param lpCmdLine Command line (not used)
@param nCmdShow Initial window visibility and location flag (not used)
@return 0 if all went well, other values upon errors
*/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Initialize stuff
	char cPath[MAX_PATH + 1];
	char cFile[MAX_PATH + 128];
	char cInclude[3 * MAX_PATH + 7];
	cErr[0] = '\0';


	// Read configuration file
	HMODULE hModule = GetModuleHandle(NULL);
	TCHAR iniPath[MAX_PATH];
	GetModuleFileName(hModule, iniPath, MAX_PATH);
//	PathRemoveFileSpec(iniPath, MAX_PATH);
	char * x = strstr(iniPath, "\\CCPDFConverter.exe");
	*x = 0;
	strcat( iniPath, "\\CCPDFConverterMessages.ini" );

  std::ifstream f( iniPath );
  f >> myconfigdata;
  f.close();

  std::string title = myconfigdata["title"];
  std::string directoryName = myconfigdata["directory"];
  std::string level1Text = myconfigdata["level1.prompt"];
  std::string level2Text = myconfigdata["level2.prompt"];
  std::string filenameText = myconfigdata["filename.prompt"];


#ifdef _DEBUG
	// Save a record of the original PostScript data (debug mode)
	errno_t file_err = fopen_s (&pSave, "c:\\test.ps", "w+b");
#endif

	// Delete whichever temp files might exist
	CleanTempFiles();

	// Add the include directories to the command line flags we'll use with GhostScript:
	if (::GetModuleFileName(NULL, cPath, MAX_PATH))
	{
		// Should be next to the application
		char* pPos = strrchr(cPath, '\\');
		if (pPos != NULL)
			*(pPos) = '\0';
		else
			cPath[0] = '\0';
		// OK, add the fonts and lib folders:
		sprintf_s (cInclude, sizeof(cInclude), "-I%s\\urwfonts;%s\\lib", cPath, cPath);
		ARGS[6] = cInclude;
	}

#ifdef _DEBUG_CMD
	// Sample file debug mode: open a pre-existing file
	fileInput = fopen("c:\\test1.ps", "rb");
#else
	// Get the data from stdin (that's where the redmon port monitor sends it)
	fileInput = stdin;
#endif

	// Check if we have a filename to write to:
	cPath[0] = '\0';
	bool bAutoOpen = false;
	bool bMakeTemp = false;
	// Read the start of the file; if we have a filename and/or the auto-open flag, they must be there:
	nBuffer = fread(cBuffer, 1, MAX_PATH * 2, fileInput);
	cBuffer[nBuffer] = EOF;

	myconfigdata2["nBuffer"] = cBuffer;
	myconfigdata2["nBuffer2"] = cBuffer + 9;

	// Do we have a %%File: starting the buffer?
	if ((nBuffer > 8) && (strncmp(cBuffer, "%%File: ", 8) == 0))
	{
		// Yes, so read the filename
		char ch;
		int nCount = 0;
		nInBuffer += 8;
		do
		{
			ch = cBuffer[nInBuffer++];
			if (ch == EOF)
				break;
			if (ch == '\n')
				break;
			cPath[nCount++] = ch;
		} while (true);

		if (ch == EOF)
		{
			// If we didn't find a newline, something ain't right
			return 0;
		}

		// OK, found the page, so set it as a command line variable now
		cPath[nCount] = '\0';

		// Sometimes we don't want any output:
		if (strcmp(cPath, ":dropfile:") == 0)
		{
			// Nothing doing
			CleanInput();
			return 0;
		}

		sprintf_s(cFile, sizeof(cFile), "-sOutputFile=%s", cPath);
		ARGS[5] = cFile;
#ifdef _DEBUG
		// Trace it (debug mode)
		WriteOutput("FILENAME: ", cPath, nCount);
#endif
	}
	// Do we have an auto-file-open flag?
	if ((nBuffer - nInBuffer > 14) && ((!strncmp(cBuffer + nInBuffer, "%%FileAutoOpen", 14)) || (!strncmp(cBuffer + nInBuffer, "%%CreateAsTemp", 14))))
	{
		// Yes, found it, so jump over it until the newline
		if (!strncmp(cBuffer + nInBuffer, "%%CreateAsTemp", 14))
			bMakeTemp = true;

		nInBuffer += 14;
		bAutoOpen = true;
		while ((cBuffer[nInBuffer] != EOF) && (cBuffer[nInBuffer] != '\n'))
			nBuffer++;
		if (cBuffer[nInBuffer] == EOF)
		{
			// Nothing else, leave
			return 0;
		}
	}

	// Look for the title.
	char* titleKeyword = strstr(cBuffer + 9, "%%Title: ");
	if (titleKeyword != NULL) {
		myconfigdata2["titleKeywmord"] = titleKeyword;
//		std::string * titleX = new std::string(titleKeyword);
		char * titleStart = titleKeyword + 9;
		myconfigdata2["titleStart"] = titleStart;

		char * titleEnd = strstr(titleStart, "%%") - 1;
		int titleLen = titleEnd - titleStart;
//		int titleLen = strlen(titleStart);
		memcpy(docName, titleStart, titleLen);
		docName[titleLen] = 0;
//		std::string::size_type begin = titleX->find_first_of( " " );
//		std::string::size_type end = titleX->find_first_of( "\f\n\r\t\v", begin );
 //       if (begin == std::string::npos) {
	//		docName[0] = 0;
		//} else {

      // Extract the key value
//		std::string F = titleX->substr( begin, end - begin );
//		F.copy(docName, 100);
	//	}
	}

	myconfigdata2["docName"] = docName;

	// Did we find a filename?
	if (cPath[0] == '\0')
	{
		// Do we make it a temp file?
		if (bMakeTemp) {
			char sTempFolder[MAX_PATH];
			GetTempPath(MAX_PATH, sTempFolder);
			sprintf_s (cPath, MAX_PATH, "%s%s%u.%s", sTempFolder, TEMP_FILENAME, GetTickCount(), TEMP_EXTENSION);

			HANDLE test = CreateFile (cPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
			if (test == INVALID_HANDLE_VALUE) {
				// If we can't write this file, for some reason:
				bMakeTemp = false;
			}
			else {
				CloseHandle (test);	
				sprintf_s (cFile, sizeof(cFile), "-sOutputFile=%s", cPath);
				ARGS[5] = cFile;
			}			
		}

		okPressed = FALSE;

		// Create the dialog
		HWND hDlg;
		hDlg = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);

		HWND OK = GetDlgItem(hDlg, IDOK);
		EnableWindow(OK, false);

		SetWindowText(hDlg, title.c_str());
		SetDlgItemText(hDlg, IDC_LABEL_LEVEL1, level1Text.c_str());
		SetDlgItemText(hDlg, IDC_LABEL_LEVEL2, level2Text.c_str());
		SetDlgItemText(hDlg, IDC_LABEL_DOC_NAME, filenameText.c_str());
		
		// Find last occurance of " - "
		TCHAR *ptr = docName;
		TCHAR *prevptr = NULL;
		while( (ptr = strstr(ptr, " - ")))
		{
			prevptr = ptr;
			// move pointer to end of match
			ptr = ptr + 3;
		}
		// now, prevptr contains the last occurrence
		if (prevptr != NULL) {
			*prevptr = 0;
		}

		SetDlgItemText(hDlg, IDC_EDIT_DOC_NAME, docName);

		ShowWindow(hDlg, nCmdShow);


		TCHAR homeDir[MAX_PATH] = { 0 };

		GetPublicDocsDir(homeDir);
		
		TCHAR *directoryNameAsTChar=new TCHAR[directoryName.size()+1];
		directoryNameAsTChar[directoryName.size()]=0;
		std::copy(directoryName.begin(),directoryName.end(),directoryNameAsTChar);

		combine(path, homeDir, directoryNameAsTChar);

		// Now read the writable properties
		TCHAR writableConfig[MAX_PATH] = { 0 };
		combine(writableConfig, path, _T("CCPDFConverter.ini")); 
  std::ifstream f( writableConfig );
  f >> myconfigdata2;
  f.close();

  std::string recent1 = myconfigdata2["recent1"];
  
  myconfigdata2["debug"] = lpCmdLine;

		HWND LIST = GetDlgItem(hDlg, IDC_LIST1);

		FillChildDirectories(LIST, path);

		BOOL ret;
		MSG msg;
		while((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
			if(ret == -1) /* error found */
				return -1;

			if(!IsDialogMessage(hDlg, &msg)) {
				TranslateMessage(&msg); /* translate virtual-key messages */
				DispatchMessage(&msg); /* send it to dialog procedure */
			}
		}
		if (okPressed)
		{
						// OK, get a filename, write it up
				sprintf_s (cFile, sizeof(cFile), "-sOutputFile=%s.inprogress", fullFileName);
				ARGS[5] = cFile;
				bMakeTemp = true;

#ifdef _DEBUG
				// Also trace it (debug mode)
				WriteOutput("FILENAME (USER): ", cFile, strlen(cFile));
#endif

				// Save the settings
//		  std::ofstream f2( writableConfig );
//  f2 << myconfigdata2;
//  f2.close();
	}

	// First try to initialize a new GhostScript instance
	void* pGS;
	if (gsapi_new_instance(&pGS, NULL) < 0)
	{
		// Error 
		return -1;
	}

	// Set up the callbacks
	if (gsapi_set_stdio(pGS, my_in, my_out, my_err) < 0)
	{
		// Failed...
		gsapi_delete_instance(pGS);
		return -2;
	}

	// Now run the GhostScript engine to transform PostScript into PDF
	int nRet = gsapi_init_with_args(pGS, sizeof(ARGS)/sizeof(char*), (char**)ARGS);

	gsapi_exit(pGS);
	gsapi_delete_instance(pGS);

#ifdef _DEBUG
	// Close the PostScript copy file (debug mode)
	if (pSave != NULL) {
		fclose(pSave);
	}
#endif
#ifdef _DEBUG_CMD
	// Close the sample file (sample file debug mode)
	fclose(fileInput);
#endif

		// Rename the file.
	// This is done so that directory listeners don't see the PDF file until
	// it is completely written.

	TCHAR src_file[MAX_PATH + 128];
				sprintf_s (src_file, sizeof(src_file), "%s.inprogress", fullFileName);
const char *dest_file = fullFileName;
 
	if (!MoveFileEx(src_file, dest_file, MOVEFILE_REPLACE_EXISTING)) {
		/* Handle error condition */
	}

			// Now write the writable properties
		TCHAR writableConfig2[MAX_PATH] = { 0 };
		combine(writableConfig2, path, _T("CCPDFConverter.ini")); 
			  std::ofstream f2( writableConfig2 );
  f2 << myconfigdata2;
  f2.close();


	// Did we get an error?
	if (strlen(cErr) > 0)
	{
		// Yes, show it
		MessageBox(NULL, cErr, PRODUCT_NAME, MB_ICONERROR|MB_OK);
		return 0;
	}

	}

	return 0;
}
