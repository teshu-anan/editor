#-------------------------------------------------
#
# 项目名称：SimpleEditor（简单文本编辑器）
# 课程：Qt 应用程序开发 —— 实验二
# 说明：多窗口文本编辑器，使用 Qt Widgets + Qt Designer 实现
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET   = SimpleEditor
TEMPLATE = app

# C++11 及以上
CONFIG += c++11

# 让 MSVC 也按 UTF-8 读取源码（MinGW 默认即为 UTF-8）
msvc: QMAKE_CXXFLAGS += /utf-8

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    codeeditor.cpp \
    finddialog.cpp \
    replacedialog.cpp \
    aboutdialog.cpp

HEADERS += \
    mainwindow.h \
    codeeditor.h \
    finddialog.h \
    replacedialog.h \
    aboutdialog.h

FORMS += \
    mainwindow.ui \
    finddialog.ui \
    replacedialog.ui \
    aboutdialog.ui

RESOURCES += \
    resources.qrc
