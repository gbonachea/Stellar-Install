#include "MainWindow.h"

#include "DebParser.h"
#include "PackageManager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QSize>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

namespace {

QPalette makeLightPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#f4f6fa")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#1f2430")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f3f5f8")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#1f2430")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#1f2430")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#eef1f6")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#1f2430")));
    palette.setColor(QPalette::BrightText, QColor(Qt::white));
    palette.setColor(QPalette::Link, QColor(QStringLiteral("#2f6fed")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#2f6fed")));
    palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#7b8494")));
#endif

    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor(QStringLiteral("#9aa1ae")));
    palette.setColor(QPalette::Disabled, QPalette::Text,
                     QColor(QStringLiteral("#9aa1ae")));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor(QStringLiteral("#9aa1ae")));
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
                     QColor(QStringLiteral("#d9dee7")));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(QStringLiteral("#667085")));
    return palette;
}

QPalette makeDarkPalette()
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(QStringLiteral("#1b1f27")));
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#e7eaf0")));
    palette.setColor(QPalette::Base, QColor(QStringLiteral("#151920")));
    palette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#20252f")));
    palette.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#2a303c")));
    palette.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#e7eaf0")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#e7eaf0")));
    palette.setColor(QPalette::Button, QColor(QStringLiteral("#232833")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#e7eaf0")));
    palette.setColor(QPalette::BrightText, QColor(Qt::white));
    palette.setColor(QPalette::Link, QColor(QStringLiteral("#77a8ff")));
    palette.setColor(QPalette::Highlight, QColor(QStringLiteral("#3b7bf5")));
    palette.setColor(QPalette::HighlightedText, QColor(Qt::white));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    palette.setColor(QPalette::PlaceholderText, QColor(QStringLiteral("#8b93a1")));
#endif

    palette.setColor(QPalette::Disabled, QPalette::WindowText,
                     QColor(QStringLiteral("#6c7482")));
    palette.setColor(QPalette::Disabled, QPalette::Text,
                     QColor(QStringLiteral("#6c7482")));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     QColor(QStringLiteral("#6c7482")));
    palette.setColor(QPalette::Disabled, QPalette::Highlight,
                     QColor(QStringLiteral("#3a4150")));
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     QColor(QStringLiteral("#9aa1ae")));
    return palette;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_parser(new DebParser(this))
    , m_manager(new PackageManager(this))
    , m_checker(new InstalledPackageChecker(this))
{
    setWindowTitle(tr("Stellar Install"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/stellar-install.png")));
    setMinimumSize(860, 580);
    resize(1040, 700);
    setAcceptDrops(true);

    static const QPalette initialSystemPalette = QApplication::palette();
    m_systemPalette = initialSystemPalette;

    QSettings settings;
    m_appearance = settings.value(QStringLiteral("appearance"),
                                  QStringLiteral("system"))
                       .toString();
    if (m_appearance != QLatin1String("light") && m_appearance != QLatin1String("dark"))
        m_appearance = QStringLiteral("system");

    applyPalette();

    buildUi();
    applyTheme();
    setupSettingsMenu();

    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(m_installButton, &QPushButton::clicked, this, &MainWindow::installPackage);

    connect(m_detailsToggle, &QToolButton::toggled, this, [this](bool checked) {
        m_logView->setVisible(checked);
        m_detailsToggle->setArrowType(checked ? Qt::DownArrow : Qt::RightArrow);
        m_detailsToggle->setText(checked ? tr("Ocultar detalles") : tr("Mostrar detalles"));
    });

    connect(m_parser, &DebParser::started, this, [this](const QString &path) {
        statusBar()->showMessage(
            tr("Leyendo metadatos de %1…").arg(QFileInfo(path).fileName()));
    });
    connect(m_parser, &DebParser::finished, this, &MainWindow::onParsed);
    connect(m_parser, &DebParser::failed, this, &MainWindow::onParseFailed);

    connect(m_manager, &PackageManager::started, this, &MainWindow::onInstallStarted);
    connect(m_manager, &PackageManager::outputReceived, this, &MainWindow::onInstallOutput);
    connect(m_manager, &PackageManager::finished, this, &MainWindow::onInstallFinished);

    connect(m_checker, &InstalledPackageChecker::finished,
            this, &MainWindow::onInstalledStatusChecked);

    resetPackage();
    showPlaceholder(tr("Selecciona un paquete .deb para comenzar"));
    statusBar()->showMessage(tr("Listo. Abre un archivo .deb para comenzar."));
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("centralWidget"));

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(20, 18, 20, 14);
    root->setSpacing(14);

    // Encabezado
    auto *header = new QHBoxLayout;
    header->setSpacing(12);

    auto *iconLabel = new QLabel(central);
    iconLabel->setPixmap(QIcon(QStringLiteral(":/icons/stellar-install.png")).pixmap(46, 46));
    iconLabel->setFixedSize(46, 46);
    header->addWidget(iconLabel, 0, Qt::AlignTop);

    auto *titleLabel = new QLabel(tr("Stellar Install"), central);
    titleLabel->setObjectName(QStringLiteral("appTitle"));
    auto *subtitleLabel = new QLabel(
        tr("Inspecciona metadatos e instala paquetes Debian de forma segura"), central);
    subtitleLabel->setObjectName(QStringLiteral("appSubtitle"));
    subtitleLabel->setWordWrap(true);

    auto *titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    header->addLayout(titleLayout, 1);

    const QIcon settingsIcon(QStringLiteral(":/icons/configuracion.png"));

    m_settingsButton = new QToolButton(central);
    m_settingsButton->setObjectName(QStringLiteral("settingsButton"));
    m_settingsButton->setIcon(settingsIcon);
    m_settingsButton->setIconSize(QSize(20, 20));
    m_settingsButton->setFixedSize(34, 34);
    m_settingsButton->setToolTip(tr("Configuración y apariencia"));
    m_settingsButton->setAccessibleName(tr("Configuración"));
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    m_settingsButton->setPopupMode(QToolButton::InstantPopup);

    header->addWidget(m_settingsButton, 0, Qt::AlignTop);
    root->addLayout(header);

    // Selector de archivo
    auto *fileRow = new QHBoxLayout;
    fileRow->setSpacing(10);

    m_pathEdit = new QLineEdit(central);
    m_pathEdit->setObjectName(QStringLiteral("pathEdit"));
    m_pathEdit->setReadOnly(true);
    m_pathEdit->setPlaceholderText(tr("Ningún paquete seleccionado"));
    m_pathEdit->setAccessibleName(tr("Ruta del paquete seleccionado"));
    m_pathEdit->setMinimumHeight(36);
    m_pathEdit->setCursorPosition(0);

    m_openButton = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")),
                                   tr("Abrir archivo…"), central);
    m_openButton->setMinimumHeight(36);
    m_openButton->setToolTip(tr("Selecciona un paquete .deb (Ctrl+O)"));
    m_openButton->setShortcut(QKeySequence::Open);

    fileRow->addWidget(m_pathEdit, 1);
    fileRow->addWidget(m_openButton);
    root->addLayout(fileRow);

    // Cuerpo: metadatos a la izquierda, dependencias y acciones a la derecha
    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(12);

    auto *infoGroup = new QGroupBox(tr("Información del paquete"), splitter);
    auto *infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setContentsMargins(10, 14, 10, 10);

    m_metadataView = new QTextBrowser(infoGroup);
    m_metadataView->setObjectName(QStringLiteral("metadataView"));
    m_metadataView->setAccessibleName(tr("Información del paquete"));
    m_metadataView->setFrameShape(QFrame::NoFrame);
    m_metadataView->setOpenExternalLinks(false);
    m_metadataView->setOpenLinks(false);

    QTextOption textOption = m_metadataView->document()->defaultTextOption();
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_metadataView->document()->setDefaultTextOption(textOption);

    infoLayout->addWidget(m_metadataView);

    auto *rightPanel = new QWidget(splitter);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    m_statusLabel = new QLabel(rightPanel);
    m_statusLabel->setObjectName(QStringLiteral("packageStatus"));
    m_statusLabel->setAccessibleName(tr("Estado del paquete en el sistema"));
    m_statusLabel->setTextFormat(Qt::RichText);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setOpenExternalLinks(false);
    m_statusLabel->hide();
    rightLayout->addWidget(m_statusLabel);

    m_dependenciesGroup = new QGroupBox(tr("Dependencias"), rightPanel);
    auto *dependenciesLayout = new QVBoxLayout(m_dependenciesGroup);
    dependenciesLayout->setContentsMargins(10, 14, 10, 10);
    dependenciesLayout->setSpacing(6);

    m_dependenciesTable = new QTableWidget(0, 2, m_dependenciesGroup);
    m_dependenciesTable->setAccessibleName(tr("Dependencias del paquete"));
    m_dependenciesTable->setHorizontalHeaderLabels({tr("Paquete"), tr("Restricción")});
    m_dependenciesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_dependenciesTable->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    m_dependenciesTable->horizontalHeader()->setHighlightSections(false);
    m_dependenciesTable->verticalHeader()->hide();
    m_dependenciesTable->setShowGrid(false);
    m_dependenciesTable->setAlternatingRowColors(true);
    m_dependenciesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dependenciesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_dependenciesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dependenciesTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_dependenciesTable->setWordWrap(false);
    m_dependenciesTable->setCornerButtonEnabled(false);
    m_dependenciesTable->setMinimumHeight(160);

    m_dependenciesEmpty = new QLabel(tr("Abre un archivo .deb para consultar sus dependencias."),
                                     m_dependenciesGroup);
    m_dependenciesEmpty->setObjectName(QStringLiteral("emptyHint"));
    m_dependenciesEmpty->setAlignment(Qt::AlignCenter);
    m_dependenciesEmpty->setWordWrap(true);

    dependenciesLayout->addWidget(m_dependenciesTable);
    dependenciesLayout->addWidget(m_dependenciesEmpty);
    rightLayout->addWidget(m_dependenciesGroup, 2);

    auto *actionRow = new QHBoxLayout;
    actionRow->setSpacing(12);

    auto *authHint = new QLabel(
        tr("La instalación se realizará con apt y pedirá la contraseña de administrador."),
        rightPanel);
    authHint->setObjectName(QStringLiteral("hintLabel"));
    authHint->setWordWrap(true);

    m_installButton = new QPushButton(tr("Instalar paquete"), rightPanel);
    m_installButton->setObjectName(QStringLiteral("installButton"));
    m_installButton->setAccessibleName(tr("Instalar paquete"));
    m_installButton->setMinimumHeight(42);
    m_installButton->setMinimumWidth(170);
    m_installButton->setEnabled(false);
    m_installButton->setToolTip(tr("Instala el paquete y resuelve sus dependencias con apt"));

    actionRow->addWidget(authHint, 1);
    actionRow->addWidget(m_installButton, 0, Qt::AlignVCenter);
    rightLayout->addLayout(actionRow);

    m_progressBar = new QProgressBar(rightPanel);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedHeight(18);
    m_progressBar->hide();
    rightLayout->addWidget(m_progressBar);

    auto *detailsRow = new QHBoxLayout;
    detailsRow->addStretch(1);

    m_detailsToggle = new QToolButton(rightPanel);
    m_detailsToggle->setObjectName(QStringLiteral("detailsToggle"));
    m_detailsToggle->setCheckable(true);
    m_detailsToggle->setChecked(false);
    m_detailsToggle->setArrowType(Qt::RightArrow);
    m_detailsToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_detailsToggle->setText(tr("Mostrar detalles"));
    m_detailsToggle->setCursor(Qt::PointingHandCursor);
    detailsRow->addWidget(m_detailsToggle);

    rightLayout->addLayout(detailsRow);

    m_logView = new QPlainTextEdit(rightPanel);
    m_logView->setObjectName(QStringLiteral("logView"));
    m_logView->setAccessibleName(tr("Registro de instalación"));
    m_logView->setReadOnly(true);
    m_logView->setUndoRedoEnabled(false);
    m_logView->setMaximumBlockCount(5000);
    m_logView->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_logView->setMinimumHeight(150);
    m_logView->hide();
    rightLayout->addWidget(m_logView, 1);

    splitter->addWidget(infoGroup);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 5);
    splitter->setStretchFactor(1, 4);
    splitter->setSizes({540, 460});

    root->addWidget(splitter, 1);

    setCentralWidget(central);
    statusBar()->setSizeGripEnabled(true);
}

void MainWindow::setupSettingsMenu()
{
    auto *menu = new QMenu(this);
    auto *appearanceMenu = menu->addMenu(tr("Apariencia"));

    m_appearanceGroup = new QActionGroup(this);
    m_appearanceGroup->setExclusive(true);

    const struct {
        const char *mode;
        QString label;
    } appearanceOptions[] = {
        {"system", tr("Seguir el tema del sistema")},
        {"light", tr("Claro")},
        {"dark", tr("Oscuro")},
    };

    for (const auto &option : appearanceOptions) {
        QAction *action = appearanceMenu->addAction(option.label);
        action->setCheckable(true);
        action->setData(QString::fromLatin1(option.mode));
        action->setChecked(m_appearance == QLatin1String(option.mode));
        m_appearanceGroup->addAction(action);
    }

    connect(m_appearanceGroup, &QActionGroup::triggered, this,
            [this](QAction *action) {
                if (action)
                    setAppearance(action->data().toString());
            });

    menu->addSeparator();
    QAction *aboutAction = menu->addAction(tr("Acerca de Stellar Install"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    m_settingsButton->setMenu(menu);
    m_settingsButton->setToolTip(tr("Configuración: apariencia y Acerca de"));
}

void MainWindow::applyPalette()
{
    if (m_appearance == QLatin1String("dark"))
        QApplication::setPalette(makeDarkPalette());
    else if (m_appearance == QLatin1String("light"))
        QApplication::setPalette(makeLightPalette());
    else
        QApplication::setPalette(m_systemPalette);
}

void MainWindow::setAppearance(const QString &mode)
{
    QString appearance = mode;
    if (appearance != QLatin1String("light") && appearance != QLatin1String("dark"))
        appearance = QStringLiteral("system");

    if (appearance == m_appearance)
        return;

    m_appearance = appearance;
    applyPalette();
    applyTheme();
    refreshThemedContent();

    QSettings settings;
    settings.setValue(QStringLiteral("appearance"), m_appearance);

    const QString appearanceName =
        m_appearance == QLatin1String("dark")
            ? tr("Oscuro")
            : m_appearance == QLatin1String("light") ? tr("Claro")
                                                     : tr("Seguir el tema del sistema");
    statusBar()->showMessage(tr("Apariencia: %1").arg(appearanceName), 4000);
}

void MainWindow::refreshThemedContent()
{
    if (m_currentInfo.isValid()) {
        showMetadata(m_currentInfo);
        updateStatusDisplay();
    } else if (!m_placeholderMessage.isEmpty()) {
        showPlaceholder(m_placeholderMessage);
    }
}

void MainWindow::showAboutDialog()
{
    QMessageBox about(this);
    about.setWindowTitle(tr("Acerca de Stellar Install"));
    about.setIconPixmap(windowIcon().pixmap(64, 64));
    about.setTextFormat(Qt::RichText);
    about.setText(
        QStringLiteral("<p style=\"font-size:20px; font-weight:700; margin:0;\">"
                       "Stellar Install</p>"
                       "<p style=\"margin:3px 0 0 0; color:%1;\">%2</p>")
            .arg(m_mutedColor,
                 tr("Versión %1").arg(QStringLiteral(APP_VERSION)).toHtmlEscaped()));
    about.setInformativeText(
        tr("Stellar Install es una aplicación ligera para inspeccionar paquetes "
           "Debian (.deb), comprobar si ya están instalados y comparar sus "
           "versiones, e instalarlos, actualizarlos o reinstalarlos de forma "
           "segura mediante apt y PolicyKit.\n\n"
           "Muestra los metadatos y las dependencias antes de instalar, sin "
           "bloquear la interfaz, y solicita la autenticación de administrador "
           "a través del escritorio.\n\n"
           "Compilada con Qt %1.")
            .arg(QString::fromLatin1(qVersion())));
    about.setStandardButtons(QMessageBox::Ok);
    about.exec();
}

void MainWindow::applyTheme()
{
    const QPalette currentPalette = QApplication::palette();
    const bool dark = currentPalette.color(QPalette::Window).lightness() < 128;

    m_mutedColor = dark ? QStringLiteral("#9aa1ae") : QStringLiteral("#667085");
    m_accentColor = dark ? QStringLiteral("#5b95ff") : QStringLiteral("#2f6fed");
    m_successColor = dark ? QStringLiteral("#6fcf80") : QStringLiteral("#1e7a3c");
    m_warningColor = dark ? QStringLiteral("#ffb74d") : QStringLiteral("#a35f00");
    m_errorColor = dark ? QStringLiteral("#ef6f6f") : QStringLiteral("#b3261e");

    const QString windowColor = currentPalette.color(QPalette::Window).name();
    const QString baseColor = currentPalette.color(QPalette::Base).name();
    const QString textColor = currentPalette.color(QPalette::WindowText).name();
    const QString borderColor = dark ? QStringLiteral("#3a4150") : QStringLiteral("#d9dee7");
    const QString subtleColor = dark ? QStringLiteral("#232833") : QStringLiteral("#f3f5f8");
    const QString accentColor = m_accentColor;
    const QString accentHover = dark ? QStringLiteral("#77a8ff") : QStringLiteral("#3b7bf5");
    const QString accentPressed = dark ? QStringLiteral("#447fe8") : QStringLiteral("#2459c4");

    QString styleSheet = QStringLiteral(R"QSS(
QWidget#centralWidget {
    background-color: @window;
}
QLabel#appTitle {
    color: @text;
    font-size: 22px;
    font-weight: 700;
}
QLabel#appSubtitle {
    color: @muted;
    font-size: 13px;
}
QLabel#hintLabel {
    color: @muted;
    font-size: 12px;
    font-weight: 400;
}
QLabel#emptyHint {
    color: @muted;
    font-size: 13px;
    font-weight: 400;
}
QLabel#packageStatus {
    background-color: @subtle;
    border: 1px solid @border;
    border-radius: 7px;
    padding: 8px 10px;
    font-size: 12px;
    font-weight: 400;
}
QLineEdit#pathEdit {
    background-color: @base;
    color: @text;
    border: 1px solid @border;
    border-radius: 6px;
    padding: 7px 10px;
    selection-background-color: @accent;
    selection-color: #ffffff;
}
QLineEdit#pathEdit:focus {
    border-color: @accent;
}
QPushButton {
    background-color: @subtle;
    color: @text;
    border: 1px solid @border;
    border-radius: 6px;
    padding: 7px 14px;
    font-weight: 500;
}
QPushButton:hover {
    border-color: @accent;
}
QPushButton:pressed {
    background-color: @border;
}
QPushButton:disabled {
    color: @muted;
}
QPushButton#installButton {
    background-color: @accent;
    color: #ffffff;
    border: none;
    border-radius: 7px;
    padding: 10px 22px;
    font-weight: 600;
}
QPushButton#installButton:hover {
    background-color: @accentHover;
}
QPushButton#installButton:pressed {
    background-color: @accentPressed;
}
QPushButton#installButton:disabled {
    background-color: @border;
    color: @muted;
}
QToolButton#settingsButton {
    background: transparent;
    border: none;
    border-radius: 6px;
    padding: 4px;
}
QToolButton#settingsButton:hover {
    background-color: @subtle;
}
QToolButton#settingsButton:pressed {
    background-color: @border;
}
QToolButton#settingsButton::menu-indicator {
    image: none;
}
QMenu {
    background-color: @base;
    color: @text;
    border: 1px solid @border;
    border-radius: 6px;
    padding: 5px;
}
QMenu::item {
    padding: 6px 24px 6px 20px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: @accent;
    color: #ffffff;
}
QMenu::item:disabled {
    color: @muted;
}
QMenu::separator {
    height: 1px;
    background: @border;
    margin: 4px 8px;
}
QGroupBox {
    border: 1px solid @border;
    border-radius: 8px;
    margin-top: 12px;
    padding: 16px 8px 8px 8px;
    font-weight: 700;
    color: @text;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 12px;
    padding: 0 6px;
    background-color: @window;
}
QGroupBox QLabel {
    font-weight: 400;
}
QTextBrowser#metadataView {
    background: transparent;
    border: none;
    font-weight: 400;
}
QTableWidget {
    background-color: @base;
    alternate-background-color: @subtle;
    color: @text;
    border: none;
    border-radius: 4px;
    font-weight: 400;
}
QTableWidget::item {
    padding: 5px 4px;
}
QTableWidget::item:selected {
    background-color: @accent;
    color: #ffffff;
}
QHeaderView::section {
    background-color: @window;
    color: @muted;
    border: none;
    border-bottom: 1px solid @border;
    padding: 6px 4px;
    font-weight: 600;
}
QTableCornerButton::section {
    background-color: @window;
    border: none;
}
QPlainTextEdit#logView {
    background-color: #14161b;
    color: #dfe3ea;
    border: 1px solid #2b3140;
    border-radius: 6px;
    padding: 8px;
    font-family: "DejaVu Sans Mono", "Noto Sans Mono", monospace;
    font-size: 12px;
    font-weight: 400;
}
QProgressBar {
    background-color: @base;
    color: @text;
    border: 1px solid @border;
    border-radius: 6px;
    text-align: center;
    font-size: 11px;
}
QProgressBar::chunk {
    background-color: @accent;
    border-radius: 5px;
}
QToolButton#detailsToggle {
    border: none;
    background: transparent;
    color: @accent;
    padding: 2px;
    font-weight: 600;
}
QToolButton#detailsToggle:hover {
    color: @accentHover;
}
QStatusBar {
    color: @muted;
}
QStatusBar::item {
    border: none;
}
QSplitter::handle {
    background: transparent;
}
)QSS");

    styleSheet.replace(QStringLiteral("@window"), windowColor);
    styleSheet.replace(QStringLiteral("@base"), baseColor);
    styleSheet.replace(QStringLiteral("@text"), textColor);
    styleSheet.replace(QStringLiteral("@border"), borderColor);
    styleSheet.replace(QStringLiteral("@subtle"), subtleColor);
    styleSheet.replace(QStringLiteral("@muted"), m_mutedColor);
    styleSheet.replace(QStringLiteral("@accentHover"), accentHover);
    styleSheet.replace(QStringLiteral("@accentPressed"), accentPressed);
    styleSheet.replace(QStringLiteral("@accent"), accentColor);

    setStyleSheet(styleSheet);
}

void MainWindow::resetPackage()
{
    m_checker->cancel();
    m_checkPending = false;
    m_installedStatus = InstalledPackageStatus();
    m_currentInfo = DebPackageInfo();

    if (m_statusLabel) {
        m_statusLabel->clear();
        m_statusLabel->hide();
    }

    m_dependenciesTable->clearContents();
    m_dependenciesTable->setRowCount(0);
    m_dependenciesTable->hide();
    m_dependenciesEmpty->setText(tr("Abre un archivo .deb para consultar sus dependencias."));
    m_dependenciesEmpty->show();
    m_dependenciesGroup->setTitle(tr("Dependencias"));

    m_installButton->setEnabled(false);
    m_installButton->setText(tr("Instalar paquete"));

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->hide();

    m_detailsToggle->setChecked(false);
    m_logView->clear();

    setWindowTitle(tr("Stellar Install"));
}

void MainWindow::chooseFile()
{
    QSettings settings;
    QString startDirectory = settings.value(QStringLiteral("lastDirectory")).toString();
    if (startDirectory.isEmpty() || !QDir(startDirectory).exists()) {
        startDirectory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (startDirectory.isEmpty() || !QDir(startDirectory).exists())
            startDirectory = QDir::homePath();
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Seleccionar paquete Debian"),
        startDirectory,
        tr("Paquetes Debian (*.deb);;Todos los archivos (*)"));

    if (!filePath.isEmpty())
        openDebFile(filePath);
}

void MainWindow::openDebFile(const QString &filePath)
{
    if (m_installing) {
        QMessageBox::information(this, tr("Instalación en curso"),
                                 tr("Espera a que termine la instalación antes de abrir "
                                    "otro paquete."));
        return;
    }
    if (m_parser->isRunning())
        return;

    const QFileInfo fileInfo(filePath);
    if (filePath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(this, tr("Archivo no encontrado"),
                             tr("No se encontró el archivo:\n%1").arg(filePath));
        return;
    }
    if (fileInfo.suffix().compare(QStringLiteral("deb"), Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, tr("Formato no admitido"),
                             tr("«%1» no es un archivo .deb.").arg(fileInfo.fileName()));
        return;
    }
    if (!fileInfo.isReadable()) {
        QMessageBox::warning(this, tr("Sin permisos de lectura"),
                             tr("No se puede leer el archivo:\n%1")
                                 .arg(QDir::toNativeSeparators(fileInfo.absoluteFilePath())));
        return;
    }

    resetPackage();

    const QString absolutePath = fileInfo.absoluteFilePath();
    m_pathEdit->setText(QDir::toNativeSeparators(absolutePath));
    m_pathEdit->setCursorPosition(0);
    m_pathEdit->setToolTip(QDir::toNativeSeparators(absolutePath));

    m_openButton->setEnabled(false);
    showPlaceholder(tr("Analizando «%1»…").arg(fileInfo.fileName()));
    statusBar()->showMessage(tr("Leyendo los metadatos del paquete…"));

    QSettings settings;
    settings.setValue(QStringLiteral("lastDirectory"), fileInfo.absolutePath());

    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_parser->parse(absolutePath);
}

void MainWindow::onParsed(const DebPackageInfo &info)
{
    QApplication::restoreOverrideCursor();

    m_currentInfo = info;
    m_openButton->setEnabled(true);

    m_pathEdit->setText(QDir::toNativeSeparators(info.filePath));
    m_pathEdit->setCursorPosition(0);
    m_pathEdit->setToolTip(QDir::toNativeSeparators(info.filePath));

    m_installedStatus = InstalledPackageStatus();
    m_checkPending = true;

    showMetadata(info);
    showDependencies(info);

    setWindowTitle(tr("%1 — Stellar Install").arg(info.package));
    startInstalledCheck();
}

void MainWindow::onParseFailed(const QString &message)
{
    QApplication::restoreOverrideCursor();

    resetPackage();
    m_openButton->setEnabled(true);
    showPlaceholder(tr("No se pudo leer el paquete seleccionado"));
    statusBar()->showMessage(tr("No se pudo leer el paquete."));

    QMessageBox::warning(
        this,
        tr("Paquete no válido"),
        tr("%1\n\nComprueba que el archivo sea un paquete Debian (.deb) válido y que "
           "tengas permisos de lectura.").arg(message));
}

void MainWindow::installPackage()
{
    if (m_installing || m_parser->isRunning() || m_checker->isRunning()
        || !m_currentInfo.isValid()) {
        return;
    }

    if (m_installedStatus.state == InstalledPackageStatus::InstalledIsNewer) {
        QMessageBox::information(
            this,
            tr("Versión más reciente ya instalada"),
            tr("El sistema ya tiene instalada la versión %1 de «%2», que es más "
               "reciente que la versión %3 del archivo seleccionado.\n\nNo se "
               "realizará ninguna instalación para no degradar el paquete.")
                .arg(m_installedStatus.installedVersion, m_currentInfo.package,
                     m_currentInfo.version));
        return;
    }

    const QFileInfo fileInfo(m_currentInfo.filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QMessageBox::warning(
            this,
            tr("Archivo no disponible"),
            tr("El paquete ya no está disponible en:\n%1")
                .arg(QDir::toNativeSeparators(m_currentInfo.filePath)));
        return;
    }

    const bool updating = m_installedStatus.state == InstalledPackageStatus::UpdateAvailable;
    const bool reinstalling = m_installedStatus.state == InstalledPackageStatus::SameVersion;

    QMessageBox confirmation(this);
    confirmation.setWindowTitle(updating ? tr("Confirmar actualización")
                                         : reinstalling ? tr("Confirmar reinstalación")
                                                        : tr("Confirmar instalación"));
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setTextFormat(Qt::RichText);

    if (updating) {
        confirmation.setText(tr("Se va a actualizar <b>%1</b> de la versión <b>%2</b> "
                                "a la versión <b>%3</b>.")
                                 .arg(m_currentInfo.package.toHtmlEscaped(),
                                      m_installedStatus.installedVersion.toHtmlEscaped(),
                                      m_currentInfo.version.toHtmlEscaped()));
    } else if (reinstalling) {
        confirmation.setText(tr("La versión <b>%1</b> de <b>%2</b> ya está instalada. "
                                "Se reinstalará el paquete con el archivo seleccionado.")
                                 .arg(m_currentInfo.version.toHtmlEscaped(),
                                      m_currentInfo.package.toHtmlEscaped()));
    } else {
        confirmation.setText(tr("Se va a instalar <b>%1 %2</b>.")
                                 .arg(m_currentInfo.package.toHtmlEscaped(),
                                      m_currentInfo.version.toHtmlEscaped()));
    }

    confirmation.setInformativeText(
        tr("apt instalará también las dependencias que falten desde los repositorios "
           "configurados.\n\nSe solicitará la contraseña de administrador mediante "
           "PolicyKit."));
    QPushButton *confirmButton = confirmation.addButton(
        updating ? tr("Actualizar") : reinstalling ? tr("Reinstalar") : tr("Instalar"),
        QMessageBox::AcceptRole);
    confirmation.addButton(tr("Cancelar"), QMessageBox::RejectRole);
    confirmation.setDefaultButton(confirmButton);
    confirmation.exec();

    if (confirmation.clickedButton() != confirmButton)
        return;

    setInstalling(true);
    m_manager->install(m_currentInfo.filePath);
}

void MainWindow::setInstalling(bool installing)
{
    m_installing = installing;

    m_openButton->setEnabled(!installing);

    if (installing) {
        const bool updating =
            m_installedStatus.state == InstalledPackageStatus::UpdateAvailable;
        m_installButton->setEnabled(false);
        m_installButton->setText(updating ? tr("Actualizando…") : tr("Instalando…"));
    } else {
        updateInstallAction();
    }

    if (installing) {
        m_logView->clear();
        m_detailsToggle->setChecked(true);
        m_progressBar->setRange(0, 0);
        m_progressBar->setFormat(tr("Instalando…"));
        m_progressBar->show();
        statusBar()->showMessage(tr("Iniciando la instalación…"));
    }
}

void MainWindow::onInstallStarted(const QString &commandLine)
{
    appendLog(tr("$ %1\n\n").arg(commandLine));
    statusBar()->showMessage(tr("Esperando autenticación o procesando la instalación…"));
}

void MainWindow::onInstallOutput(const QString &text)
{
    appendLog(text);
}

void MainWindow::onInstallFinished(bool success, int exitCode, const QString &message)
{
    setInstalling(false);

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(success ? 100 : 0);
    m_progressBar->setFormat(success ? tr("Completado") : tr("No completado"));

    appendLog(QStringLiteral("\n\n"));
    if (exitCode >= 0)
        appendLog(tr("[Proceso finalizado con código %1]\n").arg(exitCode));
    else
        appendLog(tr("[El proceso no llegó a iniciarse]\n"));
    appendLog(message + QLatin1Char('\n'));

    if (success) {
        statusBar()->showMessage(tr("Instalación completada correctamente."));
        startInstalledCheck();
        QMessageBox::information(
            this,
            tr("Instalación completada"),
            tr("%1\n\nEl paquete ya está instalado en el sistema.").arg(message));
        return;
    }

    const bool authorizationProblem = (exitCode == 125 || exitCode == 126 || exitCode == 127);
    statusBar()->showMessage(authorizationProblem
                                 ? tr("Instalación cancelada. No se realizaron cambios.")
                                 : tr("La instalación no se completó."));

    QMessageBox resultBox(this);
    resultBox.setWindowTitle(authorizationProblem ? tr("Instalación cancelada")
                                                  : tr("Error de instalación"));
    resultBox.setIcon(authorizationProblem ? QMessageBox::Warning : QMessageBox::Critical);
    resultBox.setText(message);
    if (!authorizationProblem) {
        resultBox.setInformativeText(
            tr("Consulta el registro de instalación para obtener más información."));
    }
    resultBox.exec();
}

void MainWindow::showPlaceholder(const QString &message)
{
    m_placeholderMessage = message;
    const QString hint = tr("Arrastra un archivo .deb aquí o pulsa «Abrir archivo…».");
    m_metadataView->setHtml(
        QStringLiteral("<div style=\"margin-top:64px; text-align:center; color:%1;\">"
                       "<p style=\"font-size:16px; margin:0 0 6px 0;\">%2</p>"
                       "<p style=\"font-size:12px; margin:0;\">%3</p>"
                       "</div>")
            .arg(m_mutedColor, message.toHtmlEscaped(), hint.toHtmlEscaped()));
}

void MainWindow::showMetadata(const DebPackageInfo &info)
{
    m_metadataView->setHtml(buildMetadataHtml(info));
}

QString MainWindow::buildMetadataHtml(const DebPackageInfo &info) const
{
    QString html;
    html.reserve(4096);

    html += QStringLiteral("<div style=\"font-size:13px;\">");
    html += QStringLiteral("<p style=\"font-size:20px; font-weight:700; margin:0;\">%1</p>")
                .arg(info.package.toHtmlEscaped());

    const QString shortDescription = info.shortDescription.isEmpty()
                                         ? tr("Sin descripción disponible.")
                                         : info.shortDescription;
    html += QStringLiteral("<p style=\"color:%1; margin:2px 0 14px 0;\">%2</p>")
                .arg(m_mutedColor, shortDescription.toHtmlEscaped());

    html += QStringLiteral("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"3\">");

    const auto addRow = [&html, this](const QString &label, const QString &value) {
        if (value.trimmed().isEmpty())
            return;
        html += QStringLiteral("<tr><td style=\"color:%1; padding:3px 14px 3px 0; "
                               "white-space:nowrap; vertical-align:top;\">%2</td>"
                               "<td style=\"padding:3px 0;\">%3</td></tr>")
                    .arg(m_mutedColor, label.toHtmlEscaped(), value.toHtmlEscaped());
    };

    addRow(tr("Estado"), installedStateText());
    addRow(tr("Versión"), info.version);
    addRow(tr("Arquitectura"), info.architecture);
    addRow(tr("Tamaño instalado"), installedSizeText(info.installedSize));

    const QFileInfo packageFile(info.filePath);
    if (packageFile.exists()) {
        addRow(tr("Tamaño del archivo"),
               QLocale().formattedDataSize(packageFile.size(), 2,
                                           QLocale::DataSizeTraditionalFormat));
    }

    addRow(tr("Mantenedor"), info.maintainer);
    if (!info.originalMaintainer.isEmpty() && info.originalMaintainer != info.maintainer)
        addRow(tr("Mantenedor original"), info.originalMaintainer);
    addRow(tr("Sección"), info.section);
    addRow(tr("Prioridad"), info.priority);
    addRow(tr("Código fuente"), info.source);
    addRow(tr("Pre-dependencias"), info.preDepends);
    addRow(tr("Sitio web"), info.homepage);
    addRow(tr("Archivo"), QDir::toNativeSeparators(info.filePath));

    html += QStringLiteral("</table>");

    const int lineBreak = info.description.indexOf(QLatin1Char('\n'));
    const QString longDescription = lineBreak >= 0 ? info.description.mid(lineBreak + 1)
                                                   : QString();
    if (!longDescription.trimmed().isEmpty()) {
        QString escaped = longDescription.toHtmlEscaped();
        escaped.replace(QLatin1Char('\n'), QStringLiteral("<br/>"));
        html += QStringLiteral("<p style=\"margin:16px 0 4px 0; font-weight:700;\">%1</p>")
                    .arg(tr("Descripción"));
        html += QStringLiteral("<p style=\"color:%1; margin:0;\">%2</p>")
                    .arg(m_mutedColor, escaped);
    }

    html += QStringLiteral("</div>");
    return html;
}

QString MainWindow::installedSizeText(const QString &kib) const
{
    bool ok = false;
    const qlonglong sizeInKib = kib.toLongLong(&ok);
    if (!ok || sizeInKib <= 0)
        return kib;

    const double sizeInMib = static_cast<double>(sizeInKib) / 1024.0;
    return tr("%1 MiB (%2 KiB)")
        .arg(QString::number(sizeInMib, 'f', 1), QLocale().toString(sizeInKib));
}

void MainWindow::startInstalledCheck()
{
    if (!m_currentInfo.isValid()) {
        m_checkPending = false;
        updateStatusDisplay();
        updateInstallAction();
        return;
    }

    if (m_checker->isRunning())
        m_checker->cancel();

    m_installedStatus = InstalledPackageStatus();
    m_checkPending = true;

    updateStatusDisplay();
    updateInstallAction();

    statusBar()->showMessage(
        tr("Comprobando si «%1» ya está instalado en el sistema…")
            .arg(m_currentInfo.package));

    m_checker->check(m_currentInfo.package, m_currentInfo.version,
                     m_currentInfo.architecture);
}

void MainWindow::onInstalledStatusChecked(const InstalledPackageStatus &status)
{
    m_checkPending = false;
    m_installedStatus = status;

    updateStatusDisplay();
    updateInstallAction();

    if (m_currentInfo.isValid())
        showMetadata(m_currentInfo);

    switch (status.state) {
    case InstalledPackageStatus::NotInstalled:
        statusBar()->showMessage(
            tr("%1 %2 no está instalado. Listo para instalar.")
                .arg(m_currentInfo.package, m_currentInfo.version));
        break;
    case InstalledPackageStatus::SameVersion:
        statusBar()->showMessage(
            tr("%1 %2 ya está instalado con la misma versión.")
                .arg(m_currentInfo.package, m_currentInfo.version));
        break;
    case InstalledPackageStatus::UpdateAvailable:
        statusBar()->showMessage(
            tr("Actualización disponible para %1: de %2 a %3.")
                .arg(m_currentInfo.package, status.installedVersion,
                     m_currentInfo.version));
        break;
    case InstalledPackageStatus::InstalledIsNewer:
        statusBar()->showMessage(
            tr("La versión instalada de %1 (%2) es más reciente que la del archivo (%3).")
                .arg(m_currentInfo.package, status.installedVersion,
                     m_currentInfo.version));
        break;
    case InstalledPackageStatus::Unknown:
        statusBar()->showMessage(
            tr("No se pudo comprobar si %1 ya está instalado.")
                .arg(m_currentInfo.package));
        break;
    }
}

void MainWindow::updateStatusDisplay()
{
    if (!m_statusLabel)
        return;

    if (!m_currentInfo.isValid()) {
        m_statusLabel->clear();
        m_statusLabel->hide();
        return;
    }

    QString color;
    QString title;
    QString detail;

    if (m_checker->isRunning() || m_checkPending) {
        color = m_accentColor;
        title = tr("Comprobando el estado del paquete…");
        detail = tr("Consultando la versión instalada en el sistema.");
    } else {
        switch (m_installedStatus.state) {
        case InstalledPackageStatus::NotInstalled:
            color = m_accentColor;
            title = tr("Este paquete no está instalado.");
            detail = tr("Se instalará la versión %1 desde el archivo seleccionado.")
                         .arg(m_currentInfo.version);
            break;
        case InstalledPackageStatus::SameVersion:
            color = m_successColor;
            title = tr("Ya está instalado.");
            detail = tr("La versión instalada (%1) es la misma que la del archivo. "
                        "Puedes reinstalarlo si lo necesitas.")
                         .arg(m_installedStatus.installedVersion);
            break;
        case InstalledPackageStatus::UpdateAvailable:
            color = m_warningColor;
            title = tr("Actualización disponible.");
            detail = tr("Versión instalada: %1 · Versión del archivo: %2.")
                         .arg(m_installedStatus.installedVersion,
                              m_currentInfo.version);
            break;
        case InstalledPackageStatus::InstalledIsNewer:
            color = m_warningColor;
            title = tr("Ya hay una versión más reciente instalada.");
            detail = tr("Versión instalada: %1 · Versión del archivo: %2. "
                        "No se permitirá degradar el paquete.")
                         .arg(m_installedStatus.installedVersion,
                              m_currentInfo.version);
            break;
        case InstalledPackageStatus::Unknown:
            color = m_errorColor;
            title = tr("No se pudo comprobar el estado del paquete.");
            detail = m_installedStatus.errorMessage.isEmpty()
                         ? tr("Puedes intentar la instalación de todos modos.")
                         : m_installedStatus.errorMessage.simplified();
            break;
        }
    }

    m_statusLabel->setText(
        QStringLiteral("<span style=\"color:%1; font-size:14px;\">●</span>&nbsp;"
                       "<b style=\"color:%1;\">%2</b><br/>"
                       "<span style=\"color:%3;\">%4</span>")
            .arg(color, title.toHtmlEscaped(), m_mutedColor, detail.toHtmlEscaped()));
    m_statusLabel->show();
}

void MainWindow::updateInstallAction()
{
    if (m_installing)
        return;

    m_installButton->setText(installButtonText());

    if (!m_currentInfo.isValid()) {
        m_installButton->setEnabled(false);
        m_installButton->setToolTip(tr("Abre un paquete .deb para poder instalarlo"));
        return;
    }

    if (m_checker->isRunning() || m_checkPending) {
        m_installButton->setEnabled(false);
        m_installButton->setToolTip(
            tr("Esperando a que termine la comprobación del estado del paquete"));
        return;
    }

    switch (m_installedStatus.state) {
    case InstalledPackageStatus::InstalledIsNewer:
        m_installButton->setEnabled(false);
        m_installButton->setToolTip(
            tr("La versión instalada (%1) es más reciente que la del archivo (%2)")
                .arg(m_installedStatus.installedVersion, m_currentInfo.version));
        break;
    case InstalledPackageStatus::UpdateAvailable:
        m_installButton->setEnabled(true);
        m_installButton->setToolTip(
            tr("Actualiza el paquete de la versión %1 a la %2 con apt")
                .arg(m_installedStatus.installedVersion, m_currentInfo.version));
        break;
    case InstalledPackageStatus::SameVersion:
        m_installButton->setEnabled(true);
        m_installButton->setToolTip(
            tr("Reinstala la misma versión (%1) con apt")
                .arg(m_currentInfo.version));
        break;
    case InstalledPackageStatus::NotInstalled:
        m_installButton->setEnabled(true);
        m_installButton->setToolTip(
            tr("Instala el paquete y resuelve sus dependencias con apt"));
        break;
    case InstalledPackageStatus::Unknown:
        m_installButton->setEnabled(true);
        m_installButton->setToolTip(
            tr("No se pudo comprobar el estado del paquete; se intentará instalar"));
        break;
    }
}

QString MainWindow::installButtonText() const
{
    if (!m_currentInfo.isValid())
        return tr("Instalar paquete");

    if (m_checker->isRunning() || m_checkPending)
        return tr("Comprobando…");

    switch (m_installedStatus.state) {
    case InstalledPackageStatus::UpdateAvailable:
        return tr("Actualizar paquete");
    case InstalledPackageStatus::SameVersion:
        return tr("Reinstalar paquete");
    case InstalledPackageStatus::InstalledIsNewer:
        return tr("Versión más reciente");
    case InstalledPackageStatus::NotInstalled:
    case InstalledPackageStatus::Unknown:
        break;
    }

    return tr("Instalar paquete");
}

QString MainWindow::installedStateText() const
{
    if (!m_currentInfo.isValid())
        return QString();

    if (m_checker->isRunning() || m_checkPending)
        return tr("Comprobando…");

    switch (m_installedStatus.state) {
    case InstalledPackageStatus::NotInstalled:
        return tr("No instalado");
    case InstalledPackageStatus::SameVersion:
        return tr("Instalado: %1 (misma versión)")
            .arg(m_installedStatus.installedVersion);
    case InstalledPackageStatus::UpdateAvailable:
        return tr("Instalado: %1 (actualización disponible a %2)")
            .arg(m_installedStatus.installedVersion, m_currentInfo.version);
    case InstalledPackageStatus::InstalledIsNewer:
        return tr("Instalado: %1 (más reciente que la versión %2 del archivo)")
            .arg(m_installedStatus.installedVersion, m_currentInfo.version);
    case InstalledPackageStatus::Unknown:
        return m_installedStatus.errorMessage.isEmpty()
                   ? tr("No se pudo comprobar")
                   : tr("No se pudo comprobar: %1")
                         .arg(m_installedStatus.errorMessage.simplified());
    }

    return tr("No se pudo comprobar");
}

void MainWindow::showDependencies(const DebPackageInfo &info)
{
    m_dependenciesTable->clearContents();
    m_dependenciesTable->setRowCount(info.dependencies.size());

    for (int row = 0; row < info.dependencies.size(); ++row) {
        const DebDependency &dependency = info.dependencies.at(row);

        auto *packageItem = new QTableWidgetItem(
            dependency.package.isEmpty() ? dependency.raw : dependency.package);
        packageItem->setToolTip(tr("Entrada completa: %1").arg(dependency.raw));

        auto *constraintItem = new QTableWidgetItem(
            dependency.constraint.isEmpty() ? QString::fromUtf8("—")
                                            : dependency.constraint);
        constraintItem->setToolTip(tr("Entrada completa: %1").arg(dependency.raw));
        constraintItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        m_dependenciesTable->setItem(row, 0, packageItem);
        m_dependenciesTable->setItem(row, 1, constraintItem);
    }

    const bool hasDependencies = !info.dependencies.isEmpty();
    m_dependenciesTable->setVisible(hasDependencies);
    m_dependenciesEmpty->setVisible(!hasDependencies);

    if (hasDependencies) {
        m_dependenciesGroup->setTitle(tr("Dependencias (%1)").arg(info.dependencies.size()));
    } else {
        m_dependenciesGroup->setTitle(tr("Dependencias"));
        if (info.depends.trimmed().isEmpty())
            m_dependenciesEmpty->setText(tr("Este paquete no declara dependencias."));
        else
            m_dependenciesEmpty->setText(
                tr("No se pudieron interpretar las dependencias declaradas."));
    }
}

void MainWindow::appendLog(const QString &text)
{
    if (text.isEmpty())
        return;

    m_logView->moveCursor(QTextCursor::End);
    m_logView->insertPlainText(text);

    QScrollBar *scrollBar = m_logView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_installing || m_parser->isRunning()) {
        event->ignore();
        return;
    }

    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = mimeData->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()
        && urls.first().toLocalFile().endsWith(QStringLiteral(".deb"),
                                               Qt::CaseInsensitive)) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (m_installing || m_parser->isRunning()) {
        event->ignore();
        return;
    }

    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) {
        event->ignore();
        return;
    }

    const QList<QUrl> urls = mimeData->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()
        && urls.first().toLocalFile().endsWith(QStringLiteral(".deb"),
                                               Qt::CaseInsensitive)) {
        event->acceptProposedAction();
        openDebFile(urls.first().toLocalFile());
    } else {
        event->ignore();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_manager->isRunning()) {
        QMessageBox::warning(
            this,
            tr("Instalación en curso"),
            tr("Hay una instalación en curso. Espera a que termine antes de cerrar "
               "la aplicación para no dejar dpkg en un estado inconsistente."));
        event->ignore();
        return;
    }

    QMainWindow::closeEvent(event);
}
