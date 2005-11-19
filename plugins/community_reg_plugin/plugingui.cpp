/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "plugingui.h"

//qt includes
#include <qmessagebox.h>
#include <q3url.h>
#include <qlineedit.h>
#include <q3http.h>
#include <qregexp.h>
#include <qcursor.h> 
#include <qapplication.h> 

#include <iostream>
//plugin includes
#include "qgslocationcapturewidget.h"

//standard includes

QgsCommunityRegPluginGui::QgsCommunityRegPluginGui() : QgsCommunityRegPluginGuiBase()
{

}

    QgsCommunityRegPluginGui::QgsCommunityRegPluginGui( QWidget* parent , const char* name , bool modal , Qt::WFlags fl  )
: QgsCommunityRegPluginGuiBase( parent, name, modal, fl )
{
  mConnection = new Q3Http();
  connect(mConnection, SIGNAL(done(bool)), this, SLOT(registrationDone(bool)));
}  
QgsCommunityRegPluginGui::~QgsCommunityRegPluginGui()
{
  delete mConnection;
  delete mHttp;
}

void QgsCommunityRegPluginGui::pbnOK_clicked()
{
  Q3Url myTargetUrl("http://community.qgis.org");
  QString myHost = myTargetUrl.host();
  int myPort = myTargetUrl.port();
  mHttp = new Q3Http(myHost, myPort == -1 ? 80 : myPort);
  connect(mHttp, SIGNAL(requestStarted(int)), this, 
          SLOT(slotRequestStarted(int)));
  connect(mHttp, SIGNAL(requestFinished(int, bool)), this, 
          SLOT(slotRequestFinished(int, bool)));
  connect(mHttp, SIGNAL(done(bool)), this, SLOT(slotDone(bool)));
  connect(mHttp, SIGNAL(dataReadProgress(int,int)), this, 
          SLOT(slotDataReadProgress(int,int)));
  connect(mHttp, SIGNAL(responseHeaderReceived(const 
                  Q3HttpResponseHeader&)),
          this, SLOT(slotResponseHeaderReceived(const Q3HttpResponseHeader 
                  &)));
  QByteArray *myByteArray=new QByteArray();
  QString tmp = "name=shie";
  myByteArray->setRawData(tmp.latin1(), tmp.length());     
  mHttp->post("/qgis_users/index.php", *myByteArray);
  done(1);
} 
void QgsCommunityRegPluginGui::pbnCancel_clicked()
{
  close(1);
}

void QgsCommunityRegPluginGui::pbnGetCoords_clicked()
{
  std::cout << "Get coords clicked" << std::endl;
  QgsLocationCaptureWidget * myWidget = new QgsLocationCaptureWidget();
  myWidget->show();
  delete myWidget;
}



void QgsCommunityRegPluginGui::submit()
{
  if (mConnection->state() == Q3Http::HostLookup
          || mConnection->state() == Q3Http::Connecting
          || mConnection->state() == Q3Http::Sending
          || mConnection->state() == Q3Http::Reading) {
    mConnection->abort();
  }

  if (leName->text().isEmpty() | 
      leEmail->text().isEmpty() |
      leLatitude->text().isEmpty() |
      leLongitude->text().isEmpty()   
      ) 
  {
    QMessageBox::critical(this, "Empty query",
            "Please type a submit string.",
            QMessageBox::Ok, Qt::NoButton);
  } 
  else 
  {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    mConnection->setHost("community.qgis.org");

    Q3HttpRequestHeader myHeader("POST", "/qgis_users/index.php");
    myHeader.setValue("Host", "community.qgis.org");
    myHeader.setContentType("application/x-www-form-urlmyEncoded");

    QString myEncodedName = leName->text();
    Q3Url::encode(myEncodedName);
    QString myEncodedEmail = leEmail->text();
    Q3Url::encode(myEncodedEmail);
    QString myEncodedImageUrl = leImageUrl->text();
    Q3Url::encode(myEncodedImageUrl);
    QString myEncodedHomeUrl = leHomeUrl->text();
    Q3Url::encode(myEncodedHomeUrl);
    QString myEncodedCountry = leCountry->text();
    Q3Url::encode(myEncodedCountry);
    QString myEncodedPlaceDescription = lePlaceDescription->text();
    Q3Url::encode(myEncodedPlaceDescription);
    QString myEncodedLatitude = leLatitude->text();
    Q3Url::encode(myEncodedLatitude);
    QString myEncodedLongitude = leLongitude->text();
    Q3Url::encode(myEncodedLongitude);
    QString myPostString = "formAction=save";
    myPostString += "&fldname=" + myEncodedName;
    myPostString += "&fldemail="+myEncodedEmail;
    myPostString += "&fldimage_url="+myEncodedImageUrl;
    myPostString += "&fldhome_url="+myEncodedHomeUrl;
    myPostString += "&fldcountry="+myEncodedCountry;
    myPostString += "&fldplace_description="+myEncodedPlaceDescription;
    myPostString += "&fldlatitude="+myEncodedLatitude;
    myPostString += "&fldlongitude="+myEncodedLongitude;

    mConnection->request(myHeader, myPostString.utf8());
  }

}

void QgsCommunityRegPluginGui::submitDone( bool error )
{
  if (error) {
    QMessageBox::critical(this, "Error submiting",
            "An error occurred when submiting: "
            + mConnection->errorString(),
            QMessageBox::Ok, Qt::NoButton);
  } else {
    QString result(mConnection->readAll());
    /*
     * TODO Implement checking of returned submission results page
     * 
     QRegExp rx("<a href=\"(http://lists\\.trolltech\\.com/qt-interest/.*)\">(.*)</a>");
     rx.setMinimal(TRUE);
     int pos = 0;
     while (pos >= 0) 
     {
     pos = rx.search(result, pos);
     if (pos > -1) 
     {
     pos += rx.matchedLength();
     new QListViewItem(myListView, rx.cap(2), rx.cap(1));
     }
     }
     */
    QApplication::restoreOverrideCursor();

  }
}
