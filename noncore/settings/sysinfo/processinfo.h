/**********************************************************************
** ProcessInfo
**
** Display process information
**
** Copyright (C) 2002, Dan Williams
**                    williamsdr@acm.org
**                    http://draknor.net
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
**********************************************************************/

#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <qwidget.h>

class Detail;
class QComboBox;
class QListViewItem;
namespace Opie {namespace Ui {class OListView;}}
namespace Opie {namespace Ui {class OListViewItem;}}

class ProcessInfo : public QWidget
{
    Q_OBJECT
public:
    ProcessInfo( QWidget *parent = 0, const char *name = 0, WFlags f = 0 );
    ~ProcessInfo();

private:
    Opie::Ui::OListView*   ProcessView;
    QComboBox*   SignalCB;
    QPushButton* SendButton;

	Detail *ProcessDtl;

private slots:
    void updateData();
    void slotSendClicked();
    void viewProcess( QListViewItem* );
    void viewProcess( Opie::Ui::OListViewItem * );
};

#endif
