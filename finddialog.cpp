#include "finddialog.h"
#include "ui_finddialog.h"

#include "codeeditor.h"

#include <QApplication>
#include <QTextCursor>

FindDialog::FindDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FindDialog)
    , editor_(editor)
{
    ui->setupUi(this);
    setModal(false);

    connect(ui->findButton, &QPushButton::clicked, this, &FindDialog::findNext);
    connect(ui->closeButton, &QPushButton::clicked, this, &FindDialog::close);
    connect(ui->findEdit, &QLineEdit::returnPressed, this, &FindDialog::findNext);
    // 勾选“向上查找”后，按钮文字跟着变化
    connect(ui->backwardCheckBox, &QCheckBox::toggled, this, [this](bool on) {
        ui->findButton->setText(on ? tr("查找上一个(&F)") : tr("查找下一个(&F)"));
    });

    // 打开对话框时自动带上编辑器里已选中的文字
    if (editor_ && editor_->textCursor().hasSelection())
        ui->findEdit->setText(editor_->textCursor().selectedText());
}

FindDialog::~FindDialog()
{
    delete ui;
}

QTextDocument::FindFlags FindDialog::findFlags(bool backward) const
{
    QTextDocument::FindFlags flags;
    if (ui->caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;
    if (backward)
        flags |= QTextDocument::FindBackward;
    return flags;
}

void FindDialog::findNext()
{
    doFind(ui->backwardCheckBox->isChecked());
}

void FindDialog::findPrevious()
{
    doFind(true);
}

bool FindDialog::doFind(bool backward)
{
    const QString keyword = ui->findEdit->text();
    if (keyword.isEmpty()) {
        ui->resultLabel->setText(tr("请输入要查找的内容。"));
        ui->findEdit->setFocus();
        return false;
    }
    if (!editor_)
        return false;

    const QTextDocument::FindFlags flags = findFlags(backward);

    // 第一次：从当前光标处往后（或往前）找
    QTextCursor found = editor_->document()->find(keyword, editor_->textCursor(), flags);
    bool wrapped = false;

    // 没找到就“环回”到文档另一端，实现连续查找
    if (found.isNull()) {
        QTextCursor wrapCursor(editor_->document());
        if (backward)
            wrapCursor.movePosition(QTextCursor::End);
        found = editor_->document()->find(keyword, wrapCursor, flags);
        wrapped = !found.isNull();
    }

    if (found.isNull()) {
        ui->resultLabel->setText(tr("未找到“%1”。").arg(keyword));
        QApplication::beep();
        return false;
    }

    editor_->setTextCursor(found);
    editor_->ensureCursorVisible();
    // 注意：这里不把焦点抢回编辑器，否则用户无法继续按回车“连续查找”

    const QString tip = wrapped
            ? (backward ? tr("（已从文末继续查找）") : tr("（已从文首继续查找）"))
            : QString();
    ui->resultLabel->setText(tr("找到“%1”%2").arg(keyword, tip));
    return true;
}
