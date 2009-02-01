#-----------------------------------------------------------
# 
# Spatial Join
#
# Performing an attribute join between vector layers based on spatial
# relationship. Attributes from one vector layer are appended to the
# attribute table of another layer based on the relative locations of
# the features in the two layers.
#
# Copyright (C) 2008  Carson Farmer
#
# EMAIL: carson.farmer (at) gmail.com
# WEB  : www.geog.uvic.ca/spar/carson
#
#-----------------------------------------------------------
# 
# licensed under the terms of GNU GPL 2
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# 
#---------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui import *
import ftools_utils
from qgis.core import *
from frmSpatialJoin import Ui_Dialog

class Dialog(QDialog, Ui_Dialog):

	def __init__(self, iface):
		QDialog.__init__(self)
		self.iface = iface
		# Set up the user interface from Designer.
		self.setupUi(self)
		QObject.connect(self.toolOut, SIGNAL("clicked()"), self.outFile)
		self.setWindowTitle("Join attributes by location")
		# populate layer list
		self.progressBar.setValue(0)
		mapCanvas = self.iface.mapCanvas()
		for i in range(mapCanvas.layerCount()):
			layer = mapCanvas.layer(i)
			if layer.type() == layer.VectorLayer:
				self.inShape.addItem(layer.name())
				self.joinShape.addItem(layer.name())
		
	def accept(self):
		if self.inShape.currentText() == "":
			QMessageBox.information(self, "Spatial Join", "Please specify target vector layer")
		elif self.outShape.text() == "":
			QMessageBox.information(self, "Spatial Join", "Please specify output shapefile")
		elif self.joinShape.currentText() == "":
			QMessageBox.information(self, "Spatial Join", "Please specify join vector layer")
		elif self.rdoSummary.isChecked() and not (self.chkMean.isChecked() or self.chkSum.isChecked() or self.chkMin.isChecked() or self.chkMax.isChecked() or self.chkMean.isChecked()):
			QMessageBox.information(self, "Spatial Join", "Please specify at least one summary statistic")
		else:
			inName = self.inShape.currentText()
			joinName = self.joinShape.currentText()
			outPath = self.outShape.text()
			if self.rdoSummary.isChecked():
				summary = True
				sumList = []
				if self.chkSum.isChecked(): sumList.append("SUM")
				if self.chkMean.isChecked(): sumList.append("MEAN")
				if self.chkMin.isChecked(): sumList.append("MIN")
				if self.chkMax.isChecked(): sumList.append("MAX")
			else:
				summary = False
				sumList = ["all"]
			if self.rdoKeep.isChecked(): keep = True
			else: keep = False
			if outPath.contains("\\"):
				outName = outPath.right((outPath.length() - outPath.lastIndexOf("\\")) - 1)
			else:
				outName = outPath.right((outPath.length() - outPath.lastIndexOf("/")) - 1)
			if outName.endsWith(".shp"):
				outName = outName.left(outName.length() - 4)
			self.compute(inName, joinName, outPath, summary, sumList, keep, self.progressBar)
			self.outShape.clear()
			addToTOC = QMessageBox.question(self, "Spatial Join", "Created output Shapefile:\n" + outPath 
			+ "\n\nWould you like to add the new layer to the TOC?", QMessageBox.Yes, QMessageBox.No, QMessageBox.NoButton)
			if addToTOC == QMessageBox.Yes:
				self.vlayer = QgsVectorLayer(outPath, unicode(outName), "ogr")
				QgsMapLayerRegistry.instance().addMapLayer(self.vlayer)
		self.progressBar.setValue(0)

	def outFile(self):
		self.outShape.clear()
		( self.shapefileName, self.encoding ) = ftools_utils.saveDialog( self )
		if self.shapefileName is None or self.encoding is None:
			return
		self.outShape.setText( QString( self.shapefileName ) )

	def compute(self, inName, joinName, outName, summary, sumList, keep, progressBar):
		layer1 = self.getVectorLayerByName(inName)
		provider1 = layer1.dataProvider()
		allAttrs = provider1.attributeIndexes()
		provider1.select(allAttrs)
		fieldList1 = self.getFieldList(layer1).values()

		layer2 = self.getVectorLayerByName(joinName)
		provider2 = layer2.dataProvider()
		allAttrs = provider2.attributeIndexes()
		provider2.select(allAttrs)
		fieldList2 = self.getFieldList(layer2)
		fieldList = []
		if provider1.crs() <> provider2.crs():
			QMessageBox.warning(self, "CRS warning!", "Warning: Input layers have non-matching CRS.\nThis may cause unexpected results.")
		if not summary:
			fieldList2 = self.testForUniqueness(fieldList1, fieldList2.values())
			seq = range(0, len(fieldList1) + len(fieldList2))
			fieldList1.extend(fieldList2)
			fieldList1 = dict(zip(seq, fieldList1))
		else:
			numFields = {}
			for j in fieldList2.keys():
				if fieldList2[j].type() == QVariant.Int or fieldList2[j].type() == QVariant.Double:
					numFields[j] = []
					for i in sumList:
						field = QgsField(i + unicode(fieldList2[j].name()), QVariant.Double, "real", 24, 16, "Summary field")
						fieldList.append(field)
			field = QgsField("COUNT", QVariant.Double, "real", 24, 16, "Summary field")
			fieldList.append(field)
			fieldList2 = self.testForUniqueness(fieldList1, fieldList)
			fieldList1.extend(fieldList)
			seq = range(0, len(fieldList1))
			fieldList1 = dict(zip(seq, fieldList1))

		sRs = provider1.crs()
		progressBar.setValue(13)
		check = QFile(self.shapefileName)
		if check.exists():
			if not QgsVectorFileWriter.deleteShapeFile(self.shapefileName):
				return
		writer = QgsVectorFileWriter(self.shapefileName, self.encoding, fieldList1, provider1.geometryType(), sRs)
		#writer = QgsVectorFileWriter(outName, "UTF-8", fieldList1, provider1.geometryType(), sRs)
		inFeat = QgsFeature()
		outFeat = QgsFeature()
		joinFeat = QgsFeature()
		inGeom = QgsGeometry()
		progressBar.setValue(15)
		start = 15.00
		add = 85.00 / provider1.featureCount()
		provider1.rewind()

		while provider1.nextFeature(inFeat):
			inGeom = inFeat.geometry()
			atMap1 = inFeat.attributeMap()
			outFeat.setGeometry(inGeom)
			none = True
			joinList = []
			if inGeom.type() == QGis.Point:
				#(check, joinList) = layer2.featuresInRectangle(inGeom.buffer(10,2).boundingBox(), True, True)
				layer2.select(inGeom.buffer(10,2).boundingBox(), False)
				joinList = layer2.selectedFeatures()
				if len(joinList) > 0: check = 0
				else: check = 1
			else:
				#(check, joinList) = layer2.featuresInRectangle(inGeom.boundingBox(), True, True)
				layer2.select(inGeom.boundingBox(), False)
				joinList = layer2.selectedFeatures()
				if len(joinList) > 0: check = 0
				else: check = 1
			if check == 0:
				count = 0
				for i in joinList:
					tempGeom = i.geometry()
					if inGeom.intersects(tempGeom):
						count = count + 1
						none = False
						atMap2 = i.attributeMap()
						if not summary:	
							atMap = atMap1.values()
							atMap2 = atMap2.values()
							atMap.extend(atMap2)
							atMap = dict(zip(seq, atMap))					
							break
						else:
							for j in numFields.keys():
								numFields[j].append(atMap2[j].toDouble()[0])
				if summary and not none:
					atMap = atMap1.values()
					for j in numFields.keys():
						for k in sumList:
							if k == "SUM": atMap.append(QVariant(sum(numFields[j])))
							elif k == "MEAN": atMap.append(QVariant(sum(numFields[j]) / count))
							elif k == "MIN": atMap.append(QVariant(min(numFields[j])))
							else: atMap.append(QVariant(max(numFields[j])))
						numFields[j] = []
					atMap.append(QVariant(count))
					atMap = dict(zip(seq, atMap))
			if none:
				outFeat.setAttributeMap(atMap1)
			else:
				outFeat.setAttributeMap(atMap)
			if keep: # keep all records
				writer.addFeature(outFeat)
			else: # keep only matching records
				if not none:
					writer.addFeature(outFeat)
			start = start + add
			progressBar.setValue(start)
		del writer

	def testForUniqueness(self, fieldList1, fieldList2):
		changed = True
		while changed:
			changed = False
			for i in fieldList1:
				for j in fieldList2:
					if j.name() == i.name():
						j = self.createUniqueFieldName(j)
						changed = True
		return fieldList2
				
	def createUniqueFieldName(self, field):
		check = field.name().right(2)
		if check.startsWith("_"):
			(val,test) = check.right(1).toInt()
			if test:
				if val < 2:
					val = 2
				else:
					val = val + 1
				field.setName(field.name().left(len(field.name())-1) + unicode(val))
			else:
				field.setName(field.name() + "_2")
		else:
			field.setName(field.name() + "_2")
		return field
				
	def getVectorLayerByName(self, myName):
		mc = self.iface.mapCanvas()
		nLayers = mc.layerCount()
		for l in range(nLayers):
			layer = mc.layer(l)
			if layer.name() == unicode(myName):
				vlayer = QgsVectorLayer(unicode(layer.source()),  unicode(myName),  unicode(layer.dataProvider().name()))
				if vlayer.isValid():
					return vlayer
				else:
					QMessageBox.information(self, "Spatial Join", "Vector layer is not valid")

	def getFieldList(self, vlayer):
		fProvider = vlayer.dataProvider()
		feat = QgsFeature()
		allAttrs = fProvider.attributeIndexes()
		fProvider.select(allAttrs)
		myFields = fProvider.fields()
		return myFields
