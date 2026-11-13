/*******************************************************************************
 * plugins/VOIP/gui/MacCameraPermission.h                                      *
 *                                                                             *
 * Copyright (C) 2026 by Retroshare Team <retroshare.project@gmail.com>        *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef MAC_CAMERA_PERMISSION_H
#define MAC_CAMERA_PERMISSION_H

#include <functional>

// macOS only. Requests camera access straight from AVFoundation, bypassing Qt6's
// QPermission API.
//
// Why: on macOS, qApp->requestPermission(QCameraPermission) returns Denied WITHOUT
// ever contacting the OS -- proven with the TCC log, where opening the VOIP panel
// produces a kTCCServiceMicrophone request (granted) but NO kTCCServiceCamera
// request at all. Calling [AVCaptureDevice requestAccessForMediaType:] ourselves
// forces the real system request and prompt, exactly like the microphone path.
//
// cb is invoked with true if access is granted, false otherwise. When the status
// is already determined it fires synchronously; when it is undetermined the system
// prompt is shown and cb is invoked on the main thread once the user answers.
void requestMacCameraAccess(std::function<void(bool)> cb);

#endif // MAC_CAMERA_PERMISSION_H
