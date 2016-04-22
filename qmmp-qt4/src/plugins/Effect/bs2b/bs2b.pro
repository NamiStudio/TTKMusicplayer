include(../../plugins.pri)

HEADERS += bs2bplugin.h \
           effectbs2bfactory.h \
           settingsdialog.h

SOURCES += bs2bplugin.cpp \
           effectbs2bfactory.cpp \
           settingsdialog.cpp

TARGET =$$PLUGINS_PREFIX/Effect/bs2b

INCLUDEPATH += ../../../ \
            $$EXTRA_PREFIX/libbs2b/include

CONFIG += warn_on \
plugin \
link_pkgconfig

TEMPLATE = lib
QMAKE_LIBDIR += ../../../../lib

FORMS += settingsdialog.ui

unix {
    isEmpty(LIB_DIR){
        LIB_DIR = /lib
    }
    target.path = $$LIB_DIR/qmmp/Effect
    INSTALLS += target
    LIBS += -L$$EXTRA_PREFIX/libbs2b/lib -lbs2b -lqmmp
}

win32 {
    QMAKE_LIBDIR += ../../../../bin
    LIBS += -L$$EXTRA_PREFIX/libbs2b/lib -lbs2b -lqmmp0
#    LIBS += -lqmmp0 -lbs2b
}