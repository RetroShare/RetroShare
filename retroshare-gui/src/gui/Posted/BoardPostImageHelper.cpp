/*******************************************************************************
 * retroshare-gui/src/gui/Posted/BoardPostImageHelper.cpp                      *
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

#include "BoardPostImageHelper.h"
#include <QBuffer>
#include <QImageReader>
#include <QMovie>
#include <QString>

bool BoardPostImageHelper::isAnimatedImage(const uint8_t* data, uint32_t size, QString* format)
{
    if (!data || size == 0 || size > MAX_ANIMATED_SIZE)
        return false;

    // Create QByteArray from raw data (no copy, just wraps pointer)
    QByteArray imageData = QByteArray::fromRawData(reinterpret_cast<const char*>(data), size);
    QBuffer buffer;
    buffer.setData(imageData);
    buffer.open(QIODevice::ReadOnly);

    QImageReader reader(&buffer);
    QString detectedFormat = QString(reader.format()).toUpper();

    // Check if format is GIF or WEBP (case-insensitive)
    if (detectedFormat != "GIF" && detectedFormat != "WEBP")
        return false;

    // Check frame count - more than 1 frame means animation
    int frameCount = reader.imageCount();
    
    // For WEBP, imageCount() might return 0 if plugin doesn't support it
    // In that case, assume it's not animated
    if (frameCount <= 0)
        return false;
        
    bool isAnimated = (frameCount > 1);

    if (isAnimated && format)
        *format = detectedFormat;

    return isAnimated;
}

QMovie* BoardPostImageHelper::createMovieFromData(const uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
        return nullptr;

    // Create persistent QByteArray (QMovie needs data to persist)
    QByteArray* imageData = new QByteArray(reinterpret_cast<const char*>(data), size);
    
    QBuffer* buffer = new QBuffer(imageData);
    buffer->open(QIODevice::ReadOnly);

    QMovie* movie = new QMovie();
    movie->setDevice(buffer);

    // Set buffer and data as children of movie for automatic cleanup
    buffer->setParent(movie);
    
    // Verify movie is valid
    if (!movie->isValid()) {
        delete movie;
        delete imageData;
        return nullptr;
    }

    return movie;
}
