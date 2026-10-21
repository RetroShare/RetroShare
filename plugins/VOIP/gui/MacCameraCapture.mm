/*******************************************************************************
 * plugins/VOIP/gui/MacCameraCapture.mm                                        *
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

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreMedia/CoreMedia.h>

#include "MacCameraCapture.h"
#include <QImage>
#include <iostream>

// Delegate: receives sample buffers on a background queue, converts them to a
// deep-copied QImage and forwards them on the main (GUI) thread.
@interface RsMacFrameGrabber : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
@public
	std::function<void(const QImage&)> cb;
}
@end

@implementation RsMacFrameGrabber
- (void)captureOutput:(AVCaptureOutput*)output
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection*)connection
{
	(void)output; (void)connection;
	CVImageBufferRef px = CMSampleBufferGetImageBuffer(sampleBuffer);
	if(!px || !cb) return;

	CVPixelBufferLockBaseAddress(px, kCVPixelBufferLock_ReadOnly);
	const int w = (int)CVPixelBufferGetWidth(px);
	const int h = (int)CVPixelBufferGetHeight(px);
	const int stride = (int)CVPixelBufferGetBytesPerRow(px);
	// kCVPixelFormatType_32BGRA == QImage::Format_ARGB32 byte order on little-endian.
	QImage img((const uchar*)CVPixelBufferGetBaseAddress(px), w, h, stride, QImage::Format_ARGB32);
	QImage copy = img.copy();   // detach from the pixel buffer before unlocking
	CVPixelBufferUnlockBaseAddress(px, kCVPixelBufferLock_ReadOnly);

	// Re-check cb on the main thread at delivery time (not the value captured here):
	// stop() nulls it before the owner is destroyed, so a frame still in flight after
	// stop() is dropped instead of calling into a freed object. The block strongly
	// holds `self`, so the grabber stays alive until the block runs.
	dispatch_async(dispatch_get_main_queue(), ^{
		if(self->cb) self->cb(copy);
	});
}
@end

// Owns the capture session and the delegate so their lifetime is tied to the
// C++ MacCameraCapture instance (kept via a retained void* on the C++ side).
@interface RsMacCaptureHolder : NSObject
@property (nonatomic, strong) AVCaptureSession* session;
@property (nonatomic, strong) RsMacFrameGrabber* grabber;
@end
@implementation RsMacCaptureHolder
@end

MacCameraCapture::MacCameraCapture() : _holder(nullptr) {}
MacCameraCapture::~MacCameraCapture() { stop(); }

bool MacCameraCapture::isRunning() const
{
	RsMacCaptureHolder* h = (__bridge RsMacCaptureHolder*)_holder;
	return h != nil && h.session != nil && h.session.isRunning;
}

static AVCaptureDevice* rsPickDevice(const QString& description)
{
	if(description.isEmpty())
		return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

	for(AVCaptureDevice* d in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo])
		if(description == QString::fromUtf8(d.localizedName.UTF8String))
			return d;

	return [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
}

bool MacCameraCapture::start(const QString& description, std::function<void(const QImage&)> onFrame)
{
	stop();

	AVCaptureDevice* dev = rsPickDevice(description);
	if(!dev) { std::cerr << "VOIP: [mac] no AVCaptureDevice available" << std::endl; return false; }

	NSError* err = nil;
	AVCaptureDeviceInput* in = [AVCaptureDeviceInput deviceInputWithDevice:dev error:&err];
	if(!in) { std::cerr << "VOIP: [mac] cannot create device input" << std::endl; return false; }

	AVCaptureSession* session = [[AVCaptureSession alloc] init];
	if([session canAddInput:in]) [session addInput:in];

	AVCaptureVideoDataOutput* out = [[AVCaptureVideoDataOutput alloc] init];
	out.videoSettings = @{ (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) };
	out.alwaysDiscardsLateVideoFrames = YES;

	RsMacFrameGrabber* g = [[RsMacFrameGrabber alloc] init];
	g->cb = onFrame;
	[out setSampleBufferDelegate:g queue:dispatch_queue_create("org.retroshare.voip.camera", NULL)];
	if([session canAddOutput:out]) [session addOutput:out];

	[session startRunning];

	RsMacCaptureHolder* h = [[RsMacCaptureHolder alloc] init];
	h.session = session;
	h.grabber = g;
	_holder = (__bridge_retained void*)h;

	std::cerr << "VOIP: [mac] AVFoundation capture running=" << (int)session.isRunning
	        << " device=" << dev.localizedName.UTF8String << std::endl;
	return session.isRunning;
}

void MacCameraCapture::stop()
{
	if(!_holder) return;

	RsMacCaptureHolder* h = (__bridge_transfer RsMacCaptureHolder*)_holder;
	_holder = nullptr;

	if(h.grabber) h.grabber->cb = nullptr;   // drop the C++ callback first
	if(h.session) [h.session stopRunning];
	// ARC releases h (and its session + grabber) at end of scope.
}

void MacCameraCapture::availableDevices(QList<QString>& names)
{
	names.clear();
	for(AVCaptureDevice* d in [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo])
		names.push_back(QString::fromUtf8(d.localizedName.UTF8String));
}
