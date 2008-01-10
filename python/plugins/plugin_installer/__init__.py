
def name():
  return "Plugin installer"

def description():
  return "Downloads and installs QGIS python plugins"

def version():
  return "Version 0.02"

def classFactory(iface):
  # load TestPlugin class from file testplugin.py
  from installer_plugin import InstallerPlugin
  return InstallerPlugin(iface)
