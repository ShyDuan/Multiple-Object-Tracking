#-------------------------------------------------
#
# Project created by QtCreator 2021-05-10T15:31:08
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MOTqt
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp \
    kalman_filter.cpp \
    matrix.cpp \
    munkres.cpp \
    track.cpp \
    tracker.cpp

HEADERS  += mainwindow.h \
    kalman_filter.h \
    matrix.h \
    munkres.h \
    track.h \
    tracker.h \
    utils.h

FORMS    += \
    mainwindow.ui

# 引入OpenCV库、Boost库、Eigen库
# 如果额外宏为zynq
ZYNQ_LIB=/opt/alinx/opencv_zynq_lib
INCLUDEPATH +=$$ZYNQ_LIB/include
if(contains(DEFINES,zynq)){
    message("compile for zynq")
    QMAKE_LIBDIR_FLAGS +=-L$$ZYNQ_LIB/lib
    QMAKE_LIBDIR_FLAGS +=-L/opt/alinx/zynq_boost/lib

    INCLUDEPATH += /opt/alinx/opencv_zynq_lib/include \
                   /opt/alinx/zynq_boost/include \
                   /usr/local/include/eigen3
    LIBS += \
            -lz  \
            -lavcodec \
            -lavformat \
            -lavutil \
            -lswscale \
            -lswresample \
            -lx264\
            -lxvidcore\
            -lpng \
            -ljpeg \
            -ltiff \
}else{
    message("compile for host")
    INCLUDEPATH +=  /usr/local/include \
                /usr/local/include/opencv \
                /usr/local/include/opencv2 \
                /usr/local/include/boost \
                /usr/local/include/eigen3
}

LIBS += \
        -lopencv_shape \
        -lopencv_stitching \
        -lopencv_objdetect \
        -lopencv_superres \
        -lopencv_videostab \
        -lopencv_calib3d \
        -lopencv_features2d \
#        -lopencv_highgui \
#        -lopencv_videoio \
        -lopencv_imgcodecs \
        -lopencv_video \
        -lopencv_photo \
        -lopencv_ml \
        -lopencv_imgproc \
        -lopencv_flann \
        -lopencv_core \
        -ldl  \
        # undefined reference to symbol 'pthread_getspecific@@GLIBC_2.4'
        -lpthread \
        -lboost_filesystem \
        -lboost_program_options
