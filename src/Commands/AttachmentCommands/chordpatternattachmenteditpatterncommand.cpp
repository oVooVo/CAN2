#include "chordpatternattachmenteditpatterncommand.h"
#include "global.h"

//TODO merge commands.
// do not merge over whitespace
// or timeouts
ChordPatternAttachmentEditPatternCommand::ChordPatternAttachmentEditPatternCommand( ChordPatternAttachment* attachment, const QString & pattern ) :
    AttachmentCommand(attachment),
    m_oldPattern( attachment->chordPattern() ),
    m_newPattern( pattern )
{
    setText( CommandTranslator::tr("Edit Chord Pattern") );
}

void ChordPatternAttachmentEditPatternCommand::undo()
{
    model()->setPattern( m_oldPattern );
}

void ChordPatternAttachmentEditPatternCommand::redo()
{
    model()->setPattern( m_newPattern );
}
