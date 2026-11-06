#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QTextDocument>

class CodeEditor;

namespace Ui {
class FindDialog;
}

/**
 * @brief 查找对话框（非模态，可以一边开着一边继续编辑）
 */
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(CodeEditor *editor, QWidget *parent = nullptr);
    ~FindDialog() override;

public slots:
    void findNext();
    void findPrevious();

private:
    QTextDocument::FindFlags findFlags(bool backward) const;
    bool doFind(bool backward);

    Ui::FindDialog *ui;
    CodeEditor *editor_;
};

#endif // FINDDIALOG_H
