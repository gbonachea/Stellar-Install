#include "PackageManager.h"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>

namespace {

QString stripAnsi(QString text)
{
    static const QRegularExpression ansi(QStringLiteral("\x1B\\[[0-9;?]*[ -/]*[@-~]"));
    text.remove(ansi);
    return text;
}

/*!
 * Normaliza la salida de apt para mostrarla en un QPlainTextEdit: quita
 * secuencias ANSI y convierte los retornos de carro (barras de progreso)
 * en saltos de línea.
 */
QString normalizeChunk(QString text)
{
    text = stripAnsi(text);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return text;
}

QString commandForDisplay(const QString &program, const QStringList &arguments)
{
    QStringList parts;
    parts.append(program);
    for (const QString &argument : arguments) {
        if (argument.contains(QLatin1Char(' ')))
            parts.append(QStringLiteral("\"%1\"").arg(argument));
        else
            parts.append(argument);
    }
    return parts.join(QLatin1Char(' '));
}

} // namespace

PackageManager::PackageManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);
    // Sin terminal interactiva: si apt o dpkg intentan leer, reciben EOF en
    // lugar de dejar la instalación bloqueada esperando una respuesta.
    m_process->setStandardInputFile(QProcess::nullDevice());

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this] { drainOutput(); });
    connect(m_process, &QProcess::readyReadStandardError, this, [this] { drainOutput(); });

    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) { handleFinished(); });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_active)
            return;
        if (error == QProcess::FailedToStart) {
            reportFailure(tr("No se pudo iniciar el proceso de instalación: %1")
                              .arg(m_process->errorString()));
        }
    });
}

bool PackageManager::isRunning() const
{
    return m_active;
}

void PackageManager::install(const QString &debFilePath)
{
    if (m_active)
        return;

    const QFileInfo fileInfo(debFilePath);
    if (debFilePath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
        reportFailure(tr("El archivo .deb ya no está disponible:\n%1").arg(debFilePath));
        return;
    }

    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (pkexec.isEmpty()) {
        reportFailure(tr("No se encontró «pkexec». Instala «policykit-1» o «polkitd» "
                         "para poder solicitar permisos de administrador."));
        return;
    }

    if (QStandardPaths::findExecutable(QStringLiteral("apt")).isEmpty()) {
        reportFailure(tr("No se encontró el gestor de paquetes «apt» en el sistema."));
        return;
    }

    const QString absolutePath = fileInfo.absoluteFilePath();
    QStringList arguments;
    arguments << QStringLiteral("apt")
              << QStringLiteral("install")
              << QStringLiteral("-y")
              // Ante conflictos de ficheros de configuración se conserva la
              // versión instalada, evitando preguntas que no se pueden mostrar.
              << QStringLiteral("-o")
              << QStringLiteral("Dpkg::Options::=--force-confdef")
              << QStringLiteral("-o")
              << QStringLiteral("Dpkg::Options::=--force-confold")
              << absolutePath;

    m_active = true;
    m_process->setWorkingDirectory(fileInfo.absolutePath());

    emit started(commandForDisplay(QStringLiteral("pkexec"), arguments));
    m_process->start(pkexec, arguments);
}

void PackageManager::drainOutput()
{
    const QByteArray standardOutput = m_process->readAllStandardOutput();
    if (!standardOutput.isEmpty()) {
        const QString text = normalizeChunk(QString::fromLocal8Bit(standardOutput));
        if (!text.isEmpty())
            emit outputReceived(text);
    }

    const QByteArray standardError = m_process->readAllStandardError();
    if (!standardError.isEmpty()) {
        const QString text = normalizeChunk(QString::fromLocal8Bit(standardError));
        if (!text.isEmpty())
            emit outputReceived(text);
    }
}

void PackageManager::handleFinished()
{
    if (!m_active)
        return;

    drainOutput();
    m_active = false;

    const int exitCode = m_process->exitCode();
    const bool crashed = m_process->exitStatus() == QProcess::CrashExit;
    const bool success = !crashed && exitCode == 0;

    QString message;
    if (success) {
        message = tr("El paquete se instaló correctamente.");
    } else if (crashed) {
        message = tr("El proceso de instalación terminó de forma inesperada.");
    } else {
        switch (exitCode) {
        case 125:
            message = tr("PolicyKit no pudo iniciar la operación. Comprueba que el servicio "
                         "de autenticación esté activo.");
            break;
        case 126:
            message = tr("La autenticación fue cancelada. No se realizaron cambios en el sistema.");
            break;
        case 127:
            message = tr("PolicyKit denegó la autorización. No se realizaron cambios en el sistema.");
            break;
        case 100:
            message = tr("apt no pudo completar la instalación (código 100). Puede haber "
                         "dependencias sin resolver, un bloqueo de dpkg o falta de espacio.");
            break;
        case 1:
            message = tr("La operación se canceló o apt encontró un error (código 1).");
            break;
        default:
            message = tr("La instalación falló con el código de salida %1.").arg(exitCode);
            break;
        }
    }

    emit finished(success, crashed ? -1 : exitCode, message);
}

void PackageManager::reportFailure(const QString &message)
{
    m_active = false;
    // Se emite en el siguiente ciclo del bucle de eventos para que la
    // interfaz termine de actualizar su estado antes de mostrar el error.
    QTimer::singleShot(0, this, [this, message] { emit finished(false, -1, message); });
}
