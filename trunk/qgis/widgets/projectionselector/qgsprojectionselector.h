/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef QGSPROJECTIONSELECTOR_H
#define QGSPROJECTIONSELECTOR_H

#ifndef WIN32
#include "qgsprojectionselectorbase.uic.h"
#else
#include "qgsprojectionselectorbase.h"
#endif

#include <qgis.h>
#include <qstring.h>
#include <qlistview.h>


/**
@author Tim Sutton
*/
class QgsProjectionSelector: public QgsProjectionSelectorBase
{
Q_OBJECT
public:
    QgsProjectionSelector( QWidget* parent , const char* name ,WFlags fl =0  );
    ~QgsProjectionSelector();
  //! Populate the proj tree view with  user defined projection names...
  void getUserProjList();
  //! Populate the proj tree view with system projection names...
  void getProjList();
  
    
public slots:
    void setSelectedWKT(QString theWKT);
    QString getSelectedWKT();
    void setSelectedSRID(QString theSRID);
    QString getCurrentWKT();
    QString getCurrentSRID();
private:

  // List view nodes for the tree view of projections
  //! User defined projections node
  QListViewItem *mUserProjList;
  //! GEOGCS node
  QListViewItem *mGeoList;
  //! PROJCS node
  QListViewItem *mProjList;
  //! Users custom coordinate system file
  QString mCustomCsFile;
  //! File name of the sqlite3 database
  QString mSrsDatabaseFileName;

  //private handler for when user selects a cs
  //it will cause wktSelected and sridSelected events to be spawned
  void coordinateSystemSelected(QListViewItem*);
  
signals:
    void wktSelected(QString theWKT);
    void sridSelected(QString theSRID);
    //! Refresh any listening canvases
    void refresh();
};

#endif
