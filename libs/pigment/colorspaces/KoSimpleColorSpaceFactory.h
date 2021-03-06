/*
 *  Copyright (c) 2002 Patrick Julien  <freak@codepimps.org>
 *  Copyright (c) 2005-2006 C. Boemann <cbo@boemann.dk>
 *  Copyright (c) 2004,2006-2007 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KOSIMPLECOLORSPACEFACTORY_H_
#define KOSIMPLECOLORSPACEFACTORY_H_

#include "KoColorConversionTransformationFactory.h"
#include <colorprofiles/KoDummyColorProfile.h>

#include <KoColorModelStandardIds.h>

class KoSimpleColorSpaceFactory : public KoColorSpaceFactory
{

public:

    KoSimpleColorSpaceFactory(const QString& id,
                              const QString& name,
                              bool userVisible,
                              const KoID& colorModelId,
                              const KoID& colorDepthId,
                              int referenceDepth = -1)
        : m_id(id)
        , m_name(name)
        , m_userVisible(userVisible)
        , m_colorModelId(colorModelId)
        , m_colorDepthId(colorDepthId)
        , m_referenceDepth(referenceDepth)
    {
        if (colorDepthId == Integer8BitsColorDepthID) {
            m_referenceDepth = 1 * 8;
        }
        else if (colorDepthId == Integer16BitsColorDepthID) {
            m_referenceDepth = 2 * 8;
        }
        if (colorDepthId == Float16BitsColorDepthID) {
            m_referenceDepth = 2 * 8;
        }
        if (colorDepthId == Float32BitsColorDepthID) {
            m_referenceDepth = 4 * 8;
        }
        if (colorDepthId == Float64BitsColorDepthID) {
            m_referenceDepth = 8 * 8;
        }
    }


    virtual QString id() const {
        return m_id;
    }

    virtual QString name() const {
        return m_name;
    }

    virtual bool userVisible() const {
        return m_userVisible;
    }

    virtual KoID colorModelId() const {
        return m_colorModelId;
    }

    virtual KoID colorDepthId() const {
        return m_colorDepthId;
    }

    virtual bool profileIsCompatible(const KoColorProfile* profile) const {
        return dynamic_cast<const KoDummyColorProfile*>(profile);
    }

    virtual QString colorSpaceEngine() const {
        return "simple";
    }

    virtual bool isHdr() const {
        return false;
    }

    virtual int referenceDepth() const {
        return m_referenceDepth;
    }


    virtual QList<KoColorConversionTransformationFactory*> colorConversionLinks() const {
        return QList<KoColorConversionTransformationFactory*>();
    }

    virtual QString defaultProfile() const {
        return QString("default");
    }
protected:
    virtual KoColorProfile* createColorProfile(const QByteArray& /*rawData*/) const {
        return 0;
    }
private:

    QString m_id;
    QString m_name;
    bool    m_userVisible;
    KoID    m_colorModelId;
    KoID    m_colorDepthId;
    int     m_referenceDepth;

};

#endif
