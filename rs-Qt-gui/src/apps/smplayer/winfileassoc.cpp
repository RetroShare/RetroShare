/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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
	Handles file associations in Windows
	Author: Florin Braghis (florin@libertv.ro)
*/

#include "winfileassoc.h"
#include <QSettings>

WinFileAssoc::WinFileAssoc( const QString& ClassId )
{
	m_ClassId = ClassId;
}

bool WinFileAssoc::CreateFileAssociation(const QString& fileExtension)
{
	//Registry keys modified:
	//HKEY_CLASSES_ROOT\.extension 
	//HKEY_CLASSES_ROOT\smplayer.exe
	//Shell 'Open With...' entry:
	//HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.avi

	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	QSettings RegCU ("HKEY_CURRENT_USER", QSettings::NativeFormat);

	if (!RegCR.isWritable() || RegCR.status() != QSettings::NoError)
		return false; 

	if (!RegCU.isWritable() || RegCU.status() != QSettings::NoError)
		return false; 

	QString ExtKeyName = QString("SOFTWARE/Microsoft/Windows/CurrentVersion/Explorer/FileExts/.%1").arg(fileExtension);
	QString ClassesKeyName = m_ClassId;

	QString BackupKeyName = ClassesKeyName + "/" + fileExtension; 

	//Save last ClassId from the extension class
	QString KeyVal = RegCR.value("." + fileExtension + "/.").toString();
	if (KeyVal != m_ClassId)
		RegCR.setValue(BackupKeyName + "/OldClassId", KeyVal); 

	//Save last ProgId and Application values from the Exts key
	KeyVal = RegCU.value(ExtKeyName + "/Progid").toString();
	if (KeyVal != m_ClassId)
		RegCR.setValue(BackupKeyName + "/OldProgId", KeyVal);

	KeyVal = RegCU.value(ExtKeyName + "/Application").toString(); 
	if (KeyVal != m_ClassId) 
		RegCR.setValue(BackupKeyName + "/OldApplication", KeyVal); 

	//Create the associations
	RegCR.setValue("." + fileExtension + "/.", m_ClassId); 		//Extension class
	RegCU.setValue(ExtKeyName + "/Progid", m_ClassId);	//Explorer FileExt association
	return true; 
}

bool WinFileAssoc::RestoreFileAssociation(const QString& fileExtension)
{
	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	QSettings RegCU ("HKEY_CURRENT_USER", QSettings::NativeFormat);

	if (!RegCR.isWritable() || RegCR.status() != QSettings::NoError)
		return false; 

	if (!RegCU.isWritable() || RegCU.status() != QSettings::NoError)
		return false; 


	QString ClassesKeyName = m_ClassId; 
	QString ExtKeyName = QString("SOFTWARE/Microsoft/Windows/CurrentVersion/Explorer/FileExts/.%1").arg(fileExtension);

	QString BackupKeyName = ClassesKeyName + "/" + fileExtension; 
	QString OldProgId = RegCR.value(BackupKeyName + "/OldProgId").toString(); 
	QString OldApp  = RegCR.value(BackupKeyName + "/OldApplication").toString(); 
	QString OldClassId = RegCR.value(BackupKeyName + "/OldClassId").toString(); 

	//Restore old association
	if (!OldProgId.isEmpty() && OldProgId != m_ClassId)
	{
		RegCU.setValue(ExtKeyName + "/Progid", OldProgId);
	}
	else
	{
		RegCU.remove(ExtKeyName + "/Progid"); 
	}

	if (!OldApp.isEmpty() && OldApp != m_ClassId)
	{
		RegCU.setValue(ExtKeyName + "/Application", OldApp); 
	}
	else
	{
		RegCU.remove(ExtKeyName + "/Application");
	}

	if (!OldClassId.isEmpty() && OldClassId != m_ClassId)
	{
		RegCR.setValue("." + fileExtension + "/.", OldClassId); 
	}
	else
	{
		//No old association with this extension, it's better to remove it entirely
		RegCR.remove("." + fileExtension); 
	}

	//Remove our keys
	RegCR.remove(BackupKeyName + "/OldProgId"); 
	RegCR.remove(BackupKeyName + "/OldApplication"); 
	RegCR.remove(BackupKeyName + "/OldClassId"); 
	RegCR.remove(BackupKeyName); 
	return true; 
}

bool WinFileAssoc::CreateClassId(const QString& executablePath, const QString& friendlyName)
{
	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	if (!RegCR.isWritable() || RegCR.status() != QSettings::NoError)
		return false; 

	QString appPath = executablePath;
	appPath.replace('/', '\\'); //Explorer gives 'Access Denied' if we write the path with forward slashes to the registry

	//Add our ProgId to the HKCR classes
	RegCR.setValue(m_ClassId + "/shell/open/FriendlyAppName", friendlyName);
	RegCR.setValue(m_ClassId + "/shell/open/command/.", QString("\"%1\" \"%2\"").arg(appPath, "%1"));
	RegCR.setValue(m_ClassId + "/DefaultIcon/.", QString("\"%1\",0").arg(appPath));
	return true; 
}

//Called when no associations exist
bool WinFileAssoc::RemoveClassId()
{
	QSettings RegCR ("HKEY_CLASSES_ROOT", QSettings::NativeFormat);

	if (!RegCR.isWritable() || RegCR.status() != QSettings::NoError)
		return false; 

	RegCR.remove(m_ClassId + "/shell/open/FriendlyAppName");
	RegCR.remove(m_ClassId + "/shell/open/command/.");
	RegCR.remove(m_ClassId + "/DefaultIcon/.");
	RegCR.remove(m_ClassId);
	return true; 
}
