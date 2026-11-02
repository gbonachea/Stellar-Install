#ifndef DEBPARSER_H
#define DEBPARSER_H

#include "DebPackageInfo.h"

#include <QObject>
#include <QString>

class QProcess;

/*!
 * Lee los metadatos de un archivo .deb ejecutando \c{dpkg-deb -I}
 * de forma asíncrona. No requiere privilegios de root.
 */
class DebParser : public QObject
{
    Q_OBJECT

public:
    explicit DebParser(QObject *parent = nullptr);

    bool isRunning() const;
    void parse(const QString &filePath);

signals:
    void started(const QString &filePath);
    void finished(const DebPackageInfo &info);
    void failed(const QString &message);

private:
    void handleFinished();
    void reportFailure(const QString &message);

    QProcess *m_process = nullptr;
    QString m_filePath;
    bool m_active = false;
};

#endif // DEBPARSER_H
