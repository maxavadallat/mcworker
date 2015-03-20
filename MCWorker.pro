
# Target
TARGET                  = mcworker

# Destination Dir
DESTDIR                 = ../MaxCommander

# Application Version
VERSION                 = 0.0.1

# Template
TEMPLATE                = app

# Qt Modules/Config
QT                      += core
QT                      -= gui
QT                      += network
QT                      += widgets

CONFIG                  -= app_bundle


# Sources
SOURCES                 += src/mcwmain.cpp \
                        src/mcwfileserver.cpp \
                        src/mcwutility.cpp \
                        src/mcwfileserverconnection.cpp \
                        src/mcwfileserverconnectionworker.cpp


# Headera
HEADERS                 += \
                        src/mcwconstants.h \
                        src/mcwutility.h \
                        src/mcwfileserver.h \
                        src/mcwinterface.h \
                        src/mcwfileserverconnection.h \
                        src/mcwfileserverconnectionworker.h

# Other Files
OTHER_FILES             += \

exportedheaders.files   += src/mcwinterface.h
exportedheaders.path    = /usr/local/include/mcw

# Installs
INSTALLS                += exportedheaders


# Output/Intermediate Dirs
OBJECTS_DIR             = ./objs
OBJMOC                  = ./objs
MOC_DIR                 = ./objs
UI_DIR                  = ./objs
RCC_DIR                 = ./objs


