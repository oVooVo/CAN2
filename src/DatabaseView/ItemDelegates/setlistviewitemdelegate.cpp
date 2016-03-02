#include "setlistviewitemdelegate.h"
#include "Database/EventDatabase/setlist.h"
#include "Database/EventDatabase/setlist.h"

#include <QLineEdit>
#include <QComboBox>
#include "Database/EventDatabase/event.h"
#include "Database/SongDatabase/songdatabase.h"

#include "util.h"
#include "Commands/SetlistCommands/setlistitemchangesongcommand.h"
#include "Commands/DatabaseCommands/databaseeditcommand.h"

SetlistViewItemDelegate::SetlistViewItemDelegate(QObject *parent) :
    QItemDelegate(parent)
{
}

QWidget* SetlistViewItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_ASSERT(itemFromIndex(index));
    switch (itemFromIndex(index)->attribute("type").value<SetlistItem::Type>())
    {
    case SetlistItem::SongType:
        return new QComboBox(parent);
    case SetlistItem::LabelType:
        return new QLineEdit(parent);
    default:
        assert(false);
        return nullptr;
    }
}

void SetlistViewItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    SetlistItem* item = itemFromIndex(index);
    switch (item->attribute("type").value<SetlistItem::Type>())
    {
    case SetlistItem::SongType:
    {
        QComboBox* comboBox = qobject_assert_cast<QComboBox*>(editor);
        QList<Song*> availableSongs = app().project()->songDatabase()->items();
        QStringList songLabels;
        for (const Song* song : availableSongs)
        {
            songLabels << song->label();
        }
        comboBox->addItems(songLabels);
        comboBox->setEditable(true);
        comboBox->setInsertPolicy(QComboBox::NoInsert);

        int index = availableSongs.indexOf(const_cast<Song*>(item->attribute("song").value<const Song*>()));
        comboBox->setCurrentIndex(index);
        comboBox->lineEdit()->selectAll();
    }
        break;
    case SetlistItem::LabelType:
    {
        QLineEdit* lineEdit = qobject_assert_cast<QLineEdit*>(editor);
        lineEdit->setText(item->attribute("label").toString());
    }
        break;
    }
}

void SetlistViewItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    SetlistItem* item = itemFromIndex(index);
    switch (item->attribute("type").value<SetlistItem::Type>())
    {
    case SetlistItem::SongType:
    {
        QComboBox* comboBox = qobject_assert_cast<QComboBox*>(editor);
        QList<Song*> availableSongs = app().project()->songDatabase()->items();

        int comboBoxIndex = comboBox->currentIndex();
        if (comboBoxIndex >= 0)
        {
            const Song* song = availableSongs[comboBoxIndex];
            if (item->attribute("song").value<const Song*>() != song)
            {
                pushCommand( new SetlistItemChangeSongCommand(item, song));
            }
        }
    }
        break;
    case SetlistItem::LabelType:
    {
        QLineEdit* lineEdit = qobject_assert_cast<QLineEdit*>(editor);
        QVariant newValue = lineEdit->text();
        if (model->data(index) != newValue)
        {
            pushCommand( new DatabaseEditCommand( model, index, newValue) );
        }
    }
        break;
    }
}

SetlistItem* SetlistViewItemDelegate::itemFromIndex(const QModelIndex &index)
{
    return index.model()->data(index, Qt::UserRole).value<SetlistItem*>();
}
