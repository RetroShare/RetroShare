/*******************************************************************************
 * plugins/VOIP/gui/MacCameraPermission.mm                                     *
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
#import <dispatch/dispatch.h>

#include "MacCameraPermission.h"
#include <iostream>

void requestMacCameraAccess(std::function<void(bool)> cb)
{
    AVAuthorizationStatus st =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    std::cerr << "VOIP: [mac] AVFoundation camera authorizationStatus=" << (int)st
            << " (0=NotDetermined,1=Restricted,2=Denied,3=Authorized)" << std::endl;

    switch(st)
    {
    case AVAuthorizationStatusAuthorized:
        cb(true);
        return;

    case AVAuthorizationStatusDenied:
    case AVAuthorizationStatusRestricted:
        cb(false);
        return;

    case AVAuthorizationStatusNotDetermined:
    default:
        std::cerr << "VOIP: [mac] requesting camera access from AVFoundation "
                   "(system dialog should appear now)" << std::endl;
        // The completion handler runs on an arbitrary AVFoundation queue; hop back
        // to the main thread before touching Qt objects in the callback.
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                                 completionHandler:^(BOOL granted)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
                std::cerr << "VOIP: [mac] AVFoundation camera access granted="
                        << (granted ? 1 : 0) << std::endl;
                cb(granted ? true : false);
            });
        }];
        return;
    }
}
