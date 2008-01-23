/***************************************************************************
                          qgsprojectfiletransform.h  -  description
                             -------------------
    begin                : Sun 15 dec 2007
    copyright            : (C) 2007 by Magnus Homann
    email                : magnus at homann.se
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * This class provides possibility to store a project file as a QDomDocument,
 * and provides the ability to specify version of the project file, and 
 * perform upgrades to a more recent version
 */

/* $Id: $ */

#ifndef QGSPROJECTFILETRANSFORM_H
#define QGSPROJECTFILETRANSFORM_H

#include <QString>
#include <QDomDocument>
#include <vector>
#include "qgsprojectversion.h"

class QgsProjectFileTransform 
{
 public:
  //Default constructor
  //QgsProjectfiletransform() {}
  ~QgsProjectFileTransform() {}
    
  /*! Create an instance from a DOM and a supplied version
   * @param domDocument The DOM document to use as content
   * @param version Version number
   */
  QgsProjectFileTransform(QDomDocument & domDocument,
                 QgsProjectVersion version)
    {
      mDom = domDocument;
      mCurrentVersion = version;
    }


  bool updateRevision(QgsProjectVersion version);

  /*! Prints the contents via QgsDebugMsg()
   */
  void dump();


 private:

  typedef struct {
    QgsProjectVersion from;
    QgsProjectVersion to;
    void (QgsProjectFileTransform::* transformFunc)();
  } transform;

  static transform transformers[];;

  QDomDocument mDom;
  QgsProjectVersion mCurrentVersion;

  // Transformer functions below. Declare functions here,
  // define them in qgsprojectfiletransform.cpp and add them
  // to the transformArray with proper version number
  void transformNull() {}; // Do absolutely nothing
  void transform081to090();
  void transform091to092();
};


#endif //QGSPROJECTFILETRANSFORM_H

