#include "DebParser.h"

#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

namespace {

QString stripAnsi(const QString &input)
{
    static const QRegularExpression ansi(QStringLiteral("\x1B\\[[0-9;?]*[ -/]*[@-~]"));
    QString text = input;
    text.remove(ansi);
    return text;
}

/*!
 * Divide \a value por \a separator ignorando los separadores que aparecen
 * dentro de paréntesis, como en "libfoo (>= 1.0), libbar".
 */
QStringList splitTopLevel(const QString &value, QChar separator)
{
    QStringList parts;
    QString current;
    int depth = 0;

    for (const QChar character : value) {
        if (character == QLatin1Char('(')) {
            ++depth;
        } else if (character == QLatin1Char(')')) {
            if (depth > 0)
                --depth;
        }

        if (character == separator && depth == 0) {
            const QString part = current.trimmed();
            if (!part.isEmpty())
                parts.append(part);
            current.clear();
        } else {
            current.append(character);
        }
    }

    const QString part = current.trimmed();
    if (!part.isEmpty())
        parts.append(part);
    return parts;
}

/*!
 * dpkg-deb -I imprime el bloque de control con un espacio de sangría y,
 * antes, un resumen del archivo. Localizamos la línea "Package:" y
 * devolvemos el bloque sin esa sangría adicional.
 */
bool extractControlStanza(const QString &output, QString *stanza)
{
    static const QRegularExpression packagePattern(
        QStringLiteral("^\\s*Package\\s*:"));
    const QStringList lines = output.split(QLatin1Char('\n'));

    int start = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (packagePattern.match(lines.at(i)).hasMatch()) {
            start = i;
            break;
        }
    }
    if (start < 0)
        return false;

    QStringList result;
    for (int i = start; i < lines.size(); ++i) {
        QString line = lines.at(i);
        if (line.trimmed().isEmpty())
            break;
        if (line.startsWith(QLatin1Char(' ')))
            line.remove(0, 1);
        result.append(line);
    }

    if (result.isEmpty())
        return false;

    *stanza = result.join(QLatin1Char('\n'));
    return true;
}

/*!
 * Interpreta un bloque de control estilo RFC 822. Las líneas que empiezan
 * por espacio o tabulador continúan el valor del campo anterior, algo
 * habitual en Description y en campos Depends partidos en varias líneas.
 */
QHash<QString, QString> parseControlFields(const QString &stanza)
{
    static const QRegularExpression fieldPattern(
        QStringLiteral("^([A-Za-z0-9][A-Za-z0-9-]*)\\s*:\\s*(.*)$"));

    QHash<QString, QString> fields;
    QString currentKey;

    const QStringList lines = stanza.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1Char(' ')) || line.startsWith(QLatin1Char('\t'))) {
            if (currentKey.isEmpty())
                continue;
            QString &value = fields[currentKey];
            if (!value.isEmpty())
                value.append(QLatin1Char('\n'));
            value.append(line.trimmed());
            continue;
        }

        const QRegularExpressionMatch match = fieldPattern.match(line);
        if (!match.hasMatch())
            continue;

        currentKey = match.captured(1).toLower();
        fields.insert(currentKey, match.captured(2).trimmed());
    }

    return fields;
}

QList<DebDependency> parseDependencies(const QString &depends)
{
    static const QRegularExpression alternativePattern(
        QStringLiteral(R"(^\s*([^\s(]+)\s*(?:\(\s*([^)]*?)\s*\))?\s*$)"));

    QList<DebDependency> dependencies;
    const QString simplified = depends.simplified();
    if (simplified.isEmpty())
        return dependencies;

    const QStringList entries = splitTopLevel(simplified, QLatin1Char(','));
    for (const QString &entry : entries) {
        QStringList names;
        QStringList constraints;

        const QStringList alternatives = splitTopLevel(entry, QLatin1Char('|'));
        for (const QString &alternative : alternatives) {
            const QRegularExpressionMatch match = alternativePattern.match(alternative);
            if (match.hasMatch()) {
                names.append(match.captured(1).trimmed());
                const QString constraint = match.captured(2).trimmed();
                if (!constraint.isEmpty())
                    constraints.append(constraint);
            } else {
                names.append(alternative.trimmed());
            }
        }

        if (names.isEmpty())
            continue;

        DebDependency dependency;
        dependency.raw = entry;
        dependency.package = names.join(QStringLiteral(" | "));
        constraints.removeDuplicates();
        dependency.constraint = constraints.join(QStringLiteral(" | "));
        dependencies.append(dependency);
    }

    return dependencies;
}

QString cleanDescription(const QString &value)
{
    QStringList lines = value.split(QLatin1Char('\n'));
    for (QString &line : lines) {
        if (line.trimmed() == QLatin1String("."))
            line.clear();
    }
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
        lines.removeLast();
    return lines.join(QLatin1Char('\n'));
}

} // namespace

DebParser::DebParser(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) { handleFinished(); });

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (!m_active)
            return;
        if (error == QProcess::FailedToStart) {
            m_active = false;
            emit failed(tr("No se pudo ejecutar dpkg-deb: %1").arg(m_process->errorString()));
        }
    });
}

bool DebParser::isRunning() const
{
    return m_active;
}

void DebParser::parse(const QString &filePath)
{
    if (m_active)
        return;

    const QFileInfo fileInfo(filePath);
    if (filePath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile()) {
        reportFailure(tr("El archivo no existe o no es un archivo regular:\n%1").arg(filePath));
        return;
    }
    if (!fileInfo.isReadable()) {
        reportFailure(tr("No hay permisos de lectura para el archivo:\n%1").arg(filePath));
        return;
    }

    const QString program = QStandardPaths::findExecutable(QStringLiteral("dpkg-deb"));
    if (program.isEmpty()) {
        reportFailure(tr("No se encontró «dpkg-deb». Instala el paquete «dpkg» para poder "
                         "leer paquetes Debian."));
        return;
    }

    m_filePath = fileInfo.absoluteFilePath();
    m_active = true;

    m_process->setWorkingDirectory(fileInfo.absolutePath());
    emit started(m_filePath);
    m_process->start(program, {QStringLiteral("-I"), m_filePath});
}

void DebParser::handleFinished()
{
    if (!m_active)
        return;

    m_active = false;

    const QString errorOutput = stripAnsi(
        QString::fromLocal8Bit(m_process->readAllStandardError())).trimmed();
    const QString standardOutput = QString::fromUtf8(m_process->readAllStandardOutput());

    if (m_process->exitStatus() != QProcess::NormalExit) {
        emit failed(tr("dpkg-deb terminó de forma inesperada mientras leía el paquete."));
        return;
    }

    if (m_process->exitCode() != 0) {
        if (!errorOutput.isEmpty()) {
            emit failed(errorOutput);
        } else {
            emit failed(tr("dpkg-deb devolvió el código de salida %1.")
                            .arg(m_process->exitCode()));
        }
        return;
    }

    QString stanza;
    if (!extractControlStanza(standardOutput, &stanza)) {
        emit failed(tr("No se encontró el bloque de control del paquete. "
                       "Es posible que el archivo no sea un paquete Debian válido."));
        return;
    }

    const QHash<QString, QString> fields = parseControlFields(stanza);

    DebPackageInfo info;
    info.filePath = m_filePath;
    info.package = fields.value(QStringLiteral("package"));
    info.version = fields.value(QStringLiteral("version"));
    info.architecture = fields.value(QStringLiteral("architecture"));
    info.installedSize = fields.value(QStringLiteral("installed-size"));
    info.maintainer = fields.value(QStringLiteral("maintainer"));
    info.originalMaintainer = fields.value(QStringLiteral("original-maintainer"));
    info.section = fields.value(QStringLiteral("section"));
    info.priority = fields.value(QStringLiteral("priority"));
    info.homepage = fields.value(QStringLiteral("homepage"));
    info.source = fields.value(QStringLiteral("source"));
    info.preDepends = fields.value(QStringLiteral("pre-depends")).simplified();
    info.depends = fields.value(QStringLiteral("depends")).simplified();
    info.description = cleanDescription(fields.value(QStringLiteral("description")));
    info.shortDescription = info.description.section(QLatin1Char('\n'), 0, 0).trimmed();
    info.dependencies = parseDependencies(info.depends);

    if (!info.isValid()) {
        emit failed(tr("El archivo no contiene los metadatos mínimos (Package y Version) "
                       "de un paquete Debian."));
        return;
    }

    emit finished(info);
}

void DebParser::reportFailure(const QString &message)
{
    m_active = false;
    emit failed(message);
}
