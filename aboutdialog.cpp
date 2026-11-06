#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QApplication>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->versionLabel->setText(tr("版本 %1").arg(QApplication::applicationVersion()));

    ui->descriptionLabel->setText(
                tr("一个使用 Qt %1 编写的多窗口文本编辑器，支持文件读写、"
                   "撤销/重做、查找替换、字体与颜色设置、行号显示、"
                   "自动换行以及定时自动保存。\n\n"
                   "本程序为《Qt 应用程序开发》课程实验二的作业。")
                .arg(QString::fromLatin1(qVersion())));

    ui->copyrightLabel->setText(tr("东莞理工学院  计算机科学与技术学院"));

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
