// -*- c-basic-offset: 4 -*-
/** @file 
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id: $
*
*  This is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this software; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.
*
*  Hereby the author, Ippei UKAI, grant the license of this particular file to
*  be relaxed to the GNU Lesser General Public License as published by the Free
*  Software Foundation; either version 2 of the License, or (at your option)
*  any later version. Please note however that when the file is linked to or
*  compiled with other files in this library, the GNU General Public License as
*  mentioned above is likely to restrict the terms of use further.
*
*/

namespace QtAppBase {


    typedef AppBase::Command<QString> QACommand;


    class QAUndoStack: public QUndoStack
    {
        Q_OBJECT
        
        public:
            virtual ~QAUndoStack() {}
            
        public:
            void push(QACommand* cmd);
            
        protected:
            class QACommandAdaptor: public QUndoCommand
            {
                public:
                    QACommand(QACommand* command, QUndoCommand* parent = NULL);
                    virtual ~QACommand() { delete o_command; }
                    
                public:
                    virtual void undo(); //call QUndoCommand::undo(), then o_command->undo()
                    virtual void redo(); //call o_command->redo(), then QUndoCommand::redo()
                    virtual void setText(const QString& text); //call QUndoCommand::setName(), then o_command->setName()
                    virtual QString text() const; //call o_command->getName()
                    
                protected:
                    QACommand* o_command;
            };

    };


}