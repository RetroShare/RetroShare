/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


	Winfileassoc.cpp

	Handles file associations in Windows Vista/XP/2000/NT/ME/98/95.
	We assume that the code is run without administrator privileges, so the associations are done for current user only.
	System-wide associations require writing to HKEY_CLASSES_ROOT and we don't want to get our hands dirty with that.
	Each user on the computer can configure his own set of file associations for SMPlayer, which is extremely cool.

	Optionally, during uninstall, it would be a good idea to call RestoreFileAssociations for all media types so
	that we can clean up the registry and restore the old associations for current user. 

	Vista:
	The code can only register the app as default program for selected extensions and check if it is the default. 
	It cannot restore 'old' default application, since this doesn't seem to be possible with the current Vista API.

	Tested on: Win98, Win2000, WinXP, Vista. 
	NOT tested on: Win95, ME and NT 4.0 (it should work on 95, ME; Not sure about NT 4.0).

	Author: Florin Braghis (florin@libertv.ro)
*/

#include "winfileassoc.h"
#include <QSettings>
#include <QApplication>
#include <QFileInfo>

/*
   Note by RVM: Added some #ifdef Q_OS_WIN to allow the file to compile under linux. 
   It should compile the code for Windows XP.
   The registry entries are saved on a file named HKEY_CURRENT_USER.
*/

WinFileAssoc::WinFileAssoc( const QString ClassId, const QString AppName )
{
	m_ClassId = ClassId;
	m_AppName = AppName; 
	m_ClassId2 = QFileInfo(QApplication::applicationFilePath()).fileName(); 
}

//Associates all extensions in the fileExtensions list with current app.
//Returns number of extensions processed successfully. 
int WinFileAssoc::CreateFileAssociations(const QStringList& fileExtensions)
{
#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
	{
		return VistaSetAppsAsDefault(fileExtensions); 
	}
#endif

	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat); //Read only on NT+
	QSettings RegCU ("HKEY_CURRENT_USER", QSettings::NativeFormat);

	if (!RegCU.isWritable() || RegCU.status() != QSettings::NoError)
		return 0; 

	if (RegCR.status() != QSettings::NoError)
		return 0; 

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion < QSysInfo::WV_NT && !RegCR.isWritable())	//Win98
		return 0; 
#endif

	//Check if classId exists in the registry
	if (!RegCR.contains(m_ClassId) && !RegCU.contains("Software/Classes/" + m_ClassId))
	{
		//If doesn't exist (user didn't run the setup program), try to create the ClassId for current user.
		if (!CreateClassId(QApplication::applicationFilePath(), "SMPlayer Media Player"))
			return 0; 
	}
	
	int count = 0; 
	foreach(const QString& fileExtension, fileExtensions)
	{
		QString ExtKeyName = QString("Software/Microsoft/Windows/CurrentVersion/Explorer/FileExts/.%1").arg(fileExtension);
		QString ClassesKeyName = m_ClassId;

		QString BackupKeyName = ClassesKeyName + "/" + fileExtension; 
		QString CUKeyName = "Software/Classes/." + fileExtension; 

		//Save current ClassId for current user
		QString KeyVal = RegCU.value(CUKeyName + "/.").toString(); 

		if (KeyVal.length() == 0 || KeyVal == m_ClassId)
		{
			//No registered app for this extension for current user.
			//Check the system-wide (HKEY_CLASSES_ROOT) ClassId for this extension 
			KeyVal = RegCR.value("." + fileExtension + "/.").toString();
		}

		if (KeyVal != m_ClassId)
			RegCU.setValue(CUKeyName + "/MPlayer_Backup", KeyVal); 

		//Save last ProgId and Application values from the Exts key
		KeyVal = RegCU.value(ExtKeyName + "/Progid").toString();
		
		if (KeyVal != m_ClassId && KeyVal != m_ClassId2)
			RegCU.setValue(ExtKeyName + "/MPlayer_Backup_ProgId", KeyVal);

		KeyVal = RegCU.value(ExtKeyName + "/Application").toString(); 
		if (KeyVal != m_ClassId || KeyVal != m_ClassId2) 
			RegCU.setValue(ExtKeyName + "/MPlayer_Backup_Application", KeyVal); 

		//Create the associations
#ifdef Q_OS_WIN
		if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT)
#endif
		{
			RegCU.setValue(CUKeyName + "/.", m_ClassId); 		//Extension class
			RegCU.setValue(ExtKeyName + "/Progid", m_ClassId);	//Explorer FileExt association

		}
#ifdef Q_OS_WIN
		else
		{
			//Windows ME/98/95 support
			RegCR.setValue("." + fileExtension + "/.", m_ClassId); 
		}
#endif

		if (RegCU.status() == QSettings::NoError && RegCR.status() == QSettings::NoError)
			count++; 
	}

	return count; 
}

//Checks if extensions in extensionsToCheck are registered with this application. Returns a list of registered extensions. 
//Returns false if there was an error accessing the registry. 
//Returns true and 0 elements in registeredExtensions if no extension is associated with current app.
bool WinFileAssoc::GetRegisteredExtensions( const QStringList& extensionsToCheck, QStringList& registeredExtensions)
{
	registeredExtensions.clear(); 

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
	{
		return VistaGetDefaultApps(extensionsToCheck, registeredExtensions); 
	}
#endif

	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	QSettings RegCU ("HKEY_CURRENT_USER", QSettings::NativeFormat);

	if (RegCR.status() != QSettings::NoError)
		return false; 

	if (RegCU.status() != QSettings::NoError)
		return false; 

	foreach(const QString& fileExtension, extensionsToCheck)
	{
		bool bRegistered = false; 
		//Check the explorer extension (Always use this program to open this kind of file...)

#ifdef Q_OS_WIN
		if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT)
#endif
		{
			QString FileExtsKey = QString("Software/Microsoft/Windows/CurrentVersion/Explorer/FileExts/.%1").arg(fileExtension);
			QString CurClassId = RegCU.value(FileExtsKey + "/Progid").toString(); 
			QString CurAppId = RegCU.value(FileExtsKey + "/Application").toString();

			if (CurClassId.size())	//Registered with Open With... / ProgId ?
			{
				bRegistered = (CurClassId == m_ClassId) || (0 == CurClassId.compare(m_ClassId2, Qt::CaseInsensitive));
			}
			else
			if (CurAppId.size())
			{
				//If user uses Open With..., explorer creates it's own ClassId under Application, usually "smplayer.exe"
				bRegistered = (CurAppId == m_ClassId) || (0 == CurAppId.compare(m_ClassId2, Qt::CaseInsensitive)); 
			}
			else
			{
				//No classId means that no associations exists in Default Programs or Explorer
				//Check the default per-user association 
				bRegistered = RegCU.value("Software/Classes/." + fileExtension + "/.").toString() == m_ClassId; 
			}
		}
		
		//Finally, check the system-wide association
		if (!bRegistered)
			bRegistered = RegCR.value("." + fileExtension + "/.").toString() == m_ClassId;


		if (bRegistered)
			registeredExtensions.append(fileExtension); 
	}
	
	return true; 
}

//Restores file associations to old defaults (if any) for all extensions in the fileExtensions list.
//Cleans up our backup keys from the registry.
//Returns number of extensions successfully processed (error if fileExtensions.count() != return value && count > 0).
int WinFileAssoc::RestoreFileAssociations(const QStringList& fileExtensions)
{
#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
		return 0; //Not supported by the API
#endif

	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	QSettings RegCU ("HKEY_CURRENT_USER", QSettings::NativeFormat);

	if (!RegCU.isWritable() || RegCU.status() != QSettings::NoError)
		return 0; 

	if (RegCR.status() != QSettings::NoError)
		return 0; 

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion < QSysInfo::WV_NT && !RegCR.isWritable())	//Win98
		return 0; 
#endif

	int count = 0; 
	foreach(const QString& fileExtension, fileExtensions)
	{
		QString ExtKeyName = QString("Software/Microsoft/Windows/CurrentVersion/Explorer/FileExts/.%1").arg(fileExtension);
		QString OldProgId = RegCU.value(ExtKeyName + "/MPlayer_Backup_ProgId").toString(); 
		QString OldApp  = RegCU.value(ExtKeyName + "/MPlayer_Backup_Application").toString(); 
		QString OldClassId = RegCU.value("Software/Classes/." + fileExtension + "/MPlayer_Backup").toString(); 

		//Restore old explorer ProgId
		if (!OldProgId.isEmpty() && OldProgId != m_ClassId)
			RegCU.setValue(ExtKeyName + "/Progid", OldProgId);
		else
		{
			QString CurProgId = RegCU.value(ExtKeyName + "/Progid").toString();
			if ((CurProgId == m_ClassId) || (0 == CurProgId.compare(m_ClassId2, Qt::CaseInsensitive))) //Only remove if we own it
				RegCU.remove(ExtKeyName + "/Progid");	
		}

		//Restore old explorer Application 
		if (!OldApp.isEmpty() && OldApp != m_ClassId)
			RegCU.setValue(ExtKeyName + "/Application", OldApp); 
		else
		{
			QString CurApp = RegCU.value(ExtKeyName + "/Application").toString();
			if ((CurApp == m_ClassId) || (0 == CurApp.compare(m_ClassId2, Qt::CaseInsensitive))) //Only remove if we own it
				RegCU.remove(ExtKeyName + "/Application"); 
		}

#ifdef Q_OS_WIN
		if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT)
#endif
		{
			//Restore old association for current user
			if (!OldClassId.isEmpty() && OldClassId != m_ClassId)
				RegCU.setValue("Software/Classes/." + fileExtension + "/.", OldClassId); 
			else
			{
				if (RegCU.value("Software/Classes/." + fileExtension + "/.").toString() == m_ClassId) //Only remove if we own it
					RegCU.remove("Software/Classes/." + fileExtension); 
			}
		}
#ifdef Q_OS_WIN
		else
		{
			//Windows 98 ==> Write to HKCR
			if (!OldClassId.isEmpty() && OldClassId != m_ClassId)
				RegCR.setValue("." + fileExtension + "/.", OldClassId); 
			else
			{
				if (RegCR.value("." + fileExtension + "/.").toString() == m_ClassId)
					RegCR.remove("." + fileExtension); 
			}
		}
#endif
		//Remove our keys:
		//CurrentUserClasses/.ext/MPlayerBackup
		//Explorer: Backup_Application and Backup_ProgId
		RegCU.remove("Software/Classes/." + fileExtension + "/MPlayer_Backup"); 
		RegCU.remove(ExtKeyName + "/MPlayer_Backup_Application"); 
		RegCU.remove(ExtKeyName + "/MPlayer_Backup_ProgId"); 
	}
	return count; 
}

//Creates a ClassId for current application. 
//Note: It's better to create the classId from the installation program.
bool WinFileAssoc::CreateClassId(const QString& executablePath, const QString& friendlyName)
{
	QString RootKeyName;
	QString classId; 

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT) 
#endif
	{
		classId = "Software/Classes/" + m_ClassId; 
		RootKeyName = "HKEY_CURRENT_USER";
	}
#ifdef Q_OS_WIN
	else 
	{
		classId = m_ClassId; 
		RootKeyName = "HKEY_CLASSES_ROOT";	//Windows 95/98/ME
	}
#endif

	QSettings Reg (RootKeyName, QSettings::NativeFormat);
	if (!Reg.isWritable() || Reg.status() != QSettings::NoError)
		return false; 

	QString appPath = executablePath;
	appPath.replace('/', '\\'); //Explorer gives 'Access Denied' if we write the path with forward slashes to the registry

	//Add our ProgId to the HKCR classes
	Reg.setValue(classId + "/shell/open/FriendlyAppName", friendlyName);
	Reg.setValue(classId + "/shell/open/command/.", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
	Reg.setValue(classId + "/DefaultIcon/.", QString("\"%1\",1").arg(appPath));
	//Add "Enqueue" command
	Reg.setValue(classId + "/shell/enqueue/.", QObject::tr("Enqueue in SMPlayer"));
	Reg.setValue(classId + "/shell/enqueue/command/.", QString("\"%1\" -add-to-playlist \"%2\"").arg(appPath, "%1"));
	return true; 
}
//Remove ClassId from the registry.
//Called when no associations exist. Note: It's better to do this in the Setup program.
bool WinFileAssoc::RemoveClassId()
{
	QString RootKeyName;
	QString classId; 

#ifdef Q_OS_WIN
	if (QSysInfo::WindowsVersion >= QSysInfo::WV_NT) 
#endif
	{
		classId = "Software/Classes/" + m_ClassId; 
		RootKeyName = "HKEY_CURRENT_USER";
	}
#ifdef Q_OS_WIN
	else 
	{
		classId = m_ClassId; 
		RootKeyName = "HKEY_CLASSES_ROOT";	//Windows 95/98/ME
	}
#endif

	QSettings RegCU (RootKeyName, QSettings::NativeFormat);

	if (!RegCU.isWritable() || RegCU.status() != QSettings::NoError)
		return false; 

	RegCU.remove(classId);
	return true; 
}

//Windows Vista specific implementation
//Add libole32.a library if compiling with mingw.
//In smplayer.pro, under win32{ :
//	LIBS += libole32
#ifdef WIN32
#include <windows.h>

#if !defined(IApplicationAssociationRegistration)

typedef enum tagASSOCIATIONLEVEL
{
	AL_MACHINE,
	AL_EFFECTIVE,
	AL_USER
} ASSOCIATIONLEVEL;

typedef enum tagASSOCIATIONTYPE
{
	AT_FILEEXTENSION,
	AT_URLPROTOCOL,
	AT_STARTMENUCLIENT,
	AT_MIMETYPE
} ASSOCIATIONTYPE;

MIDL_INTERFACE("4e530b0a-e611-4c77-a3ac-9031d022281b")
IApplicationAssociationRegistration : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE QueryCurrentDefault(LPCWSTR pszQuery,
		ASSOCIATIONTYPE atQueryType,
		ASSOCIATIONLEVEL alQueryLevel,
		LPWSTR *ppszAssociation) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryAppIsDefault(LPCWSTR pszQuery,
		ASSOCIATIONTYPE atQueryType,
		ASSOCIATIONLEVEL alQueryLevel,
		LPCWSTR pszAppRegistryName,
		BOOL *pfDefault) = 0;
	virtual HRESULT STDMETHODCALLTYPE QueryAppIsDefaultAll(ASSOCIATIONLEVEL alQueryLevel,
		LPCWSTR pszAppRegistryName,
		BOOL *pfDefault) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetAppAsDefault(LPCWSTR pszAppRegistryName,
		LPCWSTR pszSet,
		ASSOCIATIONTYPE atSetType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetAppAsDefaultAll(LPCWSTR pszAppRegistryName) = 0;
	virtual HRESULT STDMETHODCALLTYPE ClearUserAssociations( void) = 0;
};
#endif

static const CLSID CLSID_ApplicationAssociationReg = {0x591209C7,0x767B,0x42B2,{0x9F,0xBA,0x44,0xEE,0x46,0x15,0xF2,0xC7}};
static const IID   IID_IApplicationAssociationReg  = {0x4e530b0a,0xe611,0x4c77,{0xa3,0xac,0x90,0x31,0xd0,0x22,0x28,0x1b}};

int WinFileAssoc::VistaSetAppsAsDefault(const QStringList& fileExtensions)
{
	IApplicationAssociationRegistration* pAAR;
	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationReg,
		NULL, CLSCTX_INPROC, IID_IApplicationAssociationReg,	(void**)&pAAR);

	int count = 0; 
	if (SUCCEEDED(hr) && (pAAR != NULL))
	{
		foreach(const QString& fileExtension, fileExtensions)
		{
			hr = pAAR->SetAppAsDefault((const WCHAR*)m_AppName.utf16(),
				(const WCHAR*)QString("." + fileExtension).utf16(),
				AT_FILEEXTENSION);

			if (SUCCEEDED(hr)) 
				count++; 
		}
		pAAR->Release(); 
	}
	return count; 
}

bool WinFileAssoc::VistaGetDefaultApps(const QStringList &extensions, QStringList& registeredExt)
{
	IApplicationAssociationRegistration* pAAR;

	HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationReg,
		NULL, CLSCTX_INPROC, IID_IApplicationAssociationReg,	(void**)&pAAR);

	if (SUCCEEDED(hr) && (pAAR != NULL))
	{
		foreach(const QString& fileExtension, extensions)
		{
			BOOL bIsDefault = FALSE; 
			hr = pAAR->QueryAppIsDefault((const WCHAR*)QString("." + fileExtension).utf16(),
				AT_FILEEXTENSION,
				AL_EFFECTIVE,
				(const WCHAR*)m_AppName.utf16(),
				&bIsDefault);
			if (SUCCEEDED(hr) && bIsDefault)
			{
				registeredExt.append(fileExtension); 
			}
		}

		pAAR->Release();
		return true;
	}
	return false;
}
#else
bool WinFileAssoc::VistaGetDefaultApps(const QStringList &extensions, QStringList& registeredExt)
{
	return false; 
}

int WinFileAssoc::VistaSetAppsAsDefault(const QStringList& extensions)
{
	return 0; 
}
#endif

