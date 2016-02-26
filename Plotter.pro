#-------------------------------------------------
#
# Project created by QtCreator 2014-12-22T14:53:33
#
#-------------------------------------------------

QT       += core gui network
QT       += serialport
CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = SerialPortPlotter
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    	qcustomplot.cpp \
    myserver.cpp \
    mythread.cpp

HEADERS  += mainwindow.hpp \
    		qcustomplot.h \
    myserver.h \
    mythread.h


FORMS    += mainwindow.ui

RC_FILE = myapp.rc

RESOURCES += \
    qdarkstyle/style.qrc

DISTFILES += \
    README.md


