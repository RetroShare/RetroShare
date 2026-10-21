/*******************************************************************************
 * plugins/VOIP/gui/MacCameraCapture.h                                         *
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

#ifndef MAC_CAMERA_CAPTURE_H
#define MAC_CAMERA_CAPTURE_H

#include <functional>
#include <QString>
#include <QList>

class QImage;

// macOS camera capture straight from AVFoundation, bypassing Qt's
// QMediaCaptureSession/QVideoSink pipeline.
//
// Why: on macOS (verified on Qt 6.11) that pipeline never delivers a frame -- a
// bare QVideoSink AND a QGraphicsVideoItem both report the camera active but emit
// zero frames; only a QVideoWidget renders, and it exposes no accessible sink.
// RetroShare needs the raw frames (to encode + display), so we grab them directly
// with AVCaptureSession + AVCaptureVideoDataOutput, exactly the path Photo Booth
// and the AVFoundation permission request already prove works on this machine.
class MacCameraCapture
{
public:
	MacCameraCapture();
	~MacCameraCapture();

	// Start capturing from the camera whose localized name equals `description`
	// (or the default camera when empty/null). Frames are delivered as QImage on
	// the GUI/main thread through onFrame. Returns true if the session is running.
	bool start(const QString& description, std::function<void(const QImage&)> onFrame);
	void stop();
	bool isRunning() const;

	// Localized names of the available video capture devices.
	static void availableDevices(QList<QString>& names);

private:
	void* _holder;   // retained Objective-C session holder (opaque to C++ callers)
};

#endif // MAC_CAMERA_CAPTURE_H
