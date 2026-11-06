#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"
#include "codeeditor.h"
#include "finddialog.h"
#include "replacedialog.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSaveFile>
#include <QStatusBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>

namespace {
const int kAutoSaveIntervalMs = 30 * 1000;      // 定时自动保存间隔：30 秒
const qint64 kMaxOpenFileSize = 10 * 1024 * 1024;   // 打开文件的大小上限：10 MB

const char *kFileFilter =
        QT_TRANSLATE_NOOP("MainWindow",
                          "文本文件 (*.txt *.log *.md *.cpp *.h *.pro);;所有文件 (*.*)");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , fontSizeBox_(nullptr)
    , positionLabel_(nullptr)
    , infoLabel_(nullptr)
    , stateLabel_(nullptr)
    , autoSaveTimer_(new QTimer(this))
    , findDialog_(nullptr)
    , replaceDialog_(nullptr)
    , forceClose_(false)
{
    ui->setupUi(this);

    // 关闭即销毁，避免多开窗口时内存泄漏
    setAttribute(Qt::WA_DeleteOnClose, true);

    initUi();
    buildToolBarExtras();
    buildStatusBar();
    connectActions();

    autoSaveTimer_->setInterval(kAutoSaveIntervalMs);
    connect(autoSaveTimer_, &QTimer::timeout, this, &MainWindow::autoSave);

    setCurrentFile(QString());
    updateActionStates();
    updateStatusBar();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---------------------------------------------------------------- 初始化

void MainWindow::initUi()
{
    ui->actionNew->setIcon(QIcon(QStringLiteral(":/icons/new.svg")));
    ui->actionOpen->setIcon(QIcon(QStringLiteral(":/icons/open.svg")));
    ui->actionSave->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));
    ui->actionSaveAs->setIcon(QIcon(QStringLiteral(":/icons/save.svg")));
    ui->actionUndo->setIcon(QIcon(QStringLiteral(":/icons/undo.svg")));
    ui->actionRedo->setIcon(QIcon(QStringLiteral(":/icons/redo.svg")));
    ui->actionCut->setIcon(QIcon(QStringLiteral(":/icons/cut.svg")));
    ui->actionCopy->setIcon(QIcon(QStringLiteral(":/icons/copy.svg")));
    ui->actionPaste->setIcon(QIcon(QStringLiteral(":/icons/paste.svg")));
    ui->actionFind->setIcon(QIcon(QStringLiteral(":/icons/find.svg")));
    ui->actionReplace->setIcon(QIcon(QStringLiteral(":/icons/replace.svg")));
    ui->actionTextColor->setIcon(QIcon(QStringLiteral(":/icons/textcolor.svg")));
    ui->actionBackgroundColor->setIcon(QIcon(QStringLiteral(":/icons/bgcolor.svg")));
    ui->actionWordWrap->setIcon(QIcon(QStringLiteral(":/icons/wrap.svg")));
    ui->actionShowLineNumbers->setIcon(QIcon(QStringLiteral(":/icons/linenum.svg")));
    ui->actionAutoSave->setIcon(QIcon(QStringLiteral(":/icons/autosave.svg")));
    ui->actionAbout->setIcon(QIcon(QStringLiteral(":/icons/about.svg")));

    // 按 .ui 中“自动换行 / 显示行号”的初始勾选状态设置编辑器
    ui->textEdit->setLineWrapMode(ui->actionWordWrap->isChecked()
                                  ? QPlainTextEdit::WidgetWidth
                                  : QPlainTextEdit::NoWrap);
    ui->textEdit->setLineNumbersVisible(ui->actionShowLineNumbers->isChecked());
    ui->textEdit->setFocus();
}

void MainWindow::buildToolBarExtras()
{
    // 工具栏尾部追加“字号”下拉框
    ui->mainToolBar->addSeparator();

    QLabel *tip = new QLabel(tr(" 字号 "), this);
    ui->mainToolBar->addWidget(tip);

    fontSizeBox_ = new QComboBox(this);
    fontSizeBox_->setEditable(true);
    fontSizeBox_->setFixedWidth(72);
    fontSizeBox_->setToolTip(tr("设置字体大小（可输入 1~200 之间的数字）"));
    const int sizes[] = {9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 36, 48};
    for (int s : sizes)
        fontSizeBox_->addItem(QString::number(s));
    fontSizeBox_->setCurrentText(QString::number(ui->textEdit->font().pointSize()));
    ui->mainToolBar->addWidget(fontSizeBox_);

    connect(fontSizeBox_, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        applyFontSize(fontSizeBox_->currentText().toInt());
    });
    if (fontSizeBox_->lineEdit()) {
        connect(fontSizeBox_->lineEdit(), &QLineEdit::editingFinished, this, [this]() {
            applyFontSize(fontSizeBox_->currentText().toInt());
        });
    }
}

void MainWindow::buildStatusBar()
{
    positionLabel_ = new QLabel(this);
    infoLabel_ = new QLabel(this);
    stateLabel_ = new QLabel(this);

    positionLabel_->setMinimumWidth(110);
    infoLabel_->setMinimumWidth(110);
    stateLabel_->setMinimumWidth(80);

    ui->statusbar->addPermanentWidget(positionLabel_);
    ui->statusbar->addPermanentWidget(infoLabel_);
    ui->statusbar->addPermanentWidget(stateLabel_);

    ui->statusbar->showMessage(tr("就绪"));
}

void MainWindow::connectActions()
{
    // ---- 文件 ----
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFileDialog);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveAs);
    connect(ui->actionAutoSave, &QAction::toggled, this, &MainWindow::toggleAutoSave);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    // ---- 编辑 ----
    connect(ui->actionUndo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->textEdit, &QPlainTextEdit::redo);
    connect(ui->actionCut, &QAction::triggered, this, &MainWindow::cut);
    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::copy);
    connect(ui->actionPaste, &QAction::triggered, this, &MainWindow::paste);
    connect(ui->actionDelete, &QAction::triggered, this, &MainWindow::deleteSelection);
    connect(ui->actionSelectAll, &QAction::triggered, ui->textEdit, &QPlainTextEdit::selectAll);

    // ---- 格式 ----
    connect(ui->actionTextColor, &QAction::triggered, this, &MainWindow::chooseTextColor);
    connect(ui->actionBackgroundColor, &QAction::triggered, this,
            &MainWindow::chooseBackgroundColor);
    connect(ui->actionWordWrap, &QAction::toggled, this, &MainWindow::toggleWordWrap);
    connect(ui->actionShowLineNumbers, &QAction::toggled, this,
            &MainWindow::toggleLineNumbers);

    // ---- 查看 ----
    connect(ui->actionShowToolBar, &QAction::toggled, this, &MainWindow::toggleToolBar);
    connect(ui->actionShowStatusBar, &QAction::toggled, this, &MainWindow::toggleStatusBar);
    // 用户右键隐藏工具栏时，菜单项的勾选状态同步更新
    connect(ui->mainToolBar, &QToolBar::visibilityChanged,
            ui->actionShowToolBar, &QAction::setChecked);

    // ---- 查找 / 替换 / 关于 ----
    connect(ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(ui->actionReplace, &QAction::triggered, this, &MainWindow::showReplaceDialog);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    // ---- 文档状态 ----
    QTextDocument *doc = ui->textEdit->document();
    connect(doc, &QTextDocument::modificationChanged, this, &MainWindow::documentWasModified);
    connect(doc, &QTextDocument::undoAvailable, this, &MainWindow::updateActionStates);
    connect(doc, &QTextDocument::redoAvailable, this, &MainWindow::updateActionStates);
    connect(doc, &QTextDocument::modificationChanged, this, &MainWindow::updateActionStates);
    connect(ui->textEdit, &QPlainTextEdit::copyAvailable, this, &MainWindow::updateActionStates);
    connect(ui->textEdit, &QPlainTextEdit::cursorPositionChanged, this,
            &MainWindow::updateStatusBar);
    connect(ui->textEdit, &QPlainTextEdit::textChanged, this, &MainWindow::updateActionStates);
}

// ---------------------------------------------------------------- 文件操作

void MainWindow::newFile()
{
    // 多窗口：新建时开一个新窗口，互不干扰
    MainWindow *other = new MainWindow;
    other->resize(size());
    other->move(pos() + QPoint(28, 28));
    other->show();
    other->ui->textEdit->setFocus();
}

void MainWindow::openFileDialog()
{
    const QString fileName = QFileDialog::getOpenFileName(
                this, tr("打开文件"), QDir::homePath(), tr(kFileFilter));
    if (fileName.isEmpty())
        return;

    // 当前窗口是“空白且未修改”的，就直接用它；否则另开一个窗口，避免丢失内容
    const bool reusable = curFile_.isEmpty() && !ui->textEdit->document()->isModified()
            && ui->textEdit->toPlainText().isEmpty();

    if (reusable) {
        openFile(fileName);
    } else {
        MainWindow *other = new MainWindow;
        other->show();
        other->openFile(fileName);
    }
}

bool MainWindow::openFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    if (!maybeSave())
        return false;
    return loadFromFile(fileName);
}

bool MainWindow::loadFromFile(const QString &fileName)
{
    const QFileInfo info(fileName);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("文件不存在：\n%1").arg(QDir::toNativeSeparators(fileName)));
        return false;
    }

    if (info.size() > kMaxOpenFileSize) {
        const auto ret = QMessageBox::question(
                    this, tr("打开文件"),
                    tr("该文件大小为 %1 MB，超过 10 MB。\n继续打开可能需要较长时间，是否继续？")
                    .arg(QString::number(info.size() / 1024.0 / 1024.0, 'f', 1)),
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
            return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("无法读取文件“%1”：\n%2")
                             .arg(info.fileName(), file.errorString()));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();

    // 二进制文件判断：排除 UTF-16 带 BOM 的情况后，纯文本里不应出现 '\0'
    const bool utf16 = data.startsWith("\xFF\xFE") || data.startsWith("\xFE\xFF");
    if (!utf16 && data.contains('\0')) {
        QMessageBox::warning(this, tr("打开文件"),
                             tr("“%1”似乎不是文本文件（包含二进制数据），已取消打开。")
                             .arg(info.fileName()));
        return false;
    }

    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);
    QTextStream in(&buffer);
    in.setCodec("UTF-8");
    in.setAutoDetectUnicode(true);          // 自动识别 UTF-8 / UTF-16 的 BOM
    QString text = in.readAll();
    if (text.startsWith(QChar(0xFEFF)))     // 去掉 UTF-8 BOM 残留的零宽字符
        text.remove(0, 1);

    ui->textEdit->setPlainText(text);
    setCurrentFile(fileName);
    ui->textEdit->document()->setModified(false);
    ui->textEdit->moveCursor(QTextCursor::Start);
    showMessage(tr("已打开 %1").arg(info.fileName()));
    return true;
}

bool MainWindow::save()
{
    if (curFile_.isEmpty())
        return saveAs();
    return saveToFile(curFile_);
}

bool MainWindow::saveAs()
{
    QFileDialog dialog(this, tr("另存为"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter(tr(kFileFilter));
    dialog.setDirectory(curFile_.isEmpty() ? QDir::homePath() : QFileInfo(curFile_).absolutePath());
    dialog.selectFile(curFile_.isEmpty() ? QStringLiteral("无标题.txt") : displayName());
    dialog.setDefaultSuffix(QStringLiteral("txt"));
    if (dialog.exec() != QDialog::Accepted)
        return false;

    const QString fileName = dialog.selectedFiles().value(0);
    if (fileName.isEmpty())
        return false;

    return saveToFile(fileName);
}

bool MainWindow::saveToFile(const QString &fileName)
{
    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存文件"),
                             tr("无法写入文件“%1”：\n%2")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << ui->textEdit->toPlainText();
    out.flush();

    if (!file.commit()) {
        QMessageBox::warning(this, tr("保存文件"),
                             tr("保存失败：\n%1").arg(file.errorString()));
        return false;
    }

    setCurrentFile(fileName);
    showMessage(tr("已保存到 %1").arg(QFileInfo(fileName).fileName()));
    return true;
}

bool MainWindow::maybeSave()
{
    if (!ui->textEdit->document()->isModified())
        return true;

    const auto ret = QMessageBox::warning(
                this, tr("简单文本编辑器"),
                tr("文档“%1”已被修改。\n是否保存修改？").arg(displayName()),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);

    if (ret == QMessageBox::Save)
        return save();
    if (ret == QMessageBox::Cancel)
        return false;
    return true;    // Discard
}

void MainWindow::toggleAutoSave(bool enabled)
{
    if (enabled) {
        autoSaveTimer_->start();
        showMessage(tr("已开启自动保存：每 30 秒保存一次"));
    } else {
        autoSaveTimer_->stop();
        showMessage(tr("已关闭自动保存"));
    }
}

void MainWindow::autoSave()
{
    // 只有“已命名文件”且“内容有改动”时才真正写盘
    if (curFile_.isEmpty() || !ui->textEdit->document()->isModified())
        return;
    if (saveToFile(curFile_))
        showMessage(tr("已自动保存 %1").arg(QFileInfo(curFile_).fileName()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (forceClose_ || maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    curFile_ = fileName;
    ui->textEdit->document()->setModified(false);
    refreshTitle();
    updateActionStates();
    updateStatusBar();
}

void MainWindow::refreshTitle()
{
    setWindowTitle(tr("%1[*] - 简单文本编辑器").arg(displayName()));
    setWindowModified(ui->textEdit->document()->isModified());
}

QString MainWindow::displayName() const
{
    if (curFile_.isEmpty())
        return tr("无标题.txt");
    return QFileInfo(curFile_).fileName();
}

void MainWindow::documentWasModified()
{
    refreshTitle();
    updateActionStates();
    updateStatusBar();
}

// ---------------------------------------------------------------- 编辑操作

void MainWindow::cut()
{
    if (!ui->textEdit->textCursor().hasSelection())
        return;
    ui->textEdit->cut();
    updateActionStates();
}

void MainWindow::copy()
{
    if (!ui->textEdit->textCursor().hasSelection())
        return;
    ui->textEdit->copy();
    updateActionStates();
}

void MainWindow::paste()
{
    if (!ui->textEdit->canPaste())
        return;
    ui->textEdit->paste();
    updateActionStates();
}

void MainWindow::deleteSelection()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    if (!cursor.hasSelection())
        return;
    cursor.removeSelectedText();
    updateActionStates();
}

// ---------------------------------------------------------------- 格式设置

void MainWindow::applyFontSize(int pointSize)
{
    if (pointSize < 1 || pointSize > 200) {
        showMessage(tr("字号必须在 1~200 之间"), 4000);
        syncFontSizeBox();
        return;
    }

    QTextCharFormat format;
    format.setFontPointSize(pointSize);

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection()) {
        cursor.mergeCharFormat(format);      // 只影响选中文字
    } else {
        ui->textEdit->mergeCurrentCharFormat(format);   // 影响当前光标处（后续输入）
    }

    ui->textEdit->setFocus();
}

void MainWindow::chooseTextColor()
{
    const QColor currentColor = ui->textEdit->currentCharFormat().foreground().color();
    const QColor color = QColorDialog::getColor(currentColor.isValid() ? currentColor : Qt::black,
                                                this, tr("选择字体颜色"));
    if (!color.isValid())
        return;

    QTextCharFormat format;
    format.setForeground(color);

    QTextCursor cursor = ui->textEdit->textCursor();
    if (cursor.hasSelection())
        cursor.mergeCharFormat(format);
    else
        ui->textEdit->mergeCurrentCharFormat(format);

    ui->textEdit->setFocus();
}

void MainWindow::chooseBackgroundColor()
{
    QPalette palette = ui->textEdit->palette();
    const QColor current = palette.color(QPalette::Base);
    const QColor color = QColorDialog::getColor(current, this, tr("选择编辑器背景颜色"));
    if (!color.isValid())
        return;

    palette.setColor(QPalette::Base, color);
    // 文字颜色与背景反差过小时给出提醒
    const int gray = qGray(color.rgb());
    palette.setColor(QPalette::Text, gray < 128 ? Qt::white : Qt::black);
    ui->textEdit->setPalette(palette);
    ui->textEdit->setStyleSheet(QStringLiteral("QPlainTextEdit { background-color: %1; }")
                                .arg(color.name()));
    ui->textEdit->setFocus();
}

void MainWindow::toggleWordWrap(bool enabled)
{
    ui->textEdit->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth
                                          : QPlainTextEdit::NoWrap);
    showMessage(enabled ? tr("已开启自动换行") : tr("已关闭自动换行"));
}

void MainWindow::toggleLineNumbers(bool enabled)
{
    if (auto *editor = qobject_cast<CodeEditor *>(ui->textEdit))
        editor->setLineNumbersVisible(enabled);
    showMessage(enabled ? tr("已显示行号") : tr("已隐藏行号"));
}

// ---------------------------------------------------------------- 视图

void MainWindow::toggleToolBar(bool visible)
{
    ui->mainToolBar->setVisible(visible);
}

void MainWindow::toggleStatusBar(bool visible)
{
    ui->statusbar->setVisible(visible);
}

// ---------------------------------------------------------------- 状态维护

void MainWindow::updateStatusBar()
{
    const QTextCursor cursor = ui->textEdit->textCursor();
    positionLabel_->setText(tr("行 %1，列 %2")
                            .arg(cursor.blockNumber() + 1)
                            .arg(cursor.columnNumber() + 1));
    infoLabel_->setText(tr("字符数：%1").arg(ui->textEdit->toPlainText().size()));

    const bool modified = ui->textEdit->document()->isModified();
    stateLabel_->setText(modified ? tr("● 未保存") : tr("已保存"));
}

void MainWindow::updateActionStates()
{
    QTextDocument *doc = ui->textEdit->document();
    const bool hasSelection = ui->textEdit->textCursor().hasSelection();

    ui->actionUndo->setEnabled(doc->isUndoAvailable());
    ui->actionRedo->setEnabled(doc->isRedoAvailable());
    ui->actionCut->setEnabled(hasSelection);
    ui->actionCopy->setEnabled(hasSelection);
    ui->actionDelete->setEnabled(hasSelection);
    ui->actionPaste->setEnabled(ui->textEdit->canPaste());
    ui->actionSave->setEnabled(doc->isModified() || curFile_.isEmpty());
}

void MainWindow::syncFontSizeBox()
{
    if (fontSizeBox_)
        fontSizeBox_->setCurrentText(QString::number(ui->textEdit->font().pointSize()));
}

void MainWindow::showMessage(const QString &text, int timeout)
{
    ui->statusbar->showMessage(text, timeout);
}

// ---------------------------------------------------------------- 对话框

void MainWindow::showFindDialog()
{
    if (!findDialog_)
        findDialog_ = new FindDialog(ui->textEdit, this);

    findDialog_->show();
    findDialog_->raise();
    findDialog_->activateWindow();
}

void MainWindow::showReplaceDialog()
{
    if (!replaceDialog_)
        replaceDialog_ = new ReplaceDialog(ui->textEdit, this);

    replaceDialog_->show();
    replaceDialog_->raise();
    replaceDialog_->activateWindow();
}

void MainWindow::showAboutDialog()
{
    AboutDialog dialog(this);
    dialog.exec();
}
