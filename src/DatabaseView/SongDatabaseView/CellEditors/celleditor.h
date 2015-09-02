#ifndef CELLEDITOR_H
#define CELLEDITOR_H

#include <QWidget>
#include <QStyleOption>

#include "creatable.h"

template<typename T> class DatabaseSortProxy;
class Song;
class CellEditor : public QWidget, public Creatable
{
    Q_OBJECT
public:
    explicit CellEditor(QWidget *parent = 0);

    void setStyleOption( QStyleOptionViewItem option );
    void setIndex(const QModelIndex & index);
    void setModel(DatabaseSortProxy<Song> *songDatabase );
    void setCurrentData( const QVariant & data );
    QVariant currentData() const { return m_currentData; }
    virtual void polish() = 0;
    virtual QVariant editedData() const = 0;

protected:
    QStyleOptionViewItem styleOption() const { return m_styleOption; }
    QModelIndex index() const { return m_index; }
    DatabaseSortProxy<Song>* model() const { return m_model; }
    void setSolitaryWidget(QWidget* widget);
    bool eventFilter(QObject *, QEvent *);

private:
    QStyleOptionViewItem m_styleOption;
    QModelIndex m_index;
    DatabaseSortProxy<Song>* m_model;
    QVariant m_currentData;
    QWidget* m_solitaryWidget = 0;



};

#endif // CELLEDITOR_H
