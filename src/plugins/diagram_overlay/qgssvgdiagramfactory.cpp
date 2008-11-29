/***************************************************************************
                         qgssvgdiagramfactory.cpp  -  description
                         ------------------------
    begin                : November 2007
    copyright            : (C) 2007 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssvgdiagramfactory.h"
#include "qgsrendercontext.h"
#include <QImage>
#include <QPainter>

QgsSVGDiagramFactory::QgsSVGDiagramFactory(): QgsDiagramFactory()
{
  
}

QgsSVGDiagramFactory::~QgsSVGDiagramFactory()
{
  
}

QImage* QgsSVGDiagramFactory::createDiagram(int size, const QgsFeature& f, const QgsRenderContext& renderContext) const
{
  //check default size
  QSize defaultSize = mRenderer.defaultSize();
  qreal scaleFactor;
  int imageWidth, imageHeight;

  //size parameter applies to maximum of width, height
  if(defaultSize.width() >= defaultSize.height())
    {
      scaleFactor = ((double)size * diagramSizeScaleFactor(renderContext) * renderContext.rasterScaleFactor()) / defaultSize.width();
    }
  else
    {
      scaleFactor = ((double)size * diagramSizeScaleFactor(renderContext) * renderContext.rasterScaleFactor()) / defaultSize.height();
    }

  imageWidth = (int)(defaultSize.width() * scaleFactor);
  imageHeight = (int)(defaultSize.height() * scaleFactor);
  QImage* diagramImage = new QImage(QSize(imageWidth, imageHeight), QImage::Format_ARGB32_Premultiplied);
  diagramImage->fill(qRgba(0, 0, 0, 0)); //transparent background
  
  QPainter p;
  p.begin(diagramImage);
  p.setRenderHint(QPainter::Antialiasing);
  //p.scale(scaleFactor, scaleFactor);

  //render image
  mRenderer.render(&p);

  p.end();
  return diagramImage;
}

int QgsSVGDiagramFactory::getDiagramDimensions(int size, const QgsFeature& f, const QgsRenderContext& context, int& width, int& height) const
{
  return 1; //soon...
}

bool QgsSVGDiagramFactory::setSVGData(const QByteArray& data)
{
  return mRenderer.load(data);
}



