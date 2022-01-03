/*******************************************************************************
 * gui/statistics/Histogram.cpp                                                *
 *                                                                             *
 * Copyright (c) 2020 Retroshare Team <retroshare.project@gmail.com>           *
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

#include <math.h>

#include "Histogram.h"

Histogram::Histogram()
	: mStart(0),mEnd(1.0),mBins(10,0)
{}

Histogram::Histogram(double start, double end, int bins)
	: mStart(start),mEnd(end),mBins(bins,0)
{
	if(mEnd <= mStart)
		std::cerr << "Null histogram created! Please check your parameters" << std::endl;
}

void Histogram::draw(QPainter */*painter*/) const
{
}

void Histogram::insert(double val)
{
    long int bin = (uint32_t)floor((val - mStart)/(mEnd - mStart) * mBins.size());

    if(bin >= 0 && bin < mBins.size())
		++mBins[bin];
}

std::ostream& operator<<(std::ostream& o,const Histogram& h)
{
	o << "Histogram: [" << h.mStart << "..." << h.mEnd << "] " << h.mBins.size() << " bins." << std::endl;
	for(uint32_t i=0;i<h.mBins.size();++i)
		o << "  " << h.mStart + i*(double)(h.mEnd - h.mStart)/(double)h.mBins.size() << " : " << h.mBins[i] << std::endl;

    return o;
}
