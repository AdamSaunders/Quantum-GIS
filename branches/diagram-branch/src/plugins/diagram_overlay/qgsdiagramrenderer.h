/***************************************************************************
                         qgsdiagramrenderer.h  -  description
                         --------------------
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

#ifndef QGSDIAGRAMRENDERER_H
#define QGSDIAGRAMRENDERER_H

#include <QList>
#include <QMap>
#include <QVariant>

class QgsDiagramFactory;
class QgsFeature;
class QDomDocument;
class QDomNode;
class QImage;

//structure that describes a renderer entry
struct QgsDiagramItem
{
  QVariant value;
  int size;
};

/**An interface class for diagram renderer. The main method is 'renderDiagram', which returns the diagram image for a GIS feature. The renderer has a set of classification attributes and a reference to a diagram factory object, which is responsible for creating the specific diagram type.
Subclasses need to implement the method calculate diagram size.*/
class QgsDiagramRenderer
{
 public:

  //describes how the items are interpreted for diagram size calculation
  enum ItemInterpretation
    {
      DISCRETE, //lower item is used if item falls between lower and upper item
      LINEAR, //linear interpolation between items containing the value
      ATTRIBUTE, //only attribute values are considered
      CONSTANT //constant value is used (the size of the first item)
    };

  QgsDiagramRenderer(const QList<int>& classificationAttributes);
  virtual ~QgsDiagramRenderer();
  /**Returns a diagram image for a feature.*/
  virtual QImage* renderDiagram(const QgsFeature& f) const;
  /**Returns only the size of the diagram.
     @param width the width of the diagram
     @param height the height of the diagram
     @param f feature that is associated with the diagram
     @return 0 in case of success*/
  virtual int getDiagramDimensions(int& width, int& height, const QgsFeature& f) const;
  //setters and getters
  QgsDiagramFactory* factory() const {return mFactory;}
  /**Set a (properly configured) factory class. Takes ownership of the factory object*/
  void setFactory(QgsDiagramFactory* f){mFactory = f;}
  void addClassificationAttribute(int attrNr);
  /**Reads the specific renderer settings from project file*/
  virtual bool readXML(const QDomNode& rendererNode);
  /**Saves settings to project file. Returns true in case of success*/
  virtual bool writeXML(QDomNode& overlay_node, QDomDocument& doc) const;
  /**Creates pairs of strings / images for use in the legend
   @return 0 in case of success*/
  virtual int createLegendContent(QMap<QString, QImage*> items) const;
  /**Sets the items for interpolation. The values of the items must be in ascending order*/
  void setDiagramItems(const QList<QgsDiagramItem>& items) {mItems = items;}
  /**Returns the interpolation items*/
  QList<QgsDiagramItem> diagramItems() const {return mItems;}
  void setItemInterpretation(ItemInterpretation i){mItemInterpretation = i;}

 private:
  QgsDiagramRenderer();

 protected:
  /**The object to generate the diagrams*/
  QgsDiagramFactory* mFactory;
  /**Attributes for determining the size of the diagram.
   If there are several attributes, their sum is used.*/
  QList<int> mClassificationAttributes;
  /**Value/size pairs for determination of the diagram size*/
  QList<QgsDiagramItem> mItems;
  /**Describes the type of interpolation (linear by default)*/
  ItemInterpretation mItemInterpretation;

  /**Searches the value of the classification attribute(s). Considers that there 
     may be several attributes in case of numeric values (sum).
   @return 0 in case of success*/
  int classificationValue(const QgsFeature& f, QVariant& value) const;
  /**Gets diagram size
   @return 0 in case of success*/
  virtual int calculateDiagramSize(const QgsFeature& f, int& size) const;
  /**Does (linear or discrete) interpolation*/
  int interpolateSize(double value, double lowerValue, double upperValue, int lowerSize, \
		      int upperSize) const;
};

#endif
