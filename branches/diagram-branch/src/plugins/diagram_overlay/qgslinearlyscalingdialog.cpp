/***************************************************************************
                         qgslinearlyscalingdialog.cpp  -  description
                         ----------------------------
    begin                : January 2007
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

#include "qgslinearlyscalingdialog.h"
#include "qgslinearlyscalingdiagramrenderer.h"
#include "qgsvectordataprovider.h"
#include "qgswkndiagramfactory.h"

QgsLinearlyScalingDialog::QgsLinearlyScalingDialog(QgsVectorLayer* vl): QgsDiagramRendererWidget(vl)
{
  setupUi(this);
  QObject::connect(mFindMaximumValueButton, SIGNAL(clicked()), this, SLOT(insertMaximumAttributeValue()));
}

QgsLinearlyScalingDialog::~QgsLinearlyScalingDialog()
{

}

QgsDiagramRenderer* QgsLinearlyScalingDialog::createRenderer(const QString& type, int classAttr, const QgsAttributeList& attributes, const std::list<QColor>& colors) const
{
  //create a linearly scaling renderer
  QList<int> attributesList;
  attributesList.push_back(classAttr);
  QgsLinearlyScalingDiagramRenderer* renderer = new QgsLinearlyScalingDiagramRenderer(attributesList);
  
  //and items of renderer
  QList<QgsDiagramItem> itemList;
  QgsDiagramItem firstItem; firstItem.value = QVariant(0.0); firstItem.size = 0;
  QgsDiagramItem secondItem; 
  secondItem.value = QVariant(mValueLineEdit->text().toDouble());
  secondItem.size = mSizeSpinBox->value();
  itemList.push_back(firstItem);
  itemList.push_back(secondItem);
  renderer->setDiagramItems(itemList);
  
  QgsWKNDiagramFactory* f = new QgsWKNDiagramFactory();
   f->setDiagramType(type);
   f->setAttributes(attributes);
   f->setColorSeries(colors);
   f->setScalingAttribute(classAttr);
   renderer->setFactory(f);

   return renderer;
}

void QgsLinearlyScalingDialog::applySettings(const QgsDiagramRenderer* renderer)
{
  const QgsLinearlyScalingDiagramRenderer* linearRenderer = dynamic_cast<const QgsLinearlyScalingDiagramRenderer*>(renderer);
  if(linearRenderer)
    {
      QList<QgsDiagramItem> itemList = linearRenderer->diagramItems();
      QgsDiagramItem theItem = itemList.at(1); //take the upper item
      mValueLineEdit->setText(theItem.value.toString());
      mSizeSpinBox->setValue(theItem.size);
    }
}

void QgsLinearlyScalingDialog::insertMaximumAttributeValue()
{
  //find the maximum value for this attribute
  if(mVectorLayer)
    {
      QgsVectorDataProvider *provider = dynamic_cast<QgsVectorDataProvider *>(mVectorLayer->getDataProvider());
      if(provider)
	{
	  mValueLineEdit->setText(provider->maxValue(mClassificationField).toString());
	}
    }
}
