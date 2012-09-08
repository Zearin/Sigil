/************************************************************************
**
**  Copyright (C) 2012  Daniel Pavel <daniel.pavel@gmail.com>
**
**  This file is part of Sigil.
**
**  Sigil is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Sigil is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Sigil.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#include <QtCore/QProcess>
#include <QtGui/QDesktopServices>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include "Misc/OpenExternally.h"

// resource types that are watched for modifications from outside Sigil
// only watch certain types of resources (auxiliaries)
// (X)HTML files would have to go through the validator first
static const int WATCHED_RESOURCE_TYPES = Resource::ImageResourceType |
                                          Resource::CSSResourceType   |
                                          Resource::SVGResourceType   |
                                          Resource::FontResourceType;

bool OpenExternally::mayOpen( const Resource::ResourceType type )
{
    return type & ( WATCHED_RESOURCE_TYPES );
}

bool OpenExternally::openFile( const QString& filePath, const QString& application )
{
    if ( QFile::exists( filePath ) && QFile::exists( application ) )
    {
        QStringList arguments = QStringList( QDir::toNativeSeparators(filePath) );
        return QProcess::startDetached( QDir::toNativeSeparators(application), arguments, QFileInfo(filePath).absolutePath() );
    }

    return false;
}
