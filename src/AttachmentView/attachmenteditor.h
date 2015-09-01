#ifndef ATTACHMENTEDITOR_H
#define ATTACHMENTEDITOR_H

#include <QWidget>
#include <QMap>

class QScrollArea;
class Attachment;
class AttachmentView;
class AttachmentEditor : public QWidget
{
    Q_OBJECT
public:
    explicit AttachmentEditor(QWidget *parent = 0);
    void setAttachment(Attachment* attachment);

public slots:
    void updateAttachmentView();

private:
    AttachmentView* m_currentView = 0;
    QScrollArea* m_scrollArea = 0;
    QMap<Attachment*, QByteArray> m_attachmentViewOptions;

signals:
    void focusAttachment( const Attachment* );

};

#endif // ATTACHMENTEDITOR_H
