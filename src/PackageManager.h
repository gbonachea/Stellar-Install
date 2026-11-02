#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>
#include <QString>

class QProcess;

/*!
 * Instala un archivo .deb local mediante apt, elevando privilegios con
 * pkexec (PolicyKit). Todo el proceso es asíncrono y su salida se
 * reenvía mediante la señal outputReceived().
 */
class PackageManager : public QObject
{
    Q_OBJECT

public:
    explicit PackageManager(QObject *parent = nullptr);

    bool isRunning() const;
    void install(const QString &debFilePath);

signals:
    void started(const QString &commandLine);
    void outputReceived(const QString &text);
    void finished(bool success, int exitCode, const QString &message);

private:
    void handleFinished();
    void drainOutput();
    void reportFailure(const QString &message);

    QProcess *m_process = nullptr;
    bool m_active = false;
};

#endif // PACKAGEMANAGER_H
