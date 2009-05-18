/***************************************************************************
 *   Copyright (C) 2009                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef RETROSHARE_LINK_ANALYZER
#define RETROSHARE_LINK_ANALYZER

#include <QString>
#include <QVector>

struct RetroShareLinkData
{
public:
    RetroShareLinkData () {}
    RetroShareLinkData (const QString& n, const QString& s, const QString& h)
        : name (n)
        , size (s)
        , hash (h)
    {}

    const QString& getName () const {return name;}
    const QString& getSize () const {return size;}
    const QString& getHash () const {return hash;}

private:
    QString name;
    QString size;
    QString hash;
};

class RetroShareLinkAnalyzer
{
public:
    RetroShareLinkAnalyzer(const QString& url = "");

    void setRetroShareLink(const QString& name, const QString& size, const QString& hash);
    void setRetroShareLink(const QString& url) {this->url = url;}
    const QString& getRetroShareLink() const {return url;}

    bool isValid() const;

    // Get file name, size, hash from kommute link list.
    // Return true if kommute link is valid, otherwise return false.
    bool getFileInformation(QVector<RetroShareLinkData>& linkList);

private:
    enum PARTITION
    {
        HEADER_,
        NAME_,
        SIZE_,
        HASH_,
        NUMBER_OF_PARTITIONS
    };

    static const QString HEADER_NAME;

    QString url;
};

#endif
