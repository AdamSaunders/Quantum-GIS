/***************************************************************************
  plugin.cpp 
  Import tool for various worldmap analysis output files
Functions:

-------------------
begin                : Jan 21, 2004
copyright            : (C) 2004 by Tim Sutton
email                : tim@linfiniti.com

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

#include <qgsmaplayer.h>
//#include <qgsrasterlayer.h>
#include <qgisapp.h>
#include "plugin.h"


#include <qtoolbar.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qpopupmenu.h>
#include <qlineedit.h>
#include <qaction.h>
#include <qapplication.h>
#include <qcursor.h>

//non qt includes
#include <iostream>

//the gui subclass
#include "plugingui.h"

// xpm for creating the toolbar icon
#include "icon.xpm"
// 
static const char * const ident_ = "$Id$";

static const char * const name_ = "HttpServer";
static const char * const description_ = "A simple http server plugin that allows remote control of qgis.";
static const char * const version_ = "Version 0.1";
static const QgisPlugin::PLUGINTYPE type_ = QgisPlugin::UI;


/**
 * Constructor for the plugin. The plugin is passed a pointer to the main app
 * and an interface object that provides access to exposed functions in QGIS.
 * @param qgis Pointer to the QGIS main window
 * @param _qI Pointer to the QGIS interface object
 */
Plugin::Plugin(QgisApp * theQGisApp, QgisIface * theQgisInterFace):
          qgisMainWindowPointer(theQGisApp), 
          qGisInterface(theQgisInterFace),
          QgisPlugin(name_,description_,version_,type_)
{
  startServer();  
}

Plugin::~Plugin()
{
 unload();
}

/*
 * Initialize the GUI interface for the plugin 
 */
void Plugin::initGui()
{
  // add a menu with 2 items
  QPopupMenu *pluginMenu = new QPopupMenu(qgisMainWindowPointer);

  pluginMenu->insertItem(QIconSet(icon),"&HttpServer", this, SLOT(run()));

  menuBarPointer = ((QMainWindow *) qgisMainWindowPointer)->menuBar();

  menuIdInt = qGisInterface->addMenu("&Tools", pluginMenu);
  // Create the action for tool
  QAction *myQActionPointer = new QAction("QGis Http Server", QIconSet(icon), "&Wmi",0, this, "run");
  // Connect the action to the run
  connect(myQActionPointer, SIGNAL(activated()), this, SLOT(run()));
  // Add the toolbar
  toolBarPointer = new QToolBar((QMainWindow *) qgisMainWindowPointer, "Tools");
  toolBarPointer->setLabel("QGis Http Server");
  // Add the zoom previous tool to the toolbar
  myQActionPointer->addTo(toolBarPointer);


}
//method defined in interface
void Plugin::help()
{
  //implement me!
}

// Slot called when the menu item is activated
void Plugin::run()
{
  PluginGui *myPluginGui=new PluginGui(qgisMainWindowPointer,"QGis Http Server",true,0);
  //listen for when the layer has been made so we can draw it

  connect(mHttpDaemon, SIGNAL(newConnect(QString)), myPluginGui, SLOT(newConnect(QString)));
  connect(mHttpDaemon, SIGNAL(endConnect(QString)), myPluginGui, SLOT(endConnect(QString)));
  connect(mHttpDaemon, SIGNAL(wroteToClient(QString)), myPluginGui, SLOT(wroteToClient(QString)));
  connect(mHttpDaemon, SIGNAL(requestReceived(QString)), myPluginGui, SLOT(requestReceived(QString)));
  myPluginGui->show();
}

// Unload the plugin by cleaning up the GUI
void Plugin::unload()
{
  //kill the web server daemon
  stopServer();
  //delete mHttpDaemon;
  // remove the GUI
  menuBarPointer->removeItem(menuIdInt);
  delete toolBarPointer;
  //kill any connections to this object
  disconnect( this, 0, 0, 0 );
}

void Plugin::startServer()
{ 
  mHttpDaemon = new HttpDaemon( this );
  connect(mHttpDaemon, SIGNAL(loadRasterFile(QString)), this, SLOT(loadRasterFile(QString)));
  connect(mHttpDaemon, SIGNAL(loadRasterFile(QString,QString)), this, SLOT(loadRasterFile(QString,QString)));
  connect(mHttpDaemon, SIGNAL(loadVectorFile(QString)),this, SLOT(loadVectorFile(QString))) ;
  connect(mHttpDaemon, SIGNAL(loadVectorFile(QString,QString)), this, SLOT(loadVectorFile(QString,QString)));

}

void Plugin::stopServer()
{
  //break all connections to httpdeamon signals
  disconnect( mHttpDaemon, 0, 0, 0 );
  delete mHttpDaemon;
  
}

void Plugin::loadProject(QString theProjectFile)
{
  std::cout << "Recevied loadProject request to open " << theProjectFile << std::endl;
  //do all the stuff needed to open the project and take a snapshot of it
  if (!qGisInterface->addProject(theProjectFile))
  {
    //let the httpdserver know we are finished and pass it back the output filename
    mHttpDaemon->requestCompleted(QString("Failed opening project!"));
  }
}

void Plugin::loadRasterFile(QString theRasterFile)
{

  if ( qGisInterface->addRasterLayer(theRasterFile))
  {
    //let the httpdserver know we are finished and pass it back the canvas image
    mHttpDaemon->requestCompleted(qGisInterface->getMapCanvas()->canvasPixmap());
  }
  else
  {
    //let the httpdserver know we are finished and pass it back the error
    mHttpDaemon->closeStreamWithError(QString("Failed opening raster layer : ")+theRasterFile);
  }
}

void Plugin::loadRasterFile(QString theRasterFile, QString theProjectFile)
{
  loadProject(theProjectFile);
  loadRasterFile(theRasterFile);
}

void Plugin::loadVectorFile(QString theVectorFile)
{

  // Add a vector layer given vectorLayerPath, layer name, providerKey ("ogr" or "postgres");
  if ( qGisInterface->addVectorLayer(theVectorFile, QString("MapLayer"),QString("ogr")))
  {
    //let the httpdserver know we are finished and pass it back the canvas image
    mHttpDaemon->requestCompleted(qGisInterface->getMapCanvas()->canvasPixmap());
  }
  else
  {
    //let the httpdserver know we are finished and pass it back the error
    mHttpDaemon->closeStreamWithError(QString("Failed opening vector layer : ")+theVectorFile);
  }
}

void Plugin::loadVectorFile(QString theVectorFile, QString theProjectFile)
{
  loadProject(theProjectFile);
  loadVectorFile(theVectorFile);
}


///////////////////////////////////////////////////////////





/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
extern "C" QgisPlugin * classFactory(QgisApp * theQGisAppPointer, QgisIface * theQgisInterfacePointer)
{
  return new Plugin(theQGisAppPointer, theQgisInterfacePointer);
}

// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
extern "C" QString name()
{
    return name_;
}

// Return the description
extern "C" QString description()
{
    return description_;
}

// Return the type (either UI or MapLayer plugin)
extern "C" int type()
{
    return type_;
}

// Return the version number for the plugin
extern "C" QString version()
{
  return version_;
}

// Delete ourself
extern "C" void unload(QgisPlugin * thePluginPointer)
{
  delete thePluginPointer;
}
