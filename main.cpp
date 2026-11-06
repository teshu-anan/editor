#include "mainwindow.h"

#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>

// Qt 自带的标准控件中文翻译（文件对话框、消息框按钮等）
static QTranslator qtTranslator;
static QTranslator qtBaseTranslator;

static void loadQtTranslations()
{
    const QString dir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if (qtTranslator.load(QStringLiteral("qt_zh_CN.qm"), dir))
        qApp->installTranslator(&qtTranslator);
    if (qtBaseTranslator.load(QStringLiteral("qtbase_zh_CN.qm"), dir))
        qApp->installTranslator(&qtBaseTranslator);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    loadQtTranslations();

    // 这些信息会显示在“关于”对话框中
    QApplication::setApplicationName(QStringLiteral("简单文本编辑器"));
    QApplication::setApplicationVersion(QStringLiteral("1.0"));
    QApplication::setOrganizationName(QStringLiteral("DGUT"));

    MainWindow window;
    window.show();

    return app.exec();
}
