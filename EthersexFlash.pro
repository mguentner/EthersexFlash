#-------------------------------------------------
#
# Project created by QtCreator 2012-06-05T10:56:30
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = EthersexFlash
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qtftp.cpp

HEADERS  += mainwindow.h \
    qtftp.h \
    qendian.h

FORMS    += mainwindow.ui

RESOURCES += \
    icons.qrc
