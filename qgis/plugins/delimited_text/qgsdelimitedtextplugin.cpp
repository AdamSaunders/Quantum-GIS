/***************************************************************************
  qgsdelimitedtextplugin.cpp 
  Import tool for various worldmap analysis output files
Functions:

-------------------
  begin                : Feb 21, 2004
  copyright            : (C) 2004 by Gary Sherman
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
/*  $Id$ */

// includes

#include "qgisapp.h"
#include "qgsmaplayer.h"
#include "qgsdelimitedtextplugin.h"


#include <q3toolbar.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <q3popupmenu.h>
#include <qlineedit.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcursor.h>
#include <q3whatsthis.h>
#include <qglobal.h>

//non qt includes
#include <iostream>

//the gui subclass
#include "qgsdelimitedtextplugingui.h"

// xpm for creating the toolbar icon
#include "icon.xpm"
// 
#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif

static const char *pluginVersion = "Version 0.2";
/**
 * Constructor for the plugin. The plugin is passed a pointer to the main app
 * and an interface object that provides access to exposed functions in QGIS.
 * @param qgis Pointer to the QGIS main window
 * @param _qI Pointer to the QGIS interface object
 */
  QgsDelimitedTextPlugin::QgsDelimitedTextPlugin(QgisApp * theQGisApp, QgisIface * theQgisInterFace):
qgisMainWindowPointer(theQGisApp), qGisInterface(theQgisInterFace)
{
  /** Initialize the plugin and set the required attributes */
  pluginNameQString = "DelimitedTextLayer";
  pluginVersionQString = "Version 0.1";
  pluginDescriptionQString = "This plugin provides support for delimited text files containing x,y coordinates";

}

QgsDelimitedTextPlugin::~QgsDelimitedTextPlugin()
{

}

/* Following functions return name, description, version, and type for the plugin */
QString QgsDelimitedTextPlugin::name()
{
  return pluginNameQString;
}

QString QgsDelimitedTextPlugin::version()
{
  return pluginVersionQString;

}

QString QgsDelimitedTextPlugin::description()
{
  return pluginDescriptionQString;

}

int QgsDelimitedTextPlugin::type()
{
  return QgisPlugin::UI;
}
//method defined in interface
void QgsDelimitedTextPlugin::help()
{
  //implement me!
}

/*
 * Initialize the GUI interface for the plugin 
 */
void QgsDelimitedTextPlugin::initGui()
{
  QMenu *pluginMenu = qGisInterface->getPluginMenu("&Delimited text");
  menuId = pluginMenu->insertItem(QIcon(icon),"&Add Delimited Text Layer", this, SLOT(run()));

  pluginMenu->setWhatsThis(menuId, "Add a delimited text file as a map layer. "
      "The file must have a header row containing the field names. "
      "X and Y fields are required and must contain coordinates in decimal units.");

  // Create the action for tool
#if QT_VERSION < 0x040000
  myQActionPointer = new QAction("Add Delimited Text Layer", QIcon(icon), "&Wmi",0, this, "run");
#else
  myQActionPointer = new QAction(QIcon(icon), "Add Delimited Text Layer", this);
#endif

  myQActionPointer->setWhatsThis("Add a delimited text file as a map layer. "
      "The file must have a header row containing the field names. "
      "X and Y fields are required and must contain coordinates in decimal units.");
  // Connect the action to the run
  connect(myQActionPointer, SIGNAL(activated()), this, SLOT(run()));
  // Add the icon to the toolbar
  qGisInterface->addToolBarIcon(myQActionPointer);

}

// Slot called when the buffer menu item is activated
void QgsDelimitedTextPlugin::run()
{
  QgsDelimitedTextPluginGui *myQgsDelimitedTextPluginGui=
    new QgsDelimitedTextPluginGui(qGisInterface, qgisMainWindowPointer,
        "Add Delimited Text Layer",true,0);
  //listen for when the layer has been made so we can draw it
  connect(myQgsDelimitedTextPluginGui, 
      SIGNAL(drawRasterLayer(QString)), 
      this, SLOT(drawRasterLayer(QString)));
  connect(myQgsDelimitedTextPluginGui, 
      SIGNAL(drawVectorLayer(QString,QString,QString)),
      this, SLOT(drawVectorLayer(QString,QString,QString)));
  myQgsDelimitedTextPluginGui->show();
}
//!draw a vector layer in the qui - intended to respond to signal 
//sent by diolog when it as finished creating a layer
////needs to be given vectorLayerPath, baseName, 
//providerKey ("ogr" or "postgres");
void QgsDelimitedTextPlugin::drawVectorLayer(QString thePathNameQString, 
    QString theBaseNameQString, QString theProviderQString)
{
  std::cerr << "Calling addVectorLayer with:" 
    << thePathNameQString.toLocal8Bit().data() << ", " << theBaseNameQString.toLocal8Bit().data()
    << ", " << theProviderQString.toLocal8Bit().data() << std::endl; 
  qGisInterface->addVectorLayer( thePathNameQString, 
      theBaseNameQString, theProviderQString);
}

// Unload the plugin by cleaning up the GUI
void QgsDelimitedTextPlugin::unload()
{
  // remove the GUI
  qGisInterface->removePluginMenuItem("&Delimited text",menuId);
  qGisInterface->removeToolBarIcon(myQActionPointer); 
  delete myQActionPointer;
}
/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory(QgisApp * theQGisAppPointer, 
    QgisIface * theQgisInterfacePointer)
{
  return new QgsDelimitedTextPlugin(theQGisAppPointer, theQgisInterfacePointer);
}

// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return QString("Add Delimited Text Layer");
}

// Return the description
QGISEXTERN QString description()
{
  return QString("This plugin provides support for delimited text files containing x,y coordinates");
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return QgisPlugin::UI;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return pluginVersion;
}

// Delete ourself
QGISEXTERN void unload(QgisPlugin * theQgsDelimitedTextPluginPointer)
{
  delete theQgsDelimitedTextPluginPointer;
}
