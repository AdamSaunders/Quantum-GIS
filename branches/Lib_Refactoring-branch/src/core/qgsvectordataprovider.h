/***************************************************************************
    qgsvectordataprovider.h - DataProvider Interface for vector layers
     --------------------------------------
    Date                 : 23-Sep-2004
    Copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#ifndef QGSVECTORDATAPROVIDER_H
#define QGSVECTORDATAPROVIDER_H


class QTextCodec;

#include <QList>
#include <QSet>
#include <QMap>

//QGIS Includes
#include "qgis.h"
#include "qgsdataprovider.h"

#include "qgsvectorlayer.h"


/** Base class for vector data providers
 */
class CORE_EXPORT QgsVectorDataProvider : public QgsDataProvider
{
    public:

      // If you add to this, please also add to capabilitiesString()
      /**
       * enumeration with capabilities that providers might implement
       */
      enum Capability
      {
        NoCapabilities =                     0,
        AddFeatures =                        1,
        DeleteFeatures =               1 <<  1,
        ChangeAttributeValues =        1 <<  2,
        AddAttributes =                1 <<  3,
        DeleteAttributes =             1 <<  4,
        SaveAsShapefile =              1 <<  5,
        CreateSpatialIndex =           1 <<  6,
        SelectAtId =                   1 <<  7,
        ChangeGeometries =             1 <<  8,
        SelectGeometryAtId =           1 <<  9,
        RandomSelectGeometryAtId =     1 << 10,
        SequentialSelectGeometryAtId = 1 << 11
      };

      /**
       * Constructor of the vector provider
       * @param uri  uniform resource locator (URI) for a dataset
       */
      QgsVectorDataProvider(QString uri = QString());

      /**
       * Destructor
       */
      virtual ~QgsVectorDataProvider();

      /**
       * Returns the permanent storage type for this layer as a friendly name.
       */
      virtual QString storageType() const;

      /**
       * Select features based on a bounding rectangle. Features can be retrieved 
       * with calls to getFirstFeature and getNextFeature. Request for features 
       * for use in drawing the map canvas should set useIntersect to false.
       * @param mbr QgsRect containing the extent to use in selecting features
       * @param useIntersect If true, use the intersects function to select features
       * rather than the PostGIS && operator that selects based on bounding box
       * overlap.
       *
       */
      virtual void select(QgsRect mbr, bool useIntersect = false) = 0;

      /**
       * Update the feature count based on current spatial filter. If not
       * overridden in the data provider this function returns -1
       */
      virtual long updateFeatureCount();

      /** 
       * Gets the feature at the given feature ID.
       * @param featureId id of the feature
       * @param feature feature which will receive the data
       * @param fetchGeoemtry if true, geometry will be fetched from the provider
       * @param fetchAttributes a list containing the indexes of the attribute fields to copy
       * @return True when feature was found, otherwise false
       */
      virtual bool getFeatureAtId(int featureId,
                                  QgsFeature& feature,
                                  bool fetchGeometry = true,
                                  QgsAttributeList fetchAttributes = QgsAttributeList());

      /**
       * Get the next feature resulting from a select operation.
       * @param feature feature which will receive data from the provider
       * @param fetchGeoemtry if true, geometry will be fetched from the provider
       * @param fetchAttributes a list containing the indexes of the attribute fields to copy
       * @param featureQueueSize  a hint to the provider as to how many features are likely to be retrieved in a batch
       * @return true when there was a feature to fetch, false when end was hit
       */
      virtual bool getNextFeature(QgsFeature& feature,
                                  bool fetchGeometry = true,
                                  QgsAttributeList fetchAttributes = QgsAttributeList(),
                                  uint featureQueueSize = 1) = 0;

      /**
       * Get feature type.
       * @return int representing the feature type
       */
      virtual QGis::WKBTYPE geometryType() const = 0;


      /**
       * Number of features in the layer
       * @return long containing number of features
       */
      virtual long featureCount() const = 0;

      /**
       * Get the attributes associated with a feature
       * TODO: Get rid of "row" and set up provider-internal caching instead
       */
      virtual void getFeatureAttributes(int key, int& row, QgsFeature *f);

      /**
       * Fetch geometry for a particular feature with id "key",
       * modifies "f" in-place.
       *
       * This function is enabled if capabilities() returns "SelectGeometryAtId".
       */
      virtual void getFeatureGeometry(int key, QgsFeature *f);

      /**
       * Number of attribute fields for a feature in the layer
       */
      virtual uint fieldCount() const = 0;

      /**
       * Return a map of indexes with field names for this layer
       * @return map of fields
       */
      virtual const QgsFieldMap & fields() const = 0;

      /** 
       * Reset the layer to clear any spatial filtering or other contstraints that
       * would prevent the entire record set from being traversed by call to 
       * getNextFeature(). Some data stores may not require any special action to
       * reset the layer. In this case, the provider should simply implement an empty
       * function body.
       */
      virtual void reset() = 0;

      /**
       * Returns the minimum value of an attributs
       * @param position the number of the attribute
       */
      virtual QString minValue(uint position) = 0;

      /**
       * Returns the maximum value of an attributs
       * @param position the number of the attribute
       */
      virtual QString maxValue(uint position) = 0;

      /**
       * Adds a list of features
       * @return true in case of success and false in case of failure
       */
      virtual bool addFeatures(QgsFeatureList & flist);

      /** 
       * Deletes a feature
       * @param id list containing feature ids to delete
       * @return true in case of success and false in case of failure
       */
      virtual bool deleteFeatures(const QgsFeatureIds & id);

      /**
       * Adds new attributes
       * @param attributes map with attribute name as key and type as value
       * @return true in case of success and false in case of failure
       */
      virtual bool addAttributes(const QgsNewAttributesMap & attributes);

      /**
       * Deletes existing attributes
       * @param attributes a set containing indexes of attributes
       * @return true in case of success and false in case of failure
       */
      virtual bool deleteAttributes(const QgsAttributeIds & attributes);

      /**
       * Changes attribute values of existing features.
       * @param attr_map a map containing changed attributes
       * @return true in case of success and false in case of failure 
       */
      virtual bool changeAttributeValues(const QgsChangedAttributesMap & attr_map);

      /**
       * Returns the default value for attribute @c attr for feature @c f.
       */
      virtual QString getDefaultValue(const QString & attr, QgsFeature* f);

      /**
       * Changes geometries of existing features
       * @param geometry_map   A std::map containing the feature IDs to change the geometries of. 
       *                       the second map parameter being the new geometries themselves
       * @return               true in case of success and false in case of failure
       */
      virtual bool changeGeometryValues(QgsGeometryMap & geometry_map);

      /**
       * Creates a spatial index on the datasource (if supported by the provider type).
       * @return true in case of success
       */
      virtual bool createSpatialIndex();

      /** Returns a bitmask containing the supported capabilities
          Note, some capabilities may change depending on whether
          a spatial filter is active on this provider, so it may
          be prudent to check this value per intended operation.
       */
      virtual int capabilities() const;

      /**
       *  Returns the above in friendly format.
       */
      QString capabilitiesString() const;

      /**
       * Set encoding used for accessing data from layer
       */
      virtual void setEncoding(const QString& e);
      
      /**
       * Get encoding which is used for accessing data
       */
      QString encoding() const;
      
      /**
       * Returns the index of a field name or -1 if the field does not exist
       */
      int indexFromFieldName(const QString& fieldName) const;
      
      /**
       * Return list of indexes to fetch all attributes in getNextFeature()
       */
      QgsAttributeList allAttributesList();

    protected:

      /** Encoding */
      QTextCodec* mEncoding;

};

#endif
