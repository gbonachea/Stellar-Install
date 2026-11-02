#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DebPackageInfo.h"
#include "InstalledPackageChecker.h"

#include <QMainWindow>
#include <QPalette>

class QActionGroup;
class QCloseEvent;
class QDragEnterEvent;
class QDropEvent;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTextBrowser;
class QToolButton;

class DebParser;
class PackageManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openDebFile(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void chooseFile();
    void onParsed(const DebPackageInfo &info);
    void onParseFailed(const QString &message);
    void installPackage();
    void onInstallStarted(const QString &commandLine);
    void onInstallOutput(const QString &text);
    void onInstallFinished(bool success, int exitCode, const QString &message);
    void onInstalledStatusChecked(const InstalledPackageStatus &status);

private:
    void buildUi();
    void setupSettingsMenu();
    void applyTheme();
    void applyPalette();
    void setAppearance(const QString &mode);
    void refreshThemedContent();
    void showAboutDialog();
    void resetPackage();
    void setInstalling(bool installing);
    void showPlaceholder(const QString &message);
    void showMetadata(const DebPackageInfo &info);
    void showDependencies(const DebPackageInfo &info);
    QString buildMetadataHtml(const DebPackageInfo &info) const;
    QString installedSizeText(const QString &kib) const;
    void appendLog(const QString &text);

    void startInstalledCheck();
    void updateStatusDisplay();
    void updateInstallAction();
    QString installedStateText() const;
    QString installButtonText() const;

    DebParser *m_parser = nullptr;
    PackageManager *m_manager = nullptr;
    InstalledPackageChecker *m_checker = nullptr;

    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_openButton = nullptr;
    QToolButton *m_settingsButton = nullptr;
    QActionGroup *m_appearanceGroup = nullptr;
    QTextBrowser *m_metadataView = nullptr;
    QGroupBox *m_dependenciesGroup = nullptr;
    QTableWidget *m_dependenciesTable = nullptr;
    QLabel *m_dependenciesEmpty = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_installButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QToolButton *m_detailsToggle = nullptr;
    QPlainTextEdit *m_logView = nullptr;

    DebPackageInfo m_currentInfo;
    InstalledPackageStatus m_installedStatus;
    QPalette m_systemPalette;
    QString m_appearance;
    QString m_placeholderMessage;
    QString m_mutedColor;
    QString m_accentColor;
    QString m_successColor;
    QString m_warningColor;
    QString m_errorColor;
    bool m_installing = false;
    bool m_checkPending = false;
};

#endif // MAINWINDOW_H
