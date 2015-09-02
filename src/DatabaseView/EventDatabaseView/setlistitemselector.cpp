#include "setlistitemselector.h"

#include "Project/project.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>

#include "application.h"
#include "DatabaseView/SongDatabaseView/songtableviewcontainer.h"
#include "Database/SongDatabase/songdatabase.h"
#include "DatabaseView/SongDatabaseView/songtableview.h"

SetlistItemSelector::SetlistItemSelector(QWidget *parent) :
    QDialog(parent)
{
    SongTableViewContainer* stc = new SongTableViewContainer( this );
    stc->songTableView()->setSelectionMode( QAbstractItemView::ExtendedSelection );
    stc->setModel( app().project()->songDatabaseProxy() );
    QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
    buttonBox->setStandardButtons( QDialogButtonBox::Close );
    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->addWidget( stc );
    layout->addWidget( buttonBox );
    this->setLayout( layout );
    connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(hide()));
}

SetlistItemSelector::~SetlistItemSelector()
{
}

void SetlistItemSelector::showEvent(QShowEvent *e)
{
    restoreGeometry( QSettings().value("SetlistItemSelector_Geometry").toByteArray() );
    app().project()->setCommandFocalizesAffiliatedView( false );
    QDialog::showEvent(e);
}

void SetlistItemSelector::hideEvent(QHideEvent *e)
{
    QSettings().setValue( "SetlistItemSelector_Geometry", saveGeometry() );
    app().project()->setCommandFocalizesAffiliatedView( true );
    QDialog::hideEvent(e);
}
