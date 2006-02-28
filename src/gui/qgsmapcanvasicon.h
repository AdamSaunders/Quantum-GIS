/***************************************************************************
    qgsmapcanvasicon.h  - canvas item which shows a simple icon
    ---------------------
    begin                : February 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSMAPCANVASICON_H
#define QGSMAPCANVASICON_H

#include "qgsmapcanvasitem.h"
#include "qgspoint.h"

class QPainter;

class QgsMapCanvasIcon : public QgsMapCanvasItem
{
  public:
    
    //! Icons
    enum IconType
    {
      ICON_NONE,
      ICON_CROSS,
      ICON_X,
      ICON_BOX
    };

    QgsMapCanvasIcon(QgsMapCanvas* mapCanvas);
    
    void setCenter(const QgsPoint& point);
    
    void setIconType(int iconType);
    
    void setIconSize(int iconSize);
    
    void drawShape(QPainter & p);
    
    //! handler for manual updating of position and size
    void updatePositionManual();
    
  protected:
    
    //! icon to be shown
    int mIconType;
    
    //! size
    int mIconSize;

    //! coordinates of the point in the center
    QgsPoint mCenter;
};

#endif
