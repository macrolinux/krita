/*
 *  Copyright (c) 2002 Patrick Julien <freak@codepimps.org>
 *  Copyright (c) 2004 Boudewijn Rempt <boud@valdyas.org>
 *  Copyright (c) 2004 Clarence Dang <dang@kde.org>
 *  Copyright (c) 2004 Adrian Page <adrian@pagenet.plus.com>
 *  Copyright (c) 2004 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_filterop.h"
#include <QDomElement>
#include <QRect>

#include <kis_debug.h>

#include <KoColorSpace.h>
#include <KoCompositeOp.h>
#include <KoInputDevice.h>

#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_filter.h>
#include <kis_filter_configuration.h>
#include <kis_filter_config_widget.h>
#include <kis_filter_processing_information.h>
#include <kis_filter_registry.h>
#include <kis_layer.h>
#include <kis_types.h>
#include <kis_iterators_pixel.h>
#include <kis_paintop.h>
#include <kis_selection.h>

#include "ui_FilterOpOptionsWidget.h"

KisPaintOp * KisFilterOpFactory::createOp(const KisPaintOpSettings *_settings, KisPainter * _painter, KisImageSP _image)
{
    Q_UNUSED(_image);
    const KisFilterOpSettings* settings = dynamic_cast<const KisFilterOpSettings*>(_settings);
    Q_ASSERT(settings);
    KisPaintOp * op = new KisFilterOp(settings, _painter);
    return op;
}

KisPaintOpSettings *KisFilterOpFactory::settings(QWidget * parent, const KoInputDevice& inputDevice, KisImageSP /*image*/)
{
    Q_UNUSED(inputDevice);
    return new KisFilterOpSettings(parent);
}

KisPaintOpSettings* KisFilterOpFactory::settings(KisImageSP image)
{
    return new KisFilterOpSettings(0);
}

KisFilterOpSettings::KisFilterOpSettings(QWidget* parent) :
        QObject(parent),
        KisPaintOpSettings(),
        m_optionsWidget(new QWidget(parent)),
        m_uiOptions(new Ui_FilterOpOptions()),
        m_currentFilterConfigWidget(0)
{
    m_uiOptions->setupUi(m_optionsWidget);

    // Check which filters support painting
    QList<KoID> l = KisFilterRegistry::instance()->listKeys();
    QList<KoID> l2;
    QList<KoID>::iterator it;
    for (it = l.begin(); it !=  l.end(); ++it) {
        KisFilterSP f = KisFilterRegistry::instance()->value((*it).id());
        if (f->supportsPainting()) {
            l2.push_back(*it);
        }
    }
    m_uiOptions->filtersList->setIDList( l2 );
    connect(m_uiOptions->filtersList, SIGNAL(activated(const KoID &)), SLOT(setCurrentFilter(const KoID &)));
    if(!l2.empty())
    {
        setCurrentFilter( l2.first() );
    }
}

void KisFilterOpSettings::setLayer( KisLayerSP layer )
{
    if(layer)
    {
        m_paintDevice = layer->paintDevice();
        // The "not m_currentFilterConfigWidget" is a corner case
        // which happen because the first configuration settings is
        // created before any layer is selected in the view
        if( !m_currentFilterConfigWidget ||
            ( m_currentFilterConfigWidget && m_currentFilterConfigWidget->configuration()->isCompatible(m_paintDevice)) )
        {
            if(m_currentFilter)
            {
                setCurrentFilter(KoID(m_currentFilter->id()));
            }
        }
    }
    else
        m_paintDevice = 0;
}

KisFilterOpSettings::~KisFilterOpSettings()
{
    delete m_uiOptions;
}

void KisFilterOpSettings::setCurrentFilter(const KoID & id)
{
    m_currentFilter = KisFilterRegistry::instance()->get(id.id());
    Q_ASSERT( m_currentFilter );
    if(m_paintDevice)
    {
        m_currentFilterConfigWidget = m_currentFilter->createConfigurationWidget(0, m_paintDevice);
        m_uiOptions->popupButtonOptions->setPopupWidget(m_currentFilterConfigWidget);
    } else {
        m_currentFilterConfigWidget = 0;
    }
}

const KisFilterSP KisFilterOpSettings::filter() const
{
    return m_currentFilter;
}

KisFilterConfiguration* KisFilterOpSettings::filterConfig() const
{
    if(!m_currentFilterConfigWidget) return 0;
    return m_currentFilterConfigWidget->configuration();
}

KisPaintOpSettings* KisFilterOpSettings::clone() const
{
    KisFilterOpSettings* s = new KisFilterOpSettings(0);
    s->m_paintDevice = m_paintDevice;
    s->setCurrentFilter( KoID(m_currentFilter->id()) );
    if(s->m_currentFilterConfigWidget && m_currentFilterConfigWidget)
    {
        s->m_currentFilterConfigWidget->setConfiguration( m_currentFilterConfigWidget->configuration() );
    }
    return s;
}

void KisFilterOpSettings::fromXML(const QDomElement& elt)
{
    QDomElement e = elt.firstChildElement( "Filter" );
    if( !e.isNull() )
    {
        QString filterName = e.attribute("name");
        m_currentFilter = KisFilterRegistry::instance()->get(filterName);
        if(m_currentFilter)
        {
            delete m_currentFilterConfigWidget;
            m_currentFilterConfigWidget = m_currentFilter->createConfigurationWidget(m_optionsWidget, m_paintDevice );
            KisFilterConfiguration * kfc = m_currentFilter->defaultConfiguration(m_paintDevice);
            if(kfc && m_currentFilterConfigWidget)
            {
                kfc->fromXML( e );
                m_currentFilterConfigWidget->setConfiguration( kfc );
            }
        }
    }
}

void KisFilterOpSettings::toXML(QDomDocument& doc, QDomElement& rootElt) const
{
    QDomElement filterElt = doc.createElement( "Filter" );
    rootElt.appendChild( filterElt );
    if( m_currentFilterConfigWidget )
    {
        KisFilterConfiguration* config = m_currentFilterConfigWidget->configuration();
        config->toXML( doc, filterElt );
        delete config;
    }
}

KisFilterOp::KisFilterOp(const KisFilterOpSettings* settings, KisPainter * painter)
    : KisPaintOp(painter), m_settings(settings)
{
}

KisFilterOp::~KisFilterOp()
{
}

void KisFilterOp::paintAt(const KisPaintInformation& info)
{
    if (!painter())
    {
      return;
    }

    KisFilterSP filter = m_settings->filter();
    if (!filter)
    {
      return;
    }

    if (!source() )
    {
      return;
    }

    KisBrush * brush = painter()->brush();
    if (!brush) return;

    const KoColorSpace * colorSpace = source()->colorSpace();

    double scale = KisPaintOp::scaleForPressure( info.pressure() );
    QPointF hotSpot = brush->hotSpot(scale, scale);
    QPointF pt = info.pos() - hotSpot;

    // Split the coordinates into integer plus fractional parts. The integer
    // is where the dab will be positioned and the fractional part determines
    // the sub-pixel positioning.
    qint32 x;
    double xFraction;
    qint32 y;
    double yFraction;

    splitCoordinate(pt.x(), &x, &xFraction);
    splitCoordinate(pt.y(), &y, &yFraction);

    // Filters always work with a mask, never with an image; that
    // wouldn't be useful at all.
//     KisQImagemaskSP mask = brush->mask(info, xFraction, yFraction);

    painter()->setPressure(info.pressure());

    qint32 maskWidth = brush->maskWidth(scale, 0.0);
    qint32 maskHeight = brush->maskHeight(scale, 0.0);

    // Create a temporary paint device
    KisPaintDeviceSP tmpDev = new KisPaintDevice(colorSpace, "filterop tmpdev");
    Q_CHECK_PTR(tmpDev);

    // Copy the layer data onto the new paint device
#if 0
    KisPainter p( tmpDev );
    p.bitBlt( 0,  0,  colorSpace->compositeOp(COMPOSITE_COPY), source(), OPACITY_OPAQUE, x, y, maskWidth, maskHeight );

    // Filter the paint device
    filter->disableProgress();
    QRect r( 0, 0, maskWidth, maskHeight );
    filter->process( tmpDev, r, m_settings->filterConfig());
    filter->enableProgress();
#endif

    // Filter the paint device
    filter->process( KisFilterConstProcessingInformation( source(), QPoint(x,y)), KisFilterProcessingInformation(tmpDev, QPoint(0,0) ), QSize(maskWidth, maskHeight), m_settings->filterConfig());

    // Apply the mask on the paint device (filter before mask because edge pixels may be important)

    brush->mask(tmpDev, scale, scale, 0.0, info, xFraction, yFraction);
    
#if 0
    KisHLineIterator hiter = tmpDev->createHLineIterator(0, 0, maskWidth);

    for (int y = 0; y < maskHeight; y++)
    {
        int x=0;
        while(! hiter.isDone())
        {
            quint8 alpha = mask->alphaAt( x++, y );
            colorSpace->setAlpha(hiter.rawData(), alpha, 1);

            ++hiter;
        }
        hiter.nextRow();

    }
#endif

    // Blit the paint device onto the layer
    QRect dabRect = QRect(0, 0, maskWidth, maskHeight);
    QRect dstRect = QRect(x, y, dabRect.width(), dabRect.height());

    if (painter()->bounds().isValid()) {
        dstRect &= painter()->bounds();
    }
    if (dstRect.isNull() || dstRect.isEmpty() || !dstRect.isValid()) return;

    qint32 sx = dstRect.x() - x;
    qint32 sy = dstRect.y() - y;
    qint32 sw = dstRect.width();
    qint32 sh = dstRect.height();

    painter()->bltSelection(dstRect.x(), dstRect.y(), painter()->compositeOp(), tmpDev, painter()->opacity(), sx, sy, sw, sh);
}

#include "kis_filterop.moc"
