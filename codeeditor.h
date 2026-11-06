#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class LineNumberArea;

/**
 * @brief 带行号显示与当前行高亮的文本编辑控件
 *
 * 继承自 QPlainTextEdit，在左侧额外绘制一个“行号栏”。
 * 在 Qt Designer 中通过“提升（Promote to...）”的方式把它当作
 * QPlainTextEdit 使用，提升类名 CodeEditor，头文件 codeeditor.h。
 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

    /// 行号栏占用的宽度（隐藏行号时为 0）
    int lineNumberAreaWidth() const;

    /// 供 LineNumberArea 回调，完成行号的绘制
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    bool lineNumbersVisible() const { return showLineNumbers_; }

public slots:
    /// 显示 / 隐藏行号
    void setLineNumbersVisible(bool visible);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    LineNumberArea *lineNumberArea_;
    bool showLineNumbers_;
};

/**
 * @brief 行号栏：一个贴在编辑器左侧的小部件，只负责把绘制工作转交给 CodeEditor
 */
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *codeEditor_;
};

#endif // CODEEDITOR_H
