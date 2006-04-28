
/***************************************************************************
                          qgslegend.cpp  -  description
                             -------------------
    begin                : Sun Jul 28 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
               Romans 3:23=>Romans 6:23=>Romans 10:9,10=>Romans 12
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

#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgslegend.h"
#include "qgslegendgroup.h"
#include "qgslegendlayer.h"
#include "qgslegendpropertygroup.h"
#include "qgslegendsymbologyitem.h"
#include "qgslegendlayerfile.h"
#include "qgslegendlayerfilegroup.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprender.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgsrasterlayerproperties.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerproperties.h"
#include "qgsvectordataprovider.h"
#include <float.h>
#include <QCoreApplication>
#include <QPixmap>
#include <QMouseEvent>
#include <iostream>
#include <QTreeWidgetItem>
#include <Q3PopupMenu>
#include <QMenu>
#include <QFont>
#include <QHeaderView>
#include <Q3ListViewItem>

static const char *const ident_ = "$Id$";

const int AUTOSCROLL_MARGIN = 16;

/**
   @note
 
   set mItemBeingMoved pointer to 0 to prevent SuSE 9.0 crash
*/
QgsLegend::QgsLegend(QgisApp* app, QWidget * parent, const char *name)
  : QTreeWidget(parent), mApp(app), mMousePressedFlag(false), mItemBeingMoved(0), mShowLegendLayerFiles(false), mMapCanvas(0), mMinimumIconSize(20, 20)
{
  connect( this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
	   this, SLOT(handleItemChange(QTreeWidgetItem*, int)));
  
  connect( this, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
	   this, SLOT(handleCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
  

 setSortingEnabled(false);
  setDragEnabled(false);
  setAutoScroll(true);
  QFont f("Arial", 10, QFont::Normal);
  setFont(f);
  setBackgroundColor(QColor(192, 192, 192));
  setColumnCount(1);
  QStringList myList("Layers");
  setHeaderLabels(myList);
  //added by Tim to hide the header - header is unneccessary
  header()->setHidden(1);
  setRootIsDecorated(true);

  initPixmaps();
}



QgsLegend::~QgsLegend()
{}


void QgsLegend::handleCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
  QgsMapLayer *layer = currentLayer();
  if(mApp)
    {
      mApp->activateDeactivateLayerRelatedActions( layer );
    }
  if(mMapCanvas)
    {
      mMapCanvas->setCurrentLayer( layer );
    }
  emit currentLayerChanged ( layer );
}   

void QgsLegend::addGroup()
{
    QgsLegendGroup* group = new QgsLegendGroup(this, tr("group"));
    mStateOfCheckBoxes.insert(std::make_pair(group, Qt::Checked)); //insert the check state into the map to query for changes later
    setExpanded(indexFromItem(group), true);
}

void QgsLegend::removeAll()
{
  mStateOfCheckBoxes.clear();
  clear();
  updateMapCanvasLayerSet();
}

void QgsLegend::removeLayer(QString layer_key)
{
  QTreeWidgetItem* theItem = firstItem();
#ifdef QGISDEBUG
  qWarning("in QgsLegend::removeLayer");
#endif
  while(theItem)
    {
	QgsLegendItem *li = dynamic_cast<QgsLegendItem*>(theItem);
	if(li)
	{
	    QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
	    if(llf)
	    {
		if (llf->layer()&&llf->layer()->getLayerID() == layer_key)
		{
		  //remove the map entry for the checkbox
		  mStateOfCheckBoxes.erase(llf);
		  removeItem(llf);
		  break;
		}
	    }
	}
	theItem = nextItem(theItem);
    }

    updateMapCanvasLayerSet();
}

void QgsLegend::mousePressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton)
  {
    mLastPressPos = e->pos();
    mMousePressedFlag = true;
  }
  else if(e->button() == Qt::RightButton)
    {
      QTreeWidgetItem* item = itemAt(e->pos());
      setCurrentItem(item);
      handleRightClickEvent(item, e->globalPos());
    }
  QTreeWidget::mousePressEvent(e);
}                               // contentsMousePressEvent

void QgsLegend::mouseMoveEvent(QMouseEvent * e)
{
    if(mMousePressedFlag)
    {
	//set the flag back such that the else if(mItemBeingMoved)
	//code part is passed during the next mouse moves
	mMousePressedFlag = false;

	// remember item we've pressed as the one being moved
	// and where it was originally
	QTreeWidgetItem* item = itemAt(mLastPressPos);
	if(item)
	{
	    mItemBeingMoved = item;
	    mItemBeingMovedOrigPos = getItemPos(mItemBeingMoved);

	    //store information to insert the item back to the original position
	    storeInitialPosition(mItemBeingMoved);
 
	    setCursor(Qt::SizeVerCursor);
	}
    }
    else if (mItemBeingMoved)
    { 
      QPoint p(e->pos());
      mLastPressPos=p;
     
      // change the cursor appropriate to if drop is allowed
      QTreeWidgetItem* item = itemAt(p);
      QgsLegendItem* origin = dynamic_cast<QgsLegendItem*>(mItemBeingMoved);
      QgsLegendItem* dest = dynamic_cast<QgsLegendItem*>(item);

      if (item && (item != mItemBeingMoved))
	{
	  QgsLegendItem::DRAG_ACTION action = dest->accept(origin);
	  if(action == QgsLegendItem::REORDER)
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::REORDER");
#endif
	      if(!yCoordAboveCenter(dest, e->y())) //over bottom of item
		{
		  if(origin->nextSibling() != dest)
		    {
		      if(origin->parent() != dest->parent())
			{
			  dest->parent()->insertChild(dest->parent()->childCount(), origin);
			  origin->moveItem(dest);
			  dest->moveItem(origin);
		      }
		      else
		      {
			  dest->moveItem(origin);
		      }
		  }
	      }
	      else //over top of item
	      {
		  if (mItemBeingMoved != dest->nextSibling())
		  {
		      origin->moveItem(dest);
		  } 
	      }
	      setCurrentItem(origin);
	    }
	    else if(action == QgsLegendItem::INSERT)
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::INSERT");
#endif
	      setCursor( QCursor(Qt::PointingHandCursor) );
	      if(origin->parent() != dest)
		{
		  insertItem(origin, dest);
		  setCurrentItem(origin);
		}
	    }
	    else//no action
	    {
#ifdef QGISDEBUG
	      qWarning("mouseMoveEvent::NO_ACTION");
#endif
	      if(origin->type() == QgsLegendItem::LEGEND_LAYER_FILE && mItemBeingMovedOrigPos != getItemPos(mItemBeingMoved))
		{
		  resetToInitialPosition(mItemBeingMoved);
		}
	      setCursor( QCursor(Qt::ForbiddenCursor) );
	    }
	}     
    }
}

void QgsLegend::mouseReleaseEvent(QMouseEvent * e)
{
  QTreeWidget::mouseReleaseEvent(e);
  setCursor(QCursor(Qt::ArrowCursor));

  if (mItemBeingMoved)
  {
      QTreeWidgetItem *destItem = itemAt(e->pos());
      
      QgsLegendItem* origin = dynamic_cast<QgsLegendItem*>(mItemBeingMoved);
      QgsLegendItem* dest = dynamic_cast<QgsLegendItem*>(destItem);

      if(!dest || !origin)
      {
	  return;
      }

      if(dest && origin && getItemPos(dest) != mItemBeingMovedOrigPos)
      {
	QgsLegendItem::LEGEND_ITEM_TYPE originType = origin->type();
	QgsLegendItem::LEGEND_ITEM_TYPE destType = dest->type();

	if(originType == QgsLegendItem::LEGEND_LAYER_FILE && destType == QgsLegendItem::LEGEND_LAYER_FILE_GROUP)
	  {
	    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
	      if(1) //todo: find a test to avoid that symbology settings are copied if an item is moved within the same legend layer  
	      {
		if(dest->childCount() > 1)
		  {
		    //find the first layer in the legend layer group != origLayer and copy its settings
		    QgsLegendItem* currentItem = dynamic_cast<QgsLegendItem*>(dest->child(0));
		    while(currentItem)
		      {
			if(currentItem != origin)
			  {
			    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
			    QgsMapLayer* currentLayer = ((QgsLegendLayerFile*)(currentItem))->layer();
			    origLayer->copySymbologySettings(*currentLayer);
			    break;
			  }
			currentItem = currentItem->nextSibling();
		      }
		  }
	      }
	  }
	else if(originType == QgsLegendItem::LEGEND_LAYER_FILE && destType == QgsLegendItem::LEGEND_LAYER_FILE)
	  {
	    QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
	    QgsMapLayer* destLayer = ((QgsLegendLayerFile*)(dest))->layer();

	    if(dest == origin)//origin item has been moved in mouseMoveEvent such that it is under the mouse cursor now
	      {
		if(1) //todo: find a test to avoid that symbology settings are copied if an item is moved within the same legend layer  
		{
		    if(origin->parent()->childCount() > 1)
		      {
			//find the first layer in the legend layer group != origLayer and copy its settings
			QTreeWidgetItem* currentItem = dest->parent()->child(0);
			while(currentItem)
			  {
			    if(currentItem != origin)
			      {
				QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
				QgsMapLayer* currentLayer = ((QgsLegendLayerFile*)(currentItem))->layer();
				origLayer->copySymbologySettings(*currentLayer);
				break;
			      }
			    currentItem = dynamic_cast<QgsLegendItem*>(currentItem)->nextSibling();
			  }
		      }
		  }
	      }
	    else
	      {
		QgsMapLayer* origLayer = ((QgsLegendLayerFile*)(origin))->layer();
		QgsMapLayer* destLayer = ((QgsLegendLayerFile*)(dest))->layer();
		origLayer->copySymbologySettings(*destLayer);
	      }
	  }
	
	 std::deque<QString> layersAfterRelease = layerIDs(); //test if canvas redraw is really necessary
   if(layersAfterRelease != mLayersPriorToMove)
   {
     // z-order has changed - update layer set
     updateMapCanvasLayerSet();
   }
      }
  }
  mMousePressedFlag = false;
  mItemBeingMoved = NULL;
}

void QgsLegend::mouseDoubleClickEvent(QMouseEvent* e)
{
  legendLayerShowProperties();
}

void QgsLegend::handleRightClickEvent(QTreeWidgetItem* item, const QPoint& position)
{
  QMenu theMenu;

  QString iconsPath = QgsApplication::themePath();

  if(mMapCanvas->isDrawing())
    {
      return;
    }

  // TODO: synchronize LEGEND_LAYER_FILE and LEGEND_LAYER menus a bit?
  
  QgsLegendItem* li = dynamic_cast<QgsLegendItem*>(item);
  if(li)
    {
      if(li->type() == QgsLegendItem::LEGEND_LAYER_FILE)
	{
    QgsMapLayer* layer = (static_cast<QgsLegendLayerFile*>(li))->layer();
	  
    theMenu.addAction(tr("&Zoom to layer extent"), this, SLOT(zoomToLayerExtent()));
    
    // TODO: decide whether to use toggle in overview or add/remove to/from overview
    //mShowInOverviewAction = popMenu->addAction(tr("Toggle in Overview"), app, SLOT(inOverview()));
    //mShowInOverviewAction->setCheckable(true);
    theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionAddAllToOverview.png"))), tr("&Add to overview"), this, SLOT(legendLayerAddToOverview()));
    theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionRemoveAllFromOverview.png"))), tr("&Remove from overview"), this, SLOT(legendLayerRemoveFromOverview()));
    theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionRemove.png"))), tr("&Remove"), this, SLOT(legendLayerRemove()));

    if (layer->type() == QgsMapLayer::VECTOR)
    {
      QgsVectorLayer* vlayer = dynamic_cast<QgsVectorLayer*>(layer);
      
      theMenu.addAction(tr("&Open attribute table"), li, SLOT(table()));
      theMenu.addSeparator();
    
      int cap = vlayer->getDataProvider()->capabilities();
      if((cap & QgsVectorDataProvider::AddFeatures)
          ||(cap & QgsVectorDataProvider::DeleteFeatures))
      {
        QAction* toggleEditingAction = theMenu.addAction(tr("Allow Editing"),vlayer,SLOT(toggleEditing()));
        toggleEditingAction->setCheckable(true);
        toggleEditingAction->blockSignals(true);
        toggleEditingAction->setChecked(vlayer->isEditable());
        toggleEditingAction->blockSignals(false);
      }
    
      // add the save as shapefile menu item
      theMenu.addSeparator();
      theMenu.addAction(tr("Save as shapefile..."), li, SLOT(saveAsShapefile()));

    }
    else if (layer->type() == QgsMapLayer::RASTER)
    {
      QgsRasterLayer* rlayer = dynamic_cast<QgsRasterLayer*>(layer);
      
      /*
      //In qt4, inserting a slider in QMenu seems difficult
      popMenu->setCheckable ( true );
    
      QLabel * myTransparencyLabel = new QLabel( popMenu );
    
      myTransparencyLabel->setFrameStyle( Q3Frame::Panel | Q3Frame::Raised );
      myTransparencyLabel->setText( tr("<center><b>Transparency</b></center>") );
    
    // TODO: Qt4 will have to use a QAction instead
    #if QT_VERSION < 0x040000
      popMenu->insertItem(myTransparencyLabel);
    
      // XXX why GUI element here?
      // XXX Dunno who put the above comment in, but whole context menu is a gui element! TS
      mTransparencySlider = new QSlider(0,255,5,255-transparencyLevelInt,Qt::Horizontal,popMenu);
      mTransparencySlider->setTickmarks(QSlider::TicksBothSides);
      mTransparencySlider->setTickInterval(25);
      mTransparencySlider->setTracking(false); //stop slider emmitting a signal until mouse released
    
      connect(mTransparencySlider, SIGNAL(valueChanged(int)), this, SLOT(popupTransparencySliderMoved(int)));
    
      popMenu->insertItem(mTransparencySlider);
    #endif
      */

      theMenu.addAction(tr("&Convert to..."), rlayer, SLOT(convertTo()));
    }
     
    // properties goes on bottom of menu for consistency with normal ui standards
    // e.g. kde stuff
    theMenu.addAction(tr("&Properties"), this, SLOT(legendLayerShowProperties()));
   
    theMenu.exec(position);
    return;
	}
      else if(li->type() == QgsLegendItem::LEGEND_LAYER)
	{
	  theMenu.addAction(tr("&Properties"), this, SLOT(legendLayerShowProperties()));
	  theMenu.addAction(tr("&Zoom to layer extent"), this, SLOT(zoomToLayerExtent()));
	  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionAddAllToOverview.png"))), tr("&Add to overview"), this, SLOT(legendLayerAddToOverview()));
	  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionRemoveAllFromOverview.png"))), tr("&Remove from overview"), this, SLOT(legendLayerRemoveFromOverview()));
	  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionRemove.png"))), tr("&Remove"), this, SLOT(legendLayerRemove()));
	  if(li->parent())
	    {
	      theMenu.addAction(tr("&Make to toplevel item"), this, SLOT(makeToTopLevelItem()));
	    }
	}
      else if(li->type() == QgsLegendItem::LEGEND_GROUP)
	{
	  theMenu.addAction(QPixmap(iconsPath+QString("/mActionRemove.png")), tr("&Remove"), this, SLOT(legendGroupRemove()));
	}

      if(li->type() == QgsLegendItem::LEGEND_LAYER || li->type() == QgsLegendItem::LEGEND_GROUP)
	{
	  theMenu.addAction(tr("Re&name"), this, SLOT(openEditor()));
	}
	
      
    }

  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/folder_new.png"))), tr("&Add group"), this, SLOT(addGroup()));
  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionExpandTree.png"))), tr("&Expand all"), this, SLOT(expandAll()));
  theMenu.addAction(QIcon(QPixmap(iconsPath+QString("/mActionCollapseTree.png"))), tr("&Collapse all"), this, SLOT(collapseAll()));
  QAction* showFileGroupsAction = theMenu.addAction(tr("Show file groups"), this, SLOT(showLegendLayerFileGroups()));
  showFileGroupsAction->setCheckable(true);
  showFileGroupsAction->blockSignals(true);
  showFileGroupsAction->setChecked(mShowLegendLayerFiles);
  showFileGroupsAction->blockSignals(false);
  theMenu.exec(position);
}

int QgsLegend::getItemPos(QTreeWidgetItem* item)
{
  int counter = 1;
  QTreeWidgetItem* theItem = firstItem();
  while(theItem)
    {
      if(theItem == item)
	{
	  return counter;
	}
      theItem = nextItem(theItem);
      ++counter;
    }
  return -1;
}

void QgsLegend::addLayer( QgsMapLayer * layer )
{
  QgsLegendLayer * llayer = new QgsLegendLayer(layer->name());//generate entry for mStateOfCheckBoxes below
    QgsLegendLayerFileGroup * llfgroup = new QgsLegendLayerFileGroup(llayer,QString("Files"));
    QgsLegendLayerFile * llfile = new QgsLegendLayerFile(llfgroup, QgsLegendLayerFile::nameFromLayer(layer), layer);
    llayer->setLayerTypeIcon();
    
    //set the correct check states
    blockSignals(true);
    if(llfile->isVisible())
      {
	llfile->setCheckState(0, Qt::Checked);
	llayer->setCheckState(0, Qt::Checked);
	mStateOfCheckBoxes.insert(std::make_pair(llfile, Qt::Checked)); //insert the check state into the map to query for changes later
	mStateOfCheckBoxes.insert(std::make_pair(llayer, Qt::Checked));
      }
    else
      {
	llfile->setCheckState(0, Qt::Unchecked);
	llayer->setCheckState(0, Qt::Unchecked);
	mStateOfCheckBoxes.insert(std::make_pair(llfile, Qt::Unchecked)); //insert the check state into the map to query for changes later
	mStateOfCheckBoxes.insert(std::make_pair(llayer, Qt::Unchecked));
      }
    blockSignals(false);
   
    //QgsLegendPropertyGroup * lpgroup = new QgsLegendPropertyGroup(llayer,QString("Properties"));

    insertTopLevelItem(0, llayer);
    
    setExpanded(indexFromItem(llayer), true);
    setExpanded(indexFromItem(llfgroup), false);
    //only if qsetting for 'legend layer file visible' is not set
    if(!mShowLegendLayerFiles)
      {
	setItemHidden(llfgroup, true);
      }
      
    llfile->updateLegendItem();
    refreshLayerSymbology(layer->getLayerID());
    
    updateMapCanvasLayerSet();
    
    // first layer?
    if (mMapCanvas->layerCount() == 1)
      mMapCanvas->zoomFullExtent();
    
    setCurrentItem(llayer);
}

QgsMapLayer* QgsLegend::currentLayer()
{
  QgsLegendItem* citem=dynamic_cast<QgsLegendItem*>(currentItem());
  
  if(citem)
    {
      QgsLegendLayerFile* llf=dynamic_cast<QgsLegendLayerFile*>(citem);
      if(llf)
	{
	  return llf->layer(); //the current item is itself a legend layer file
	}
      else
	{
	  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(citem);
	  if(ll)
	    {
	      return ll->firstMapLayer(); //the current item is a legend layer, so return its first layer
	    }
	  else
	    {
	      QgsLegendLayer* lpl = dynamic_cast<QgsLegendLayer*>(citem->parent());
	      if(lpl)
		{
		  return lpl->firstMapLayer(); //the parent of the current item is a legend layer, return its first layer
		}
	      else
		{
		  return 0;
		}
	    }
	}
    }
  else
    {
      return 0;
    }
}

void QgsLegend::legendGroupRemove()
{
    QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup*>(currentItem());
    if(lg)
    {
	//delete the legend layers first
	QTreeWidgetItem * child = lg->child(0);
        while(child) 
	{
	    setCurrentItem(child);
	    legendLayerRemove();
            child = lg->child(0);
        }
	delete lg;
    }
}

void QgsLegend::legendLayerRemove()
{
    //remove all layers of the current legendLayer
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }

   std::list<QgsMapLayer*> maplayers = ll->mapLayers();
   mStateOfCheckBoxes.erase(ll);

   //todo: also remove the entries for the QgsLegendLayerFiles from the map
   std::list<QgsLegendLayerFile*> llfiles = ll->legendLayerFiles();
   for(std::list<QgsLegendLayerFile*>::iterator it = llfiles.begin(); it != llfiles.end(); ++it)
     {
       mStateOfCheckBoxes.erase(*it);
     }

   for(std::list<QgsMapLayer*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
       //remove the layer
       if(*it)
       {
	   QgsMapLayerRegistry::instance()->removeMapLayer((*it)->getLayerID());
       }
   }

   if(maplayers.size()>0)
   {
     mMapCanvas->refresh();
   }
   removeItem(ll);
}

void QgsLegend::legendLayerAddToOverview()
{
   //add or remove all layers to/ from overview
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }

   std::list<QgsLegendLayerFile*> maplayers = ll->legendLayerFiles();
   for(std::list<QgsLegendLayerFile*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
       if(*it)
       {
	       (*it)->setInOverview(true);
       }
   }

   // TODO: new overview set
   mMapCanvas->updateOverview();
}

void QgsLegend::legendLayerRemoveFromOverview()
{
    //add or remove all layers to/ from overview
   QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(currentItem());
   if(!ll)
   {
       return;
   }
   
   std::list<QgsLegendLayerFile*> maplayers = ll->legendLayerFiles();
   for(std::list<QgsLegendLayerFile*>::iterator it = maplayers.begin(); it!=maplayers.end(); ++it)
   {
     if(*it)
     {
       (*it)->setInOverview(false);
     }
   }

   // TODO: new overview set
   mMapCanvas->updateOverview();
}

void QgsLegend::legendLayerShowProperties()
{
  QgsLegendItem* li = dynamic_cast<QgsLegendItem*>(currentItem());
  QgsLegendLayerFile* llf = 0;

  if(!li)
  {
    return;
  }
  
  if(li->type() == QgsLegendItem::LEGEND_LAYER_FILE)
  {
    llf = dynamic_cast<QgsLegendLayerFile*>(li);
  }
  else if(li->type() == QgsLegendItem::LEGEND_LAYER)
  {
    QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(li);
    llf = ll->firstLayerFile();
  }
       
  if (!llf)
  {
    return;
  }
  
  //QgsDebugMsg("Showing layer properties dialog");
  
  QgsMapLayer* ml = llf->layer();
  
  if (ml->type() == QgsMapLayer::RASTER)
  {
    QgsRasterLayerProperties *rlp = new QgsRasterLayerProperties(ml);
    connect(rlp, SIGNAL(refreshLegend(QString)), this, SLOT(refreshLayerSymbology(QString)));
    if (rlp->exec())
    {
      delete rlp;
      QCoreApplication::processEvents();
    }
      
      /*
        void QgsRasterLayer::showLayerProperties()
        {
        qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
        if ( ! mLayerProperties )
        {
        mLayerProperties = new QgsRasterLayerProperties(this);
        QgsDebugMsg("Creating new raster properties dialog instance");
      }

        mLayerProperties->sync();
        mLayerProperties->raise();
        mLayerProperties->show();
        qApp->restoreOverrideCursor();
      } // QgsRasterLayer::showLayerProperties()
      */      
  }
  else // VECTOR
  {
    QgsVectorLayer* vlayer = dynamic_cast<QgsVectorLayer*>(ml);
    
    QgsVectorLayerProperties* vlp = new QgsVectorLayerProperties(vlayer);
    connect(vlp, SIGNAL(refreshLegend(QString)), this, SLOT(refreshLayerSymbology(QString)));
    if (vlp->exec())
    {
      delete vlp;
      QCoreApplication::processEvents();
    }
    
    /*
    // TODO: this was previous implementation which saved the instance of the dialog
    
    // Set wait cursor while the property dialog is created
    // and initialized
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

    QgsVectorLayerProperties* propertiesDialog = new QgsVectorLayerProperties(vlayer);
    
    // Make sure that the UI starts out with the correct display
    // field value
    propertiesDialog->setDisplayField(displayField());

    propertiesDialog->raise();
    propertiesDialog->show();

    // restore normal cursor
    qApp->restoreOverrideCursor();
    */
  }
  
  llf->updateLegendItem();

}

void QgsLegend::expandAll()
{
    QTreeWidgetItem* theItem = firstItem();
    while(theItem)
      {
	setExpanded(indexFromItem(theItem), true);
	theItem = nextItem(theItem);
      }
}

void QgsLegend::collapseAll()
{
  QTreeWidgetItem* theItem = firstItem();
    while(theItem)
      {
	setExpanded(indexFromItem(theItem), false);
	theItem = nextItem(theItem);
      }
}

bool QgsLegend::writeXML( QDomNode & layer_node, QDomDocument & document )
{
    QDomElement legendnode = document.createElement("legend");
    layer_node.appendChild(legendnode);

    QDomElement tmplegendnode = legendnode; /*copy of the legendnode*/
    QDomElement legendgroupnode;
    QDomElement legendlayernode;
    QDomElement layerfilegroupnode;
    QDomElement legendsymbolnode;
    QDomElement legendpropertynode;
    QDomElement legendlayerfilenode;
    QgsLegendLayerFile* llf;
    Qt::CheckState cstate; //check state for legend layers and legend groups

    QTreeWidgetItem* currentItem = firstItem();
    while(currentItem) 
    {
	QgsLegendItem *item = dynamic_cast<QgsLegendItem*>(currentItem);
	if(item)
	{
	    switch(item->type())
	      {
	        case QgsLegendItem::LEGEND_GROUP:
		//make sure the legendnode is 'legend' again after a legend group
		if(!(item->parent()))
		    {
			legendnode = tmplegendnode;
		    }
		    legendgroupnode = document.createElement("legendgroup");
		    if(isItemExpanded(item))
		    {
			legendgroupnode.setAttribute("open","true");
		    }
		    else
		    {
			legendgroupnode.setAttribute("open","false");
		    }
		    legendgroupnode.setAttribute("name",item->text(0));
		    cstate = item->checkState(0);
		    if(cstate == Qt::Checked)
		      {
			legendgroupnode.setAttribute("checked","Qt::Checked");
		      }
		    else if(cstate == Qt::Unchecked)
		      {
			legendgroupnode.setAttribute("checked","Qt::Unchecked");
		      }
		    else if(cstate == Qt::PartiallyChecked)
		      {
			legendgroupnode.setAttribute("checked","Qt::PartiallyChecked");
		      }
		    legendnode.appendChild(legendgroupnode);
		    tmplegendnode =  legendnode;
		    legendnode = legendgroupnode;
		    break;

		case QgsLegendItem::LEGEND_LAYER:
		    //make sure the legendnode is 'legend' again after a legend group
		    if(!(item->parent()))
		    {
			legendnode = tmplegendnode;
		    }
		    legendlayernode = document.createElement("legendlayer");
		    if(isItemExpanded(item))
		    {
			legendlayernode.setAttribute("open","true");
		    }
		    else
		    {
			legendlayernode.setAttribute("open","false");
		    }
		    cstate = item->checkState(0);
		    if(cstate == Qt::Checked)
		      {
			legendlayernode.setAttribute("checked","Qt::Checked");
		      }
		    else if(cstate == Qt::Unchecked)
		      {
			legendlayernode.setAttribute("checked","Qt::Unchecked");
		      }
		    else if(cstate == Qt::PartiallyChecked)
		      {
			legendlayernode.setAttribute("checked","Qt::PartiallyChecked");
		      }
		    legendlayernode.setAttribute("name", item->text(0));
		    legendnode.appendChild(legendlayernode);
		    break;

		case QgsLegendItem::LEGEND_PROPERTY_GROUP:
		    legendpropertynode = document.createElement("propertygroup");
		    if(isItemExpanded(item))
		    {
			legendpropertynode.setAttribute("open","true");	
		    }
		    else
		    {
			legendpropertynode.setAttribute("open","false");
		    }
		    legendlayernode.appendChild(legendpropertynode);
		    break;

		case QgsLegendItem::LEGEND_SYMBOL_GROUP:
		    legendsymbolnode = document.createElement("symbolgroup");
		    if(isItemExpanded(item))
		    {
			legendsymbolnode.setAttribute("open", "true");
		    }
		    else
		    {
			legendsymbolnode.setAttribute("open", "false");
		    }
		    legendlayernode.appendChild(legendsymbolnode);
		    break;
		
  
		case QgsLegendItem::LEGEND_LAYER_FILE_GROUP:
		    layerfilegroupnode = document.createElement("filegroup");
		    if(isItemExpanded(item))
		    {
		      layerfilegroupnode.setAttribute("open", "true");
		    }
		    else
		    {
		      layerfilegroupnode.setAttribute("open", "false");
		    }
		    if(isItemHidden(item))
		    {
		      layerfilegroupnode.setAttribute("hidden", "true");
		    }
		    else
		    {
		      layerfilegroupnode.setAttribute("hidden", "false");
		    }
		       
		    legendlayernode.appendChild(layerfilegroupnode);
		    break;
	    
	      case QgsLegendItem::LEGEND_LAYER_FILE:
		legendlayerfilenode = document.createElement("legendlayerfile");
		llf = dynamic_cast<QgsLegendLayerFile*>(item);
		if(llf)
		  {
        QgsMapLayer* layer = llf->layer();
        
        // layer id
		    legendlayerfilenode.setAttribute("layerid", layer->getLayerID());
		    layerfilegroupnode.appendChild(legendlayerfilenode);
		  
        // visible flag
        legendlayerfilenode.setAttribute( "visible", llf->isVisible());

        // show in overview flag
        legendlayerfilenode.setAttribute( "inOverview", llf->isInOverview());
    
    }
		break;

		default: //do nothing for the leaf nodes
		    break;
	    }
	}
	currentItem = nextItem(currentItem);
    }
    return true;
}

bool QgsLegend::readXML(QDomNode& legendnode)
{
  QDomElement childelem;
  QDomNode child;
  QgsLegendGroup* lastGroup = 0; //pointer to the last inserted group
  QgsLegendLayer* lastLayer = 0; //pointer to the last inserted legendlayer
  QgsLegendLayerFileGroup* lastLayerFileGroup = 0; //pointer to the last inserted layerfilegroup
  
  child = legendnode.firstChild();

  if(!child.isNull())
    {
      clear(); //remove all items first
      mStateOfCheckBoxes.clear();

      do
	{
	  QDomElement childelem = child.toElement();
	  QString name = childelem.attribute("name");

	  //test every possibility of element...
	  if(childelem.tagName()=="legendgroup")
	    {
	      QgsLegendGroup* theGroup = new QgsLegendGroup(this, name);
	      childelem.attribute("open") == "true" ? expandItem(theGroup) : collapseItem(theGroup);
	      //set the checkbox of the legend group to the right state
	      blockSignals(true);
	      QString checked = childelem.attribute("checked");
	      if(checked == "Qt::Checked")
		{
		  theGroup->setCheckState(0, Qt::Checked);
		  mStateOfCheckBoxes.insert(std::make_pair(theGroup, Qt::Checked));
		}
	      else if(checked == "Qt::Unchecked")
		{
		  theGroup->setCheckState(0, Qt::Unchecked);
		  mStateOfCheckBoxes.insert(std::make_pair(theGroup, Qt::Unchecked));
		}
	      else if(checked == "Qt::PartiallyChecked")
		{
		  theGroup->setCheckState(0, Qt::PartiallyChecked);
		  mStateOfCheckBoxes.insert(std::make_pair(theGroup, Qt::PartiallyChecked));
		}
	      blockSignals(false);
	      lastGroup = theGroup;
	    }
	  else if(childelem.tagName()=="legendlayer")
	    {
	      //add the legendlayer to the legend (but no legendlayerfile yet, follows later)
	      //if childelem is in a legendgroup element, add the layer to the group
	      QgsLegendLayer* theLayer;
	      if(child.parentNode().toElement().tagName() == "legendgroup")
		{
		  theLayer = new QgsLegendLayer(lastGroup, name);
		}
	      else
		{
		  theLayer = new QgsLegendLayer(this, name);
		  lastGroup = 0;
		}
	      childelem.attribute("open") == "true" ? expandItem(theLayer) : collapseItem(theLayer);
	      
	      //set the checkbox of the legend layer to the right state
	      blockSignals(true);
	      QString checked = childelem.attribute("checked");
	      if(checked == "Qt::Checked")
		{
		  theLayer->setCheckState(0, Qt::Checked);
		  mStateOfCheckBoxes.insert(std::make_pair(theLayer, Qt::Checked));
		}
	      else if(checked == "Qt::Unchecked")
		{
		  theLayer->setCheckState(0, Qt::Unchecked);
		  mStateOfCheckBoxes.insert(std::make_pair(theLayer, Qt::Unchecked));
		}
	      else if(checked == "Qt::PartiallyChecked")
		{
		  theLayer->setCheckState(0, Qt::PartiallyChecked);
		  mStateOfCheckBoxes.insert(std::make_pair(theLayer, Qt::PartiallyChecked));
		}
	      blockSignals(false);

	      lastLayer = theLayer;
	    }
	  else if(childelem.tagName()=="legendlayerfile")
	    {
	      //find out the legendlayer
	      std::map<QString,QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
	      std::map<QString, QgsMapLayer*>::const_iterator iter = mapLayers.find(childelem.attribute("layerid"));
       
	      if(iter != mapLayers.end() && lastLayerFileGroup)
		{
		  QgsMapLayer* theMapLayer = iter->second;
		  QgsLegendLayerFile* theLegendLayerFile = new QgsLegendLayerFile(lastLayerFileGroup, QgsLegendLayerFile::nameFromLayer(theMapLayer), theMapLayer);

      // load layer's visibility and 'show in overview' flag
      theLegendLayerFile->setVisible(atoi(childelem.attribute("visible")));
      theLegendLayerFile->setInOverview(atoi(childelem.attribute("inOverview")));
		  
      //set the check state
		  blockSignals(true);
		  if(theLegendLayerFile->isVisible())
		    {
		      mStateOfCheckBoxes.insert(std::make_pair(theLegendLayerFile, Qt::Checked));
		      theLegendLayerFile->setCheckState(0, Qt::Checked);
		    }
		  else
		    {
		      mStateOfCheckBoxes.insert(std::make_pair(theLegendLayerFile, Qt::Unchecked));
		      theLegendLayerFile->setCheckState(0, Qt::Unchecked);
		    }
		  blockSignals(false);
		  
		  //set the layer type icon if this legendlayerfile is the last in the file group
		  if(child.nextSibling().isNull())
		  {
		    static_cast<QgsLegendLayer*>(theLegendLayerFile->parent()->parent())->setLayerTypeIcon();
		  }
    
      theLegendLayerFile->updateLegendItem();
      refreshLayerSymbology(theMapLayer->getLayerID());
  }
	    }
	  else if(childelem.tagName()=="filegroup")
	    {
	      QgsLegendLayerFileGroup* theFileGroup = new QgsLegendLayerFileGroup(lastLayer, "Files");
	      childelem.attribute("open") == "true" ? expandItem(theFileGroup) : collapseItem(theFileGroup);
	      childelem.attribute("hidden") == "true" ? setItemHidden(theFileGroup, true) : setItemHidden(theFileGroup, false);
	      lastLayerFileGroup = theFileGroup;
	    }
	  else if(childelem.tagName() == "propertygroup")
	    {
	      QgsLegendPropertyGroup* thePropertyGroup = new QgsLegendPropertyGroup(lastLayer, "Properties");
	      childelem.attribute("open") == "true" ? expandItem(thePropertyGroup) : collapseItem(thePropertyGroup);
	    }	  
	  child = nextDomNode(child);
	}
      while(!(child.isNull()));
    }
  return true;
}

void QgsLegend::storeInitialPosition(QTreeWidgetItem* li)
{
  if(li == firstItem()) //the item is the first item in the list view
    {
      mRestoreInformation = FIRST_ITEM;
      mRestoreItem = 0;
    }
  else if(li->parent() == 0) //li is a toplevel item, but not the first one
    {
      mRestoreInformation = YOUNGER_SIBLING;
      mRestoreItem = ((QgsLegendItem*)(li))->findYoungerSibling();
    }
  else if(li == li->parent()->child(0))//li is not a toplevel item, but the first child
    {
      mRestoreInformation = FIRST_CHILD;
      mRestoreItem = li->parent();
    }
  else
    {
      mRestoreInformation = YOUNGER_SIBLING;
      mRestoreItem = ((QgsLegendItem*)(li))->findYoungerSibling();
    }
  mLayersPriorToMove = layerIDs();
}

void QgsLegend::resetToInitialPosition(QTreeWidgetItem* li)
{
  if(mRestoreInformation == FIRST_ITEM)
    {
#ifdef QGISDEBUG
      qWarning("FIRST_ITEM");
#endif
      removeItem(li);
      insertTopLevelItem(0, li);
    }
  else if(mRestoreInformation == FIRST_CHILD)
    {
#ifdef QGISDEBUG
      qWarning("FIRST_CHILD");
#endif
      removeItem(li);
      mRestoreItem->insertChild(0, li);
    }
  else if(mRestoreInformation == YOUNGER_SIBLING)
    {
#ifdef QGISDEBUG
      qWarning("YOUNGER_SIBLING");
#endif
      dynamic_cast<QgsLegendItem*>(li)->moveItem(dynamic_cast<QgsLegendItem*>(mRestoreItem));
    }
}

QgsLegendLayer* QgsLegend::findLegendLayer(const QString& layerKey)
{
  QgsLegendLayer* theLegendLayer = 0;
  std::list<QgsMapLayer*> theMapLayers;
  QTreeWidgetItem* theItem = firstItem();
  do
    {
      theLegendLayer = dynamic_cast<QgsLegendLayer*>(theItem);
      if(theLegendLayer) //item is a legend layer
	{
	  theMapLayers = theLegendLayer->mapLayers();
	  for(std::list<QgsMapLayer*>::iterator it = theMapLayers.begin(); it != theMapLayers.end(); ++it)
	    {
	      if((*it)->getLayerID() == layerKey)
		{
		  return theLegendLayer;
		}
	    }
	}
    }
  while(theItem = nextItem(theItem));
  return 0;
}

void QgsLegend::adjustIconSize()
{
  if(mPixmapWidthValues.size() > 0 && mPixmapHeightValues.size() > 0)
    { 
      std::multiset<int>::const_reverse_iterator width_it = mPixmapWidthValues.rbegin();
      std::multiset<int>::const_reverse_iterator height_it = mPixmapHeightValues.rbegin();
      int maxWidth = *width_it;
      int maxHeight = *height_it;

      QSize currentIconSize = iconSize();
      if(maxWidth == currentIconSize.width() && maxHeight == currentIconSize.height())
	{
	  //no resizing necessary
	  return;
	}

      //keep the minimum size
      if(maxWidth < mMinimumIconSize.width())
	{
	  maxWidth = mMinimumIconSize.width();
	}
      if(maxHeight < mMinimumIconSize.height())
	{
	  maxHeight = mMinimumIconSize.height();
	}

      setIconSize(QSize(maxWidth, maxHeight));
    }
}

bool QgsLegend::yCoordAboveCenter(QgsLegendItem* it, int ycoord)
{
  QRect rect = visualItemRect(it);
  int height = rect.height();
  int top = rect.top();
  int mid = top + (height / 2);
  if (ycoord > mid) //bottom, remember the y-coordinate increases downwards
    {
      return false;
    }
  else//top
    {
      return true;
    }
}

/**Returns the first item in the hierarchy*/
QTreeWidgetItem* QgsLegend::firstItem()
{
  return topLevelItem(0);
}

/**Returns the next item (next sibling or next item on level above)*/
QTreeWidgetItem* QgsLegend::nextItem(QTreeWidgetItem* item)
{
  QgsLegendItem* litem = dynamic_cast<QgsLegendItem*>(item);
  if(litem->childCount() > 0)
    {
      return litem->child(0);
    }
  else if(litem->nextSibling())
    {
      return litem->nextSibling();
    }
  else if(!(litem->parent()))
    {
      return 0;
    }
  //go to other levels to find the next item
  else if(litem->parent() && ((QgsLegendItem*)(litem->parent()))->nextSibling())
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent())->nextSibling());
    }
  else if(litem->parent() && litem->parent()->parent() && ((QgsLegendItem*)(litem->parent()->parent()))->nextSibling())
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent()->parent())->nextSibling());
    }
  else if(litem->parent() && litem->parent()->parent() && litem->parent()->parent()->parent() &&\
	  ((QgsLegendItem*)(litem->parent()->parent()->parent()))->nextSibling())//maximum four nesting states in the current legend
    {
      return (dynamic_cast<QgsLegendItem*>(litem->parent()->parent()->parent())->nextSibling());
    }
  else
    {
      return 0;
    }
}

QTreeWidgetItem* QgsLegend::nextSibling(QTreeWidgetItem* item)
{
  QModelIndex thisidx = indexFromItem(item);
  QModelIndex nextsidx = thisidx.sibling(thisidx.row()+1, thisidx.column());
  if(nextsidx.isValid())
    {
      return dynamic_cast<QgsLegendItem*>(itemFromIndex(nextsidx));
    }
  else
    {
      return 0;
    }
}

QTreeWidgetItem* QgsLegend::previousSibling(QTreeWidgetItem* item)
{
  QModelIndex thisidx = indexFromItem(item);
  QModelIndex nextsidx = thisidx.sibling(thisidx.row()-1, thisidx.column());
  if(nextsidx.isValid())
    {
      return dynamic_cast<QgsLegendItem*>(itemFromIndex(nextsidx));
    }
  else
    {
      return 0;
    }
}

QDomNode QgsLegend::nextDomNode(const QDomNode& theNode)
{
  if(!theNode.firstChild().isNull())
    {
      return (theNode.firstChild());
    }
  
  QDomNode currentNode = theNode;
  do
    {
      if(!currentNode.nextSibling().isNull())
	{
	  return currentNode.nextSibling();
	}
      currentNode = currentNode.parentNode();
    }
  while(!currentNode.isNull());
  
  QDomNode nullNode;
  return nullNode;
}

void QgsLegend::insertItem(QTreeWidgetItem* move, QTreeWidgetItem* into)
{
  QgsLegendItem* movedItem = dynamic_cast<QgsLegendItem*>(move);
  QgsLegendItem* intoItem = dynamic_cast<QgsLegendItem*>(into);

  if(movedItem && intoItem)
    {
      movedItem->storeAppearanceSettings();//store settings in the moved item and its children
      removeItem(movedItem);
      intoItem->insert(movedItem);
      movedItem->restoreAppearanceSettings();//apply the settings again
    }
}

void QgsLegend::moveItem(QTreeWidgetItem* move, QTreeWidgetItem* after)
{
  static_cast<QgsLegendItem*>(move)->storeAppearanceSettings();//store settings in the moved item and its childern
  if(move->parent())
    {
      move->parent()->takeChild(move->parent()->indexOfChild(move));
    }
  else //move is toplevel item
    {
      takeTopLevelItem(indexOfTopLevelItem(move));
    }
  if(after->parent())
    {
      after->parent()->insertChild(after->parent()->indexOfChild(after)+1, move);
    }
  else //toplevel item
    {
      insertTopLevelItem(indexOfTopLevelItem(after)+1, move);
    }
  static_cast<QgsLegendItem*>(move)->restoreAppearanceSettings();//apply the settings again
}

void QgsLegend::removeItem(QTreeWidgetItem* item)
{
  if(item->parent())
    {
      item->parent()->takeChild(item->parent()->indexOfChild(item));
    }
  else
    {
      takeTopLevelItem(indexOfTopLevelItem(item));
    }
}

void QgsLegend::updateMapCanvasLayerSet()
{ 
  std::deque<QString> layers = layerIDs();
  mMapCanvas->setLayerSet(layers);
}

std::deque<QString> QgsLegend::layerIDs()
{
  std::deque<QString> layers;
  QTreeWidgetItem* theItem = firstItem();
  while (theItem)
    {
      QgsLegendItem *li = dynamic_cast<QgsLegendItem*>(theItem);
      QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(li);
      if(llf)
	{
	  QgsMapLayer *lyr = llf->layer();
	  layers.push_front(lyr->getLayerID());
	}
      theItem = nextItem(theItem);
    }
  
#ifdef QGISDEBUG
  qWarning("QgsLegend::layerIDs()");
  for(std::deque<QString>::iterator it = layers.begin(); it != layers.end(); ++it)
    {
      qWarning(*it);
    }
#endif

  return layers;
}


void QgsLegend::refreshLayerSymbology(QString key)
{
  QgsLegendLayer* theLegendLayer = findLegendLayer(key);
  if(!theLegendLayer)
  {
    return;
  }
  
  //store the current item
  QTreeWidgetItem* theCurrentItem = currentItem();

  theLegendLayer->refreshSymbology(key);
  
  //restore the current item again
  setCurrentItem(theCurrentItem);
  adjustIconSize();
}


void QgsLegend::addPixmapWidthValue(int width)
{
  mPixmapWidthValues.insert(width);
}

void QgsLegend::addPixmapHeightValue(int height)
{
  mPixmapHeightValues.insert(height);
}

void QgsLegend::removePixmapWidthValue(int width)
{
  std::multiset<int>::iterator it = mPixmapWidthValues.find(width);
  if (it != mPixmapWidthValues.end())
    {
      mPixmapWidthValues.erase(it);
    }
  //todo: adapt the icon size if width is the largest value and the size of the next element is higher than the minimum
}

void QgsLegend::removePixmapHeightValue(int height)
{
  std::multiset<int>::iterator it = mPixmapHeightValues.find(height);
  if (it != mPixmapHeightValues.end())
    {
      mPixmapHeightValues.erase(height);
    }
  //todo: adapt the icon size if height is the largest value and the size of the next element is higher than the minimum
}

void QgsLegend::setName(QgsLegendLayerFile* legendLayerFile,
                        QString layerName)
{
  if (legendLayerFile)
  {
    QTreeWidgetItem* p = legendLayerFile->parent();
    if (p)
    {
      p = p->parent();
      if (p)
        p->setText(0, layerName);
    }
  }

}

void QgsLegend::handleItemChange(QTreeWidgetItem* item, int row)
{
  if(!item)
    {
      return;
    }

  closePersistentEditor(item, row);

  std::map<QTreeWidgetItem*, Qt::CheckState>::iterator it = mStateOfCheckBoxes.find(item);
  if(it != mStateOfCheckBoxes.end())
    {
      if(it->second != item->checkState(0)) //the checkState has changed
	{
	  
	  QgsLegendLayerFile* llf = dynamic_cast<QgsLegendLayerFile*>(item); //item is a layer file
	  if(llf)
	    {
	      if(llf->layer())
		{
		  llf->setVisible(item->checkState(0) == Qt::Checked);
		}
	      //update check state of the legend layer
	      QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(item->parent()->parent());
	      if(ll)
		{
		  ll->updateCheckState();
		  mStateOfCheckBoxes[ll] = ll->checkState(0);
		}
	      //update check state of the legend group (if any)
	      if(item->parent()->parent()->parent())
		{
		  QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup*>(item->parent()->parent()->parent());
		  if(lg)
		    {
		      lg->updateCheckState();
		      mStateOfCheckBoxes[lg] = lg->checkState(0);
		    }
		}
	      mStateOfCheckBoxes[item] = item->checkState(0);
	      mMapCanvas->setRenderFlag(true);
	      return;
	    }
	  
	  std::list<QgsLegendLayerFile*> subfiles;
	  QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup*>(item); //item is a legend group
	  if(lg)
	    {
	      //set all the child layer files to the new check state
	      subfiles = lg->legendLayerFiles();
	      mMapCanvas->setRenderFlag(false);
	      for(std::list<QgsLegendLayerFile*>::iterator iter = subfiles.begin(); iter != subfiles.end(); ++iter)
		{
#ifdef QGISDEBUG
		  if(item->checkState(0) == Qt::Checked)
		    {
		      qWarning("item checked");
		    }
		  else if(item->checkState(0) == Qt::Unchecked)
		    {
		      qWarning("item unchecked");
		    }
		  else if(item->checkState(0) == Qt::PartiallyChecked)
		    {
		      qWarning("item partially checked");
		    }
#endif
		  blockSignals(true);
		  (*iter)->setCheckState(0, item->checkState(0));
		  blockSignals(false);
		  mStateOfCheckBoxes[(*iter)] = item->checkState(0);
		  if((*iter)->layer())
		    {
		      (*iter)->setVisible(item->checkState(0) == Qt::Checked);
		    }
		}
	      
	      //update the check states of all child legend layers
	      for(int i = 0; i < lg->childCount(); ++i)
		{
		  static_cast<QgsLegendLayer*>(lg->child(i))->updateCheckState();
		  mStateOfCheckBoxes[lg->child(i)] = lg->child(i)->checkState(0);
		}
	      mMapCanvas->setRenderFlag(true);
	      mStateOfCheckBoxes[item] = item->checkState(0);
	      return;
	    }
	  
	  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer*>(item); //item is a legend layer
	  if(ll)
	    {
	      //set all the child layer files to the new check state
	      subfiles = ll->legendLayerFiles();
	      mMapCanvas->setRenderFlag(false);
	      for(std::list<QgsLegendLayerFile*>::iterator iter = subfiles.begin(); iter != subfiles.end(); ++iter)
		{
		  blockSignals(true);
		  (*iter)->setCheckState(0, item->checkState(0));
		  blockSignals(false);
		  mStateOfCheckBoxes[(*iter)] = item->checkState(0);
		  if((*iter)->layer())
		    {
		      (*iter)->setVisible(item->checkState(0) == Qt::Checked);
		    }
		}
	      if(ll->parent())
		{
		  static_cast<QgsLegendGroup*>(ll->parent())->updateCheckState();
		  mStateOfCheckBoxes[ll->parent()] = ll->parent()->checkState(0);
		}
	      mMapCanvas->setRenderFlag(true);
	      //update check state of the legend group
	    }
	  mStateOfCheckBoxes[item] = item->checkState(0);
	}
    }
}

void QgsLegend::openEditor()
{
  QTreeWidgetItem* theItem = currentItem();
  if(theItem)
    {
      openPersistentEditor(theItem, 0);
    }
}

void QgsLegend::makeToTopLevelItem()
{
  QgsLegendItem* theItem = dynamic_cast<QgsLegendItem*>(currentItem());
  if(theItem)
    {
      theItem->storeAppearanceSettings();
      removeItem(theItem);
      addTopLevelItem(theItem);
      theItem->restoreAppearanceSettings();
    }
}

void QgsLegend::showLegendLayerFileGroups()
{
  // Toggle the boolean associated with the checkbox
  mShowLegendLayerFiles = !mShowLegendLayerFiles;

  QgsLegendLayerFileGroup* theFileGroup = 0;
  QTreeWidgetItem* theItem = firstItem();
  
  if(!theItem)
    {
      return;
    }

  do
    {
      theFileGroup = dynamic_cast<QgsLegendLayerFileGroup*>(theItem);
      if(theFileGroup)
	{
	  setItemHidden(theFileGroup, !mShowLegendLayerFiles);
	}
    }
  while(theItem = nextItem(theItem));
}

void QgsLegend::zoomToLayerExtent()
{
  //find current Layer
  QgsLegendLayer* currentLayer=dynamic_cast<QgsLegendLayer*>(currentItem());
  if(!currentLayer)
    {
      return;
    }

  std::list<QgsLegendLayerFile*> layerFiles = currentLayer->legendLayerFiles();
  if(layerFiles.size() == 0)
    {
      return;
    }

  QgsMapLayer* theLayer;
  for(std::list<QgsLegendLayerFile*>::iterator it= layerFiles.begin(); it != layerFiles.end(); ++it)
    {
      theLayer = (*it)->layer();
      if(theLayer)
      {
        QgsRect extent = mMapCanvas->mapRender()->layerExtentToOutputExtent(theLayer, theLayer->extent());
        //zoom to bounding box
        mMapCanvas->setExtent(extent);
        mMapCanvas->refresh();
        break;
      }
   
    }

}

void QgsLegend::initPixmaps()
{
  QString myThemePath = QgsApplication::themePath();
  mPixmaps.mOriginalPixmap.load(myThemePath + "/mActionFileSmall.png");
  mPixmaps.mInOverviewPixmap.load(myThemePath + "/mActionInOverview.png");
  mPixmaps.mEditablePixmap.load(myThemePath + "/mIconEditable.png");
  mPixmaps.mProjectionErrorPixmap.load(myThemePath + "/mIconProjectionProblem.png");
}
