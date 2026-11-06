#include "replacedialog.h"
#include "ui_replacedialog.h"

#include "codeeditor.h"

#include <QApplication>
#include <QTextCursor>

ReplaceDialog::ReplaceDialog(CodeEditor *editor, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ReplaceDialog)
    , editor_(editor)
{
    ui->setupUi(this);
    setModal(false);

    connect(ui->findButton, &QPushButton::clicked, this, &ReplaceDialog::findNext);
    connect(ui->replaceButton, &QPushButton::clicked, this, &ReplaceDialog::replaceOne);
    connect(ui->replaceAllButton, &QPushButton::clicked, this, &ReplaceDialog::replaceAll);
    connect(ui->closeButton, &QPushButton::clicked, this, &ReplaceDialog::close);

    if (editor_ && editor_->textCursor().hasSelection())
        ui->findEdit->setText(editor_->textCursor().selectedText());
}

ReplaceDialog::~ReplaceDialog()
{
    delete ui;
}

QTextDocument::FindFlags ReplaceDialog::findFlags() const
{
    QTextDocument::FindFlags flags;
    if (ui->caseCheckBox->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (ui->wholeCheckBox->isChecked())
        flags |= QTextDocument::FindWholeWords;
    return flags;
}

bool ReplaceDialog::doFind()
{
    const QString keyword = ui->findEdit->text();
    if (keyword.isEmpty() || !editor_) {
        ui->resultLabel->setText(tr("请输入要查找的内容。"));
        return false;
    }

    const QTextDocument::FindFlags flags = findFlags();
    QTextCursor found = editor_->document()->find(keyword, editor_->textCursor(), flags);

    bool wrapped = false;
    if (found.isNull()) {   // 环回到文首继续找
        QTextCursor wrapCursor(editor_->document());
        found = editor_->document()->find(keyword, wrapCursor, flags);
        wrapped = !found.isNull();
    }

    if (found.isNull()) {
        ui->resultLabel->setText(tr("未找到“%1”。").arg(keyword));
        return false;
    }

    editor_->setTextCursor(found);
    editor_->ensureCursorVisible();
    if (wrapped)
        ui->resultLabel->setText(tr("已从文首继续查找“%1”。").arg(keyword));
    return true;
}

void ReplaceDialog::findNext()
{
    doFind();
}

void ReplaceDialog::replaceOne()
{
    const QString keyword = ui->findEdit->text();
    if (keyword.isEmpty() || !editor_) {
        ui->resultLabel->setText(tr("请输入要查找的内容。"));
        return;
    }

    // 当前选中内容是否就是要查找的词？不是的话先找一处
    const Qt::CaseSensitivity cs = ui->caseCheckBox->isChecked() ? Qt::CaseSensitive
                                                                 : Qt::CaseInsensitive;
    QTextCursor cursor = editor_->textCursor();
    const bool selectionIsMatch = cursor.hasSelection()
            && QString::compare(cursor.selectedText(), keyword, cs) == 0;

    if (!selectionIsMatch && !doFind())
        return;

    QTextCursor replaceCursor = editor_->textCursor();
    replaceCursor.insertText(ui->replaceEdit->text());
    ui->resultLabel->setText(tr("已替换 1 处。"));

    // 替换后自动跳到下一处，方便连续点“替换”
    doFind();
}

void ReplaceDialog::replaceAll()
{
    const QString keyword = ui->findEdit->text();
    if (keyword.isEmpty() || !editor_) {
        ui->resultLabel->setText(tr("请输入要查找的内容。"));
        return;
    }

    const QString replacement = ui->replaceEdit->text();
    const QTextDocument::FindFlags flags = findFlags();
    QTextDocument *doc = editor_->document();

    int count = 0;
    QTextCursor workCursor(doc);
    workCursor.beginEditBlock();        // 让“全部替换”可以一次性撤销

    while (true) {
        QTextCursor found = doc->find(keyword, workCursor, flags);
        if (found.isNull())
            break;
        found.insertText(replacement);
        workCursor = found;             // 从替换后的位置继续找，避免死循环
        ++count;
    }

    workCursor.endEditBlock();

    if (count == 0) {
        ui->resultLabel->setText(tr("未找到“%1”，没有做任何替换。").arg(keyword));
        QApplication::beep();
    } else {
        ui->resultLabel->setText(tr("全部替换完成，共替换 %1 处。").arg(count));
    }
}
