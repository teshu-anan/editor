#include "codeeditor.h"

#include <QFontMetrics>
#include <QPainter>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

// ---------------------------------------------------------------- CodeEditor

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , lineNumberArea_(new LineNumberArea(this))
    , showLineNumbers_(true)
{
    // 行数变化 -> 重新计算行号栏宽度；滚动/更新 -> 重绘行号
    connect(this, &CodeEditor::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    // 默认字体与缩进：一个 Tab 等于 4 个空格宽
    QFont f = font();
    f.setPointSize(12);
    setFont(f);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    setCursorWidth(2);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!showLineNumbers_)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // 左右各留 6 像素的空白，中间是数字宽度
    const int space = 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    if (showLineNumbers_ == visible)
        return;

    showLineNumbers_ = visible;
    lineNumberArea_->setVisible(visible);
    updateLineNumberAreaWidth(0);
    lineNumberArea_->update();
    update();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(),
                                       lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    // 把编辑区域往右挤，给行号栏腾出空间
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0)
        lineNumberArea_->scroll(0, dy);
    else
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    if (!showLineNumbers_)
        return;

    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor(0xF3, 0xF5, 0xF8));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    painter.setPen(QColor(0x93, 0x9E, 0xAA));
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top,
                             lineNumberArea_->width() - 6,
                             fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    highlightCurrentLine();
}

void CodeEditor::focusOutEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusOutEvent(event);
    highlightCurrentLine();
}

void CodeEditor::highlightCurrentLine()
{
    // 只在有焦点时高亮当前行，避免多窗口时“花屏”
    if (!hasFocus()) {
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
        return;
    }

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(0xE8, 0xF1, 0xFE));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();

    setExtraSelections(QList<QTextEdit::ExtraSelection>() << selection);
}

// ------------------------------------------------------------ LineNumberArea

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor)
    , codeEditor_(editor)
{
    setCursor(Qt::ArrowCursor);
}

QSize LineNumberArea::sizeHint() const
{
    return QSize(codeEditor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    codeEditor_->lineNumberAreaPaintEvent(event);
}
