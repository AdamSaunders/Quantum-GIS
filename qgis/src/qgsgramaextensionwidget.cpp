/***************************************************************************
                          qgsgramaextensionwidget.cpp  -  description
                             -------------------
    begin                : April 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id */

#include "qgsgramaextensionwidget.h"
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpicture.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include "qgsmarkerdialog.h"
#include "qgsdataprovider.h"
#include "qgsvectorlayer.h"

QgsGraMaExtensionWidget::QgsGraMaExtensionWidget(QWidget* parent, int classfield, QgsGraSyDialog::mode mode, int nofclasses, QgsVectorLayer* vlayer): QScrollView(parent), mClassField(classfield), mWidget(new QWidget(viewport())), mGridLayout(new QGridLayout(mWidget, 1, 5)), mMode(mode), mNumberOfClasses(nofclasses), mVectorLayer(vlayer)
{
#ifdef QGISDEBUG
    qWarning("constructor QgsGraSyExtensionWidget");
#endif
 
    setResizePolicy(QScrollView::AutoOneFit);
    mGridLayout->setSpacing(5);

    //fill the title line into the grid layout
    QLabel *lvaluelabel = new QLabel(tr("Lower"),mWidget);
    lvaluelabel->setMaximumHeight(50);
    mGridLayout->addWidget(lvaluelabel, 0, 0);
    QLabel *uvaluelabel = new QLabel(tr("Upper"),mWidget);
    uvaluelabel->setMaximumHeight(50);
    mGridLayout->addWidget(uvaluelabel, 0, 1);
    QLabel *labellabel = new QLabel(tr("Label"),mWidget);
    labellabel->setMaximumHeight(50);
    mGridLayout->addWidget(labellabel, 0, 2);
    QLabel *imagelabel = new QLabel(tr("Image"),mWidget);
    mGridLayout->addWidget(imagelabel,0,3);
    QLabel *scalelabel = new QLabel(tr("Scale\nFactor"),mWidget);
    mGridLayout->addWidget(scalelabel,0,4);

    //find the minimum and maximum of the classification variable
    double minimum, maximum;

    QgsDataProvider *provider = mVectorLayer->getDataProvider();
    if (provider)
    {
	if (mMode == QgsGraSyDialog::EQUAL_INTERVAL)
	{
	    minimum = provider->minValue(mClassField).toDouble();
	    maximum = provider->maxValue(mClassField).toDouble();
	} 
	else                    //don't waste performance if mMode is QgsGraSyDialog::EMPTY
	{
	    minimum = 0;
	    maximum = 0;
	}
    } 
    else
    {
      qWarning("Warning, provider is null in QgsGraMaExtensionWidget::QgsGraMaExtensionWidget(...)");
      return;
    }

    mWidgetVector.resize(mNumberOfClasses * 5);

    //create the required number of rows
    for (int i = 1; i <= mNumberOfClasses; i++)
    {
	QLineEdit *ltextfield = new QLineEdit(mWidget);
	mGridLayout->addWidget(ltextfield, i, 0);
	mWidgetVector[5 * (i - 1)] = ltextfield;
	ltextfield->setAlignment(Qt::AlignLeft);

	QLineEdit *utextfield = new QLineEdit(mWidget);
	mGridLayout->addWidget(utextfield, i, 1);
	mWidgetVector[5 * (i - 1) + 1] = utextfield;
	ltextfield->setAlignment(Qt::AlignLeft);

	QLineEdit *labeltextfield = new QLineEdit(mWidget);
	mGridLayout->addWidget(labeltextfield, i, 2);
	mWidgetVector[5 * (i - 1) + 2] = labeltextfield;
	ltextfield->setAlignment(Qt::AlignLeft);

	QPushButton *imageButton = new QPushButton(mWidget);
	mGridLayout->addWidget(imageButton, i, 3);
	mWidgetVector[ 5 * (i - 1) + 3] = imageButton;
	QObject::connect(imageButton, SIGNAL(clicked()), this, SLOT(selectMarker()));

	QLineEdit *scaleedit = new QLineEdit(mWidget);
	mGridLayout->addWidget(scaleedit,i,4);
	mWidgetVector[ 5 * (i - 1) + 4] = scaleedit;
	scaleedit->setText("1.0");
	QObject::connect(scaleedit, SIGNAL(returnPressed()), this, SLOT(handleReturnPressed()));

        /*Set the default values of the lower and upper bounds according to the chosen mode */
	if (mMode == QgsGraSyDialog::EQUAL_INTERVAL)
        {
	    ltextfield->setText(QString::number(minimum + (maximum - minimum) / mNumberOfClasses * (i - 1), 'f', 2));
	    utextfield->setText(QString::number(minimum + (maximum - minimum) / mNumberOfClasses * i, 'f', 2));
        }

	
    }

    addChild(mWidget);
    //resizeContents(200,50*(mNumberOfClasses+1));
    //updateContents();
    
}

int QgsGraMaExtensionWidget::classfield()
{
    return mClassField;
}

QgsGraMaExtensionWidget::~QgsGraMaExtensionWidget()
{
#ifdef QGISDEBUG
    qWarning("destructor QgsGraMaExtensionWidget");
#endif
}

QgsGraMaExtensionWidget::QgsGraMaExtensionWidget()
{
#ifdef QGISDEBUG
    qWarning("constructor QgsGraMaExtensionWidget");
#endif
}

void QgsGraMaExtensionWidget::selectMarker()
{
    QgsMarkerDialog mdialog(QString(PKGDATAPATH)+"/svg");

    if(mdialog.exec()==QDialog::Accepted)
    {
	QString svgfile=mdialog.selectedMarker();
	((QPushButton *) sender())->setName(svgfile);
	//draw the SVG-Image on the button
	QPicture pic;

	int indexnumber;//index position of the sender button

	//find out the row of the sender to check for the chosen scale factor
	for(int i=0;i<=mNumberOfClasses;++i)
	{
	    if(sender()==mWidgetVector[5*i+3])
	    {
		indexnumber=i;
		break;
	    } 
	}

#ifdef QGISDEBUG	
	qWarning("indexnumber: "+QString::number(indexnumber));
#endif
	adjustMarker(indexnumber);
    }
}

void QgsGraMaExtensionWidget::adjustMarkers()
{
    for(int i=0;i<mNumberOfClasses;++i)
    {
	adjustMarker(i);
    }
}

void QgsGraMaExtensionWidget::adjustMarker(int row)
{
    double scalefactor=((QLineEdit*)mWidgetVector[row*5+4])->text().toDouble();	
    QPicture pic;
    pic.load(((QPushButton*)mWidgetVector[row*5+3])->name(),"svg");
    int width=(int)(pic.boundingRect().width()*scalefactor);
    int height=(int)(pic.boundingRect().height()*scalefactor);
	
    //prevent 0 width or height, which would cause a crash
    if(width==0)
    {
	width=1;
    }
    if(height==0)
    {
	height=1;
    }

    QPixmap pixmap(height,width);
    pixmap.fill();
    QPainter p(&pixmap);
    p.scale(scalefactor,scalefactor);
    p.drawPicture(0,0,pic);
    ((QPushButton *)mWidgetVector[row*5+3])->setPixmap(pixmap); 
}

void QgsGraMaExtensionWidget::handleReturnPressed()
{
    //find out the row of the sender
    int indexnumber;
    for(int i=0;i<=mNumberOfClasses;++i)
    {
	if(sender()==mWidgetVector[5*i+4])
	{
	    indexnumber=i;
	    break;
	} 
    }
    adjustMarker(indexnumber); 
}
