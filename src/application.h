#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include "global.h"
#include "FileIndex/fileindex.h"
#include <QSettings>

class Application : public QApplication
{
    Q_OBJECT
public:
    explicit Application(int & argc, char** argv);
    ~Application();

    FileIndex & fileIndex() { return m_fileIndex; }

    //TODO guess most methodes are obsolete. tidy up!
    static QString applicationName() { return "CAN2"; }
    static QString organizationDomain() { return "none@none.none"; }
    static QString organizationName() { return "CAN2 Developer"; }

    /**
     * @brief settings application name etc. is set after static initialization. Therefore, set it manually
     * @return settings object with appropriate applicationName, organizationName and organizationDomain.
     */
    QSettings &settings();
    const QSettings &settings() const;

    const QString username() const { return "Detlef"; } //TODO

private:
    FileIndex m_fileIndex;
    QSettings m_settings;

    static Application* m_singleton;
    friend Application & app();

};

Application & app();

#endif // APPLICATION_H
