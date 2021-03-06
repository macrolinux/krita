/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
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

#include "KoShapeResizeCommand.h"

#include <KoShape.h>
#include "kis_command_ids.h"


struct Q_DECL_HIDDEN KoShapeResizeCommand::Private
{
    QList<KoShape *> shapes;
    qreal scaleX;
    qreal scaleY;
    QPointF absoluteStillPoint;
    bool useGlobalMode;
    bool usePostScaling;
    QTransform postScalingCoveringTransform;

    QList<QSizeF> oldSizes;
    QList<QTransform> oldTransforms;
};


KoShapeResizeCommand::KoShapeResizeCommand(const QList<KoShape*> &shapes,
                                           qreal scaleX, qreal scaleY,
                                           const QPointF &absoluteStillPoint,
                                           bool useGLobalMode,
                                           bool usePostScaling,
                                           const QTransform &postScalingCoveringTransform,
                                           KUndo2Command *parent)
    : SkipFirstRedoBase(false, kundo2_i18n("Resize"), parent),
      m_d(new Private)
{
    m_d->shapes = shapes;
    m_d->scaleX = scaleX;
    m_d->scaleY = scaleY;
    m_d->absoluteStillPoint = absoluteStillPoint;
    m_d->useGlobalMode = useGLobalMode;
    m_d->usePostScaling = usePostScaling;
    m_d->postScalingCoveringTransform = postScalingCoveringTransform;

    Q_FOREACH (KoShape *shape, m_d->shapes) {
        m_d->oldSizes << shape->size();
        m_d->oldTransforms << shape->transformation();
    }
}

KoShapeResizeCommand::~KoShapeResizeCommand()
{
}

void KoShapeResizeCommand::redoImpl()
{
    Q_FOREACH (KoShape *shape, m_d->shapes) {
        shape->update();

        KoFlake::resizeShape(shape,
                             m_d->scaleX, m_d->scaleY,
                             m_d->absoluteStillPoint,
                             m_d->useGlobalMode,
                             m_d->usePostScaling,
                             m_d->postScalingCoveringTransform);

        shape->update();
    }
}

void KoShapeResizeCommand::undoImpl()
{
    for (int i = 0; i < m_d->shapes.size(); i++) {
        KoShape *shape = m_d->shapes[i];

        shape->update();
        shape->setSize(m_d->oldSizes[i]);
        shape->setTransformation(m_d->oldTransforms[i]);
        shape->update();
    }
}

int KoShapeResizeCommand::id() const
{
    return KisCommandUtils::ResizeShapeId;
}

bool KoShapeResizeCommand::mergeWith(const KUndo2Command *command)
{
    const KoShapeResizeCommand *other = dynamic_cast<const KoShapeResizeCommand*>(command);

    if (!other ||
        other->m_d->absoluteStillPoint != m_d->absoluteStillPoint ||
        other->m_d->shapes != m_d->shapes ||
        other->m_d->useGlobalMode != m_d->useGlobalMode ||
        other->m_d->usePostScaling != m_d->usePostScaling) {

        return false;
    }

    // check if the significant orientations coincide
    if (m_d->useGlobalMode && !m_d->usePostScaling) {
        Qt::Orientation our = KoFlake::significantScaleOrientation(m_d->scaleX, m_d->scaleY);
        Qt::Orientation their = KoFlake::significantScaleOrientation(other->m_d->scaleX, other->m_d->scaleY);

        if (our != their) {
            return false;
        }
    }

    m_d->scaleX *= other->m_d->scaleX;
    m_d->scaleY *= other->m_d->scaleY;
    return true;
}
