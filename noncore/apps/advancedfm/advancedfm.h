/***************************************************************************
   advancedfm.h
                             -------------------
                             ** Created: Sat Mar 9 23:33:09 2002
    copyright            : (C) 2002 by ljp
    email                : ljp@llornkcor.com
    *   This program is free software; you can redistribute it and/or modify  *
    *   it under the terms of the GNU General Public License as published by  *
    *   the Free Software Foundation; either version 2 of the License, or     *
    *   (at your option) any later version.                                   *
    ***************************************************************************/
#ifndef ADVANCEDFM_H
#define ADVANCEDFM_H
#define QTOPIA_INTERNAL_FSLP // to get access to fileproperties
#define QT_QWS_OPIE

#include <opie2/oprocess.h>
#include <opie2/osplitter.h>

#include <qpe/ir.h>
#include <qpe/qcopenvelope_qws.h>

#include <qvariant.h>
#include <qdialog.h>
#include <qmainwindow.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qstring.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qpixmap.h>


class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QComboBox;
class QListView;
class QListviewItem;
class QLabel;
class QProgressBar;
class QSpinBox;
class QWidget;
class QPopupMenu;
class QFile;
class QListViewItem;
class QLineEdit;
class MenuButton;

class QToolButton;
class Ir;

class AdvancedFm : public QMainWindow
{
    Q_OBJECT
public:
    static QString appName() { return QString::fromLatin1("advancedfm"); }
    AdvancedFm(QWidget *p = 0, const char* name = 0, WFlags fl = 0);
protected:

    Opie::Ui::OSplitter *TabWidget;
    QCopChannel * channel;
    QPixmap unknownXpm;
    int whichTab;
    QWidget *tab, *tab_2, *tab_3;
    QListView *Local_View, *Remote_View;

    QLineEdit *currentPathEdit;
    QPopupMenu *fileMenu, *localMenu, *remoteMenu, *viewMenu;
    QToolButton  *homeButton;
    QToolButton *docButton;
    QToolButton *cdUpButton;
    QToolButton *fsButton;
    QToolButton *qpeDirButton;
    QDir currentDir, currentRemoteDir;
    QComboBox *currentPathCombo;
    QString filterStr, s_addBookmark, s_removeBookmark;
    QListViewItem * item;
    bool b;
    QStringList fileSystemTypeList, fsList;
    int currentServerConfig;
    QGridLayout *tabLayout, *tabLayout_2, *tabLayout_3;
    QStringList remoteDirPathStringList, localDirPathStringList;
    QLineEdit *renameBox;

    void init();
    void initConnections();
    void keyReleaseEvent( QKeyEvent *);
    void keyPressEvent( QKeyEvent *);
    QString getFileSystemType(const QString &);
    QString getDiskSpace(const QString &);
    void parsetab(const QString &fileName);
    QString checkDiskSpace(const QString &);
    QString dealWithSymName(const QString &);
    QString getSelectedFile();
    QDir *CurrentDir();
    QDir *OtherDir();
    QListView *CurrentView();
    QListView *OtherView();
    void setOtherTabCurrent();

protected slots:
    void changeTo(const QString &);
    void selectAll();
    void addToDocs();
    void doDirChange();
    void mkDir();
    void del();
    void rn();
    void populateView();
    void rePopulate();
    void showMenuHidden();
    void ListClicked(QListViewItem *);
    void ListPressed( int, QListViewItem *, const QPoint&, int);
    void makeDir();
    void doDelete();
    void tabChanged(QWidget*);
    void cleanUp();
    void renameIt();
    void runThis();
    void runText();
    void filePerms();
    void doProperties();
    void runCommand();
    void runCommandStd();
    QStringList getPath();
    void mkSym();
    void switchToLocalTab();
    void switchToRemoteTab();
    void refreshCurrentTab();

    void openSearch();
    void dirMenuSelected(int);
    void showFileMenu();
    void homeButtonPushed();
    void docButtonPushed();
    void QPEButtonPushed();
    void upDir();
    void currentPathComboChanged();

    void copy();
    void copyTimer();
    void copyAs();
    void copyAsTimer();
    void copySameDir();
    void copySameDirTimer();
    void move();
    void moveTimer();

    void fillCombo(const QString &);
    bool copyFile( const QString & , const QString & );
    void fileStatus();
    void doAbout();
    void doBeam();
    void fileBeamFinished( Ir *);
    bool copyDirectory( const QString & , const QString & );
    bool moveDirectory( const QString & , const QString & );

private:
    MenuButton *menuButton;
    QString oldName, localViewDir, remoteViewDir;
    void startProcess(const QString &);
    bool eventFilter( QObject * , QEvent * );
    void cancelRename();
    void doRename(QListView *);
    void okRename();
    void customDirsToMenu();
    void addCustomDir();
    void removeCustomDir();
    void navigateToSelected();
    void showHidden();

private slots:
    void processEnded(Opie::Core::OProcess *);
    void oprocessStderr(Opie::Core::OProcess *, char *, int);
    void gotoCustomDir(const QString &);
    void qcopReceive(const QCString&, const QByteArray&);
    void setDocument(const QString &);
    void enableDisableMenu();
};

#endif // ADVANCEDFM_H
