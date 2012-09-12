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
#include <QtGui/QFileDialog>

#include "Misc/OpenExternally.h"
#include "Misc/SettingsStore.h"


static const QString SETTINGS_GROUP = "open_with";
static const QString EMPTY;

// resource types that are watched for modifications from outside Sigil
// only watch certain types of resources (auxiliaries)
// (X)HTML files would have to go through the validator first
static const int WATCHED_RESOURCE_TYPES = Resource::CSSResourceType   |
                                          Resource::ImageResourceType |
                                          Resource::SVGResourceType   |
                                          Resource::FontResourceType;

// not very elegant, but much lighter than a std::map (and with less initialization trouble)
// the switch _must_ always be in sync with WATCHED_RESOURCE_TYPES
static inline const char* const RESOURCE_TYPE_NAME( const Resource::ResourceType type )
{
    switch (type)
    {
    case Resource::CSSResourceType:     return "stylesheet";
    case Resource::ImageResourceType:   return "image";
    case Resource::SVGResourceType:     return "SVG";
    case Resource::FontResourceType:    return "font";
    default:                            return "";
    }
}


bool OpenExternally::mayOpen( const Resource::ResourceType type )
{
    return type & ( WATCHED_RESOURCE_TYPES );
}

bool OpenExternally::openFile( const QString& filePath, const QString& application )
{
#if defined(Q_WS_MAC)
    if ( QFile::exists( filePath ) && QDir( application ).exists() )
    {
        QStringList arguments = QStringList() << "-a" << application << filePath;
        return QProcess::startDetached( "/usr/bin/open", arguments );
    }
#else
    if ( QFile::exists( filePath ) && QFile::exists( application ) )
    {
        QStringList arguments = QStringList( QDir::toNativeSeparators(filePath) );
        return QProcess::startDetached( QDir::toNativeSeparators(application), arguments, QFileInfo(filePath).absolutePath() );
    }
#endif

    return false;
}

const QString OpenExternally::editorForResourceType( const Resource::ResourceType type )
{
    if ( mayOpen( type ) )
    {
        SettingsStore settings;
        settings.beginGroup( SETTINGS_GROUP );

        const QString& editorKey = QString("editor_") + RESOURCE_TYPE_NAME(type);
        if ( settings.contains( editorKey ) )
        {
            const QString& editorPath = settings.value(editorKey).toString();
            return QFile::exists( editorPath ) ? editorPath : EMPTY;
        }
    }

    return EMPTY;
}


const QString OpenExternally::prettyApplicationName( const QString& applicationpath )
{
    return QFileInfo( applicationpath ).completeBaseName();
}

const QString OpenExternally::selectEditorForResourceType( const Resource::ResourceType type )
{
    if ( !mayOpen( type ) )
    {
        return EMPTY;
    }

    const QString& editorKey = QString("editor_") + RESOURCE_TYPE_NAME(type);

    SettingsStore settings;
    settings.beginGroup( SETTINGS_GROUP );

    static QString LAST_LOCATION = QDesktopServices::storageLocation( QDesktopServices::ApplicationsLocation );

    QString lastEditor = settings.value( editorKey ).toString();
    if ( !QFile::exists(lastEditor) )
    {
        lastEditor = LAST_LOCATION;
        if ( !QFile::exists(lastEditor) )
        {
            lastEditor = LAST_LOCATION = QDesktopServices::storageLocation( QDesktopServices::HomeLocation );
        }
    }

    static const QString NAME_FILTER = QObject::tr("Applications")
#if defined(Q_WS_WIN)
            + " (*.exe *.com *.bat *.cmd)"
#elif defined(Q_WS_MAC)
            + " (*.app)"
#elif defined(Q_WS_X11)
            + " (*)"
#endif
    ;

#if defined(USE_NATIVE_DIALOG)
    const QString selectedFile = QFileDialog::getOpenFileName(0,
                                                               QObject::tr("Open With"),
                                                               lastEditor,
                                                               NAME_FILTER,
                                                               0,
                                                               QFileDialog::ReadOnly | QFileDialog::HideNameFilterDetails);
#else
    QFileDialog fileDialog(0, QObject::tr("Open With"), lastEditor, NAME_FILTER);
    fileDialog.setOptions( QFileDialog::ReadOnly | QFileDialog::HideNameFilterDetails );
    fileDialog.setFileMode( QFileDialog::ExistingFile );

    const QString selectedFile = fileDialog.exec() ? fileDialog.selectedFiles().first() : EMPTY;
#endif

    if ( !selectedFile.isEmpty() )
    {
        settings.setValue( editorKey, selectedFile );
        LAST_LOCATION = selectedFile;
    }

    return selectedFile;
}
