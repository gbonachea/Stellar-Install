#ifndef DEBPACKAGEINFO_H
#define DEBPACKAGEINFO_H

#include <QList>
#include <QMetaType>
#include <QString>

/*!
 * Una entrada del campo Depends. Cada entrada puede contener varias
 * alternativas separadas por '|'; en ese caso \c package las conserva
 * unidas para poder mostrarlas en la interfaz.
 */
struct DebDependency
{
    QString raw;        // Texto original completo, p. ej. "libfoo | libbar (>= 1.0)"
    QString package;    // Nombres de las alternativas, p. ej. "libfoo | libbar"
    QString constraint; // Restricciones de versión, p. ej. ">= 1.0"

    bool isAlternative() const { return raw.contains(QLatin1Char('|')); }
};

/*!
 * Metadatos de un paquete Debian obtenidos con \c{dpkg-deb -I}.
 * Todos los campos son texto ya que se muestran tal cual al usuario.
 */
struct DebPackageInfo
{
    QString filePath;
    QString package;
    QString version;
    QString architecture;
    QString installedSize; // En KiB, tal como lo declara el paquete
    QString maintainer;
    QString originalMaintainer;
    QString description;
    QString shortDescription;
    QString section;
    QString priority;
    QString homepage;
    QString source;
    QString preDepends;
    QString depends;
    QList<DebDependency> dependencies;

    bool isValid() const { return !package.isEmpty() && !version.isEmpty(); }
};

Q_DECLARE_METATYPE(DebPackageInfo)

#endif // DEBPACKAGEINFO_H
