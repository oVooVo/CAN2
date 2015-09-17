#ifndef PROJECT_H
#define PROJECT_H

#include <QUndoStack>
#include "configurable.h"


class SongDatabase;
class EventDatabase;
class Command;
class Project : public QUndoStack
{
    DECL_CONFIG(Project)
    Q_OBJECT
public:
    Project();
    ~Project();


    SongDatabase* songDatabase() const { return m_songDatabase; }
    EventDatabase* eventDatabase() const { return m_eventDatabase; }
    void setCommandFocalizesAffiliatedView(bool on);

    QString ending() const { return "can"; }

public slots:
    void pushCommand(Command* command);
    void reset();
    void undo();
    void redo();

signals:
    void songDatabaseCommandPushed();
    void eventDatabaseCommandPushed();
    void commandPushed();
    void canCloseChanged( bool );
    void undoStackCleared();

private:
    SongDatabase* m_songDatabase;
    EventDatabase* m_eventDatabase;
    bool m_canClose = true;
    bool m_isSynchronized = false;
    bool m_commandFocalizesAffiliatedView;

public:
    bool canClose() const;
    void setCanClose( bool b );

    friend QDataStream& operator<<(QDataStream& out, const Project& project);
    friend QDataStream& operator>>(QDataStream& in, Project& project);
};

QDataStream& operator<<(QDataStream& out, const Project& project);
QDataStream& operator>>(QDataStream& in, Project& project);

#endif // PROJECT_H
