TEMPLATE = lib
QT += qml quick
CONFIG += qt plugin

CONFIG(debug, debug|release) {
    DESTDIR = $${PWD}/bin/debug/
} else {
    DESTDIR = $${PWD}/bin/release/
}

TARGET = photosphere

HEADERS += $$files(src/*.h)
SOURCES += $$files(src/*.cpp)

RESOURCES += $$files(*.qrc)

OTHER_FILES +=  $$files(src/js/*.js) \
		$$files(src/qml/*.qml) \
                *.pri \
                *.qrc \
                qmldir

LIBS += -ltinyxml2  # tinyxml2 needed for tinyEXIF
LIBS += -lGLU

#message( $${DESTDIR}/$${MODULE_NAME} - $${QMLDIR_DEST_PATH})

qmldir.path = $${DESTDIR}/PhotoSphere
qmldir.extra =  mkdir -p $${DESTDIR}/PhotoSphere
qmldir.files = qmldir
INSTALLS += qmldir
