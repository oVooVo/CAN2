#include "event.h"
#include "setlist.h"

const QStringList Event::ATTRIBUTE_KEYS = { "type", "beginning", "label", "notices" };

Event::Event( Database<Event>* database, const QDateTime& beginning, Type type, const QString & label) :
    DatabaseItem( ATTRIBUTE_KEYS, database ),
    m_setlist( new Setlist(this) )
{
    setAttribute("beginning", beginning);
    setAttribute("type", type);
    setAttribute("label", label);
}

Event::~Event()
{
    delete m_setlist;
    m_setlist = nullptr;
}

QString Event::description() const
{
    if (attributeDisplay("label").isEmpty())
    {
        return attributeDisplay("type");
    }
    else
    {
        return attributeDisplay("label");
    }
}

QString Event::eventTypeName(Type type)
{
    return eventTypeNames()[static_cast<int>(type)];
}

void Event::serialize(QDataStream &out) const
{
    DatabaseItem::serialize(out);
    out << m_setlist;
}

void Event::deserialize(QDataStream &in)
{
    DatabaseItem::deserialize(in);
    in >> m_setlist;

}

QString Event::label() const
{
    QString label = attributeDisplay("label");
    if (!label.isEmpty())
    {
        label = attributeDisplay("type");
    }
    return QString("%1 (%2)").arg(label, attributeDisplay("beginning"));
}

QStringList Event::eventTypeNames()
{
    return QStringList({ app().translate("Event", "Rehearsal"), app().translate("Event", "Gig"), app().translate("Event", "Other") });
}

QString Event::attributeDisplay(const QString &key) const
{
    QVariant attribute = DatabaseItem::attribute(key);
    if (key == "beginning" || key == "creationDateTime")
    {
        return attribute.toDateTime().toString(preference<QString>("dateTimeFormat"));
        return attribute.toDateTime().toString(preference<QString>("dateTimeFormat"));
    }
    if (key == "type")
    {
        return eventTypeName(attribute.value<Type>());
    }
    if (attribute.isNull())
    {
        return "";
    }
    if (attribute.canConvert<QString>())
    {
        return attribute.toString();
    }
    Q_UNREACHABLE();
    return "";
}

QDataStream& operator <<(QDataStream& out, const Event::Type& type)
{
    out << static_cast<EnumSurrogate_t>(type);
    return out;
}

QDataStream& operator >>(QDataStream& in,        Event::Type& type)
{
    EnumSurrogate_t ftype;
    in >> ftype;
    type = static_cast<Event::Type>(ftype);
    return in;
}






