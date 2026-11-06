#ifndef REPLACEDIALOG_H
#define REPLACEDIALOG_H

#include <QDialog>
#include <QTextDocument>

class CodeEditor;

namespace Ui {
class ReplaceDialog;
}

/**
 * @brief 替换对话框（非模态）
 */
class ReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReplaceDialog(CodeEditor *editor, QWidget *parent = nullptr);
    ~ReplaceDialog() override;

private slots:
    void findNext();
    void replaceOne();
    void replaceAll();

private:
    QTextDocument::FindFlags findFlags() const;
    bool doFind();

    Ui::ReplaceDialog *ui;
    CodeEditor *editor_;
};

#endif // REPLACEDIALOG_H
