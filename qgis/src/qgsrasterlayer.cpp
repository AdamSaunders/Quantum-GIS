/***************************************************************************
  qgsrasterlayer.cpp -  description
  -------------------
begin                : Sat Jun 22 2002
copyright            : (C) 2003 by Tim Sutton, Steve Halasz and Gary E.Sherman
email                : tim at linfiniti.com
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

/*
 Please observe the following variable naming guidelines when editing this class:
---------------------------------------------------------------------------------
In my opinion, clarity of code is more important than brevity, so variables should be 
given clear, unambiguous names. Variables names should be written in mixed case, with 
a lowercase first letter. Each variable name should include a scope resolution 
indicator and a type indicator, in the form:
 
[scope]+[name]+[type]
 
Where scope resolution indicators are:
 
- global vars and class members : [none]
- variables passed as parameters to a function/method: the
- variables declared locally in a method or function: my
 
For example:
 
class FooClass {
  int fooInt;  //class var has no prefix
 
  void FooClass::fooMethod (int theBarInt)  //function parameter prefixed by 'the'
  {
    fooInt=1;
    int myLocalInt=0; //function members prefixed by 'my'
    myLocalInt=fooInt+theBarInt;
  }
}
 
Using this scope resolution naming scheme makes the origin of each variable unambiguous
and the code easy to read (especially by people who did not write it!).
 
The [name] part of the variable should be short and descriptive, usually a noun.
 
The [type] part of the variable should be the type class of the variable written out in full.
*/

#include <qpainter.h>
#include <qimage.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qwmatrix.h>
#include <qpopupmenu.h>
#include <stdio.h>
#include "qgsrasterlayer.h"
#include "qgsrect.h"
#include "qgisapp.h"
#include "qgsrasterlayerproperties.h"
#include "gdal_priv.h"

QgsRasterLayer::QgsRasterLayer(QString path, QString baseName)
        :QgsMapLayer(RASTER, baseName, path)
{
    popMenu=0;
    //std::cout << "QgsRasterLayer::QgsRasterLayer()" << std::endl;

    GDALAllRegister();
    gdalDataset = (GDALDataset *) GDALOpen( path, GA_ReadOnly );
    if ( gdalDataset == NULL )
    {
        valid = FALSE;
        return;
    }
    //std::cout << "Raster Count: " << gdalDataset->GetRasterCount() << std::endl;

    if( gdalDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        printf( "Origin = (%.6f,%.6f)\n",
                adfGeoTransform[0], adfGeoTransform[3] );

        printf( "Pixel Size = (%.6f,%.6f)\n",
                adfGeoTransform[1], adfGeoTransform[5] );
    }

    double myXMaxDouble = adfGeoTransform[0] + gdalDataset->GetRasterXSize() * adfGeoTransform[1] +
                          gdalDataset->GetRasterYSize() * adfGeoTransform[2];
    double myYMinDouble  = adfGeoTransform[3] + gdalDataset->GetRasterXSize() * adfGeoTransform[4] +
                           gdalDataset->GetRasterYSize() * adfGeoTransform[5];

    layerExtent.setXmax(myXMaxDouble);
    layerExtent.setXmin(adfGeoTransform[0]);
    layerExtent.setYmax(adfGeoTransform[3]);
    layerExtent.setYmin(myYMinDouble);

    //decide what type of layer this is...
    if ((gdalDataset->GetRasterCount() > 1))
    {
        rasterLayerType = MULTIBAND;
    }
    else if (hasBand("Palette"))
    {
        rasterLayerType = PALETTE;
    }
    else
    {
        rasterLayerType = GRAY_OR_UNDEFINED;
    }

    if (rasterLayerType==PALETTE)
    {
        redBandNameQString="Red"; // sensible default
        greenBandNameQString="Green"; // sensible default
        blueBandNameQString="Blue"; // sensible default
        grayBandNameQString="Not Set"; //sensible default
        drawingStyle=PALETTED_MULTI_BAND_COLOR; //sensible default
    }
    else if (rasterLayerType==MULTIBAND)
    {
        redBandNameQString="Red"; // sensible default
        greenBandNameQString="Green"; // sensible default
        blueBandNameQString="Blue"; // sensible default
        grayBandNameQString="Not Set"; //sensible default
        drawingStyle=MULTI_BAND_COLOR; //sensible default
    }
    else //GRAY_OR_UNDEFINED
    {
        calculateStats("");
        redBandNameQString="Not Set"; //sensible default
        greenBandNameQString="Not Set"; //sensible default
        blueBandNameQString="Not Set"; //sensible default
        if (hasBand("Gray"))
        {
            grayBandNameQString="Gray"; // sensible default
        }
        else if (hasBand("Undefined"))
        {
            grayBandNameQString="Undefined"; // sensible default
        }
    }

    invertHistogramFlag=false; // sensible default
    stdDevsToPlotDouble = 0; // sensible default
    transparencyLevelInt = 255; //sensible default 0 is transparent
    showDebugOverlayFlag=false; //sensible default
    // emit a signal asking for a repaint
    emit repaintRequested();
}

QgsRasterLayer::~QgsRasterLayer()
{
    GDALClose(gdalDataset);
}
void QgsRasterLayer::showLayerProperties()
{
    QgsRasterLayerProperties *myRasterLayerProperties = new QgsRasterLayerProperties(this);

}
// emit a signal asking for a repaint
void QgsRasterLayer::triggerRepaint()
{
    emit repaintRequested();
}
//
//should be between 0 and 255
void QgsRasterLayer::setTransparency(unsigned int theInt)
{
    if (theInt > 255)
    {
        transparencyLevelInt= 255;
    }
    else
    {
        transparencyLevelInt=theInt;
    }
}
unsigned int QgsRasterLayer::getTransparency()
{
    return  transparencyLevelInt;
}

bool QgsRasterLayer::hasBand(QString theBandName)
{
    RasterStatsMap::Iterator myIterator;
    std::cout << "Looking for band : " << theBandName << std::endl;
    for (int i = 1; i <= gdalDataset->GetRasterCount(); i++)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( i );
        QString myColorQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        std::cout << "band : " << i << std::endl;
        if  (myColorQString==theBandName)
        {
            std::cout << "band : " << i << std::endl;
            std::cout << "Found band : " << theBandName << std::endl;
            return true;
        }
        std::cout << "Found unmatched band : " << i << " " << myColorQString << std::endl;
    }
    return false;
}

void QgsRasterLayer::draw(QPainter * theQPainter, QgsRect * theViewExtent, QgsCoordinateTransform * theQgsCoordinateTransform)
{
    //
    // ---------------------------------------------------
    //These are typical assignements made in this routine:
    // ---------------------------------------------------
    //
    //QPainter Variables:
    //{ "wx", "QCOORD", "0" }
    //{ "wy", "QCOORD", "0" }
    //{ "ww", "QCOORD", "552" }
    //{ "wh", "QCOORD", "360" }
    //{ "vx", "QCOORD", "0" }
    //{ "vy", "QCOORD", "0" }
    //{ "vw", "QCOORD", "552" }
    //{ "vh", "QCOORD", "360" }
    //
    // ViewExtent Variables:
    // { "xmin", "double", "9.5477478992443516e-307" }
    // { "ymin", "double", "2.5145588030892154e-91" }
    // { "xmax", "double", "1.4158750209781043e-308" }
    // { "ymax", "double", "571082560307492" }
    //
    //myRasterExtent Variables:
    //{ "xmin", "double", "-10.729980070443938" }
    //{ "ymin", "double", "35.899496437673918" }
    //{ "xmax", "double", "29.896874152460249" }
    //{ "ymax", "double", "69.805551356045626" }

    //RasterLayer Draw Variables:
    //{ "rectXOffsetInt", "int", "0" }
    //{ "rectYOffsetInt", "int", "0" }
    //{ "clippedXMinDouble", "double", "0" }
    //{ "clippedXMaxDouble", "double", "7532" }
    //{ "clippedYMinDouble", "double", "1" }
    //{ "clippedWidthInt", "int", "7532" }
    //{ "clippedHeightInt", "int", "6285" }

    //TopLeftPoint Variable:
    //{ "m_x", "double", "79.927920631706826" }
    //{ "m_y", "double", "16.363636363636321" }

    //BottomRightPoint Variable:
    //{ "m_x", "double", "472.0720793682932" }
    //{ "m_y", "double", "343.63636363636363" }

    // Layer Variables:
    //{ "drawableAreaXDimInt", "int", "393" }
    //{ "drawableAreaYDimInt", "int", "327" }

    //
    // ---------------------------------------------------
    //

    //std::cout << "QgsRasterLayer::draw()" << std::endl;
    //std::cout << "gdalDataset->GetRasterCount(): " << gdalDataset->GetRasterCount() << std::endl;
    //std::cout << "Layer extent: " << layerExtent.stringRep() << std::endl;

    // clip raster extent to view extent
    QgsRect myRasterExtent = theViewExtent->intersect(&layerExtent);
    if (myRasterExtent.isEmpty())
    {
        // nothing to do
        return;
    }

    //
    // The first thing we do is set up the RasterViewPort. This struct stores all the settings
    // relating to the size (in pixels and coordinate system units) of the raster part that is
    // in view in the map window. It also stores the origin.
    //
    //this is not a class level member because every time the user pans or zooms
    //the contents of the rasterViewPort will change
    RasterViewPort * myRasterViewPort = new RasterViewPort();

    // calculate raster pixel offsets from origin to clipped rect
    // we're only interested in positive offsets where the origin of the raster
    // is northwest of the origin of the view
    myRasterViewPort->rectXOffsetInt = static_cast<int>((theViewExtent->xMin() - layerExtent.xMin()) / fabs(adfGeoTransform[1]));
    myRasterViewPort->rectXOffsetInt = myRasterViewPort->rectXOffsetInt >? 0;
    myRasterViewPort->rectYOffsetInt = static_cast<int>((layerExtent.yMax() - theViewExtent->yMax()) / fabs(adfGeoTransform[5]));
    myRasterViewPort->rectYOffsetInt = myRasterViewPort->rectYOffsetInt >? 0;

    //get the first raster band so we can dinf out the nodata value
    GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( 1 );



    myRasterViewPort->noDataDouble = myGdalBand->GetNoDataValue();
    //std::cout << "Nodata value for band " << i << " is " << noDataDouble << "\n" << std::endl;
    //std::cout << "myGdalBand->GetOverviewCount(): " << myGdalBand->GetOverviewCount() <<std::endl;

    // get dimensions of clipped raster image in raster pixel space/ RasterIO will do the scaling for us.
    // So for example, if the user is zoomed in a long way, there may only be e.g. 5x5 pixels retrieved from
    // the raw raster data, but rasterio will seamlessly scale the up to whatever the screen coordinats are
    // e.g. a 600x800 display window (see next section below)
    myRasterViewPort->clippedXMinDouble = (myRasterExtent.xMin() - adfGeoTransform[0]) / adfGeoTransform[1];
    myRasterViewPort->clippedXMaxDouble = (myRasterExtent.xMax() - adfGeoTransform[0]) / adfGeoTransform[1];
    myRasterViewPort->clippedYMinDouble = (myRasterExtent.yMin() - adfGeoTransform[3]) / adfGeoTransform[5];
    myRasterViewPort->clippedYMaxDouble = (myRasterExtent.yMax() - adfGeoTransform[3]) / adfGeoTransform[5];
    myRasterViewPort->clippedWidthInt = abs(static_cast<int>(myRasterViewPort->clippedXMaxDouble - myRasterViewPort->clippedXMinDouble));
    myRasterViewPort->clippedHeightInt = abs(static_cast<int>(myRasterViewPort->clippedYMaxDouble - myRasterViewPort->clippedYMinDouble));
    // make sure we don't exceed size of raster
    myRasterViewPort->clippedWidthInt = myRasterViewPort->clippedWidthInt <? myGdalBand->GetXSize();
    myRasterViewPort->clippedHeightInt = myRasterViewPort->clippedHeightInt <? myGdalBand->GetYSize();

    // get dimensions of clipped raster image in device coordinate space (this is the size of the viewport)
    myRasterViewPort->topLeftPoint = theQgsCoordinateTransform->transform(myRasterExtent.xMin(), myRasterExtent.yMax());
    myRasterViewPort->bottomRightPoint = theQgsCoordinateTransform->transform(myRasterExtent.xMax(), myRasterExtent.yMin());
    myRasterViewPort->drawableAreaXDimInt = myRasterViewPort->bottomRightPoint.xToInt() -
                                            myRasterViewPort->topLeftPoint.xToInt();
    myRasterViewPort->drawableAreaYDimInt = myRasterViewPort->bottomRightPoint.yToInt() -
                                            myRasterViewPort->topLeftPoint.yToInt();
    QString myColorQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());


    //
    //
    // The goal here is to make as many decisions as possible early on (outside of the rendering loop)
    // so that we can maximise performance of the rendering process. So now we check which drawing
    // procedure to use :
    //
    switch (drawingStyle)
    {
        // a "Gray" or "Undefined" layer drawn as a range of gray colors
    case SINGLE_BAND_GRAY:
            drawSingleBandGray(theQPainter,myRasterViewPort,myGdalBand);
        break;
        // a "Gray" or "Undefined" layer drawn using a pseudocolor algorithm
    case SINGLE_BAND_PSEUDO_COLOR:
        drawSingleBandPseudoColor(theQPainter,myRasterViewPort,myGdalBand);
        break;
        // a "Palette" layer drawn in gray scale (using only one of the color components)
    case PALETTED_SINGLE_BAND_GRAY:
        drawPalettedSingleBandGray(theQPainter, myRasterViewPort, myGdalBand,grayBandNameQString);
        break;
        // a "Palette" layer having only one of its color components rendered as psuedo color
    case PALETTED_SINGLE_BAND_PSEUDO_COLOR:
        drawPalettedSingleBandPseudoColor(theQPainter, myRasterViewPort, myGdalBand, grayBandNameQString);
        break;
        //a "Palette" image where the bands contains 24bit color info and 8 bits is pulled out per color
    case PALETTED_MULTI_BAND_COLOR:
        drawPalettedMultiBandColor(theQPainter, myRasterViewPort, myGdalBand);
        break;
        // a layer containing 2 or more bands, but using only one band to produce a grayscale image
    case MULTI_BAND_SINGLE_BAND_GRAY:
       drawMultiBandSingleBandGray(theQPainter, myRasterViewPort, myGdalBand);
        break;
        //a layer containing 2 or more bands, but using only one band to produce a pseudocolor image
    case MULTI_BAND_SINGLE_BAND_PSEUDO_COLOR:
        drawMultiBandSingleBandPseudoColor(theQPainter, myRasterViewPort, myGdalBand);
        break;
        //a layer containing 2 or more bands, mapped to the three RGBcolors.
        //In the case of a multiband with only two bands, one band will have to be mapped to more than one color
    case MULTI_BAND_COLOR:
        drawMultiBandColor(theQPainter,myRasterViewPort);    
        break;

    default:
        break;

    }

    //see if debug info is wanted
    if (showDebugOverlayFlag)
    {
        showDebugOverlay(theQPainter,myRasterViewPort)
        ;
    };
    delete myRasterViewPort;

} //end of draw method

void QgsRasterLayer::drawSingleBandGray(QPainter * theQPainter,
                                        RasterViewPort * theRasterViewPort,
                                        GDALRasterBand   * theGdalBand)
{
    std::cout << "QgsRasterLayer::drawSingleBandGray called" << std::endl;
    //create the outout image that the layer will be drawn on before placing it in the qcanvas

   // QPixmap * myQPixmap=new QPixmap(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt);

    // read entire clipped area of raster band
    // treat myGdalScanData as a pseudo-multidimensional array
    // RasterIO() takes care of scaling down image
    uint *myGdalScanData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    CPLErr myResultCPLerr = theGdalBand->RasterIO(
                                GF_Read, theRasterViewPort->rectXOffsetInt,
                                theRasterViewPort->rectYOffsetInt,
                                theRasterViewPort->clippedWidthInt,
                                theRasterViewPort->clippedHeightInt,
                                myGdalScanData,
                                theRasterViewPort->drawableAreaXDimInt,
                                theRasterViewPort->drawableAreaYDimInt,
                                GDT_UInt32, 0, 0 );

    QString myColorInterpretation = GDALGetColorInterpretationName(theGdalBand->GetColorInterpretation());
    //std::cout << "Colour Interpretation for this band is : " << myColorInterpretation << std::endl;

    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);
    RasterBandStats myRasterBandStats = rasterStatsMap[myColorInterpretation];
    double myRangeDouble=myRasterBandStats.rangeDouble;
    // print each point in myGdalScanData with equal parts R, G ,B o make it show as gray
    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            int myGrayValInt=myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt];
            //remove these lines!
            //if (myColumnInt==0)
            //std::cout << "Checking if " << myGrayValInt << " = " << myRasterBandStats.noDataDouble << std::endl;

            //dont draw this point if it is no data !
            if (myGrayValInt != myRasterBandStats.noDataDouble)
            {
                // We need to make sure the values are 0-255
                myGrayValInt = static_cast<int>((255/myRangeDouble) * myGrayValInt);

                if (invertHistogramFlag)
                    myGrayValInt=255-myGrayValInt;
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( myGrayValInt, myGrayValInt, myGrayValInt, transparencyLevelInt ));
            }
            else //render no data as 100% transparent
            {
                //0 alpha = completely transparent
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( 255, 255, 255, 0 ));
            }
        }
    }
    //part of the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);
}


void QgsRasterLayer::drawSingleBandPseudoColor(QPainter * theQPainter, RasterViewPort * theRasterViewPort, GDALRasterBand * theGdalBand)
{
    std::cout << "QgsRasterLayer::drawSingleBandPseudoColor called" << std::endl;
    //create the outout image that the layer will be drawn on before placing it in the qcanvas

    QPixmap * myQPixmap=new QPixmap(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt);
    //myQPixmap.setAlphaBuffer(true); //only supported for qimage
    //create a temporary qpainter that can be used for xoring red, green and blue data onto
    //the image.
    QPainter myQPainter;
    myQPainter.begin(myQPixmap);
    // read entire clipped area of raster band
    // treat myGdalScanData as a pseudo-multidimensional array
    // RasterIO() takes care of scaling down image
    uint *myGdalScanData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    CPLErr myResultCPLerr = theGdalBand->RasterIO(
                                GF_Read, theRasterViewPort->rectXOffsetInt,
                                theRasterViewPort->rectYOffsetInt,
                                theRasterViewPort->clippedWidthInt,
                                theRasterViewPort->clippedHeightInt,
                                myGdalScanData,
                                theRasterViewPort->drawableAreaXDimInt,
                                theRasterViewPort->drawableAreaYDimInt,
                                GDT_UInt32, 0, 0 );

    QString myColorInterpretation = GDALGetColorInterpretationName(theGdalBand->GetColorInterpretation());
    //std::cout << "Colour Interpretation for this band is : " << myColorInterpretation << std::endl;

    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);
    RasterBandStats myRasterBandStats = rasterStatsMap[myColorInterpretation];
    double myRangeDouble=myRasterBandStats.rangeDouble;
    int myRedInt=0;
    int myGreenInt=0;
    int myBlueInt=0;
    //calculate the adjusted matrix stats - which come into affect if the user has chosen
    RasterBandStats myAdjustedRasterBandStats = rasterStatsMap[myColorInterpretation];
    myAdjustedRasterBandStats.noDataDouble=0;//hard coding for now
    //to histogram stretch to a given number of std deviations
    //see if we are using histogram stretch using stddev and plot only within the selected number of deviations if we are
    //cout << "stdDevsToPlotDouble: " << cboStdDev->currentText() << " converted to " << stdDevsToPlotDouble << endl;
    if (stdDevsToPlotDouble > 0)
    {
        //work out how far on either side of the mean we should include data
        float myTotalDeviationDouble = stdDevsToPlotDouble * myAdjustedRasterBandStats.stdDevDouble;
        //printf("myTotalDeviationDouble: %i\n" , myTotalDeviationDouble );
        //adjust min and max accordingly
        //only change min if it is less than mean  -  (n  x  deviations)
        if (myAdjustedRasterBandStats.minValDouble < (myAdjustedRasterBandStats.meanDouble-myTotalDeviationDouble))
        {
            myAdjustedRasterBandStats.minValDouble=(myAdjustedRasterBandStats.meanDouble-myTotalDeviationDouble);
            //cout << "Adjusting minValDouble to: " << myAdjustedRasterBandStats.minValDouble << endl;
        }
        //only change max if it is greater than mean  +  (n  x  deviations)
        if (myAdjustedRasterBandStats.maxValDouble > (myAdjustedRasterBandStats.meanDouble + myTotalDeviationDouble))
        {
            myAdjustedRasterBandStats.maxValDouble=(myAdjustedRasterBandStats.meanDouble+myTotalDeviationDouble);
            //cout << "Adjusting maxValDouble to: " << myAdjustedRasterBandStats.maxValDouble << endl;
        }
        //update the range
        myAdjustedRasterBandStats.rangeDouble = myAdjustedRasterBandStats.maxValDouble-myAdjustedRasterBandStats.minValDouble;
    }
    //set up the three class breaks for pseudocolour mapping
    double myBreakSizeDouble = myAdjustedRasterBandStats.rangeDouble / 3;
    double myClassBreakMin1 = myAdjustedRasterBandStats.minValDouble;
    double myClassBreakMax1 = myAdjustedRasterBandStats.minValDouble + myBreakSizeDouble;
    double myClassBreakMin2 = myClassBreakMax1;
    double myClassBreakMax2 = myClassBreakMin2 + myBreakSizeDouble;
    double myClassBreakMin3 = myClassBreakMax2;
    double myClassBreakMax3 = myAdjustedRasterBandStats.maxValDouble;


    myQImage.setAlphaBuffer(true);

    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            //hardcoding to white to start with
            myRedInt   = 255;
            myBlueInt  = 255;
            myGreenInt = 255;
            int myInt=myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt];
            // draw this point if it is not  no_data !
            if (myInt!=myAdjustedRasterBandStats.noDataDouble)
            {
                //double check that myInt >= min and <= max
                //this is relevant if we are plotting within stddevs
                if (myInt < myAdjustedRasterBandStats.minValDouble )
                {
                    myInt = static_cast<int>(myAdjustedRasterBandStats.minValDouble);
                }
                if (myInt > myAdjustedRasterBandStats.maxValDouble)
                {
                    myInt = static_cast<int>(myAdjustedRasterBandStats.maxValDouble);
                }
                if(!invertHistogramFlag)
                {
                    //check if we are in the first class break
                    if ((myInt >= myClassBreakMin1) &&  (myInt < myClassBreakMax1) )
                    {
                        myRedInt = 0;
                        myBlueInt = 255;
                        myGreenInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * (myInt-myClassBreakMin1))*3);
                    }
                    //check if we are in the second class break
                    else if ((myInt >= myClassBreakMin2) &&  (myInt < myClassBreakMax2) )
                    {
                        myRedInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3);
                        myBlueInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3));
                        myGreenInt = 255;
                    }
                    //otherwise we must be in the third classbreak
                    else
                    {
                        myRedInt = 255;
                        myBlueInt = 0;
                        myGreenInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin3)/1)*3)));
                    }
                }
                else  //invert histogram toggle is on
                {
                    //check if we are in the first class break
                    if ((myInt >= myClassBreakMin1) &&  (myInt < myClassBreakMax1) )
                    {
                        myRedInt = 255;
                        myBlueInt = 0;
                        myGreenInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin1)/1)*3));
                    }
                    //check if we are in the second class break
                    else if ((myInt >= myClassBreakMin2) &&  (myInt < myClassBreakMax2) )
                    {
                        myRedInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3));
                        myBlueInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3);
                        myGreenInt = 255;
                    }
                    //otherwise we must be in the third classbreak
                    else
                    {
                        myRedInt = 0;
                        myBlueInt = 255;
                        myGreenInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * (myInt-myClassBreakMin3))*3));
                    }
                }
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( myRedInt, myGreenInt, myBlueInt, transparencyLevelInt ));
            } //end of nodata=false check
            else //nodata so draw transparent
            {
                //0 alpha = completely transparent
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( 255, 255, 255, 0 ));
            }
        }//end of columnwise loop
    }//end of towwise loop
    //draw with the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);
}

/**
* This method is used to render a paletted raster layer as a gray image.
* @param theQPainter - pointer to the QPainter onto which the layer should be drawn.
* @param theRasterViewPort - pointer to the ViewPort struct containing dimensions of viewable area and subset area to be extracted from data file.
* @param theGdalBand - pointer to the GDALRasterBand which should be rendered.
* @param theColorQString - QString containing either 'Red' 'Green' or 'Blue' indicating which part of the rgb triplet will be used to render gray.
*/
void QgsRasterLayer::drawPalettedSingleBandGray(QPainter * theQPainter,
        RasterViewPort * theRasterViewPort,
        GDALRasterBand * theGdalBand,
        QString theColorQString)
{
    std::cout << "QgsRasterLayer::drawPalettedSingleBandGray called" << std::endl;
    // read entire clipped area of raster band
    // treat myGdalScanData as a pseudo-multidimensional array
    // RasterIO() takes care of scaling down image
    uint *myGdalScanData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    CPLErr myResultCPLerr = theGdalBand->RasterIO(
                                GF_Read, theRasterViewPort->rectXOffsetInt,
                                theRasterViewPort->rectYOffsetInt,
                                theRasterViewPort->clippedWidthInt,
                                theRasterViewPort->clippedHeightInt,
                                myGdalScanData,
                                theRasterViewPort->drawableAreaXDimInt,
                                theRasterViewPort->drawableAreaYDimInt,
                                GDT_UInt32, 0, 0 );

    // print each point in myGdalScanData using color looked up in color table
    GDALColorTable *colorTable = theGdalBand->GetColorTable();
    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);



    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            const GDALColorEntry *myColorEntry = GDALGetColorEntry(colorTable, myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt]);
            //dont draw this point if it is no data !
            if (theRasterViewPort->noDataDouble != myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt])
            {
                int myGrayValueInt=0; //color 1 int
                //check colorEntry is valid
                if (myColorEntry!=NULL)
                {
                    //check for alternate color mappings
                    if (theColorQString=="Red")
                    {
                        myGrayValueInt=myColorEntry->c1;
                    }
                    if (theColorQString=="Green")
                    {
                        myGrayValueInt=myColorEntry->c2;
                    }
                    if (theColorQString=="Blue")
                    {
                        myGrayValueInt=myColorEntry->c3;
                    }
                }
                else
                {
                    //there is no guarantee that there will be a matching palette entry for
                    //every cell in the raster. If there is no match, do nothing.
                    myGrayValueInt=0;
                }
                if (invertHistogramFlag)
                {
                    myGrayValueInt=255-myGrayValueInt;
                }
                //set the pixel based on the above color mappings
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( myGrayValueInt,myGrayValueInt,myGrayValueInt, transparencyLevelInt ));
            }
        }
    }
    //part of the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);
}


/**
* This method is used to render a paletted raster layer as a pseudocolor image.
* @param theQPainter - pointer to the QPainter onto which the layer should be drawn.
* @param theRasterViewPort - pointer to the ViewPort struct containing dimensions of viewable area and subset area to be extracted from data file.
* @param theGdalBand - pointer to the GDALRasterBand which should be rendered.
* @param theColorQString - QString containing either 'Red' 'Green' or 'Blue' indicating which part of the rgb triplet will be used to render gray.
*/
void QgsRasterLayer::drawPalettedSingleBandPseudoColor(QPainter * theQPainter,
        RasterViewPort * theRasterViewPort,
        GDALRasterBand * theGdalBand,
        QString theColorQString)
{
    std::cout << "QgsRasterLayer::drawPalettedSingleBandPseudoColor called" << std::endl;
    // read entire clipped area of raster band
    // treat myGdalScanData as a pseudo-multidimensional array
    // RasterIO() takes care of scaling down image
    uint *myGdalScanData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    CPLErr myResultCPLerr = theGdalBand->RasterIO(
                                GF_Read, theRasterViewPort->rectXOffsetInt,
                                theRasterViewPort->rectYOffsetInt,
                                theRasterViewPort->clippedWidthInt,
                                theRasterViewPort->clippedHeightInt,
                                myGdalScanData,
                                theRasterViewPort->drawableAreaXDimInt,
                                theRasterViewPort->drawableAreaYDimInt,
                                GDT_UInt32, 0, 0 );

    // print each point in myGdalScanData using color looked up in color table
    GDALColorTable *colorTable = theGdalBand->GetColorTable();
    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);


    RasterBandStats myRasterBandStats = rasterStatsMap["Palette"];
    double myRangeDouble=myRasterBandStats.rangeDouble;
    int myRedInt=0;
    int myGreenInt=0;
    int myBlueInt=0;
    //calculate the adjusted matrix stats - which come into affect if the user has chosen
    RasterBandStats myAdjustedRasterBandStats = rasterStatsMap["Palette"];
    myAdjustedRasterBandStats.noDataDouble=0;//hard coding for now
    //to histogram stretch to a given number of std deviations
    //see if we are using histogram stretch using stddev and plot only within the selected number of deviations if we are
    //cout << "stdDevsToPlotDouble: " << cboStdDev->currentText() << " converted to " << stdDevsToPlotDouble << endl;
    if (stdDevsToPlotDouble > 0)
    {
        //work out how far on either side of the mean we should include data
        float myTotalDeviationDouble = stdDevsToPlotDouble * myAdjustedRasterBandStats.stdDevDouble;
        //printf("myTotalDeviationDouble: %i\n" , myTotalDeviationDouble );
        //adjust min and max accordingly
        //only change min if it is less than mean  -  (n  x  deviations)
        if (myAdjustedRasterBandStats.minValDouble < (myAdjustedRasterBandStats.meanDouble-myTotalDeviationDouble))
        {
            myAdjustedRasterBandStats.minValDouble=(myAdjustedRasterBandStats.meanDouble-myTotalDeviationDouble);
            //cout << "Adjusting minValDouble to: " << myAdjustedRasterBandStats.minValDouble << endl;
        }
        //only change max if it is greater than mean  +  (n  x  deviations)
        if (myAdjustedRasterBandStats.maxValDouble > (myAdjustedRasterBandStats.meanDouble + myTotalDeviationDouble))
        {
            myAdjustedRasterBandStats.maxValDouble=(myAdjustedRasterBandStats.meanDouble+myTotalDeviationDouble);
            //cout << "Adjusting maxValDouble to: " << myAdjustedRasterBandStats.maxValDouble << endl;
        }
        //update the range
        myAdjustedRasterBandStats.rangeDouble = myAdjustedRasterBandStats.maxValDouble-myAdjustedRasterBandStats.minValDouble;
    }
    //set up the three class breaks for pseudocolour mapping
    double myBreakSizeDouble = myAdjustedRasterBandStats.rangeDouble / 3;
    double myClassBreakMin1 = myAdjustedRasterBandStats.minValDouble;
    double myClassBreakMax1 = myAdjustedRasterBandStats.minValDouble + myBreakSizeDouble;
    double myClassBreakMin2 = myClassBreakMax1;
    double myClassBreakMax2 = myClassBreakMin2 + myBreakSizeDouble;
    double myClassBreakMin3 = myClassBreakMax2;
    double myClassBreakMax3 = myAdjustedRasterBandStats.maxValDouble;




    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            int myInt=0;
            const GDALColorEntry *myColorEntry = GDALGetColorEntry(colorTable, myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt]);
            //check colorEntry is valid
            if (myColorEntry!=NULL)
            {
                //check for alternate color mappings
                if (theColorQString=="Red")
                {
                    myInt=myColorEntry->c1;
                }
                if (theColorQString=="Green")
                {
                    myInt=myColorEntry->c2;
                }
                if (theColorQString=="Blue")
                {
                    myInt=myColorEntry->c3;
                }

                //dont draw this point if it is no data !
                //double check that myInt >= min and <= max
                //this is relevant if we are plotting within stddevs
                if ((myInt < myAdjustedRasterBandStats.minValDouble ) && (myInt != myAdjustedRasterBandStats.noDataDouble))
                {
                    myInt = static_cast<int>(myAdjustedRasterBandStats.minValDouble);
                }
                if ((myInt > myAdjustedRasterBandStats.maxValDouble)   && (myInt != myAdjustedRasterBandStats.noDataDouble))
                {
                    myInt = static_cast<int>(myAdjustedRasterBandStats.maxValDouble);
                }
                if (myInt==myAdjustedRasterBandStats.noDataDouble)
                {
                    //hardcoding to white for now
                    myRedInt = 255;
                    myBlueInt = 255;
                    myGreenInt =255;
                }
                else if(!invertHistogramFlag)
                {
                    //check if we are in the first class break
                    if ((myInt >= myClassBreakMin1) &&  (myInt < myClassBreakMax1) )
                    {
                        myRedInt = 0;
                        myBlueInt = 255;
                        myGreenInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * (myInt-myClassBreakMin1))*3);
                    }
                    //check if we are in the second class break
                    else if ((myInt >= myClassBreakMin2) &&  (myInt < myClassBreakMax2) )
                    {
                        myRedInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3);
                        myBlueInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3));
                        myGreenInt = 255;
                    }
                    //otherwise we must be in the third classbreak
                    else
                    {
                        myRedInt = 255;
                        myBlueInt = 0;
                        myGreenInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin3)/1)*3)));
                    }
                }
                else  //invert histogram toggle is on
                {
                    //check if we are in the first class break
                    if ((myInt >= myClassBreakMin1) &&  (myInt < myClassBreakMax1) )
                    {
                        myRedInt = 255;
                        myBlueInt = 0;
                        myGreenInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin1)/1)*3));
                    }
                    //check if we are in the second class break
                    else if ((myInt >= myClassBreakMin2) &&  (myInt < myClassBreakMax2) )
                    {
                        myRedInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3));
                        myBlueInt = static_cast<int>(((255/myAdjustedRasterBandStats.rangeDouble) * ((myInt-myClassBreakMin2)/1))*3);
                        myGreenInt = 255;
                    }
                    //otherwise we must be in the third classbreak
                    else
                    {
                        myRedInt = 0;
                        myBlueInt = 255;
                        myGreenInt = static_cast<int>(255-(((255/myAdjustedRasterBandStats.rangeDouble) * (myInt-myClassBreakMin3))*3));
                    }


                }
            }//end of color palette is null check
            else
            {
                //there is no guarantee that there will be a matching palette entry for
                //every cell in the raster. If there is no match, do nothing.
                myRedInt = 255;
                myBlueInt = 255;
                myGreenInt =255;
            }
            //set the pixel based on the above color mappings
            myQImage.setPixel( myRowInt, myColumnInt, qRgba( myRedInt,myGreenInt,myBlueInt, transparencyLevelInt ));
        }
    }

    //part of the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);
}

/**
* This method is used to render a paletted raster layer as a gray image.
* @param theQPainter - pointer to the QPainter onto which the layer should be drawn.
* @param theRasterViewPort - pointer to the ViewPort struct containing dimensions of viewable area and subset area to be extracted from data file.
* @param theGdalBand - pointer to the GDALRasterBand which should be rendered.
*/
void QgsRasterLayer::drawPalettedMultiBandColor(QPainter * theQPainter,
        RasterViewPort * theRasterViewPort,
        GDALRasterBand * theGdalBand)
{
    std::cout << "QgsRasterLayer::drawPalettedMultiBandColor called" << std::endl;

    // read entire clipped area of raster band
    // treat myGdalScanData as a pseudo-multidimensional array
    // RasterIO() takes care of scaling down image
    uint *myGdalScanData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    CPLErr myResultCPLerr = theGdalBand->RasterIO(
                                GF_Read, 
                                theRasterViewPort->rectXOffsetInt, 
                                theRasterViewPort->rectYOffsetInt, 
                                theRasterViewPort->clippedWidthInt, 
                                theRasterViewPort->clippedHeightInt, 
                                myGdalScanData, 
                                theRasterViewPort->drawableAreaXDimInt, 
                                theRasterViewPort->drawableAreaYDimInt, 
                                GDT_UInt32, 0, 0 );

    QString myColorInterpretation = GDALGetColorInterpretationName(theGdalBand->GetColorInterpretation());
    //std::cout << "Colour Interpretation for this band is : " << myColorInterpretation << std::endl;

    // print each point in myGdalScanData using color looked up in color table
    GDALColorTable *colorTable = theGdalBand->GetColorTable();
    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);
    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            const GDALColorEntry *colorEntry = GDALGetColorEntry(colorTable, myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt]);
            //dont draw this point if it is no data !
            if (theRasterViewPort->noDataDouble != myGdalScanData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt])
            {
                int myRedValueInt=0; //color 1 int
                int myGreenValueInt=0; //color 2 int
                int myBlueValueInt=0; //color 3 int
                //check colorEntry is valid
                if (colorEntry!=NULL)
                {
                    //check for alternate color mappings
                    if (redBandNameQString=="Red")
                        myRedValueInt=colorEntry->c1;
                    if (redBandNameQString=="Green")
                        myRedValueInt=colorEntry->c2;
                    if (redBandNameQString=="Blue")
                        myRedValueInt=colorEntry->c3;
                    if (greenBandNameQString=="Red")
                        myGreenValueInt=colorEntry->c1;
                    if (greenBandNameQString=="Green")
                        myGreenValueInt=colorEntry->c2;
                    if (greenBandNameQString=="Blue")
                        myGreenValueInt=colorEntry->c3;
                    if (blueBandNameQString=="Red")
                        myBlueValueInt=colorEntry->c1;
                    if (blueBandNameQString=="Green")
                        myBlueValueInt=colorEntry->c2;
                    if (blueBandNameQString=="Blue")
                        myBlueValueInt=colorEntry->c3;
                }
                else
                {
                    //there is no guarantee that there will be a matching palette entry for
                    //every cell in the raster. If there is no match, do nothing.
                }
                if (invertHistogramFlag)
                {
                    myRedValueInt=255-myRedValueInt;
                    myGreenValueInt=255-myGreenValueInt;
                    myBlueValueInt=255-myBlueValueInt;

                }
                //set the pixel based on the above color mappings
                myQImage.setPixel( myRowInt, myColumnInt, qRgba( myRedValueInt,myGreenValueInt,myBlueValueInt, transparencyLevelInt ));
                //old method for painting directly to canvas with no alpha rendering
                //theQPainter->setPen(QColor(colorEntry->c1, colorEntry->c2, colorEntry->c3));
                //theQPainter->drawPoint(myTopLeftPoint.xToInt() + myRowInt, myTopLeftPoint.yToInt() + myColumnInt);
            }
        }
    }
    //part of the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);

}
void QgsRasterLayer::drawMultiBandSingleBandGray(QPainter * theQPainter, RasterViewPort * theRasterViewPort, GDALRasterBand * theGdalBand )
{
  //delegate to drawSingleBandGray!
  drawSingleBandGray(theQPainter, theRasterViewPort, theGdalBand);
}

void QgsRasterLayer::drawMultiBandSingleBandPseudoColor(QPainter * theQPainter, RasterViewPort * theRasterViewPort, GDALRasterBand * theGdalBand)
{
  //delegate to drawSinglePseudocolor!
  drawSingleBandPseudoColor(theQPainter, theRasterViewPort, theGdalBand);
}
void QgsRasterLayer::drawMultiBandColor(QPainter * theQPainter,
        RasterViewPort * theRasterViewPort)
{
    std::cout << "QgsRasterLayer::drawMultiBandColor called" << std::endl;
    //TODO check band / colour mappings when assigning bands
    
    GDALRasterBand  *myGdalRedBand = gdalDataset->GetRasterBand( 1 );
    GDALRasterBand  *myGdalGreenBand = gdalDataset->GetRasterBand( 2 );
    GDALRasterBand  *myGdalBlueBand = gdalDataset->GetRasterBand( 3 );    
    //
    // I dont know how much overhead there is in opening three matricies at the same time....
    // but this is a replacement of the old method implemented by Steve which xored the color components
    // into the pixels
    // RasterIO() takes care of scaling down image
    uint *myGdalRedData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    uint *myGdalGreenData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    uint *myGdalBlueData = (uint*) CPLMalloc(sizeof(uint)*theRasterViewPort->drawableAreaXDimInt * sizeof(uint)*theRasterViewPort->drawableAreaYDimInt);
    
    CPLErr myRedCPLerr = myGdalRedBand->RasterIO(
                                GF_Read, 
                                theRasterViewPort->rectXOffsetInt, 
                                theRasterViewPort->rectYOffsetInt, 
                                theRasterViewPort->clippedWidthInt, 
                                theRasterViewPort->clippedHeightInt, 
                                myGdalRedData, // <----- Red Layer
                                theRasterViewPort->drawableAreaXDimInt, 
                                theRasterViewPort->drawableAreaYDimInt, 
                                GDT_UInt32, 0, 0 );
    CPLErr myGreenCPLerr = myGdalGreenBand->RasterIO(
                                GF_Read, 
                                theRasterViewPort->rectXOffsetInt, 
                                theRasterViewPort->rectYOffsetInt, 
                                theRasterViewPort->clippedWidthInt, 
                                theRasterViewPort->clippedHeightInt, 
                                myGdalGreenData, // <----- Green Layer 
                                theRasterViewPort->drawableAreaXDimInt, 
                                theRasterViewPort->drawableAreaYDimInt, 
                                GDT_UInt32, 0, 0 );
    CPLErr myBlueCPLerr = myGdalBlueBand->RasterIO(
                                GF_Read, 
                                theRasterViewPort->rectXOffsetInt, 
                                theRasterViewPort->rectYOffsetInt, 
                                theRasterViewPort->clippedWidthInt, 
                                theRasterViewPort->clippedHeightInt, 
                                myGdalBlueData,  // <----- Blue Layer
                                theRasterViewPort->drawableAreaXDimInt, 
                                theRasterViewPort->drawableAreaYDimInt, 
                                GDT_UInt32, 0, 0 );                                
    //std::cout << "Colour Interpretation for this band is : " << myColorInterpretation << std::endl;
    int myRedInt, myGreenInt, myBlueInt;
    QImage myQImage=QImage(theRasterViewPort->drawableAreaXDimInt,theRasterViewPort->drawableAreaYDimInt,32);
    myQImage.setAlphaBuffer(true);
    for (int myColumnInt = 0; myColumnInt < theRasterViewPort->drawableAreaYDimInt; myColumnInt++)
    {
        for (int myRowInt =0; myRowInt < theRasterViewPort->drawableAreaXDimInt; myRowInt++)
        {
            //pull the rgb values from each band
            int myRedValueInt   = myGdalRedData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt];
            int myGreenValueInt = myGdalGreenData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt];
            int myBlueValueInt  = myGdalBlueData[myColumnInt*theRasterViewPort->drawableAreaXDimInt + myRowInt];
            
            //TODO check for nodata values
            if (invertHistogramFlag)
            {
                myRedValueInt   = 255-myRedValueInt;
                myGreenValueInt = 255-myGreenValueInt;
                myBlueValueInt  = 255-myBlueValueInt;
            }
            //set the pixel based on the above color mappings
            myQImage.setPixel( myRowInt, myColumnInt, qRgba( myRedValueInt,myGreenValueInt,myBlueValueInt, transparencyLevelInt ));            
        }
    }
    //part of the experimental transaparency support
    theQPainter->drawImage(theRasterViewPort->topLeftPoint.xToInt(), theRasterViewPort->topLeftPoint.yToInt(),myQImage);

    //free the scanline memory
    CPLFree(myGdalRedData);
    CPLFree(myGdalGreenData);
    CPLFree(myGdalBlueData);        
}

/**
  Print some debug info to the qpainter
*/

void QgsRasterLayer::showDebugOverlay(QPainter * theQPainter, RasterViewPort * theRasterViewPort)
{


    QFont myQFont( "arial", 10, QFont::Bold );
    theQPainter->setFont( myQFont );
    theQPainter->setPen( Qt::black );
    QBrush myQBrush( qRgba(128,128,164,50), Dense6Pattern );   //semi transparent
    theQPainter->setBrush( myQBrush );          // set the yellow brush
    theQPainter->drawRect(5,5,theQPainter->window().width()-10,60);
    theQPainter->setBrush( NoBrush );        // do not fill

    theQPainter->drawText(10,20,"QPainter: "
                          + QString::number(theQPainter->window().width())
                          + " x "  + QString::number(theQPainter->window().height()));
    theQPainter->drawText(10,32,"Raster Extent: "
                          + QString::number(theRasterViewPort->drawableAreaXDimInt)
                          + ","  + QString::number(theRasterViewPort->drawableAreaYDimInt));
    theQPainter->drawText(10,44,"Clipped area: "
                          + QString::number(theRasterViewPort->clippedXMinDouble)
                          + ","  + QString::number(theRasterViewPort->clippedYMinDouble)
                          + " - " + QString::number(theRasterViewPort->clippedXMaxDouble)
                          + "," + QString::number(theRasterViewPort->clippedYMinDouble));

    return;


} //end of main draw method


/**  Return the statistics for a given band number */
const  RasterBandStats QgsRasterLayer::getRasterBandStats(int theBandNo)
{
    // make sure the band stats requested correspond to a valid band number
    if (rasterStatsMap.size() >= theBandNo)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( theBandNo );
        QString myColorInterpretation = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        return rasterStatsMap[myColorInterpretation];
    }
}
/** Return the statistics for a given band name */
const  RasterBandStats QgsRasterLayer::getRasterBandStats(QString theBandNameQString)
{
    return rasterStatsMap[theBandNameQString];
}

/** Private method to calculate statistics for each band. Populates rasterStatsMap.
  * private method to calculate various stats about this layers band. If none is specified it will try to calc
  * stats for any Undefined, Gray  layers it finds. Note that this is a cpu intensive /slow task!*/
void QgsRasterLayer::calculateStats(QString theBandNameQString)
{
    for (int i = 1; i <= gdalDataset->GetRasterCount(); i++)
    {
        RasterBandStats myRasterBandStats;
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( i );
        QString myColorInterpretation = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        //Decide if we want to process this band...
        if (theBandNameQString=="" || //process if no band name was passed to this fn
                myColorInterpretation==theBandNameQString || //process if theband name passed to fn matches this band
                (myColorInterpretation=="Palette" && //process if the current band is palette and the band name
                 ( theBandNameQString=="Red" ||      //passed to this fn is red, green or blue
                   theBandNameQString=="Green" ||
                   theBandNameQString=="Blue"
                 )
                )
           )
        {
            //check if we have previously gathered stats for this band...
            rasterStatsMap[myColorInterpretation];
            //declare a colorTable to hold a palette - will only be used of the layer color interp is palette
            GDALColorTable *colorTable;
            if ( myColorInterpretation=="Palette" )
            {
                //check if we have previously gathered stats for this band...
                RasterBandStats myTempRasterBandrStats = rasterStatsMap[theBandNameQString];
                if (myTempRasterBandrStats.statsGatheredFlag)
                    continue; //should revert to outer loop if true
                colorTable = myGdalBand->GetColorTable();
                myRasterBandStats.bandName=theBandNameQString;
            }
            else
            {
                myRasterBandStats.bandName=myColorInterpretation;
            }
            myRasterBandStats.bandNo=i;
            std::cout << "Getting stats for band " << i << " (" << myColorInterpretation << ")" << std::endl;
            // get the dimensions of the raster
            int myColsInt = myGdalBand->GetXSize();
            int myRowsInt = myGdalBand->GetYSize();

            myRasterBandStats.elementCountInt=myColsInt*myRowsInt;
            myRasterBandStats.noDataDouble=myGdalBand->GetNoDataValue();

            //allocate a buffer to hold one row of ints
            int myAllocationSizeInt = sizeof(uint)*myColsInt;
            uint * myScanlineAllocInt = (uint*) CPLMalloc(myAllocationSizeInt);
            bool myFirstIterationFlag = true;
            //unfortunately we need to make two passes through the data to calculate stddev
            for (int myCurrentRowInt=0; myCurrentRowInt < myRowsInt;myCurrentRowInt++)
            {
                CPLErr myResult = myGdalBand->RasterIO(
                                      GF_Read, 0, myCurrentRowInt, myColsInt, 1, myScanlineAllocInt, myColsInt, 1, GDT_UInt32, 0, 0 );
                for (int myCurrentColInt=0; myCurrentColInt < myColsInt; myCurrentColInt++)
                {
                    double myDouble=0;
                    //get the nth element from the current row
                    if (myColorInterpretation!="Palette")
                    {
                        myDouble=myScanlineAllocInt[myCurrentColInt];
                    }
                    else
                    {
                        //this is a palette layer so red / green / blue 'layers are 'virtual'
                        //in that we need to obtain the palette entry and then get the r,g or g
                        //component from that palette entry
                        const GDALColorEntry *myColorEntry = GDALGetColorEntry(colorTable,myScanlineAllocInt[myCurrentColInt]);
                        //check colorEntry is valid
                        if (myColorEntry!=NULL)
                        {
                            //check for alternate color mappings
                            if (theBandNameQString=="Red")
                            {
                                myDouble=static_cast<double> (myColorEntry->c1);
                            }
                            if (theBandNameQString=="Green")
                            {
                                myDouble=static_cast<double> (myColorEntry->c2);
                            }
                            if (theBandNameQString=="Blue")
                            {
                                myDouble=static_cast<double> (myColorEntry->c3);
                            }
                        }


                    }
                    //only use this element if we have a non null element
                    if (myDouble != myRasterBandStats.noDataDouble )
                    {
                        if (myFirstIterationFlag)
                        {
                            //this is the first iteration so initialise vars
                            myFirstIterationFlag=false;
                            myRasterBandStats.minValDouble=myDouble;
                            myRasterBandStats.maxValDouble=myDouble;
                        } //end of true part for first iteration check
                        else
                        {
                            //this is done for all subsequent iterations
                            if (myDouble < myRasterBandStats.minValDouble)
                            {
                                myRasterBandStats.minValDouble=myDouble;
                            }
                            if (myDouble > myRasterBandStats.maxValDouble)
                            {
                                myRasterBandStats.maxValDouble=myDouble;
                            }
                            //only increment the running total if it is not a nodata value
                            if (myDouble != myRasterBandStats.noDataDouble)
                            {
                                myRasterBandStats.sumDouble += myDouble;
                                ++myRasterBandStats.elementCountInt;
                            }
                        } //end of false part for first iteration check
                    } //end of nodata chec
                } //end of column wise loop
            } //end of row wise loop
            //
            //end of first pass through data now calculate the range
            myRasterBandStats.rangeDouble = myRasterBandStats.maxValDouble-myRasterBandStats.minValDouble;
            //calculate the mean
            myRasterBandStats.meanDouble = myRasterBandStats.sumDouble / myRasterBandStats.elementCountInt;
            //for the second pass we will get the sum of the squares / mean
            for (int myCurrentRowInt=0; myCurrentRowInt < myRowsInt;myCurrentRowInt++)
            {
                CPLErr myResult = myGdalBand->RasterIO(
                                      GF_Read, 0, myCurrentRowInt, myColsInt, 1, myScanlineAllocInt, myColsInt, 1, GDT_UInt32, 0, 0 );
                for (int myCurrentColInt=0; myCurrentColInt < myColsInt; myCurrentColInt++)
                {
                    double myDouble=0;
                    //get the nth element from the current row
                    if (myColorInterpretation!="Palette")
                    {
                        myDouble=myScanlineAllocInt[myCurrentColInt];
                    }
                    else
                    {
                        //this is a palette layer so red / green / blue 'layers are 'virtual'
                        //in that we need to obtain the palette entry and then get the r,g or g
                        //component from that palette entry
                        const GDALColorEntry *myColorEntry = GDALGetColorEntry(colorTable,myScanlineAllocInt[myCurrentColInt]);
                        //check colorEntry is valid
                        if (myColorEntry!=NULL)
                        {
                            //check for alternate color mappings
                            if (theBandNameQString=="Red")
                            {
                                myDouble=myColorEntry->c1;
                            }
                            if (theBandNameQString=="Green")
                            {
                                myDouble=myColorEntry->c2;
                            }
                            if (theBandNameQString=="Blue")
                            {
                                myDouble=myColorEntry->c3;
                            }
                        }


                    }
                    myRasterBandStats.sumSqrDevDouble += static_cast<double>(pow(myDouble - myRasterBandStats.meanDouble,2));
                } //end of column wise loop
            } //end of row wise loop
            //divide result by sample size - 1 and get square root to get stdev
            myRasterBandStats.stdDevDouble = static_cast<double>(sqrt(myRasterBandStats.sumSqrDevDouble /
                                             (myRasterBandStats.elementCountInt - 1)));
            CPLFree(myScanlineAllocInt);
            //add this band to the class stats map
            rasterStatsMap[myColorInterpretation]=myRasterBandStats;
        }//end of "" / bandNameQString check
    }//end of band loop
}


//mutator for red band name (allows alternate mappings e.g. map blue as red colour)
void QgsRasterLayer::setRedBandName(QString theBandNameQString)
{
    //check if the band is unset
    if (theBandNameQString=="Not Set")
    {
        redBandNameQString=theBandNameQString;
    }
    //check that a valid band name was passed, or if this is a paletted image,
    //Red, Green or Blue are also allowed.
    for (int myInt = 1; myInt <= gdalDataset->GetRasterCount(); ++myInt)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( myInt );
        QString myBandNameQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        if (theBandNameQString==myBandNameQString)
        {
            //set the class member red band with the name passed by the calling fn
            redBandNameQString=theBandNameQString;
        }
        else if ((myBandNameQString=="Palette") && (theBandNameQString=="Red" || theBandNameQString=="Green" ||theBandNameQString=="Blue") )
        {
            //this is a special case where we allow the calling fn to set the band name to rd green or blue
            //if the layre is palette based. This allows us to plot RGB onto RBG or BGR for example. See the
            //Palette section in the paint routine for more info.
            redBandNameQString=theBandNameQString;
        }
    }
}
//mutator for green band name
void QgsRasterLayer::setGreenBandName(QString theBandNameQString)
{
    //check if the band is unset
    if (theBandNameQString=="Not Set")
    {
        greenBandNameQString=theBandNameQString;
    }
    //check that a valid band name was passed, or if this is a paletted image,
    //Red, Green or Blue are also allowed.
    for (int myInt = 1; myInt <= gdalDataset->GetRasterCount(); ++myInt)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( myInt );
        QString myBandNameQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        if (theBandNameQString==myBandNameQString)
        {
            //set the class member red band with the name passed by the calling fn
            greenBandNameQString=theBandNameQString;
        }
        else if ((myBandNameQString=="Palette") && (theBandNameQString=="Red" || theBandNameQString=="Green" ||theBandNameQString=="Blue") )
        {
            //this is a special case where we allow the calling fn to set the band name to rd green or blue
            //if the layre is palette based. This allows us to plot RGB onto RBG or BGR for example. See the
            //Palette section in the paint routine for more info.
            greenBandNameQString=theBandNameQString;
        }
    }

}
//mutator for blue band name
void QgsRasterLayer::setBlueBandName(QString theBandNameQString)
{
    //check if the band is unset
    if (theBandNameQString=="Not Set")
    {
        blueBandNameQString=theBandNameQString;
    }
    //check that a valid band name was passed, or if this is a paletted image,
    //Red, Green or Blue are also allowed.
    for (int myInt = 1; myInt <= gdalDataset->GetRasterCount(); ++myInt)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( myInt );
        QString myBandNameQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        if (theBandNameQString==myBandNameQString)
        {
            //set the class member red band with the name passed by the calling fn
            blueBandNameQString=theBandNameQString;
        }
        else if ((myBandNameQString=="Palette") && (theBandNameQString=="Red" || theBandNameQString=="Green" ||theBandNameQString=="Blue") )
        {
            //this is a special case where we allow the calling fn to set the band name to rd green or blue
            //if the layre is palette based. This allows us to plot RGB onto RBG or BGR for example. See the
            //Palette section in the paint routine for more info.
            blueBandNameQString=theBandNameQString;
        }
    }
}
//mutator for gray band name
void QgsRasterLayer::setGrayBandName(QString theBandNameQString)
{
    //check if the band is unset
    if (theBandNameQString=="Not Set")
    {
        grayBandNameQString=theBandNameQString;
    }
    //check that a valid band name was passed, or if this is a paletted image,
    //Red, Green or Blue are also allowed.
    for (int myInt = 1; myInt <= gdalDataset->GetRasterCount(); ++myInt)
    {
        GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( myInt );
        QString myBandNameQString = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
        if (theBandNameQString==myBandNameQString)
        {
            //set the class member gray band with the name passed by the calling fn
            grayBandNameQString=theBandNameQString;
        }
        else if  (myBandNameQString=="Palette" &&
                  ( theBandNameQString=="Red" ||
                    theBandNameQString=="Green" ||
                    theBandNameQString=="Blue"
                  )
                 )
        {
            calculateStats(theBandNameQString);
        }
    }

}

/** Return a pixmap representing a legend image. This is an overloaded
 * version of the method below and assumes false for the legend name flag.
*/
QPixmap QgsRasterLayer::getLegendQPixmap()
{
    return getLegendQPixmap(false);
}
/** Return a pixmap representing a legend image
* @param theWithNameFlag - boolena flag whether to overlay the legend name in the text
*/
QPixmap QgsRasterLayer::getLegendQPixmap(bool theWithNameFlag)
{
    //
    // Get the adjusted matrix stats
    //
    GDALRasterBand  *myGdalBand = gdalDataset->GetRasterBand( 1 );
    double noDataDouble = myGdalBand->GetNoDataValue();
    QString myColorInterpretation = GDALGetColorInterpretationName(myGdalBand->GetColorInterpretation());
    QPixmap myLegendQPixmap; //will be initialised once we know what drawing style is active
    QPainter myQPainter;
    //
    // Create the legend pixmap - note it is generated on the preadjusted stats
    //
    if (drawingStyle==MULTI_BAND_SINGLE_BAND_GRAY ||
            drawingStyle==PALETTED_SINGLE_BAND_GRAY ||
            drawingStyle==SINGLE_BAND_GRAY )
    {

        myLegendQPixmap = QPixmap(100,1);
        myQPainter.begin(&myLegendQPixmap);
        int myPosInt = 0;
        for (int  myDouble=0; myDouble < 255; myDouble+=2.55)
        {
            if (!invertHistogramFlag) //histogram is not inverted
            {
                //draw legend as grayscale
                int myGrayInt = static_cast<int>(myDouble);
                myQPainter.setPen( QPen( QColor(myGrayInt, myGrayInt, myGrayInt, QColor::Rgb), 0) );
            }
            else //histogram is inverted
            {
                //draw legend as inverted grayscale
                int myGrayInt = 255-static_cast<int>(myDouble);
                myQPainter.setPen( QPen( QColor(myGrayInt, myGrayInt, myGrayInt, QColor::Rgb), 0) );
            } //end of invert histogram  check
            myQPainter.drawPoint( myPosInt++,0);
        }
    } //end of gray check
    else if (drawingStyle==MULTI_BAND_SINGLE_BAND_PSEUDO_COLOR ||
             drawingStyle==PALETTED_SINGLE_BAND_PSEUDO_COLOR ||
             drawingStyle==SINGLE_BAND_PSEUDO_COLOR)
    {
 
        //set up the three class breaks for pseudocolour mapping
        double myRangeSizeDouble = 90; //hard coded for now
        double myBreakSizeDouble = myRangeSizeDouble/3;
        double myClassBreakMin1 = 0;
        double myClassBreakMax1 = myClassBreakMin1 + myBreakSizeDouble;
        double myClassBreakMin2 = myClassBreakMax1;
        double myClassBreakMax2 = myClassBreakMin2 + myBreakSizeDouble;
        double myClassBreakMin3 = myClassBreakMax2;
        double myClassBreakMax3 = myClassBreakMin3 + myBreakSizeDouble;

        //
        // Create the legend pixmap - note it is generated on the preadjusted stats
        //
        myLegendQPixmap = QPixmap(100,1);
        myQPainter.begin(&myLegendQPixmap);
        int myPosInt = 0;
        for (double myDouble=0 ;
                myDouble < myRangeSizeDouble ;
                myDouble+=myRangeSizeDouble/100.0)
        {
            //draw pseudocolor legend
            if (!invertHistogramFlag)
            {
                //check if we are in the first class break
                if ((myDouble >= myClassBreakMin1) &&  (myDouble < myClassBreakMax1) )
                {
                    int myRedInt = 0;
                    int myBlueInt = 255;
                    int myGreenInt = static_cast<int>(((255/myRangeSizeDouble) * (myDouble-myClassBreakMin1))*3);
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }
                //check if we are in the second class break
                else if ((myDouble >= myClassBreakMin2) &&  (myDouble < myClassBreakMax2) )
                {
                    int myRedInt = static_cast<int>(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin2)/1))*3);
                    int myBlueInt = static_cast<int>(255-(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin2)/1))*3));
                    int myGreenInt = 255;
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }
                //otherwise we must be in the third classbreak
                else
                {
                    int myRedInt = 255;
                    int myBlueInt = 0;
                    int myGreenInt = static_cast<int>(255-(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin3)/1)*3)));
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }
            }//end of invert histogram == false check
            else  //invert histogram toggle is off
            {
                //check if we are in the first class break
                if ((myDouble >= myClassBreakMin1) &&  (myDouble < myClassBreakMax1) )
                {
                    int myRedInt = 255;
                    int myBlueInt = 0;
                    int myGreenInt = static_cast<int>(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin1)/1)*3));
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }
                //check if we are in the second class break
                else if ((myDouble >= myClassBreakMin2) &&  (myDouble < myClassBreakMax2) )
                {
                    int myRedInt = static_cast<int>(255-(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin2)/1))*3));
                    int myBlueInt = static_cast<int>(((255/myRangeSizeDouble) * ((myDouble-myClassBreakMin2)/1))*3);
                    int myGreenInt = 255;
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }
                //otherwise we must be in the third classbreak
                else
                {
                    int myRedInt = 0;
                    int myBlueInt = 255;
                    int myGreenInt = static_cast<int>(255-(((255/myRangeSizeDouble) * (myDouble-myClassBreakMin3))*3));
                    myQPainter.setPen( QPen( QColor(myRedInt, myGreenInt, myBlueInt, QColor::Rgb), 0) );
                }

            } //end of invert histogram check
            myQPainter.drawPoint( myPosInt++,0);
        }

    }//end of pseudocolor check
    else if (drawingStyle==PALETTED_MULTI_BAND_COLOR ||
             drawingStyle==MULTI_BAND_COLOR)
    {
        //
        // Create the legend pixmap showing red green and blue band mappings
        //
        // TODO update this so it actually shows the mappings for paletted images
        myLegendQPixmap = QPixmap(3,1);
        myQPainter.begin(&myLegendQPixmap);
        //draw legend red part
        myQPainter.setPen( QPen( QColor(255,0,0, QColor::Rgb), 0) );
        myQPainter.drawPoint( 0,0);
        //draw legend green part
        myQPainter.setPen( QPen( QColor(0,255,0, QColor::Rgb), 0) );
        myQPainter.drawPoint( 1,0);
        //draw legend blue part
        myQPainter.setPen( QPen( QColor(0,0,255, QColor::Rgb), 0) );
        myQPainter.drawPoint( 2,0);
    }


    myQPainter.end();


    //see if the caller wants the name of the layer in the pixmap (used for legend bar
    if (theWithNameFlag)
    {
        QFont myQFont( "times", 12, QFont::Normal );
        QFontMetrics myQFontMetrics(myQFont);

        int myWidthInt=40+myQFontMetrics.width(this->name());
        int myHeightInt=(myQFontMetrics.height()+10>35) ? myQFontMetrics.height()+10 : 35;

        //create a matrix to
        QWMatrix myQWMatrix;
        //scale the raster legend up a bit bigger to the legend item size
        //note that scaling parameters are factors, not absolute values,
        // so scale (0.25,1) scales the painter to a quarter of its size in the x direction
        //TODO We need to decide how much to scale by later especially for rgb images which are only 3x1 pix
        //hard coding thes values for now.
        if (myLegendQPixmap.width()==3)
        {
            //scale width by factor of 40
            myQWMatrix.scale(40,myHeightInt);
        }
        else //assume 100px so scale by factor of 1.2
        {
            myQWMatrix.scale(1.2,myHeightInt);
        }
        //apply the matrix
        QPixmap myQPixmap2 =  myLegendQPixmap.xForm(myQWMatrix);
        QPainter myQPainter(&myQPixmap2);
        myQPainter.setPen( Qt::black );
        myQPainter.setFont( myQFont );
        myQPainter.drawText(5,myHeightInt-10,this->name());
        myQPainter.end();
        myLegendQPixmap = myQPixmap2;
    }
    //finish up

    return myLegendQPixmap;

}//end of getLegendQPixmap function

//similar to above but returns a pointer. Implemented for qgsmaplayer interface
QPixmap * QgsRasterLayer::legendPixmap()
{
    QPixmap myQPixmap = getLegendQPixmap(true);


    return new QPixmap(myQPixmap);
}

/** Accessor for the superclass popmenu var*/
QPopupMenu *QgsRasterLayer::contextMenu(){
  return popMenu;
}

void QgsRasterLayer::initContextMenu(QgisApp *theApp){
  popMenu = new QPopupMenu();
  popMenu->insertItem(tr("&Zoom to extent of selected layer"), theApp, SLOT(zoomToLayerExtent()));
  popMenu->insertItem(tr("&Properties"), theApp, SLOT(layerProperties()));
  popMenu->insertSeparator();
  popMenu->insertItem(tr("&Remove"), theApp, SLOT(removeLayer()));
}