/**
	@file
	@brief 
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

#include "precomp.h"
#include "CCTChar.h"

/**
	@param sString Generic string object to translate
	@return A MultiByte string object similar to the received one
*/
std::string MakeAnsiString(const std::tstring& sString)
{
#ifndef _UNICODE
	// MultiByte: just return a copy
	return sString;
#else
	// Convert unicode to ANSI
	int nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, sString.c_str(), (int)sString.size(), NULL, 0, NULL, NULL);
	LPSTR pAnsiString = new CHAR[nAnsiChars];
	nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, sString.c_str(), (int)sString.size(), pAnsiString, nAnsiChars, NULL, NULL);
	std::string sRet(pAnsiString, nAnsiChars);
	delete [] pAnsiString;
	return sRet;
#endif
}

/**
	@param pString Generic NULL-terminated buffer to translate
	@return A MultiByte string object similar to the received buffer
*/
std::string MakeAnsiString(LPCTSTR pString)
{
#ifndef _UNICODE
	// MultiByte: just create an object from the string
	return pString;
#else
	// Convert unicode to ANSI
	int nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pString, -1, NULL, 0, NULL, NULL);
	LPSTR pAnsiString = new CHAR[nAnsiChars];
	nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pString, -1, pAnsiString, nAnsiChars, NULL, NULL);
	std::string sRet(pAnsiString, nAnsiChars);
	delete [] pAnsiString;
	return sRet;
#endif
}

/**
	@param sString Generic string object to translate
	@return A Unicode string object similar to the received one
*/
std::wstring MakeWideString(const std::tstring& sString)
{
#ifdef _UNICODE
	// Unicode: just return a copy
	return sString;
#else
	// Convert unicode to ANSI
	int nWideChars = MultiByteToWideChar(CP_ACP, 0, sString.c_str(), sString.size(), NULL, 0);
	LPWSTR pWideString = new WCHAR[nWideChars];
	nWideChars = MultiByteToWideChar(CP_ACP, 0, sString.c_str(), sString.size(), pWideString, nWideChars);
	std::wstring sRet(pWideString, nWideChars);
	delete [] pWideString;
	return sRet;
#endif
}

/**
	@param pString Generic NULL-terminated buffer to translate
	@return A Unicode string object similar to the received buffer
*/
std::wstring MakeWideString(LPCTSTR pString)
{
#ifdef _UNICODE
	// Unicode: just create an object from the string
	return pString;
#else
	// Convert unicode to ANSI
	int nWideChars = MultiByteToWideChar(CP_ACP, 0, pString, -1, NULL, 0);
	LPWSTR pWideString = new WCHAR[nWideChars];
	nWideChars = MultiByteToWideChar(CP_ACP, 0, pString, -1, pWideString, nWideChars);
	std::wstring sRet(pWideString, nWideChars);
	delete [] pWideString;
	return sRet;
#endif
}

/**
	@param sString MultiByte string object to translate
	@return A generic string object (Unicode in Unicode compilations, MultiByte in non-Unicode ones)
*/
std::tstring MakeTString(const std::string& sString)
{
#ifdef _UNICODE
	// Convert unicode to ANSI
	int nWideChars = MultiByteToWideChar(CP_ACP, 0, sString.c_str(), (int)sString.size(), NULL, 0);
	LPWSTR pWideString = new WCHAR[nWideChars];
	nWideChars = MultiByteToWideChar(CP_ACP, 0, sString.c_str(), (int)sString.size(), pWideString, nWideChars);
	std::wstring sRet(pWideString, nWideChars);
	delete [] pWideString;
	return sRet;
#else
	// Just return a copy
	return sString;
#endif
}

/**
	@param sString Unicode string object to translate
	@return A generic string object (Unicode in Unicode compilations, MultiByte in non-Unicode ones)
*/
std::tstring MakeTString(const std::wstring& sString)
{
#ifdef _UNICODE
	// Just return a copy
	return sString;
#else
	// Convert unicode to ANSI
	int nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, sString.c_str(), sString.size(), NULL, 0, NULL, NULL);
	LPSTR pAnsiString = new CHAR[nAnsiChars];
	nAnsiChars = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, sString.c_str(), sString.size(), pAnsiString, nAnsiChars, NULL, NULL);
	std::string sRet(pAnsiString, nAnsiChars);
	delete [] pAnsiString;
	return sRet;
#endif
}

std::tstring MakeTStringFromUTF8(const char* pUTF8)
{
	int nWideChars = MultiByteToWideChar(CP_UTF8, 0, pUTF8, -1, NULL, 0);
	LPWSTR pWideString = new WCHAR[nWideChars];
	nWideChars = MultiByteToWideChar(CP_UTF8, 0, pUTF8, -1, pWideString, nWideChars);
	std::wstring sRet(pWideString, nWideChars - 1);
	delete [] pWideString;
	return MakeTString(sRet);
}
//////////////////////////////////////////////////////////////////////////

std::tstring RemoveHtmlTags (const std::tstring& str)
{
	std::tstring ret;

	LPCTSTR ptr = str.c_str();
	bool ignore = false;
	while (*ptr) {
		if (*ptr=='<')
			ignore = true;
		if (!ignore)
			ret += *ptr;
		if (*ptr == '>')
			ignore = false;
		++ptr;
	}

	return ret;
}