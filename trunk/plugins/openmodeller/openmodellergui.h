/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   Gyps - Species Distribution Modelling Toolkit                         *
 *   This toolkit provides data transformation and visualisation           *
 *   tools for use in species distribution modelling tools such as GARP,   *
 *   CSM, Bioclim etc.                                                     *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef OPENMODELLERGUI_H
#define OPENMODELLERGUI_H

#include <openmodellerguibase.uic.h>
#include <qstringlist.h>
#include <qstring.h>
#include <openmodeller/om.hh>
#include <qmap.h> 


/**
@author Tim Sutton
*/
class OpenModellerGui : public OpenModellerGuiBase
{
Q_OBJECT
public:
    OpenModellerGui();
    OpenModellerGui( QWidget* parent , const char* name , bool modal , WFlags fl  );
    ~OpenModellerGui();
    
  //! This method is a callback function passed to oM to keep track of preogres of mapProjection
  static void mapCallback( float progress, void *extra_param );
  
    
  void getAlgorithmList();
  void getParameterList( QString theAlgorithmNameQString );
  void parseAndRun(QString theParametersFileNameQString);
  void makeConfigFile();
  
  //
  // The following methods reimplement methods by the same name of the parent class
  //    
    
    void cboModelAlgorithm_activated(  const QString & );
  /** This method overrides the virtual OpenModellerGuiBase method (slot) of the same name. */
  void formSelected(const QString &thePageNameQString);
  void leLocalitiesFileName_textChanged( const QString &theFileNameQString );
  void setSpeciesList(QString theFileNameQSting);
  void pbnSelectOutputDirectory_clicked();
  void pbnRemoveLayerFile_clicked();
  void pbnSelectLayerFile_clicked();
  void pbnSelectLocalitiesFile_clicked();
  void leLocalitiesFileName_returnPressed();
  void accept();
  void cboModelAlgorithm_highlighted( const QString &theModelAlgorithm );
  void leOutputFileName_textChanged( const QString &theOutputFileName);
  void leOutputDirectory_textChanged( const QString &theOutputDirectory);
  void pbnSelectLayerFolder_clicked();
  void traverseDirectories(const QString& dirname);
  bool checkLocalitiesFileFormat(const QString);
  void pbnDefaultParameters_clicked();
  
private:
    QString modelNameQString;
    QString localitiesFileNameQString;
    QString coordinateSystemQString;
    QString taxonNameQString;
    QStringList layerNamesQStringList;
    QString maskNameQString;
    QStringList extraParametersQStringList;
    QString outputFileNameQString;
    bool useTheseSettingsAgainFlag;
    typedef QMap<QString, QWidget *> ParametersMap;
    //setup QMap object and layout for the frame into which the controls will go
    ParametersMap mMap;
    typedef QMap<QString, QWidget *> ParameterLabels;
    ParameterLabels mLabelsMap;
    typedef QMap<QString,QString> ProjectionWKTMap; //wkt = well known text (see gdal/ogr)
    ProjectionWKTMap mProjectionsMap;
    QGridLayout* mLayout;
    void getProjList();
    
signals:
   void drawRasterLayer(QString);
};

#endif
