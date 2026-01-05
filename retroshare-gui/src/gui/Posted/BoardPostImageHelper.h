/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardPostImageHelper.h                        *
 *                                                                             *
 * Copyright (C) 2025 RetroShare Team          <retroshare.project@gmail.com> *
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

#pragma once

#include <QMovie>
#include <QPixmap>
#include <QString>
#include <cstdint>

/**
 * Helper class for loading and detecting animated images in Board posts.
 * Supports animated GIF and WEBP formats with size limits.
 */
class BoardPostImageHelper
{
public:
    // Maximum allowed size for animated images (194KB as per issue #3095)
    static const uint32_t MAX_ANIMATED_SIZE = 194 * 1024;

    /**
     * Detect if image data contains an animated GIF or WEBP.
     * @param data Raw image data
     * @param size Size of image data in bytes
     * @param format Optional output parameter for detected format (GIF/WEBP)
     * @return true if image is animated and within size limit
     */
    static bool isAnimatedImage(const uint8_t* data, uint32_t size, QString* format = nullptr);

    /**
     * Create QMovie object from raw image data for animation playback.
     * Caller is responsible for memory management (delete when done).
     * @param data Raw image data
     * @param size Size of image data in bytes
     * @return QMovie pointer or nullptr on failure
     */
    static QMovie* createMovieFromData(const uint8_t* data, uint32_t size);

private:
    BoardPostImageHelper() = delete; // Utility class, no instantiation
};
