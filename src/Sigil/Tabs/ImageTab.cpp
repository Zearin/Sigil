/************************************************************************
**
**  Copyright (C) 2009, 2010, 2011  Strahinja Markovic  <strahinja.markovic@gmail.com>
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

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QLocale>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtGui/QLayout>
#include <QtWebKit/QWebView>

#include "Misc/SettingsStore.h"
#include "ResourceObjects/ImageResource.h"
#include "Tabs/ImageTab.h"

const QString IMAGE_HTML_BASE =
        "<html>"
        "<head>"
        "<style type=\"text/css\">"
        "body { -webkit-user-select: none; }"
        "img { display: block; margin-left: auto; margin-right: auto; border-style: solid; border-width: 1px; }"
        "hr { width: 75%; }"
        "div { text-align: center; }"
        "</style>"
        "<body>"
        "<p><img src=\"%1\" /></p>"
        "<hr />"
        "<div>%2&times;%3px | %4 KB | %5%6</div>"
        "</body>"
        "</html>";

ImageTab::ImageTab( ImageResource& resource, QWidget *parent )
    :
    ContentTab( resource, parent ),
    m_WebView(*new QWebView(this))
{
    m_WebView.setContextMenuPolicy(Qt::NoContextMenu);
    m_WebView.setFocusPolicy(Qt::NoFocus);
    m_WebView.setAcceptDrops(false);

    m_Layout.addWidget( &m_WebView);

    // Set the Zoom factor but be sure no signals are set because of this.
    SettingsStore settings;
    m_CurrentZoomFactor = settings.zoomImage();
    Zoom();

    ConnectSignalsToSlots();

    RefreshContent();
}

float ImageTab::GetZoomFactor() const
{
    return m_CurrentZoomFactor;
}

void ImageTab::SetZoomFactor( float new_zoom_factor )
{
    // Save the zoom for this type.
    SettingsStore settings;
    settings.setZoomImage(new_zoom_factor);
    m_CurrentZoomFactor = new_zoom_factor;

    Zoom();
    emit ZoomFactorChanged( m_CurrentZoomFactor );
}


void ImageTab::UpdateDisplay()
{
    SettingsStore settings;
    float stored_factor = settings.zoomImage();
    if ( stored_factor != m_CurrentZoomFactor )
    {
        m_CurrentZoomFactor = stored_factor;
        Zoom();
    }
}

void ImageTab::RefreshContent()
{
    QWebSettings::clearMemoryCaches();

    const QString path = m_Resource.GetFullPath();

    const QFileInfo fileInfo = QFileInfo(path);
    m_RefreshedTimestamp = fileInfo.lastModified().toMSecsSinceEpoch();

    const double ffsize = fileInfo.size() / 1024.0;
    const QString fsize = QLocale().toString(ffsize, 'f', 2);

    const QImage img(path);
    const QUrl imgUrl = QUrl::fromLocalFile(path);

    QString colorsInfo = ""; 
    if (img.depth() == 32) {
        colorsInfo = QString(" %1bpp").arg(img.bitPlaneCount());
    }
    else if (img.depth() > 0) {
        colorsInfo = QString(" %1bpp (%2 %3)").arg(img.bitPlaneCount()).arg(img.colorCount()).arg(tr("colors"));
    }

    const QString html = IMAGE_HTML_BASE.arg(imgUrl.toString()).arg(img.width()).arg(img.height()).arg(fsize)
            .arg(img.isGrayscale() ? "Grayscale" : "Color").arg(colorsInfo);
    m_WebView.setHtml(html, imgUrl);
}

void ImageTab::ImageFileModified()
{
    const QFileInfo fileInfo = QFileInfo( m_Resource.GetFullPath() );

    const qint64 lastModified = fileInfo.lastModified().toMSecsSinceEpoch();
    if ( lastModified == m_RefreshedTimestamp )
    {
        return;
    }

    // It's best to wait a bit before triggering the actual page refresh,
    // in case the file has not been completely written to disk. The image tab will take
    // a bit to become up-to-date with the file on disk, but it's just an informational tab
    // and instant reaction to file changes is not critical.
    // - If the file is empty, then the file-modified signal was received
    //   exactly when the editor application truncated the file, but before
    //   writing any data to it, so we'll be extra patient.
    // - If the file is larger than 512k (unlikely for images in an ebook, but possible),
    //   it might get even larger than that, and the editor application
    //   might take a long time (ms-wise) to write it, so we'll be extra patient again.
    // The values below are mostly guesswork derived from how things go on my machine;
    // they may not work just as well on slower (disk/cpu) systems.

    const int delay = 500 + ( fileInfo.size() == 0 ? 750 : 0); // + ( fileInfo.size() > 512 * 1024 ? 500 : 0 );

    if ( QDateTime::currentMSecsSinceEpoch() - lastModified < delay )
    {
        QTimer::singleShot( delay, this, SLOT( ImageFileModified() ) );
    }
    else
    {
        QTimer::singleShot( 0, this, SLOT( RefreshContent() ) );
    }
}

void ImageTab::ConnectSignalsToSlots()
{
    connect(&m_Resource, SIGNAL(Modified()), this, SLOT(ImageFileModified()), Qt::QueuedConnection);
    connect(&m_Resource, SIGNAL(Deleted(Resource)), this, SLOT(Close()));
}

void ImageTab::Zoom()
{
    m_WebView.setZoomFactor(m_CurrentZoomFactor);
}
