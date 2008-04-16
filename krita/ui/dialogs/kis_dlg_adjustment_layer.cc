/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
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

#include "kis_dlg_adjustment_layer.h"
#include <klocale.h>

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QGridLayout>

#include <klineedit.h>
#include <klocale.h>


#include "filter/kis_filter.h"
#include "filter/kis_filter_config_widget.h"
#include "filter/kis_filter_configuration.h"
#include "kis_paint_device.h"
#include "widgets/kis_previewwidget.h"
#include "kis_transaction.h"

KisDlgAdjustmentLayer::KisDlgAdjustmentLayer(KisPaintDeviceSP device,
                                             const QString & /*layerName*/,
                                             const QString & caption,
                                             QWidget *parent,
                                             const char *name)
    : KDialog(parent)
    , m_dev(device)
    , m_currentFilter(0)
    , m_customName(false)
    , m_freezeName(false)
{
    setCaption( caption );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    setObjectName(name);

    Q_ASSERT( m_dev );

    QWidget * page = new QWidget(this);
    page->setObjectName("page widget");
    QGridLayout * grid = new QGridLayout(page);
    grid->setSpacing(6);
    setMainWidget(page);

    QLabel * lblName = new QLabel(i18n("Name:"), page);
    lblName->setObjectName("lblName");
    grid->addWidget(lblName, 0, 0);

    m_layerName = new KLineEdit(page);
    m_layerName->setObjectName("m_layerName");
    grid->addWidget(m_layerName, 0, 1);
    connect( m_layerName, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotNameChanged( const QString & ) ) );

    m_preview = new KisPreviewWidget(page, "dlgadjustment.preview");
    m_preview->slotSetDevice( m_dev );

    connect( m_preview, SIGNAL(updated()), this, SLOT(refreshPreview()));
    grid->addWidget(m_preview, 1, 1);

    m_configWidgetHolder = new QGroupBox(i18n("Configuration"), page);
    m_configWidgetHolder->setObjectName("currentConfigWidget");
    QVBoxLayout *configWidgetHolderLayout = new QVBoxLayout();
    m_configWidgetHolder->setLayout(configWidgetHolderLayout);
    grid->addWidget(m_configWidgetHolder, 2, 1);

    m_labelNoConfigWidget = new QLabel(i18n("No configuration options are available for this filter"),
                                        m_configWidgetHolder);
    m_configWidgetHolder->layout()->addWidget(m_labelNoConfigWidget);
    m_labelNoConfigWidget->hide();

    resize( QSize(600, 480).expandedTo(minimumSizeHint()) );

    m_currentConfigWidget = 0;

    enableButtonOk(0);
}

void KisDlgAdjustmentLayer::slotNameChanged( const QString & text )
{
    if (m_freezeName)
        return;

    m_customName = !text.isEmpty();
    enableButtonOk( m_currentFilter && m_customName );
}

KisFilterConfiguration * KisDlgAdjustmentLayer::filterConfiguration() const
{
    if(m_currentConfigWidget)
        return m_currentConfigWidget->configuration();
    else
        return m_currentFilter->defaultConfiguration( 0);
}

QString KisDlgAdjustmentLayer::layerName() const
{
    return m_layerName->text();
}

void KisDlgAdjustmentLayer::slotConfigChanged()
{
    if(m_preview->getAutoUpdate())
    {
        refreshPreview();
    } else {
        m_preview->needUpdate();
    }
}

void KisDlgAdjustmentLayer::refreshPreview()
{
    KisPaintDeviceSP layer =  m_preview->getDevice();

    KisTransaction cmd("Temporary transaction", layer);
    KisFilterConfiguration* config = m_currentConfigWidget->configuration();

    QRect rect = layer->extent();
    m_currentFilter->process(layer, rect, config);
    m_preview->slotUpdate();
    cmd.undo();
}

void KisDlgAdjustmentLayer::selectionHasChanged ( QListWidgetItem * item )
{
/*
    KisFiltersIconViewItem* kisitem = (KisFiltersIconViewItem*) item;

    m_currentFilter = kisitem->filter();

    if ( m_currentConfigWidget != 0 )
    {
        m_configWidgetHolder->layout()->removeWidget(m_currentConfigWidget);

        delete m_currentConfigWidget;
        m_currentConfigWidget = 0;

    } else {

        m_labelNoConfigWidget->hide();
    }

    if (m_dev) {
        m_currentConfigWidget = m_currentFilter->createConfigurationWidget(m_configWidgetHolder,
                                                                           m_dev);
        if(m_currentConfigWidget)
        {
            m_currentConfigWidget->setConfiguration( kisitem->filterConfiguration() );
        }
    }

    if (m_configWidgetHolder != 0 && m_currentConfigWidget != 0)
    {
        m_configWidgetHolder->layout()->addWidget(m_currentConfigWidget);
        m_currentConfigWidget->show();
        connect(m_currentConfigWidget, SIGNAL(sigPleaseUpdatePreview()), this, SLOT(slotConfigChanged()));
    } else {
        m_labelNoConfigWidget->show();
    }

    if (!m_customName) {
        m_freezeName = true;
        m_layerName->setText(m_currentFilter->name());
        m_freezeName = false;
    }

    enableButtonOk( !m_layerName->text().isEmpty() );
    if(m_currentConfigWidget)
        refreshPreview();
*/        
}

#include "kis_dlg_adjustment_layer.moc"
