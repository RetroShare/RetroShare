/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2014, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "RsFile.h"

#ifdef WINDOWS_SYS
#include <QDir>
#include <windows.h>

#ifndef __MINGW64_VERSION_MAJOR
#include <QLibrary>
typedef BOOL (*lpCreateHardLinkW)(LPWSTR lpFileName, LPWSTR lpExisitingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
#endif

#else
#include <QFile>
#endif

bool RsFile::CreateLink(const QString &targetFileName, const QString &symbolicFileName)
{
#ifdef WINDOWS_SYS
#ifdef __MINGW64_VERSION_MAJOR
	return CreateHardLink((WCHAR*) QDir::toNativeSeparators(symbolicFileName).toStdWString().c_str(), (WCHAR*) QDir::toNativeSeparators(targetFileName).toStdWString().c_str(), NULL);
#else
	QLibrary library("kernel32");
	if (!library.load()) {
		return false;
	}

	lpCreateHardLinkW pCreateHardLinkW = (lpCreateHardLinkW) library.resolve("CreateHardLinkW");
	if (!pCreateHardLinkW) {
		return false;
	}

	return pCreateHardLinkW((WCHAR*) QDir::toNativeSeparators(symbolicFileName).toStdWString().c_str(), (WCHAR*) QDir::toNativeSeparators(targetFileName).toStdWString().c_str(), NULL);
#endif
#else
	return QFile::link(targetFileName, symbolicFileName);
#endif
}
