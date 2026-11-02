#include "DebPackageInfo.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QString>

#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Stellar"));
    QApplication::setApplicationName(QStringLiteral("Stellar Install"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("stellar-install"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/stellar-install.png")));

    qRegisterMetaType<DebPackageInfo>("DebPackageInfo");

    QCommandLineParser commandLine;
    commandLine.setApplicationDescription(
        QStringLiteral("Visor de metadatos e instalador de paquetes Debian locales."));
    commandLine.addHelpOption();
    commandLine.addVersionOption();
    commandLine.addPositionalArgument(
        QStringLiteral("archivo"),
        QStringLiteral("Ruta del paquete .deb que se abrirá al iniciar."),
        QStringLiteral("[archivo]"));
    commandLine.process(app);

    MainWindow window;
    window.show();

    const QStringList positionalArguments = commandLine.positionalArguments();
    if (!positionalArguments.isEmpty())
        window.openDebFile(positionalArguments.constFirst());

    return app.exec();
}
