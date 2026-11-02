#ifndef INSTALLEDPACKAGECHECKER_H
#define INSTALLEDPACKAGECHECKER_H

#include <QMetaType>
#include <QObject>
#include <QString>

class QProcess;
class QTimer;

/*!
 * Estado del paquete en el sistema comparado con el archivo .deb abierto.
 */
struct InstalledPackageStatus
{
    enum State {
        Unknown,         // No se pudo consultar el estado
        NotInstalled,    // El paquete no está instalado
        SameVersion,     // Instalado y con la misma versión que el archivo
        UpdateAvailable, // Instalado, pero el archivo contiene una versión más nueva
        InstalledIsNewer // Instalado con una versión más nueva que la del archivo
    };

    State state = Unknown;
    QString installedVersion;
    QString installedArchitecture;
    QString errorMessage;

    bool isInstalled() const
    {
        return state == SameVersion || state == UpdateAvailable
            || state == InstalledIsNewer;
    }
};

Q_DECLARE_METATYPE(InstalledPackageStatus)

/*!
 * Consulta de forma asíncrona si un paquete está instalado y compara su
 * versión con la del archivo .deb. Usa dpkg-query y dpkg --compare-versions,
 * que implementan la semántica oficial de versiones Debian.
 */
class InstalledPackageChecker : public QObject
{
    Q_OBJECT

public:
    explicit InstalledPackageChecker(QObject *parent = nullptr);

    bool isRunning() const;
    void check(const QString &packageName,
               const QString &localVersion,
               const QString &architecture = QString());
    void cancel();

signals:
    void finished(const InstalledPackageStatus &status);

private:
    enum class Step {
        Idle,
        Querying,
        ComparingLess,
        ComparingEqual
    };

    void handleFinished();
    void startVersionComparison();
    void finishWith(InstalledPackageStatus::State state,
                    const QString &error = QString());

    QProcess *m_process = nullptr;
    QTimer *m_timeout = nullptr;
    Step m_step = Step::Idle;
    QString m_packageName;
    QString m_localVersion;
    QString m_architecture;
    QString m_installedVersion;
    QString m_installedArchitecture;
};

#endif // INSTALLEDPACKAGECHECKER_H
