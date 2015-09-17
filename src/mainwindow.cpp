#include "mainwindow.h"

#include <QTimer>
#include <QToolButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QProcess>
#include <QClipboard>
#include <QProgressDialog>
#include <QScrollArea>
#include <QLabel>
#include <QThread>

#include "ui_mainwindow.h"
#include "util.h"
#include "application.h"
#include "Dialogs/addfilestoindexdialog.h"
#include "Dialogs/stringdialog.h"
#include "Dialogs/configurationdialog.h"
#include "Dialogs/programdialog.h"
#include "Dialogs/tagdialog.h"
#include "Commands/DatabaseCommands/databasenewitemcommand.h"
#include "Commands/DatabaseCommands/databaseeditcommand.h"
#include "Commands/DatabaseCommands/databaseremoveitemcommand.h"
#include "Commands/SongCommands/songremoveattachmentcommand.h"
#include "Commands/SongCommands/songnewattachmentcommand.h"
#include "Commands/edittagscommand.h"
#include "Commands/AttachmentCommands/attachmentrenamecommand.h"
#include "Commands/SongCommands/songeditprogramcommand.h"
#include "AttachmentView/attachmentchooser.h"
#include "Attachments/attachment.h"
#include "Database/EventDatabase/setlistitem.h"
#include "Database/SongDatabase/songdatabase.h"
#include "Database/EventDatabase/eventdatabase.h"
#include "DatabaseView/EventDatabaseView/eventtableview.h"
#include "DatabaseView/SongDatabaseView/songtableview.h"
#include "creatable.h"
#include "DatabaseView/SongDatabaseView/songtableview.h"
#include "PDFCreator/orphantsetlist.h"
#include "PDFCreator/pdfcreator.h"
#include "DatabaseView/EventDatabaseView/setlistview.h"

DEFN_CONFIG( MainWindow, "Global" );

QString defaultStyleSheet()
{
    QFile file(":/style/styles/stylesheet.qss");

    assert( file.open(QIODevice::ReadOnly ));
    return file.readAll();
}

CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, RecentProject, "");
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, RecentCloneURL, QDir::homePath());
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, FileIndexFilter, "");
CONFIGURABLE_ADD_ITEM_HIDDEN( MainWindow, locale, QLocale::system().name().toStdString().c_str());
CONFIGURABLE_ADD_ITEM( MainWindow,
                       Style,
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Style"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "ConfigurableItem/MainWindow/Style"),
                       QVariant(),  // is set in MainWindow constructor since ::defaultStyleSheet() is not available in static context.
                       ConfigurableItemOptions::TextEditOptions( QT_TRANSLATE_NOOP( "ConfigurableItem", "Stylesheet") )
                       );
CONFIGURABLE_ADD_ITEM( MainWindow,
                       AskForCommitMessage,
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Ask for commit message"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "ConfigurableItem/MainWindow/AskForCommitMessage"),
                       QVariant(true),
                       ConfigurableItemOptions::CheckboxOptions()
                       );
CONFIGURABLE_ADD_ITEM( MainWindow,
                       CommitMessage,
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Commit message"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "ConfigurableItem/MainWindow/CommitMessage"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Synchronizing"),
                       ConfigurableItemOptions::TextEditOptions( QT_TRANSLATE_NOOP( "ConfigurableItem", "commit message") )
                       );
CONFIGURABLE_ADD_ITEM( MainWindow,
                       AskForSynchronizing,
                       QT_TRANSLATE_NOOP("ConfigurableItem", "Remind synchronizing"),
                       QT_TRANSLATE_NOOP("ConfigurableItem", "ConfigurableItem/MainWindow/AskForSynchronizing"),
                       true,
                       ConfigurableItemOptions::CheckboxOptions()
                       );

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    setWindowIcon( QIcon(":/icons/icons/01-elephant-icon.png") );
    app().setMainWindow( this );
    app().setProject( &m_project );
    createAttachmentActions();

    ui->setupUi(this);
    createLanguageMenu();

    QSettings settings;
    restoreGeometry( settings.value("Geometry").toByteArray() );

    //     load stylesheet
    config.item("Style")->setDefaultValue( defaultStyleSheet() );
    connect( config.item("Style"), &ConfigurableItem::valueChanged, [this](QVariant value)
    {
        app().setStyleSheet( value.toString() );
    });
    app().setStyleSheet( config.value( "Style" ).toString() );

    //////////////////////////////////////////
    /// restore Configurable
    //////////////////////////////////////////
    /// is done implicitly when Configurable is loaded


    menuBar()->setNativeMenuBar(false);
    setupAttachmentMenu();

    //////////////////////////////////////////
    /// Undo/Redo
    //////////////////////////////////////////
    connect( &m_project, SIGNAL(canUndoChanged(bool)), ui->actionUndo, SLOT(setEnabled(bool)));
    connect( &m_project, SIGNAL(canRedoChanged(bool)), ui->actionRedo, SLOT(setEnabled(bool)));

    auto action = [this]()
    {
        const QUndoCommand* nextUndoCommand = m_project.command( m_project.QUndoStack::index() - 1);
        const QUndoCommand* nextRedoCommand = m_project.command( m_project.QUndoStack::index() );

        QString undoText = tr("&Undo %1").arg( nextUndoCommand ? nextUndoCommand->actionText() : "" );
        QString redoText = tr("&Redo %1").arg( nextRedoCommand ? nextRedoCommand->actionText() : "" );

        undoAction()->setText( undoText );
        redoAction()->setText( redoText );
    };

    connect( &m_project, &Project::commandPushed, action );
    connect( &m_project, &Project::undoStackCleared, [this]()
    {
        undoAction()->setText( tr("&Undo") );
        redoAction()->setText( tr("&Redo") );
    });
    undoAction()->setText( tr("&Undo") );
    redoAction()->setText( tr("&Redo") );
    ui->actionUndo->setEnabled( m_project.canUndo() );
    ui->actionRedo->setEnabled( m_project.canRedo() );

    // very important to set associated widget. Else, shortcuts would be ambigous.
    initAction( actionNew_Song,         ui->songDatabaseWidget->databaseView(),    tr("&New Song"),       tr("Add a new song."),        "Ctrl+N",   ui->menuSongs,  "" )
    initAction( actionDelete_Song,      ui->songDatabaseWidget->databaseView(),    tr("&Remove Song"),    tr("Remove selected song."),  "Del",      ui->menuSongs,  ":/icons/icons/rubbish7.png" )
    initAction( actionCopy_Song,        ui->songDatabaseWidget->databaseView(),    tr("&Copy Song"),      tr("Copy selected song."),    "Ctrl+C",   ui->menuSongs,  "" )
    initAction( actionPaste_Song,       ui->songDatabaseWidget->databaseView(),    tr("&Paste Song"),     tr("Paste song."),            "Ctrl+V",   ui->menuSongs,  "" )
    initAction( actionEdit_Program,     ui->songDatabaseWidget->databaseView(),    tr("&Edit Program"),   tr("Edit program."),          "",         ui->menuSongs,  "" )
    initAction( actionEdit_Song_Tags,   ui->songDatabaseWidget->databaseView(),    tr("&Edit Tags"),      tr("Edit tags of the song."), "",         ui->menuSongs,  ":/icons/icons/tag-2.png" )

    initAction( actionNew_Event,        ui->eventDatabaseWidget->databaseView(),  tr("&New Event"),      tr("Add a new event."),       "Ctrl+N",   ui->menuEvents, "" )
    initAction( actionDelete_Event,     ui->eventDatabaseWidget->databaseView(),  tr("&Remove Event"),   tr("Remove selected event."), "Del",      ui->menuEvents, ":/icons/icons/rubbish7.png" )
    initAction( actionCopy_Event,       ui->eventDatabaseWidget->databaseView(),  tr("&Copy Event"),     tr("Copy selected event."),   "Ctrl+C",   ui->menuEvents, "" )
    initAction( actionPaste_Event,      ui->eventDatabaseWidget->databaseView(),  tr("&Paste Event"),    tr("Paste event."),           "Ctrl+V",   ui->menuEvents, "" )
    initAction( actionEdit_Event_Tags,  ui->eventDatabaseWidget->databaseView(),  tr("&Edit Tags"),      tr("Edit tags of the event."),"",         ui->menuEvents, ":/icons/icons/tag-2.png" )

    initAction( actionNew_SetlistItem,    ui->eventDatabaseWidget->setlistView(),   tr("&New Item"),       tr("Insert new item"),        "Ctrl+N",   NULL, "" )
    initAction( actionDelete_SetlistItem, ui->eventDatabaseWidget->setlistView(),   tr("&Remove Item"),    tr("Delete selected items"),  "Del",      NULL, "" )
    initAction( actionCopy_SetlistItem,   ui->eventDatabaseWidget->setlistView(),   tr("&Copy Items"),     tr("Copy selected items"),    "Ctrl+C",   NULL, "" )
    initAction( actionPaste_SetlistItem,  ui->eventDatabaseWidget->setlistView(),   tr("&Paste Items"),    tr("Paste items"),            "Ctrl+V",   NULL, "" )


    //////////////////////////////////////////
    /// Splitter
    //////////////////////////////////////////
    connect( ui->splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(resizeSplitter()));
    QTimer::singleShot(10, this, SLOT(resizeSplitter()));


    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( m_project.songDatabase(), &SongDatabase::attachmentAdded, [this](int i)
    {
        setCurrentAttachment( i );
    });
    connect( m_project.songDatabase(), &SongDatabase::attachmentRemoved, [this](int i)
    {
        setCurrentAttachment( i );
    });
    connect( m_project.songDatabase(), &SongDatabase::attachmentRenamed, [this](int i, QString)
    {
        setCurrentAttachment( i );
    });
    connect( m_project.songDatabase(), &SongDatabase::dataChanged, [this]()
    {
        ui->songDatabaseWidget->attachmentChooser()->updateAttachmentView();
    });

    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( &m_project, SIGNAL(canCloseChanged(bool)), this, SLOT(updateWindowTitle()) );
    updateWindowTitle();
    connect( ui->eventDatabaseWidget->databaseView(), SIGNAL(clicked()), this, SLOT(updateActionsEnabled()) );
    connect( ui->eventDatabaseWidget->setlistView(),    SIGNAL(clicked()), this, SLOT(updateActionsEnabled()) );
    connect( ui->songDatabaseWidget->databaseView(),   SIGNAL(clicked()), this, SLOT(updateActionsEnabled()) );
    updateActionsEnabled();

    loadDefaultProject();

    connect( &m_project, SIGNAL(songDatabaseCommandPushed()), this, SLOT(gotoSongView()) );
    connect( &m_project, SIGNAL(eventDatabaseCommandPushed()), this, SLOT(gotoEventView()) );
    connect( m_project.songDatabase(), SIGNAL(songRemoved(int)), ui->songDatabaseWidget, SLOT(updateAttachmentChooser()) );

    //////////////////////////////////////////
    ///
    //////////////////////////////////////////
    connect( ui->buttonSongDatabase, &QPushButton::clicked, [this]()
    {
        selectPage( SongDatabasePage );
    });
    connect( ui->buttonEventDatabase, &QPushButton::clicked, [this]()
    {
        selectPage( EventDatabasePage );
    });
    selectPage( SongDatabasePage );

}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue( "Geometry", saveGeometry() );
    delete ui;
}

void MainWindow::resizeSplitter()
{
    if (ui->splitter->sizes()[0] != 0)
    {
        ui->splitter->setSizes(QList<int>() << 1 << 10000000);
    }
}

Song* MainWindow::currentSong() const
{
    return ui->songDatabaseWidget->currentItem();
}

Event* MainWindow::currentEvent() const
{
    return ui->eventDatabaseWidget->currentItem();
}

void MainWindow::createAttachmentActions()
{
    // gather attachment creators
    for (const QString & classname : Creatable::classnamesInCategory("Attachment"))
    {
        QAction* action = new QAction( this );
        action->setText( QString(tr("New %1")).arg( Creatable::name(classname) ) );

        if (classname == "PDFAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/pdf24.png"));
        }
        else if (classname == "ChordPatternAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/write13.png"));
        }
        else if (classname == "AudioAttachment")
        {
            action->setIcon(QIcon(":/icons/icons/song1.png"));
        }
        else if (classname == "ChordPatternProxyAttachment")
        {
            action->setIcon( QIcon(":/icons/icons/link67.png") );
        }
        else
        {
            qWarning() << QString("action <create %1> has no icon.").arg( classname );
        }

        connect(action, &QAction::triggered, [this, classname]()
        {
            Song* song = currentSong();
            if (song)
            {
                SongNewAttachmentCommand* command = new SongNewAttachmentCommand( song, Creatable::create<Attachment>(classname) );
                app().pushCommand( command );
                updateActionsEnabled();
            }
        });
        m_newAttachmentActions.insert( classname, action );
    }
}

void MainWindow::setupAttachmentMenu()
{
    // gather attachment creators
    QAction* actionBefore = ui->menuAttachments->actions().first();
    for (const QString & classname : Creatable::classnamesInCategory("Attachment"))
    {
        QAction* action = newAttachment_Action( classname );
        ui->menuAttachments->insertAction(actionBefore, action );
    }
}

void MainWindow::setCurrentAttachment( int index )
{
    ui->songDatabaseWidget->attachmentChooser()->setAttachment( index );
}


//////////////////////////////////////////////////
////
///     Saving and loading stuff
//
////////////////////////////////////////////////


QString MainWindow::projectName() const
{
    QString filename = QFileInfo(m_currentPath).fileName();
    if (filename.isEmpty())
    {
        return tr("Unnamed");
    }
    else
    {
        return filename;
    }
}

bool MainWindow::saveProject()
{
    if (m_currentPath.isEmpty())
    {
        return saveProjectAs();
    }
    else
    {
        QFile file(m_currentPath);
        if (!file.open(QIODevice::WriteOnly))
        {
            return false;
        }
        QDataStream stream(&file);
        stream << m_project;
        m_project.setCanClose(true);


        setCurrentPath( m_currentPath );   // ensure that the current filename is stored as to-default-open project
        updateWindowTitle();
        return true;
    }
}

void MainWindow::setCurrentPath(const QString &path)
{
    m_currentPath = path;
    config.set( "RecentProject", m_currentPath );
}

QString MainWindow::proposedPath() const
{
    QString filename = projectName();
    QString ending = QString(".%1").arg(m_project.ending());

    if (!filename.endsWith(ending))
    {
        filename = filename.append(ending);
    }

    if (m_currentPath.isEmpty())
    {
        return QDir::home().absoluteFilePath( filename );
    }
    else
    {
        return m_currentPath;
    }
}

bool MainWindow::saveProjectAs()
{
    QString filename =
    QFileDialog::getSaveFileName( this,
                                  tr("Save As ..."),
                                  proposedPath(),
                                  filter()          );
    if (filename.isEmpty())
    {
        return false;
    }
    else
    {
        QString ending = QString(".%1").arg(m_project.ending());
        if (!filename.endsWith( ending ))
        {
            filename.append(ending);
        }
        setCurrentPath(filename);
        return saveProject();
    }
}

bool MainWindow::newProject()
{
    if (!canProjectClose())
    {
        return false;
    }

    m_project.reset();
    setCurrentPath( QString() );
    updateWindowTitle();
    return true;
}

bool MainWindow::canProjectClose()
{
    if (m_project.canClose())
    {
        return true;
    }
    else
    {
        switch (
        QMessageBox::question( this,
                               tr("Really close %1").arg( projectName() ),
                               tr("There are unsaved changes."),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Abort,
                               QMessageBox::Abort ) )
        {
        case QMessageBox::Save:
            return saveProject();
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Abort:
            return false;
        default:
            WARNING << "Illegal case in switch statement";
            return false;
        }
    }
}

void MainWindow::updateWindowTitle()
{
    QString star = m_project.canClose() ? "" : "*";

    QString title = QString("%1%2 - %4")
            .arg( projectName() )
            .arg( star )
            .arg( qApp->applicationName() );

    setWindowTitle( title );
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (canProjectClose())
    {
        QMainWindow::closeEvent(e);
        e->accept();
    }
    else
    {
        e->ignore();
    }
}

void MainWindow::loadDefaultProject()
{
    m_currentPath = config.value( "RecentProject" ).toString();
    open( m_currentPath );
}

int MainWindow::currentAttachmentIndex() const
{
    return ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();
}

Attachment* MainWindow::currentAttachment() const
{
    return ui->songDatabaseWidget->attachmentChooser()->currentAttachment();
}

void setEnabled( QObject* o, bool enable )
{
    QAction* a = qobject_cast<QAction*>(o);
    if (a) a->setEnabled(enable);

    QWidget* w = qobject_cast<QWidget*>(o);
    if (w) w->setEnabled(enable);
}

void MainWindow::updateActionsEnabled()
{
    bool song = !!currentSong();
    bool event = !!currentEvent();
    bool attachment = !!currentAttachment();
    bool setlist = !ui->eventDatabaseWidget->setlistView()->selectionModel()->selectedRows().isEmpty();

    QList<QAction*> attachmentActions, songActions, gitActions, eventActions, setlistActions;

    for (QAction* action : m_newAttachmentActions)
    {
        songActions << action;
    }

    gitActions          << ui->actionSync;
    attachmentActions   << ui->actionDuplicate_Attachment << ui->actionRename_Attachment << ui->actionDelete_Attachment;
    songActions         << m_actionDelete_Song << m_actionCopy_Song << m_actionEdit_Program << m_actionEdit_Song_Tags;
    eventActions        << m_actionDelete_Event << m_actionCopy_Event << m_actionEdit_Event_Tags;
    setlistActions      << m_actionDelete_SetlistItem << m_actionCopy_SetlistItem;

    for (QAction* o : eventActions )
    {
        o->setEnabled(event);
    }
    for (QAction* o : songActions )
    {
        o->setEnabled(song);
    }
    for (QAction* o : attachmentActions )
    {
        o->setEnabled(attachment);
    }
    for (QAction* o : setlistActions )
    {
        o->setEnabled(setlist);
    }

    bool chordPatternProxyAttachmentEnabled = false;
    if (currentAttachment() && currentAttachment()->inherits( "ChordPatternAttachment" ))
    {
        chordPatternProxyAttachmentEnabled = true;
    }
    m_newAttachmentActions["ChordPatternProxyAttachment"]->setEnabled( chordPatternProxyAttachmentEnabled );

    {
        const QMimeData* clipboard = app().clipboard()->mimeData();
        m_actionPaste_Event->setEnabled( clipboard->hasFormat(DatabaseMimeData<Event>::mimeType()) );
        m_actionPaste_SetlistItem->setEnabled( clipboard->hasFormat(DatabaseMimeData<SetlistItem>::mimeType())) ;
        m_actionPaste_Song->setEnabled( clipboard->hasFormat(DatabaseMimeData<Song>::mimeType()) );
    }
}

MainWindow::Page MainWindow::currentPage() const
{
    switch (ui->stackedWidget->currentIndex())
    {
    case 0:
        return SongDatabasePage;
    case 1:
        return EventDatabasePage;
    default:
        assert( false );
        return (Page) -1;
    }
}

void MainWindow::gotoSongView()
{
    selectPage( SongDatabasePage );
}

void MainWindow::gotoEventView()
{
    selectPage( EventDatabasePage );
}

////////////////////////////////////////////////
////
///     Connections to Actions
//
///////////////////////////////////////////////


void MainWindow::my_on_actionNew_Song_triggered()
{
    app().pushCommand( new DatabaseNewItemCommand<Song>( m_project.songDatabase(), new Song(m_project.songDatabase()) ) );
    updateActionsEnabled();
}

void MainWindow::on_actionUndo_triggered()
{
    m_project.undo();
    updateActionsEnabled();
}

void MainWindow::on_actionRedo_triggered()
{
    m_project.redo();
    updateActionsEnabled();
}

void MainWindow::on_actionDelete_Attachment_triggered()
{
    Song* song = currentSong();
    int index = ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();

    if (song && index >= 0)
    {
        app().pushCommand( new SongRemoveAttachmentCommand( song, index ) );
        updateActionsEnabled();
    }
}

void MainWindow::on_actionNew_Project_triggered()
{
    newProject();
    updateActionsEnabled();
}

void MainWindow::on_actionSave_triggered()
{
    saveProject();
    updateActionsEnabled();
}

void MainWindow::on_actionSave_As_triggered()
{
    saveProjectAs();
    updateActionsEnabled();
}

void MainWindow::on_actionOpen_triggered()
{
    if (!canProjectClose())
    {
        return; // user aborted opening;
    }

    QString filename =
    QFileDialog::getOpenFileName( this,
                                  tr("Open ..."),
                                  proposedPath(),
                                  filter()              );
    if (filename.isEmpty())
    {
        return; // user aborted opening
    }


    open(filename);
}

bool MainWindow::canRemoveSong(Song *song)
{
    // check if song is used in setlist
    for (Event* event: m_project.eventDatabase()->items())
    {
        for (SetlistItem* item : event->setlist()->items())
        {
            if (item->song() == song)
            {
                return false;
            }
        }
    }

    return true;
}

void MainWindow::my_on_actionDelete_Song_triggered()
{
    Song* song = currentSong();
    if (song)
    {
        if (canRemoveSong( song ))
        {
            app().pushCommand( new DatabaseRemoveItemCommand<Song>( m_project.songDatabase(), song ) );
        }
        else
        {
            QMessageBox::warning( this,
                                  tr("Song cannot be removed"),
                                  tr("This song is currently in use and can thus not be removed."),
                                  QMessageBox::Ok,
                                  QMessageBox::NoButton );
        }
        updateActionsEnabled();
    }
}

void MainWindow::on_actionAdd_Folder_triggered()
{
    AddFilesToIndexDialog dialog( this );

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QProgressDialog pd( "Task in Progress", "Cancel", 0, -1, this );
    pd.setWindowModality( Qt::WindowModal );

    app().fileIndex().addSource( dialog.path(), dialog.acceptedEndings());

    QLabel* label = new QLabel(&pd);
    label->setWordWrap(true);
    pd.setLabel(label);
    pd.show();

    while (!app().fileIndex().operationIsFinished())
    {
        pd.setValue( (pd.value() + 1) % 100 );
        label->setText( QString("%1\n%2").arg(app().fileIndex().currentFilename())
                                         .arg(app().fileIndex().size())            );
        qApp->processEvents();
        QThread::msleep( 10 );
        if (pd.wasCanceled())
        {
            app().fileIndex().abortOperations();
        }
    }

}

void MainWindow::on_actionClear_Index_triggered()
{
    app().fileIndex().clear();
}

void MainWindow::on_actionRename_Attachment_triggered()
{
    ui->songDatabaseWidget->attachmentChooser()->renameCurrentAttachment();
}

void MainWindow::on_actionDuplicate_Attachment_triggered()
{
    Song* cs = currentSong();
    assert( cs );

    int index = ui->songDatabaseWidget->attachmentChooser()->currentAttachmentIndex();
    assert( index >= 0 );

    Attachment* attachment = cs->attachments()[index];
    assert( attachment );

    app().pushCommand( new SongNewAttachmentCommand( cs, attachment->copy() ) );
    updateActionsEnabled();
}


void MainWindow::on_actionSettings_triggered()
{
    ConfigurationDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_action_Index_Info_triggered()
{
    QMessageBox::information( this,
                              tr("File index information"),
                              QString(tr("Files: %1")).arg(app().fileIndex().size()) );
}

void MainWindow::my_on_actionNew_Event_triggered()
{
    app().pushCommand( new DatabaseNewItemCommand<Event>( m_project.eventDatabase(), new Event(m_project.eventDatabase())) );
}

void MainWindow::my_on_actionDelete_Event_triggered()
{
    Event* event = currentEvent();
    if (event)
    {
        app().pushCommand( new DatabaseRemoveItemCommand<Event>( m_project.eventDatabase(), event ));
        updateActionsEnabled();
    }
}


QAction* MainWindow::undoAction() const
{
    return ui->actionUndo;
}

QAction* MainWindow::redoAction() const
{
    return ui->actionRedo;
}

QAction* MainWindow::newAttachment_Action( const QString& classname )
{
    if (m_newAttachmentActions.contains(classname))
    {
        return m_newAttachmentActions[classname];
    }
    else
    {
        qWarning() << "Attachment " << classname << "not known." << m_newAttachmentActions.keys();
        return NULL;
    }
}

void MainWindow::my_on_actionCopy_Song_triggered()
{
    QModelIndexList selectedSongs = ui->songDatabaseWidget->databaseView()->selectionModel()->selectedIndexes();
    app().clipboard()->setMimeData( m_project.songDatabase()->mimeData( selectedSongs ) );
}

void MainWindow::my_on_actionPaste_Song_triggered()
{
    app().project()->songDatabase()->dropMimeData(app().clipboard()->mimeData(), Qt::CopyAction, m_project.songDatabase()->rowCount(), 0, QModelIndex());
}

void MainWindow::my_on_actionCopy_Event_triggered()
{
    QModelIndexList selectedSongs = ui->eventDatabaseWidget->databaseView()->selectionModel()->selectedIndexes();
    app().clipboard()->setMimeData( m_project.eventDatabase()->mimeData( selectedSongs ) );
}

void MainWindow::my_on_actionPaste_Event_triggered()
{
    app().project()->eventDatabase()->dropMimeData(app().clipboard()->mimeData(), Qt::CopyAction, m_project.eventDatabase()->rowCount(), 0, QModelIndex());
}

void MainWindow::my_on_actionEdit_Program_triggered()
{
    if (!currentSong())
    {
        return;
    }

    ProgramDialog pd(this);
    pd.setProgram( currentSong()->program() );
    if (pd.exec() == QDialog::Accepted)
    {
        app().pushCommand( new SongEditProgramCommand( currentSong(), pd.program() ) );
    }
}


void MainWindow::my_on_actionEdit_Song_Tags_triggered()
{
    DatabaseView<Song>* songTableView = ui->songDatabaseWidget->databaseView();
    QModelIndexList list = songTableView->selectionModel()->selectedIndexes();
    if (!list.isEmpty())
    {
        Taggable* t = songTableView->itemAtIndex(list.first());
        if (t != 0)
        {
            TagDialog d(t->tags(), this);
            if (d.exec() == QDialog::Accepted)
            {
                app().pushCommand( new EditTagsCommand( t, d.tags() ) );
            }
        }
    }
}

void MainWindow::my_on_actionEdit_Event_Tags_triggered()
{
    DatabaseView<Event>* eventTableView = ui->eventDatabaseWidget->databaseView();
    QModelIndexList list = eventTableView->selectionModel()->selectedIndexes();
    if (!list.isEmpty())
    {
        Taggable* t = eventTableView->itemAtIndex(list.first());
        if (t != 0)
        {
            TagDialog d(t->tags(), this);
            if (d.exec() == QDialog::Accepted)
            {
                app().pushCommand( new EditTagsCommand( t, d.tags() ) );
            }
        }
    }
}

void MainWindow::open(const QString &filename)
{
    if (!filename.isEmpty())
    {
        if (QFileInfo(filename).isReadable())
        {
            QFile file(filename);
            if (file.open(QIODevice::ReadOnly))
            {
                QDataStream stream(&file);
                stream >> m_project;
                setCurrentPath(filename);
            }
            else
            {
                QMessageBox::warning( this,
                                      QString(tr("Opening %1")).arg(filename),
                                      QString(tr("Cannot open %1. Unknown file format.")).arg(filename),
                                      QMessageBox::Ok
                                      );
                setCurrentPath("");
                newProject();
            }
        }
        else
        {
            QMessageBox::warning( this,
                                  QString(tr("Opening %1")).arg(filename),
                                  QString(tr("File %1 not found.")).arg(filename),
                                  QMessageBox::Ok
                                  );
            m_project.reset();
            setCurrentPath("");
            newProject();
        }

        updateWindowTitle();
        updateActionsEnabled();
    }
}

void MainWindow::createLanguageMenu()
{
    for (const QFileInfo& fileInfo : QDir(":/translations/").entryInfoList(QStringList() << "can2*.qm", QDir::Files, QDir::Name ))
    {
        QString locale = fileInfo.baseName().mid(5); // skip ending, skip can2_
        QAction* action = new QAction( ui->menu_Language );
        action->setText( QLocale(locale).nativeLanguageName() );
        ui->menu_Language->addAction(action);
        action->setCheckable( true );
        connect(ui->menu_Language, SIGNAL(aboutToHide()), action, SLOT(deleteLater()));
        if (config["locale"] == locale)
        {
            action->setChecked(true);
        }

        connect( action, &QAction::triggered, [this, locale]()
        {
            QMessageBox::information( this,
                                      tr("Information"),
                                      tr("Language changes will apply on next start."),
                                      QMessageBox::Ok,
                                      QMessageBox::NoButton );
            config.set("locale", locale);
        });
    }
}

DatabaseView<Song>* MainWindow::songTableView()
{
    return ui->songDatabaseWidget->databaseView();
}


void MainWindow::on_action_Export_all_songs_triggered()
{
    OrphantSetlist setlist( tr("All songs"));

    for( Song* song : m_project.songDatabase()->items() )
    {
        setlist.insertItem( new SetlistItem( &setlist, song ) );
    }

    PDFCreator::exportSetlist( &setlist, this );
}

void MainWindow::selectPage(Page page)
{
    switch (page)
    {
    case SongDatabasePage:
        ui->stackedWidget->setCurrentIndex( 0 );
        break;
    case EventDatabasePage:
        ui->stackedWidget->setCurrentIndex( 1 );
        break;
    }
}

void MainWindow::my_on_actionNew_SetlistItem_triggered()
{
    if (currentEvent())
    {
        Setlist* setlist = currentEvent()->setlist();
        app().pushCommand( new DatabaseNewItemCommand<SetlistItem>( setlist, new SetlistItem(setlist) ) );
    }
}

void MainWindow::my_on_actionDelete_SetlistItem_triggered()
{
    QList<SetlistItem*> si = ui->eventDatabaseWidget->setlistView()->selectedItems();
    if (currentEvent() && !si.isEmpty())
    {
        Setlist* setlist = currentEvent()->setlist();
        app().project()->beginMacro( tr("Remove Setlist Items"));
        for (SetlistItem* i : si)
        {
            app().pushCommand( new DatabaseRemoveItemCommand<SetlistItem>( setlist, i ) );
        }
        app().project()->endMacro();
    }
}

void MainWindow::my_on_actionCopy_SetlistItem_triggered()
{
    if (currentEvent())
    {
        QModelIndexList selection = ui->eventDatabaseWidget->setlistView()->selectionModel()->selectedRows();
        app().clipboard()->setMimeData( currentEvent()->setlist()->mimeData( selection ) );
    }
}

void MainWindow::my_on_actionPaste_SetlistItem_triggered()
{
    if (currentEvent())
    {
        Setlist* setlist = currentEvent()->setlist();
        int row = setlist->rowCount();
        QModelIndexList selection = ui->eventDatabaseWidget->setlistView()->selectionModel()->selectedRows();
        if (!selection.isEmpty())
        {
            row = selection.last().row() + 1;
        }
        setlist->dropMimeData(app().clipboard()->mimeData(), Qt::CopyAction, row, 0, QModelIndex());
    }
}


























