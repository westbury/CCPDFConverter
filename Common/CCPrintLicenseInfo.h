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

#ifndef _CCPRINTLICENSEINFO_H_
#define _CCPRINTLICENSEINFO_H_

#include "devmode.h"

/**
    @brief Class that contains static functions that allow reading and writing of license and location data
			to and from the registry
*/
class CCPrintLicenseInfo : public LicenseInfo
{
public:
	/// Reads license information data from the registry
	static void	ReadFromRegistry(HANDLE hPrinter, LicenseInfo& info);
	/// Writes license information data from the registry
	static bool	WriteToRegistry(HANDLE hPrinter, const LicenseInfo& info);
	/// Reads license stamp location data from the registry
	static void ReadFromRegistry(HANDLE hPrinter, LicenseLocationInfo& info);
	/// Writes license stamp location data to the registry
	static bool	WriteToRegistry(HANDLE hPrinter, const LicenseLocationInfo& info);
};

#endif   //#define _CCPRINTLICENSEINFO_H_
