/***************************************************************************
  quickprint.cpp 
  Quick Print is a plugin to quickly print a map with minimal effort.
  -------------------
         begin                : Jan 2008
         copyright            : (c) Tim Sutton, 2008
         email                : tim@linfiniti.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id: plugin.cpp 7796 2007-12-16 22:11:38Z homann $ */

//
// QGIS Specific includes
//

#include <qgisinterface.h>
#include <qgisgui.h>
#include "quickprintplugin.h"
#include "quickprintgui.h"
#include <qgsapplication.h>

//
// Qt4 Related Includes
//

#include <QAction>
#include <QToolBar>


static const char * const sIdent = "$Id: plugin.cpp 7796 2007-12-16 22:11:38Z homann $";
static const QString sName = QObject::tr("Quick Print");
static const QString sDescription = 
       QObject::tr("Quick Print is a plugin to quickly print a map with minimal effort.");
static const QString sPluginVersion = QObject::tr("Version 0.1");
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;

//////////////////////////////////////////////////////////////////////
//
// THE FOLLOWING METHODS ARE MANDATORY FOR ALL PLUGINS
//
//////////////////////////////////////////////////////////////////////

/**
 * Constructor for the plugin. The plugin is passed a pointer 
 * an interface object that provides access to exposed functions in QGIS.
 * @param theQGisInterface - Pointer to the QGIS interface object
 */
QuickPrintPlugin::QuickPrintPlugin(QgisInterface * theQgisInterface):
                 QgisPlugin(sName,sDescription,sPluginVersion,sPluginType),
                 mQGisIface(theQgisInterface)
{
  mpMapCanvas = mQGisIface->getMapCanvas();
}

QuickPrintPlugin::~QuickPrintPlugin()
{

}

/*
 * Initialize the GUI interface for the plugin - this is only called once when the plugin is 
 * added to the plugin registry in the QGIS application.
 */
void QuickPrintPlugin::initGui()
{

  // Create the action for tool
  mQActionPointer = new QAction(QIcon(":/quickprint/quick_print.png"),tr("Quick Print"), this);
  // Set the what's this text
  mQActionPointer->setWhatsThis(tr("Provides a qay to quickly produce a map with minimal user input."));
  // Connect the action to the run
  connect(mQActionPointer, SIGNAL(activated()), this, SLOT(run()));
  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon(mQActionPointer);
  mQGisIface->addPluginMenu(tr("&Quick Print"), mQActionPointer);

}
//method defined in interface
void QuickPrintPlugin::help()
{
  //implement me!
}

// Slot called when the menu item is activated
// If you created more menu items / toolbar buttons in initiGui, you should 
// create a separate handler for each action - this single run() method will
// not be enough
void QuickPrintPlugin::run()
{
  QuickPrintGui *mypPluginGui=new QuickPrintGui(
      mpMapCanvas,
      mQGisIface->getMainWindow(),
      QgisGui::ModalDialogFlags);
  mypPluginGui->setAttribute(Qt::WA_DeleteOnClose);
  mypPluginGui->exec(); //gui will decide when to call print
  
}

// Unload the plugin by cleaning up the GUI
void QuickPrintPlugin::unload()
{
  // remove the GUI
  mQGisIface->removePluginMenu("&Quick Print",mQActionPointer);
  mQGisIface->removeToolBarIcon(mQActionPointer);
  delete mQActionPointer;
}


//////////////////////////////////////////////////////////////////////////
//
//
//  THE FOLLOWING CODE IS AUTOGENERATED BY THE PLUGIN BUILDER SCRIPT
//    YOU WOULD NORMALLY NOT NEED TO MODIFY THIS, AND YOUR PLUGIN
//      MAY NOT WORK PROPERLY IF YOU MODIFY THIS INCORRECTLY
//
//
//////////////////////////////////////////////////////////////////////////


/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory(QgisInterface * theQgisInterfacePointer)
{
  return new QuickPrintPlugin(theQgisInterfacePointer);
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

// Delete ourself
QGISEXTERN void unload(QgisPlugin * thePluginPointer)
{
  delete thePluginPointer;
}
