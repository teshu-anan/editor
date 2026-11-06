#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QComboBox;
class QLabel;
class QTimer;
class FindDialog;
class ReplaceDialog;

namespace Ui {
class MainWindow;
}

/**
 * @brief 主窗口：一个窗口对应一个正在编辑的文档
 *
 * 通过“新建/打开”创建新的 MainWindow 实例，从而实现多窗口编辑。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// 载入指定文件（内部会做存在性/可读性/二进制判断）
    bool openFile(const QString &fileName);

    /// 若有未保存修改则弹出提示，返回 true 表示可以继续（保存成功或放弃修改）
    bool maybeSave();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // ---- 文件 ----
    void newFile();
    void openFileDialog();
    bool save();
    bool saveAs();
    void toggleAutoSave(bool enabled);
    void autoSave();

    // ---- 编辑 ----
    void cut();
    void copy();
    void paste();
    void deleteSelection();

    // ---- 格式 ----
    void applyFontSize(int pointSize);
    void chooseTextColor();
    void chooseBackgroundColor();
    void toggleWordWrap(bool enabled);
    void toggleLineNumbers(bool enabled);

    // ---- 查看 ----
    void toggleToolBar(bool visible);
    void toggleStatusBar(bool visible);

    // ---- 查找 / 替换 / 关于 ----
    void showFindDialog();
    void showReplaceDialog();
    void showAboutDialog();

    // ---- 状态维护 ----
    void documentWasModified();
    void updateStatusBar();
    void updateActionStates();

private:
    void initUi();
    void buildToolBarExtras();
    void buildStatusBar();
    void connectActions();

    bool loadFromFile(const QString &fileName);
    bool saveToFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    void refreshTitle();
    QString displayName() const;
    void syncFontSizeBox();
    void showMessage(const QString &text, int timeout = 3000);

    Ui::MainWindow *ui;
    QString curFile_;            // 当前文件的完整路径，空表示“未命名”
    QComboBox *fontSizeBox_;
    QLabel *positionLabel_;      // 行/列
    QLabel *infoLabel_;          // 字符数
    QLabel *stateLabel_;         // 保存状态
    QTimer *autoSaveTimer_;
    FindDialog *findDialog_;
    ReplaceDialog *replaceDialog_;
    bool forceClose_;
};

#endif // MAINWINDOW_H
