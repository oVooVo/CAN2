#ifndef DATABASESORTPROXY_H
#define DATABASESORTPROXY_H

#include <QSortFilterProxyModel>
#include "global.h"
#include <QBitArray>

class DatabaseBase;
class DatabaseSortProxyBase : public QSortFilterProxyModel
{
    Q_OBJECT
protected:
    DatabaseSortProxyBase(QObject *parent);


public slots:
    virtual void setFilter(const QString& filter);

public:
    QString filter() const;

private:
    QString m_filter;
};

template<typename T> class Database;
template<typename T>
class DatabaseSortProxy : public DatabaseSortProxyBase
{
public:
    explicit DatabaseSortProxy(QObject *parent = 0) :
        DatabaseSortProxyBase(parent)
    {
    }

    Database<T>* sourceModel() const
    {
        return static_cast<Database<T>*>(DatabaseSortProxyBase::sourceModel());
    }

    void setSourceModel(Database<T> *sourceModel)
    {
        DatabaseSortProxyBase::setSourceModel(sourceModel);
    }

    T* itemAtIndex(const QModelIndex& index) const
    {
        assert(index.model() == this);
        if (sourceModel())
        {
            return sourceModel()->itemAtIndex( mapToSource(index) );
        }
        else
        {
            return nullptr;
        }
    }


protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        assert(!source_parent.isValid());
        T* item = sourceModel()->items()[source_row];

        for (const QString& token : item->textAttributes())
        {
            if (token.contains(filter(), Qt::CaseInsensitive))
            {
                return true;
            }
        }
        return false;
    }

};

#endif // DATABASESORTPROXY_H
