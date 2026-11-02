#include "InstalledPackageChecker.h"

#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

namespace {
constexpr int kQueryTimeoutMs = 5000;
}

InstalledPackageChecker::InstalledPackageChecker(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_timeout(new QTimer(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    m_process->setStandardInputFile(QProcess::nullDevice());

    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) { handleFinished(); });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_step == Step::Idle)
            return;
        if (error == QProcess::FailedToStart) {
            finishWith(InstalledPackageStatus::Unknown,
                       tr("No se pudo ejecutar la consulta del paquete: %1")
                           .arg(m_process->errorString()));
        }
    });

    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        if (m_step == Step::Idle)
            return;
        m_process->kill();
        finishWith(InstalledPackageStatus::Unknown,
                   tr("La consulta del estado del paquete tardó demasiado."));
    });
}

bool InstalledPackageChecker::isRunning() const
{
    return m_step != Step::Idle;
}

void InstalledPackageChecker::cancel()
{
    if (!isRunning())
        return;

    m_step = Step::Idle;
    m_timeout->stop();
    m_process->kill();
    m_process->waitForFinished(1000);
    m_process->close();

    m_packageName.clear();
    m_localVersion.clear();
    m_architecture.clear();
    m_installedVersion.clear();
    m_installedArchitecture.clear();
}

void InstalledPackageChecker::check(const QString &packageName,
                                    const QString &localVersion,
                                    const QString &architecture)
{
    if (isRunning())
        return;

    m_packageName = packageName.trimmed();
    m_localVersion = localVersion.trimmed();
    m_architecture = architecture.trimmed();
    m_installedVersion.clear();
    m_installedArchitecture.clear();

    if (m_packageName.isEmpty() || m_localVersion.isEmpty()) {
        finishWith(InstalledPackageStatus::Unknown,
                   tr("Faltan el nombre o la versión del paquete para comprobar su estado."));
        return;
    }

    const QString queryProgram = QStandardPaths::findExecutable(QStringLiteral("dpkg-query"));
    if (queryProgram.isEmpty()) {
        finishWith(InstalledPackageStatus::Unknown,
                   tr("No se encontró «dpkg-query». Instala el paquete «dpkg» para poder "
                      "comprobar si el paquete ya está instalado."));
        return;
    }

    m_step = Step::Querying;
    m_timeout->start(kQueryTimeoutMs);

    const QString format = QStringLiteral("${Status}\t${Version}\t${Architecture}\n");
    m_process->start(queryProgram,
                     {QStringLiteral("-W"), QStringLiteral("-f"), format, m_packageName});
}

void InstalledPackageChecker::handleFinished()
{
    if (m_step == Step::Idle)
        return;

    const QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    const QString errors = QString::fromLocal8Bit(m_process->readAllStandardError()).trimmed();

    if (m_process->exitStatus() != QProcess::NormalExit) {
        finishWith(InstalledPackageStatus::Unknown,
                   tr("La consulta del estado del paquete terminó de forma inesperada."));
        return;
    }

    if (m_step == Step::Querying) {
        const int exitCode = m_process->exitCode();

        if (exitCode != 0) {
            if (output.trimmed().isEmpty()) {
                if (exitCode == 1) {
                    // dpkg-query devuelve 1 cuando el paquete no está instalado.
                    finishWith(InstalledPackageStatus::NotInstalled);
                } else {
                    finishWith(InstalledPackageStatus::Unknown,
                               errors.isEmpty()
                                   ? tr("No se pudo consultar el estado del paquete.")
                                   : errors);
                }
            } else {
                finishWith(InstalledPackageStatus::Unknown,
                           errors.isEmpty()
                               ? tr("No se pudo consultar el estado del paquete.")
                               : errors);
            }
            return;
        }

        bool found = false;
        const QStringList lines = output.split(QLatin1Char('\n'));
        for (const QString &rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.isEmpty())
                continue;

            const QStringList parts = line.split(QLatin1Char('\t'));
            if (parts.size() < 2)
                continue;

            const QString status = parts.at(0).trimmed();
            const QString version = parts.at(1).trimmed();
            const QString arch = parts.size() > 2 ? parts.at(2).trimmed() : QString();

            const QStringList statusFields = status.split(QLatin1Char(' '));
            const QString statusName = statusFields.isEmpty() ? QString()
                                                              : statusFields.last();
            if (statusName != QLatin1String("installed") || version.isEmpty())
                continue;

            // Si el paquete tiene instaladas varias arquitecturas, nos
            // interesa la que coincide con el archivo .deb abierto.
            if (!m_architecture.isEmpty()
                && m_architecture != QLatin1String("all")
                && !arch.isEmpty()
                && arch != m_architecture) {
                continue;
            }

            m_installedVersion = version;
            m_installedArchitecture = arch;
            found = true;
            break;
        }

        if (!found) {
            finishWith(InstalledPackageStatus::NotInstalled);
            return;
        }

        if (m_installedVersion == m_localVersion) {
            finishWith(InstalledPackageStatus::SameVersion);
            return;
        }

        startVersionComparison();
        return;
    }

    if (m_step == Step::ComparingLess) {
        if (m_process->exitCode() == 0) {
            // versión instalada < versión del archivo
            finishWith(InstalledPackageStatus::UpdateAvailable);
            return;
        }

        if (m_process->exitCode() != 1) {
            finishWith(InstalledPackageStatus::Unknown,
                       errors.isEmpty()
                           ? tr("No se pudieron comparar las versiones del paquete.")
                           : errors);
            return;
        }

        // La versión instalada no es anterior; puede ser igual o posterior.
        const QString dpkg = QStandardPaths::findExecutable(QStringLiteral("dpkg"));
        if (dpkg.isEmpty()) {
            finishWith(InstalledPackageStatus::Unknown,
                       tr("No se encontró «dpkg» para comparar las versiones."));
            return;
        }

        m_step = Step::ComparingEqual;
        m_timeout->start(kQueryTimeoutMs);
        m_process->start(dpkg,
                         {QStringLiteral("--compare-versions"), m_installedVersion,
                          QStringLiteral("eq"), m_localVersion});
        return;
    }

    if (m_step == Step::ComparingEqual) {
        if (m_process->exitCode() == 0) {
            finishWith(InstalledPackageStatus::SameVersion);
        } else if (m_process->exitCode() == 1) {
            finishWith(InstalledPackageStatus::InstalledIsNewer);
        } else {
            finishWith(InstalledPackageStatus::Unknown,
                       errors.isEmpty()
                           ? tr("No se pudieron comparar las versiones del paquete.")
                           : errors);
        }
    }
}

void InstalledPackageChecker::startVersionComparison()
{
    const QString dpkg = QStandardPaths::findExecutable(QStringLiteral("dpkg"));
    if (dpkg.isEmpty()) {
        finishWith(InstalledPackageStatus::Unknown,
                   tr("No se encontró «dpkg» para comparar las versiones."));
        return;
    }

    m_step = Step::ComparingLess;
    m_timeout->start(kQueryTimeoutMs);
    m_process->start(dpkg,
                     {QStringLiteral("--compare-versions"), m_installedVersion,
                      QStringLiteral("lt"), m_localVersion});
}

void InstalledPackageChecker::finishWith(InstalledPackageStatus::State state,
                                         const QString &error)
{
    m_timeout->stop();
    m_step = Step::Idle;

    InstalledPackageStatus status;
    status.state = state;
    status.installedVersion = m_installedVersion;
    status.installedArchitecture = m_installedArchitecture;
    status.errorMessage = error;

    emit finished(status);
}
