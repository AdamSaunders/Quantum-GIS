/***************************************************************************
                            qgsprojectproperties.cpp
       Set various project properties (inherits qgsprojectpropertiesbase)
                              -------------------
  begin                : May 18, 2004
  copyright            : (C) 2004 by Gary E.Sherman
  email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#include <cassert>
#include "qgsprojectproperties.h"
#include "qgsspatialreferences.h"
#include "qgscsexception.h"

//qgis includes
#include "qgsconfig.h"
#include "qgsproject.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsrenderer.h"
#include "qgis.h"

//qt includes
#include <qapplication.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qtextedit.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <qspinbox.h>
#include <qcolor.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qlistview.h>
#include <qprogressdialog.h> 
#include <qapplication.h>

//stdc++ includes
#include <iostream>
#include <cstdlib>
// set the default coordinate system
static const char* defaultWktKey = "4326";
  QgsProjectProperties::QgsProjectProperties(QWidget *parent, const char *name)
: QgsProjectPropertiesBase(parent, name)
{
  //    out with the old
  //    QgsProject::instance()->mapUnits( QgsScaleCalculator::METERS );
  //    in with the new...
  QgsScaleCalculator::units myUnit = QgsProject::instance()->mapUnits();
  setMapUnits(myUnit);
  title(QgsProject::instance()->title());

  //see if the user wants on the fly projection enabled
  int myProjectionEnabledFlag = 
    QgsProject::instance()->readNumEntry("SpatialRefSys","/ProjectionsEnabled",0);
  if (myProjectionEnabledFlag==0)
  {
    cbxProjectionEnabled->setChecked(false);
  }
  else
  {
    cbxProjectionEnabled->setChecked(true);
  }
  //
  // TODO: Change this to connect to the signal from the projectionselector dialog now!!!! ts
  // 
  // If the user changes the projection for the project, we need to 
  // fire a signal to each layer telling it to change its coordinateTransform
  // member's output projection. These connects I'm setting up should be
  // automatically cleaned up when this project props dialog closes
  std::map<QString, QgsMapLayer *> myMapLayers 
    = QgsMapLayerRegistry::instance()->mapLayers();
  std::map<QString, QgsMapLayer *>::iterator myMapIterator;
  for ( myMapIterator = myMapLayers.begin(); myMapIterator != myMapLayers.end(); ++myMapIterator )
  {
    QgsMapLayer * myMapLayer = myMapIterator->second;
    connect(this,
        SIGNAL(setDestWKT(QString)),
        myMapLayer->coordinateTransform(),
        SLOT(setDestWKT(QString)));   
  }

  // get the manner in which the number of decimal places in the mouse
  // position display is set (manual or automatic)
  bool automaticPrecision = QgsProject::instance()->readBoolEntry("PositionPrecision","/Automatic");
  if (automaticPrecision)
    btnGrpPrecision->setButton(0);
  else
    btnGrpPrecision->setButton(1);

  int dp = QgsProject::instance()->readNumEntry("PositionPrecision", "/DecimalPlaces");
  spinBoxDP->setValue(dp);

  //get the snapping tolerance for digitising and set the control accordingly
  double mySnapTolerance = QgsProject::instance()->readDoubleEntry("Digitizing","/Tolerance",0);
  //leSnappingTolerance->setInputMask("000000.000000");
  leSnappingTolerance->setText(QString::number(mySnapTolerance));

  //get the line width for digitised lines and set the control accordingly
  int myLineWidth = QgsProject::instance()->readNumEntry("Digitizing","/LineWidth",0);
  spinDigitisedLineWidth->setValue(myLineWidth);    

  //get the colour of digitising lines and set the button colour accordingly
  int myRedInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorRedPart",255);
  int myGreenInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorGreenPart",0);
  int myBlueInt = QgsProject::instance()->readNumEntry("Digitizing","/LineColorBluePart",0);
  QColor myColour = QColor(myRedInt,myGreenInt,myBlueInt);
  pbnDigitisedLineColour->setPaletteBackgroundColor (myColour);

  //get the colour selections and set the button colour accordingly
  myRedInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorRedPart",255);
  myGreenInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorGreenPart",255);
  myBlueInt = QgsProject::instance()->readNumEntry("Gui","/SelectionColorBluePart",0);
  myColour = QColor(myRedInt,myGreenInt,myBlueInt);
  pbnSelectionColour->setPaletteBackgroundColor (myColour);    
}

QgsProjectProperties::~QgsProjectProperties()
{}

// return the map units
QgsScaleCalculator::units QgsProjectProperties::mapUnits() const
{
  return QgsProject::instance()->mapUnits();
}


void QgsProjectProperties::mapUnitChange(int unit)
{
  /*
     QgsProject::instance()->mapUnits(
     static_cast<QgsScaleCalculator::units>(unit));
     */
}


void QgsProjectProperties::setMapUnits(QgsScaleCalculator::units unit)
{
  // select the button
  btnGrpMapUnits->setButton(static_cast<int>(unit));
  QgsProject::instance()->mapUnits(unit);
}


QString QgsProjectProperties::title() const
{
  return titleEdit->text();
} //  QgsProjectPropertires::title() const


void QgsProjectProperties::title( QString const & title )
{
  titleEdit->setText( title );
  QgsProject::instance()->title( title );
} // QgsProjectProperties::title( QString const & title )



//when user clicks apply button
void QgsProjectProperties::apply()
{
  // Set the map units
  QgsProject::instance()->mapUnits(
      static_cast<QgsScaleCalculator::units>(btnGrpMapUnits->selectedId()));

  // Set the project title
  QgsProject::instance()->title( title() );

#ifdef QGISDEBUG
  std::cout << "Projection changed, notifying all layers" << std::endl;
#endif      
  //tell the project if projections are to be used or not...      
  if (cbxProjectionEnabled->isChecked())
  {
    QgsProject::instance()->writeEntry("SpatialRefSys","/ProjectionsEnabled",1);
  }
  else
  {
    QgsProject::instance()->writeEntry("SpatialRefSys","/ProjectionsEnabled",0);
  }
  // Only change the projection if there is a node in the tree
  // selected that has an srid. This prevents error if the user
  // selects a top-level node rather than an actual coordinate
  // system
  //if(lstCoordinateSystems->currentItem()->text(1).length() > 0)
  //{
    //notify all layers the output projection has changed

   // QString selectedWkt = QgsSpatialReferences::instance()->getSrsBySrid(lstCoordinateSystems->currentItem()->text(1))->srText();
   // emit setDestWKT(selectedWkt);   //update the project props
    // write the projection's wkt to the project settings rather
    //QgsProject::instance()->writeEntry("SpatialRefSys","/WKT",
    //    lstCoordinateSystems->currentItem()->text(0));
 // }

  // set the mouse display precision method and the
  // number of decimal places for the manual option
  // Note. Qt 3.2.3 and greater have a function selectedId() that
  // can be used instead of the two part technique here
  if (btnGrpPrecision->id(btnGrpPrecision->selected()) == 0)
    QgsProject::instance()->writeEntry("PositionPrecision","/Automatic", true);
  else
    QgsProject::instance()->writeEntry("PositionPrecision","/Automatic", false);
  QgsProject::instance()->writeEntry("PositionPrecision","/DecimalPlaces", spinBoxDP->value());
  // Announce that we may have a new display precision setting
  emit displayPrecisionChanged();

  //set the snapping tolerance for digitising (we write as text but read will convert to a num
  QgsProject::instance()->writeEntry("Digitizing","/Tolerance",leSnappingTolerance->text());

  //set the line width for digitised lines and set the control accordingly
  QgsProject::instance()->writeEntry("Digitizing","/LineWidth",spinDigitisedLineWidth->value());

  //set the colour of digitising lines
  QColor myColour = pbnDigitisedLineColour->paletteBackgroundColor();
  QgsProject::instance()->writeEntry("Digitizing","/LineColorRedPart",myColour.red());
  QgsProject::instance()->writeEntry("Digitizing","/LineColorGreenPart",myColour.green());
  QgsProject::instance()->writeEntry("Digitizing","/LineColorBluePart",myColour.blue());

  //set the colour for selections
  myColour = pbnSelectionColour->paletteBackgroundColor();
  QgsProject::instance()->writeEntry("Gui","/SelectionColorRedPart",myColour.red());
  QgsProject::instance()->writeEntry("Gui","/SelectionColorGreenPart",myColour.green());
  QgsProject::instance()->writeEntry("Gui","/SelectionColorBluePart",myColour.blue()); 
  QgsRenderer::mSelectionColor=myColour;

}

//when user clicks ok
void QgsProjectProperties::accept()
{
  apply();
  close();
}

QString QgsProjectProperties::projectionWKT()
{
  // set the default wkt to WGS 84
  QString defaultWkt = QgsSpatialReferences::instance()->getSrsBySrid("4326")->srText();
  // the /WKT entry stores the wkt as the key into the projections map 
  QString srsWkt =  QgsProject::instance()->readEntry("SpatialRefSys","/WKT",defaultWkt);
  return srsWkt;
}  
