/***************************************************************************
    qgsmapcanvasitem.h  - base class for map canvas items
    ----------------------
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

#ifndef QGSMAPCANVASITEM_H
#define QGSMAPCANVASITEM_H

#include <Q3CanvasItem>
#include "qgsrect.h"

class QgsMapCanvas;
class QPainter;

class QgsMapCanvasItem : public QObject, public Q3CanvasRectangle
{
  Q_OBJECT
  
  protected:
    
    //! protected constructor: cannot be constructed directly
    QgsMapCanvasItem(QgsMapCanvas* mapCanvas);
    
    virtual ~QgsMapCanvasItem();
    
    //! function to be implemented by derived classes
    virtual void drawShape(QPainter & p);
    
    //! handler for manual updating of position and size
    virtual void updatePositionManual();
    
    //! schedules map canvas for repaint
    void updateCanvas();
    
    enum ResizeType { ResizeAuto, ResizeManual };
  
  public:
    
    //! returns canvas item rectangle
    QgsRect rect();
    
    //! sets canvas item rectangle
    void setRect(const QgsRect& r);
    
    //! transformation from screen coordinates to map coordinates
    QgsPoint toMapCoords(const QPoint& point);
    
    //! transformation from map coordinates to screen coordinates
    QPoint toCanvasCoords(const QgsPoint& point);
    
    //! changes position updating policy
    void setResizeType(ResizeType type);
    
    
  public slots:
    
    //! called on changed extents or changed item rectangle
    void updatePosition();

  protected:
    
    //! pointer to map canvas
    QgsMapCanvas* mMapCanvas;
    
    //! canvas item rectangle (in map coordinates)
    QgsRect mRect;
    
    //! determines which position updating policy will be used
    ResizeType mResizeType;
};


#endif
