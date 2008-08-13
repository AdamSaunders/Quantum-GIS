/***************************************************************************
                         qgscomposermap.cpp
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposermap.h"

#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaptopixel.h"
#include "qgsproject.h"
#include "qgsmaprender.h"
#include "qgsrendercontext.h"
#include "qgsscalecalculator.h"
#include "qgsvectorlayer.h"

#include "qgslabel.h"
#include "qgslabelattributes.h"

#include <QGraphicsScene>
#include <QPainter>
#include <iostream>
#include <cmath>

int QgsComposerMap::mCurrentComposerId = 0;

QgsComposerMap::QgsComposerMap ( QgsComposition *composition, int x, int y, int width, int height )
  : QgsComposerItem(x, y, width, height, composition)
{
    mComposition = composition;
    mMapCanvas = mComposition->mapCanvas();
    mId = mCurrentComposerId++;
    
    // Cache
    mCacheUpdated = false;
    mDrawing = false;

    //Offset
    mXOffset = 0.0;
    mYOffset = 0.0;

    //calculate mExtent based on width/height ratio and map canvas extent
    mExtent = mMapCanvas->extent();
    setSceneRect(QRectF(x, y, width, height));

    QGraphicsRectItem::setZValue(20);

    connect ( mMapCanvas, SIGNAL(layersChanged()), this, SLOT(mapCanvasChanged()) );

    setToolTip(tr("Map") + " " + QString::number(mId));

    QGraphicsRectItem::show();
}

QgsComposerMap::QgsComposerMap ( QgsComposition *composition)
    : QgsComposerItem(0, 0, 10, 10, composition)
{
  //Offset
  mXOffset = 0.0;
  mYOffset = 0.0;
  
  mComposition = composition;
  mMapCanvas = mComposition->mapCanvas();
  mId = mCurrentComposerId++;
  setToolTip(tr("Map") + " " + QString::number(mId));
  QGraphicsRectItem::show();
}

QgsComposerMap::~QgsComposerMap()
{
}

/* This function is called by paint() and cache() to render the map.  It does not override any functions
from QGraphicsItem. */
void QgsComposerMap::draw ( QPainter *painter, const QgsRect& extent, const QSize& size, int dpi)
{
  mMapCanvas->freeze(true);  // necessary ?

  if(!painter)
    {
      return;
    }

  QgsMapRender* canvasMapRender = mMapCanvas->mapRender();
  if(!canvasMapRender)
    {
      return;
    }

  QgsMapRender theMapRender;
  theMapRender.setExtent(extent);
  theMapRender.setOutputSize(size, dpi);
  theMapRender.setLayerSet(canvasMapRender->layerSet());
  theMapRender.setProjectionsEnabled(canvasMapRender->projectionsEnabled());
  theMapRender.setDestinationSrs(canvasMapRender->destinationSrs());
  
  QgsRenderContext* theRenderContext = theMapRender.renderContext();
  if(theRenderContext)
    {
      theRenderContext->setDrawEditingInformation(false);
      theRenderContext->setRenderingStopped(false);
    }

  theMapRender.render(painter);
    
  mMapCanvas->freeze(false);
}

void QgsComposerMap::cache ( void )
{
  int w = rect().width() * horizontalViewScaleFactor();
  int h = rect().height() * horizontalViewScaleFactor();

  if(w > 3000) //limit size of image for better performance
    {
      w = 3000;
    }

  if(h > 3000)
    {
      h = 3000;
    }
      
  mCachePixmap = QPixmap( w, h );
  double mupp = mExtent.width() / w;

  // WARNING: ymax in QgsMapToPixel is device height!!!
  QgsMapToPixel transform(mupp, h, mExtent.yMin(), mExtent.xMin() );
    
  mCachePixmap.fill(QColor(255,255,255));
  
  QPainter p(&mCachePixmap);
  
  draw( &p, mExtent, QSize(w, h), mCachePixmap.logicalDpiX());
  p.end();
  
  mNumCachedLayers = mMapCanvas->layerCount();
  mCachedMapExtent = mExtent;
  mCacheUpdated = true;
}

void QgsComposerMap::paint ( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget)
{
  if ( mDrawing ) 
    {
      return;
    }

  if(!mComposition)
    {
      return;
    }

  mDrawing = true;

  QRectF thisPaintRect = QRectF( 0, 0, QGraphicsRectItem::rect().width(), QGraphicsRectItem::rect().height());
  painter->save();
  painter->setClipRect (thisPaintRect);

  double currentScaleFactorX = horizontalViewScaleFactor();

  if( mComposition->plotStyle() == QgsComposition::Preview && mPreviewMode == Render /*&& screen resolution different than last time*/)
    {
      if(currentScaleFactorX != mLastScaleFactorX)
	{
	  mCacheUpdated = false;
	}
    }
    
  if ( mComposition->plotStyle() == QgsComposition::Preview && mPreviewMode != Rectangle) 
    { // Draw from cache
      if ( !mCacheUpdated || mMapCanvas->layerCount() != mNumCachedLayers || mCachedMapExtent != mExtent) 
	{
	  cache();
	}
      
      // Scale so that the cache fills the map rectangle
      double scale = 1.0 * QGraphicsRectItem::rect().width() / mCachePixmap.width();
      
      painter->save();
      painter->scale(scale,scale);
      painter->drawPixmap(mXOffset / scale, mYOffset / scale, mCachePixmap);
      painter->restore();
    } 
  else if ( mComposition->plotStyle() == QgsComposition::Print ||
            mComposition->plotStyle() == QgsComposition::Postscript) 
    {
      QPaintDevice* thePaintDevice = painter->device();
      if(!thePaintDevice)
	{
	  return;
	}
    
      QRectF bRect = boundingRect();
      QSize theSize(bRect.width(), bRect.height());
      draw( painter, mExtent, theSize, 25.4); //scene coordinates seem to be in mm
    } 

  drawFrame(painter);
  if(isSelected())
    {
      drawSelectionBoxes(painter);
    }
  
 
  painter->restore();

  mLastScaleFactorX =  currentScaleFactorX;
  mDrawing = false;
}

void QgsComposerMap::mapCanvasChanged ( void ) 
{
    mCacheUpdated = false;
    QGraphicsRectItem::update();
}

void QgsComposerMap::setCacheUpdated ( bool u ) 
{
    mCacheUpdated = u;
}    

double QgsComposerMap::scale() const
{
  QgsScaleCalculator calculator;
  calculator.setMapUnits(mMapCanvas->mapUnits());
  calculator.setDpi(25.4);  //QGraphicsView units are mm
  return calculator.calculate(mExtent, rect().width());
}

void QgsComposerMap::resize(double dx, double dy)
{
  //setRect
  QRectF currentRect = rect();
  QRectF newSceneRect = QRectF(transform().dx(), transform().dy(), currentRect.width() + dx, currentRect.height() + dy);
  setSceneRect(newSceneRect);
}

void QgsComposerMap::moveContent(double dx, double dy)
{
  QRectF itemRect = rect();
  double xRatio = dx / itemRect.width();
  double yRatio = dy / itemRect.height();

  double xMoveMapCoord = mExtent.width() * xRatio;
  double yMoveMapCoord = -(mExtent.height() * yRatio);

  mExtent.setXmin(mExtent.xMin() + xMoveMapCoord);
  mExtent.setXmax(mExtent.xMax() + xMoveMapCoord);
  mExtent.setYmin(mExtent.yMin() + yMoveMapCoord);
  mExtent.setYmax(mExtent.yMax() + yMoveMapCoord);
  emit extentChanged();
}

void QgsComposerMap::setSceneRect(const QRectF& rectangle)
{
  double w = rectangle.width();
  double h = rectangle.height();
  //prepareGeometryChange();

  QgsComposerItem::setSceneRect(rectangle);
  
  //QGraphicsRectItem::update();
  double newHeight = mExtent.width() * h / w ;
  mExtent = QgsRect(mExtent.xMin(), mExtent.yMin(), mExtent.xMax(), mExtent.yMin() + newHeight);
  mCacheUpdated = false;
  emit extentChanged();
}

void QgsComposerMap::setNewExtent(const QgsRect& extent)
{
  if(mExtent == extent)
    {
      return;
    }
  mExtent = extent;

  //adjust height
  QRectF currentRect = rect();
  
  double newHeight = currentRect.width() * extent.height() / extent.width();

  setSceneRect(QRectF(transform().dx(), transform().dy(), currentRect.width(), newHeight));
}

void QgsComposerMap::setNewScale(double scaleDenominator)
{
  double currentScaleDenominator = scale();

  if(scaleDenominator == currentScaleDenominator)
    {
      return;
    }

  double scaleRatio = scaleDenominator / currentScaleDenominator;

  double newXMax = mExtent.xMin() + scaleRatio * (mExtent.xMax() - mExtent.xMin());
  double newYMax = mExtent.yMin() + scaleRatio * (mExtent.yMax() - mExtent.yMin());

  QgsRect newExtent(mExtent.xMin(), mExtent.yMin(), newXMax, newYMax);
  mExtent = newExtent;
  mCacheUpdated = false;
  emit extentChanged();
  update();
}

void QgsComposerMap::setOffset(double xOffset, double yOffset)
{
  mXOffset = xOffset;
  mYOffset = yOffset;
}

double QgsComposerMap::horizontalViewScaleFactor() const
{
  double result = 1;
  if(scene())
    {
      QList<QGraphicsView*> viewList = scene()->views();
      if(viewList.size() > 0)
	{
	  result = viewList.at(0)->transform().m11();
	}
    }
  return result;
}

bool QgsComposerMap::writeXML(QDomElement& elem, QDomDocument & doc)
{
  if(elem.isNull())
    {
      return false;
    }

  QDomElement composerMapElem = doc.createElement("ComposerMap");
  
  //previewMode
  if(mPreviewMode == Cache)
    {
      composerMapElem.setAttribute("previewMode", "Cache");
    }
  else if(mPreviewMode == Render)
    {
      composerMapElem.setAttribute("previewMode", "Render");
    }
  else //rectangle
    {
      composerMapElem.setAttribute("previewMode", "Rectangle");
    }

  //extent
  QDomElement extentElem = doc.createElement("Extent");
  extentElem.setAttribute("xmin", QString::number(mExtent.xMin()));
  extentElem.setAttribute("xmax", QString::number(mExtent.xMax()));
  extentElem.setAttribute("ymin", QString::number(mExtent.yMin()));
  extentElem.setAttribute("ymax", QString::number(mExtent.yMax()));
  composerMapElem.appendChild(extentElem);

  mCacheUpdated = false;
  mNumCachedLayers = 0;

  elem.appendChild(composerMapElem);
  return _writeXML(composerMapElem, doc);
}

bool QgsComposerMap::readXML(const QDomElement& itemElem, const QDomDocument& doc)
{
  if(itemElem.isNull())
    {
      return false;
    }

  mPreviewMode = Rectangle;
  
#if 0 //leads to frequent crashes on Qt 4.4.0/4.4.1
  //previewMode
  QString previewMode = itemElem.attribute("previewMode");
  if(previewMode == "Cache")
    {
      mPreviewMode = Cache;
    }
  else if(previewMode == "Render")
    {
      mPreviewMode = Render;
    }
  else
    {
      mPreviewMode = Rectangle;
    }
#endif //0

  //extent
  QDomNodeList extentNodeList = itemElem.elementsByTagName("Extent");
  if(extentNodeList.size() > 0)
    {
      QDomElement extentElem = extentNodeList.at(0).toElement();
      double xmin, xmax, ymin, ymax;
      xmin = extentElem.attribute("xmin").toDouble();
      xmax = extentElem.attribute("xmax").toDouble();
      ymin = extentElem.attribute("ymin").toDouble();
      ymax = extentElem.attribute("ymax").toDouble();

      mExtent = QgsRect(xmin, ymin, xmax, ymax);
    }

  mDrawing = false;
  mNumCachedLayers = 0;
  mCacheUpdated = false;

  //restore general composer item properties
  QDomNodeList composerItemList = itemElem.elementsByTagName("ComposerItem");
  if(composerItemList.size() > 0)
    {
      QDomElement composerItemElem = composerItemList.at(0).toElement();
      _readXML(composerItemElem, doc);
    }

  return true;
}
