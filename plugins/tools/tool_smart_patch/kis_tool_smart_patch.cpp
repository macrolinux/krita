/*
 *  Copyright (c) 2017 Eugene Ingerman
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

#include "kis_tool_smart_patch.h"

#include "QApplication"

#include <klocalizedstring.h>
#include <KoCanvasBase.h>

#include <KisViewManager.h>
#include "kis_canvas2.h"
#include "kis_cursor.h"
#include "kis_config.h"
#include "kundo2magicstring.h"

#include "KoProperties.h"
#include "KoColorSpaceRegistry.h"
#include "KoShapeController.h"
#include "KoDocumentResourceManager.h"
#include "kis_node_manager.h"
#include "kis_cursor.h"

#include "kis_tool_smart_patch_options_widget.h"
#include "libs/image/kis_paint_device_debug_utils.h"

#include "kis_resources_snapshot.h"
#include "kis_layer.h"
#include "kis_transaction.h"
#include "kis_paint_layer.h"

#include "kis_inpaint_mask.h"

QRect patchImage(KisPaintDeviceSP imageDev, KisPaintDeviceSP maskDev, int radius, int accuracy);

struct KisToolSmartPatch::Private {
    KisMaskSP mask = nullptr;
    KisNodeSP maskNode = nullptr;
    KisNodeSP paintNode = nullptr;
    KisPaintDeviceSP imageDev = nullptr;
    KisPaintDeviceSP maskDev = nullptr;
    KisResourcesSnapshotSP resources = nullptr;
    KoColor currentFgColor;
    KisToolSmartPatchOptionsWidget *optionsWidget = nullptr;
};


KisToolSmartPatch::KisToolSmartPatch(KoCanvasBase * canvas)
    : KisToolFreehand(canvas,
                      KisCursor::load("tool_freehand_cursor.png", 5, 5),
                      kundo2_i18n("Smart Patch Stroke")),
      m_d(new Private)
{
    setObjectName("tool_SmartPatch");
}

KisToolSmartPatch::~KisToolSmartPatch()
{
    m_d->optionsWidget = nullptr;
}

void KisToolSmartPatch::activate(ToolActivation activation, const QSet<KoShape*> &shapes)
{
    KisToolFreehand::activate(activation, shapes);
}

void KisToolSmartPatch::deactivate()
{
    KisToolFreehand::deactivate();
}

void KisToolSmartPatch::resetCursorStyle()
{
    KisToolFreehand::resetCursorStyle();
}


bool KisToolSmartPatch::canCreateInpaintMask() const
{
    KisNodeSP node = currentNode();
    return node && node->inherits("KisPaintLayer");
}

QRect KisToolSmartPatch::inpaintImage(KisPaintDeviceSP maskDev, KisPaintDeviceSP imageDev)
{
    int accuracy = 50; //default accuracy - middle value
    int patchRadius = 4; //default radius, which works well for most cases tested

    if (!m_d.isNull() && m_d->optionsWidget) {
        accuracy = m_d->optionsWidget->getAccuracy();
        patchRadius = m_d->optionsWidget->getPatchRadius();
    }
    return patchImage(imageDev, maskDev, patchRadius, accuracy);
}

void KisToolSmartPatch::activatePrimaryAction()
{
    KisToolFreehand::activatePrimaryAction();
}

void KisToolSmartPatch::deactivatePrimaryAction()
{
    KisToolFreehand::deactivatePrimaryAction();
}

void KisToolSmartPatch::createInpaintMask(void)
{
    m_d->mask = new KisInpaintMask();

    KisLayerSP parentLayer = qobject_cast<KisLayer*>(m_d->paintNode.data());
    m_d->mask->initSelection(parentLayer);
    image()->addNode(m_d->mask, m_d->paintNode);
}

void KisToolSmartPatch::deleteInpaintMask(void)
{
    KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
    KisViewManager* viewManager = kiscanvas->viewManager();
    if (! m_d->paintNode.isNull())
        viewManager->nodeManager()->slotNonUiActivatedNode(m_d->paintNode);

    image()->removeNode(m_d->mask);
    m_d->mask = nullptr;
}

void KisToolSmartPatch::beginPrimaryAction(KoPointerEvent *event)
{
    m_d->paintNode = currentNode();

    KisCanvas2 * kiscanvas = static_cast<KisCanvas2*>(canvas());
    KisViewManager* viewManager = kiscanvas->viewManager();

    //we can only apply inpaint operation to paint layer
    if (!m_d->paintNode.isNull() && m_d->paintNode->inherits("KisPaintLayer")) {


        if (!m_d->mask.isNull()) {
            viewManager->nodeManager()->slotNonUiActivatedNode(m_d->mask);
        } else {

            createInpaintMask();
            viewManager->nodeManager()->slotNonUiActivatedNode(m_d->mask);

            //Collapse freehand drawing of the mask followed by inpaint operation into a single undo node
            canvas()->shapeController()->resourceManager()->undoStack()->beginMacro(kundo2_i18n("Smart Patch"));


            //User will be drawing on an alpha mask. Show color matching inpaint mask color.
            m_d->currentFgColor = canvas()->resourceManager()->foregroundColor();
            canvas()->resourceManager()->setForegroundColor(KoColor(Qt::magenta, image()->colorSpace()));
        }
        KisToolFreehand::beginPrimaryAction(event);
    } else {
        viewManager->
        showFloatingMessage(
            i18n("Select a paint layer to use this tool"),
            QIcon(), 2000, KisFloatingMessage::Medium, Qt::AlignCenter);
    }
}

void KisToolSmartPatch::continuePrimaryAction(KoPointerEvent *event)
{
    if (!m_d->mask.isNull())
        KisToolFreehand::continuePrimaryAction(event);
}


void KisToolSmartPatch::endPrimaryAction(KoPointerEvent *event)
{
    if (mode() != KisTool::PAINT_MODE)
        return;

    if (m_d->mask.isNull())
        return;

    KisToolFreehand::endPrimaryAction(event);

    //Next line is important. We need to wait for the paint operation to finish otherwise
    //mask will be incomplete.
    image()->waitForDone();

    //User drew a mask on the temporary inpaint mask layer. Get this mask to pass to the inpaint algorithm
    m_d->maskDev = new KisPaintDevice(KoColorSpaceRegistry::instance()->alpha8());

    if (!m_d->mask.isNull()) {
        m_d->maskDev->makeCloneFrom(m_d->mask->paintDevice(), m_d->mask->paintDevice()->extent());

        //Once we get the mask we delete the temporary layer on which user painted it
        deleteInpaintMask();

        image()->waitForDone();
        m_d->imageDev = currentNode()->paintDevice();

        KisTransaction inpaintTransaction(kundo2_i18n("Inpaint Operation"), m_d->imageDev);

        QApplication::setOverrideCursor(KisCursor::waitCursor());

        //actual inpaint operation. filling in areas masked by user
        QRect changedRect = inpaintImage(m_d->maskDev, m_d->imageDev);
        currentNode()->setDirty(changedRect);
        inpaintTransaction.commit(image()->undoAdapter());

        //Matching endmacro for inpaint operation
        canvas()->shapeController()->resourceManager()->undoStack()->endMacro();

        QApplication::restoreOverrideCursor();

        canvas()->resourceManager()->setForegroundColor(m_d->currentFgColor);
    }
}

QWidget * KisToolSmartPatch::createOptionWidget()
{
    KisCanvas2 * kiscanvas = dynamic_cast<KisCanvas2*>(canvas());

    m_d->optionsWidget = new KisToolSmartPatchOptionsWidget(kiscanvas->viewManager()->resourceProvider(), 0);
    m_d->optionsWidget->setObjectName(toolId() + "option widget");

    return m_d->optionsWidget;
}

