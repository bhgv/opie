/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qtopia Environment.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/
// code added by L. J. Potter Sat 03-02-2002 06:17:54
#define QTOPIA_INTERNAL_FSLP

#include "playlistselection.h"
#include "playlistwidget.h"
#include "mediaplayerstate.h"
#include "inputDialog.h"
#include "audiowidget.h"
#include "videowidget.h"
#include "rssparser.h"

#include <qpe/process.h>

#include <qvector.h>
#include <qxml.h>
/* OPIE */
#include <qpe/qpeapplication.h>
#include <qpe/qpemenubar.h>
#include <qpe/lnkproperties.h>
#include <opie2/odebug.h>
#include <opie2/oresource.h>

/* QT */
#include <qtoolbar.h>
#include <qaction.h>
#include <qlayout.h>
#include <qmessagebox.h>
//#include <qtimer.h>

/* STD */
#include <stdlib.h>

#include <unistd.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

// for setBacklight()
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#define BUTTONS_ON_TOOLBAR
#define SIDE_BUTTONS
#define CAN_SAVE_LOAD_PLAYLISTS

extern AudioWidget *audioUI;
extern VideoWidget *videoUI;
extern MediaPlayerState *mediaPlayerState;

static inline QString fullBaseName ( const QFileInfo &fi )
{
    QString str = fi. fileName ( );
    return str. left ( str. findRev ( '.' ));
}


QString audioMimes ="audio/mpeg;audio/x-wav;audio/x-ogg;audio/x-mod";
// class myFileSelector {

// };
class PlayListWidgetPrivate {
public:
    QToolButton *tbPlay, *tbFull, *tbLoop, *tbScale, *tbShuffle, *tbAddToList,  *tbRemoveFromList, *tbMoveUp, *tbMoveDown, *tbRemove;
    QFrame *playListFrame;
    FileSelector *files;
    PlayListSelection *selectedFiles;
    bool setDocumentUsed;
    DocLnk *current;
};


class ToolButton : public QToolButton {
public:
    ToolButton( QWidget *parent, const char *name, const QString& icon, QObject *handler, const QString& slot, bool t = FALSE )
            : QToolButton( parent, name ) {
        setTextLabel( name );
        setPixmap( Opie::Core::OResource::loadPixmap( icon, Opie::Core::OResource::SmallIcon ) );
        setAutoRaise( TRUE );
        setUsesBigPixmap( qApp->desktop()->size().width() > 330 );
        setFocusPolicy( QWidget::NoFocus );
        setToggleButton( t );
        connect( this, t ? SIGNAL( toggled(bool) ) : SIGNAL( clicked() ), handler, slot );
        QPEMenuToolFocusManager::manager()->addWidget( this );
    }
};


class MenuItem : public QAction {
public:
    MenuItem( QWidget *parent, const QString& text, QObject *handler, const QString& slot )
            : QAction( text, QString::null, 0, 0 ) {
        connect( this, SIGNAL( activated() ), handler, slot );
        addTo( parent );
    }
};


PlayListWidget::PlayListWidget( QWidget* parent, const char* name, WFlags fl )
        : QMainWindow( parent, name, fl )
{
    d = new PlayListWidgetPrivate;
    d->setDocumentUsed = FALSE;
    d->current = NULL;
    fromSetDocument = FALSE;
    insanityBool=FALSE;
    audioScan = FALSE;
    videoScan = FALSE;
//    menuTimer = new QTimer( this ,"menu timer"),
//     connect( menuTimer, SIGNAL( timeout() ), SLOT( addSelected() ) );
    channel = new QCopChannel( "QPE/Application/opieplayer", this );
    connect( channel, SIGNAL(received(const QCString&,const QByteArray&)),
        this, SLOT( qcopReceive(const QCString&,const QByteArray&)) );

    setBackgroundMode( PaletteButton );

    setCaption( tr("OpiePlayer") );
    setIcon( Opie::Core::OResource::loadPixmap( "opieplayer/OpiePlayer", Opie::Core::OResource::SmallIcon ) );

    setToolBarsMovable( FALSE );

      // Create Toolbar
    QToolBar *toolbar = new QToolBar( this );
    toolbar->setHorizontalStretchable( TRUE );

      // Create Menubar
    QMenuBar *menu = new QMenuBar( toolbar );
    menu->setMargin( 0 );

    QToolBar *bar = new QToolBar( this );
    bar->setLabel( tr( "Play Operations" ) );
//      d->tbPlayCurList =  new ToolButton( bar, tr( "play List" ), "opieplayer/play_current_list",
//                                        this , SLOT( addSelected()) );
    tbDeletePlaylist = new QPushButton( Opie::Core::OResource::loadPixmap("trash", Opie::Core::OResource::SmallIcon),"",bar,"close");
    tbDeletePlaylist->setFlat(TRUE);

    tbDeletePlaylist->setFixedSize(20,20);

    d->tbAddToList =  new ToolButton( bar, tr( "Add to Playlist" ), "opieplayer/add_to_playlist",
                                      this , SLOT(addSelected()) );
    d->tbRemoveFromList = new ToolButton( bar, tr( "Remove from Playlist" ), "opieplayer/remove_from_playlist",
                                          this , SLOT(removeSelected()) );
//    d->tbPlay    = new ToolButton( bar, tr( "Play" ), "opieplayer/play", /*this */mediaPlayerState , SLOT(setPlaying(bool)/*btnPlay()*/), TRUE );
    d->tbPlay    = new ToolButton( bar, tr( "Play" ), "opieplayer/play",
                                   this , SLOT( btnPlay(bool) ), TRUE );
    d->tbShuffle = new ToolButton( bar, tr( "Randomize" ),"opieplayer/shuffle",
                                   mediaPlayerState, SLOT(setShuffled(bool)), TRUE );
    d->tbLoop    = new ToolButton( bar, tr( "Loop" ),"opieplayer/loop",
                                   mediaPlayerState, SLOT(setLooping(bool)), TRUE );
    tbDeletePlaylist->hide();

    QPopupMenu *pmPlayList = new QPopupMenu( this );
    menu->insertItem( tr( "File" ), pmPlayList );
    new MenuItem( pmPlayList, tr( "Clear List" ), this, SLOT( clearList() ) );
    new MenuItem( pmPlayList, tr( "Add all audio files" ), this, SLOT( addAllMusicToList() ) );
    new MenuItem( pmPlayList, tr( "Add all video files" ), this, SLOT( addAllVideoToList() ) );
    new MenuItem( pmPlayList, tr( "Add all files" ), this, SLOT( addAllToList() ) );
    pmPlayList->insertSeparator(-1);
    new MenuItem( pmPlayList, tr( "Save PlayList" ), this, SLOT( saveList() ) );
    new MenuItem( pmPlayList, tr( "Open File or URL" ), this,SLOT( openFile() ) );
    pmPlayList->insertSeparator(-1);
    new MenuItem( pmPlayList, tr( "Rescan for Audio Files" ), this,SLOT( scanForAudio() ) );
    new MenuItem( pmPlayList, tr( "Rescan for Video Files" ), this,SLOT( scanForVideo() ) );

    QPopupMenu *pmView = new QPopupMenu( this );
    menu->insertItem( tr( "View" ), pmView );

    fullScreenButton = new QAction(tr("Full Screen"), Opie::Core::OResource::loadPixmap("fullscreen", Opie::Core::OResource::SmallIcon),
                                   QString::null, 0, this, 0);
    fullScreenButton->addTo(pmView);
    scaleButton = new QAction(tr("Scale"), Opie::Core::OResource::loadPixmap("opieplayer/scale", Opie::Core::OResource::SmallIcon),
                              QString::null, 0, this, 0);
    scaleButton->addTo(pmView);

    skinsMenu = new QPopupMenu( this );
    menu->insertItem( tr( "Skins" ), skinsMenu );
    skinsMenu->isCheckable();
    connect( skinsMenu, SIGNAL( activated(int) ) ,
             this, SLOT( skinsMenuActivated(int) ) );
    populateSkinsMenu();

    QVBox *vbox5 = new QVBox( this ); vbox5->setBackgroundMode( PaletteButton );
    QVBox *vbox4 = new QVBox( vbox5 ); vbox4->setBackgroundMode( PaletteButton );

    QHBox *hbox6 = new QHBox( vbox4 ); hbox6->setBackgroundMode( PaletteButton );

    tabWidget = new QTabWidget( hbox6, "tabWidget" );
//    tabWidget->setTabShape(QTabWidget::Triangular);

    QWidget *pTab;
    pTab = new QWidget( tabWidget, "pTab" );
//      playlistView = new QListView( pTab, "playlistview" );
//    playlistView->setMinimumSize(236,260);
    tabWidget->insertTab( pTab,"Playlist");

    // Add the playlist area

    QVBox *vbox3 = new QVBox( pTab ); vbox3->setBackgroundMode( PaletteButton );
    d->playListFrame = vbox3;

    QGridLayout *layoutF = new QGridLayout( pTab );
    layoutF->setSpacing( 2);
    layoutF->setMargin( 2);
    layoutF->addMultiCellWidget( d->playListFrame , 0, 0, 0, 1 );

    QHBox *hbox2 = new QHBox( vbox3 ); hbox2->setBackgroundMode( PaletteButton );

    d->selectedFiles = new PlayListSelection( hbox2);
    QVBox *vbox1 = new QVBox( hbox2 ); vbox1->setBackgroundMode( PaletteButton );

    QPEApplication::setStylusOperation( d->selectedFiles->viewport(),QPEApplication::RightOnHold);

    QVBox *stretch1 = new QVBox( vbox1 ); stretch1->setBackgroundMode( PaletteButton ); // add stretch
    new ToolButton( vbox1, tr( "Move Up" ),   "opieplayer/up",   d->selectedFiles, SLOT(moveSelectedUp()) );
    new ToolButton( vbox1, tr( "Remove" ),    "opieplayer/cut",  d->selectedFiles, SLOT(removeSelected()) );
    new ToolButton( vbox1, tr( "Move Down" ), "opieplayer/down", d->selectedFiles, SLOT(moveSelectedDown()) );
    QVBox *stretch2 = new QVBox( vbox1 ); stretch2->setBackgroundMode( PaletteButton ); // add stretch

    QWidget *aTab;
    aTab = new QWidget( tabWidget, "aTab" );
    audioView = new QListView( aTab, "Audioview" );

    QGridLayout *layoutA = new QGridLayout( aTab );
    layoutA->setSpacing( 2);
    layoutA->setMargin( 2);
    layoutA->addMultiCellWidget( audioView, 0, 0, 0, 1 );

    audioView->addColumn( tr("Title"),-1);
    audioView->addColumn(tr("Size"), -1);
    audioView->addColumn(tr("Media"),-1);
    audioView->addColumn( tr( "Path" ), -1 );

    audioView->setColumnAlignment(1, Qt::AlignRight);
    audioView->setColumnAlignment(2, Qt::AlignRight);
    audioView->setAllColumnsShowFocus(TRUE);

    audioView->setMultiSelection( TRUE );
    audioView->setSelectionMode( QListView::Extended);
    audioView->setSorting( 3, TRUE );

    tabWidget->insertTab(aTab,tr("Audio"));

    QPEApplication::setStylusOperation( audioView->viewport(),QPEApplication::RightOnHold);

//    audioView
//     populateAudioView();
// videowidget

    QWidget *vTab;
    vTab = new QWidget( tabWidget, "vTab" );
    videoView = new QListView( vTab, "Videoview" );

    QGridLayout *layoutV = new QGridLayout( vTab );
    layoutV->setSpacing( 2);
    layoutV->setMargin( 2);
    layoutV->addMultiCellWidget( videoView, 0, 0, 0, 1 );

    videoView->addColumn(tr("Title"),-1);
    videoView->addColumn(tr("Size"),-1);
    videoView->addColumn(tr("Media"),-1);
    videoView->addColumn(tr( "Path" ), -1 );
    videoView->setColumnAlignment(1, Qt::AlignRight);
    videoView->setColumnAlignment(2, Qt::AlignRight);
    videoView->setAllColumnsShowFocus(TRUE);
    videoView->setMultiSelection( TRUE );
    videoView->setSelectionMode( QListView::Extended);

    QPEApplication::setStylusOperation( videoView->viewport(),QPEApplication::RightOnHold);

    tabWidget->insertTab( vTab,tr("Video"));

    QWidget *LTab;
    LTab = new QWidget( tabWidget, "LTab" );
    playLists = new FileSelector( "playlist/plain;audio/x-mpegurl", LTab, "fileselector" , FALSE, FALSE);

    QGridLayout *layoutL = new QGridLayout( LTab );
    layoutL->setSpacing( 2);
    layoutL->setMargin( 2);
    layoutL->addMultiCellWidget( playLists, 0, 0, 0, 1 );

    tabWidget->insertTab(LTab,tr("Lists"));

    connect(tbDeletePlaylist,(SIGNAL(released())),SLOT( deletePlaylist()));
    connect( fullScreenButton, SIGNAL(activated()), mediaPlayerState, SLOT(toggleFullscreen()) );
    connect( scaleButton, SIGNAL(activated()), mediaPlayerState, SLOT(toggleScaled()) );

    connect( d->selectedFiles, SIGNAL( mouseButtonPressed(int,QListViewItem*,const QPoint&,int)),
             this,SLOT( playlistViewPressed(int,QListViewItem*,const QPoint&,int)) );


//audioView
    connect( audioView, SIGNAL( mouseButtonPressed(int,QListViewItem*,const QPoint&,int)),
             this,SLOT( viewPressed(int,QListViewItem*,const QPoint&,int)) );

    connect( audioView, SIGNAL( returnPressed(QListViewItem*)),
             this,SLOT( playIt(QListViewItem*)) );
    connect( audioView, SIGNAL( doubleClicked(QListViewItem*) ), this, SLOT( addToSelection(QListViewItem*) ) );


//videoView
    connect( videoView, SIGNAL( mouseButtonPressed(int,QListViewItem*,const QPoint&,int)),
             this,SLOT( viewPressed(int,QListViewItem*,const QPoint&,int)) );
    connect( videoView, SIGNAL( returnPressed(QListViewItem*)),
             this,SLOT( playIt(QListViewItem*)) );
    connect( videoView, SIGNAL( doubleClicked(QListViewItem*) ), this, SLOT( addToSelection(QListViewItem*) ) );

//playlists
    connect( playLists, SIGNAL( fileSelected(const DocLnk&) ), this, SLOT( loadList(const DocLnk&) ) );

    connect( tabWidget, SIGNAL (currentChanged(QWidget*)),this,SLOT(tabChanged(QWidget*)));

    connect( mediaPlayerState, SIGNAL( playingToggled(bool) ),    d->tbPlay,    SLOT( setOn(bool) ) );

    connect( mediaPlayerState, SIGNAL( loopingToggled(bool) ),    d->tbLoop,    SLOT( setOn(bool) ) );
    connect( mediaPlayerState, SIGNAL( shuffledToggled(bool) ),   d->tbShuffle, SLOT( setOn(bool) ) );
    connect( mediaPlayerState, SIGNAL( playlistToggled(bool) ),   this,         SLOT( setPlaylist(bool) ) );

    connect( d->selectedFiles, SIGNAL( doubleClicked(QListViewItem*) ), this, SLOT( playIt(QListViewItem*) ) );

    setCentralWidget( vbox5 );

    Config cfg( "OpiePlayer" );
    readConfig( cfg );

    currentPlayList = cfg.readEntry("CurrentPlaylist","default");
    loadList(DocLnk( currentPlayList));
    setCaption(tr("OpiePlayer: ")+ fullBaseName ( QFileInfo(currentPlayList)));

    initializeStates();
}

PlayListWidget::~PlayListWidget()
{
    Config cfg( "OpiePlayer" );
    if (d) {
        writeConfig( cfg );

        if ( d->current )
            delete d->current;

        delete d;
    }
}


void PlayListWidget::initializeStates()
{
    d->tbPlay->setOn( mediaPlayerState->playing() );
    d->tbLoop->setOn( mediaPlayerState->looping() );
    d->tbShuffle->setOn( mediaPlayerState->shuffled() );
    setPlaylist( true);
}


void PlayListWidget::readConfig( Config& cfg )
{
    cfg.setGroup("PlayList");
    QString currentString = cfg.readEntry("current", "" );
    int noOfFiles = cfg.readNumEntry("NumberOfFiles", 0 );
    for ( int i = 0; i < noOfFiles; i++ ) {
        QString entryName;
        entryName.sprintf( "File%i", i + 1 );
        QString linkFile = cfg.readEntry( entryName );
        DocLnk lnk( linkFile );
        if ( lnk.isValid() ) {
            d->selectedFiles->addToSelection( lnk );
        }
    }
    d->selectedFiles->setSelectedItem( currentString);
}

void PlayListWidget::writeConfig( Config& cfg ) const
{
    d->selectedFiles->writeCurrent( cfg);
    cfg.setGroup("PlayList");
    int noOfFiles = 0;
    d->selectedFiles->first();
    do {
        const DocLnk *lnk = d->selectedFiles->current();
        if ( lnk ) {
            QString entryName;
            entryName.sprintf( "File%i", noOfFiles + 1 );
//            odebug << entryName << oendl;
            cfg.writeEntry( entryName, lnk->linkFile() );
            noOfFiles++;
        }
    }
    while ( d->selectedFiles->next() );
    cfg.writeEntry("NumberOfFiles", noOfFiles );
}

void PlayListWidget::addToSelection( const DocLnk& lnk )
{
    d->setDocumentUsed = false;
    if ( mediaPlayerState->playlist() ) {
        if(QFileInfo(lnk.file()).exists() || lnk.file().left(4) == "http" )
            d->selectedFiles->addToSelection( lnk );
    }
    else
        mediaPlayerState->setPlaying( true);
}

void PlayListWidget::clearList()
{
    while ( first() )
        d->selectedFiles->removeSelected();
    Config cfg( "OpiePlayer" );
    cfg.setGroup("PlayList");
    cfg.writeEntry("CurrentPlaylist","");
    currentPlayList="";
}

void PlayListWidget::addAllToList()
{
    DocLnkSet filesAll;
    Global::findDocuments(&filesAll, "video/*;audio/*");
    QListIterator<DocLnk> Adit( filesAll.children() );
    for ( ; Adit.current(); ++Adit )
        if(QFileInfo(Adit.current()->file()).exists())
            d->selectedFiles->addToSelection( **Adit );
    tabWidget->setCurrentPage(0);

    writeCurrentM3u();
    d->selectedFiles->first();
}

void PlayListWidget::addAllMusicToList()
{
    QListIterator<DocLnk> dit( files.children() );
    for ( ; dit.current(); ++dit )
        if(QFileInfo(dit.current()->file()).exists())
            d->selectedFiles->addToSelection( **dit );
    tabWidget->setCurrentPage(0);

    writeCurrentM3u();
    d->selectedFiles->first();
}

void PlayListWidget::addAllVideoToList()
{
    QListIterator<DocLnk> dit( vFiles.children() );
    for ( ; dit.current(); ++dit )
        if(QFileInfo( dit.current()->file()).exists())
            d->selectedFiles->addToSelection( **dit );
    tabWidget->setCurrentPage(0);

    writeCurrentM3u();
    d->selectedFiles->first();
}

void PlayListWidget::setDocument(const QString& fileref)
{
    fromSetDocument = true;
    d->setDocumentUsed = TRUE;
    setDocumentEx(fileref);
}

void PlayListWidget::setDocumentEx(const QString& fileref)
{
    owarn << "opieplayer receive "+fileref << oendl;
    clearList();
    DocLnk lnk;
    QFileInfo fileInfo(fileref);
    if ( !fileInfo.exists() ) {
        QMessageBox::critical( 0, tr( "Invalid File" ),
                                tr( "There was a problem in getting the file." ) );
        return;
    }
//   odebug << "<<<<<<<<<<<<<<<<<<<<<<setDocument "+fileref << oendl;
    QString extension = fileInfo.extension(false);
        if( extension.find( "m3u", 0, false) != -1) { //is m3u
            readm3u( fileref);
        }
    else if( extension.find( "pls", 0, false) != -1 ) { //is pls
        readPls( fileref);
    }
    else if( fileref.find("playlist",0,TRUE) != -1) {//is playlist
        clearList();
        lnk.setName( fileInfo.baseName() ); //sets name
        lnk.setFile( fileref ); //sets file name
        lnk.setIcon("Sound");
        //addToSelection( lnk );

        loadList( lnk);
        d->selectedFiles->first();
    } else {
        if( fileref.find(".desktop",0,TRUE) != -1) {
            lnk = DocLnk(fileref);
        } else {
            lnk.setName( fileInfo.baseName() ); //sets name
            lnk.setFile( fileref ); //sets file name
            lnk.setIcon("Sound");
        }

        addToSelection( lnk );
//        addToSelection( DocLnk( fileref ) );
        lnk.removeLinkFile();
//        qApp->processEvents();
    }
    setCaption(tr("OpiePlayer"));
    d->setDocumentUsed = TRUE;
    d->selectedFiles->setSelected(d->selectedFiles->firstChild(),true );
    mediaPlayerState->setPlaying( FALSE );
    qApp->processEvents();
    mediaPlayerState->setPlaying( TRUE );
}

void PlayListWidget::setActiveWindow()
{
//    odebug << "SETTING active window" << oendl;
    // When we get raised we need to ensure that it switches views
    char origView = mediaPlayerState->view();
    mediaPlayerState->setView( 'l' ); // invalidate
    mediaPlayerState->setView( origView ); // now switch back
}

void PlayListWidget::useSelectedDocument()
{
    d->setDocumentUsed = FALSE;
}

const DocLnk *PlayListWidget::current()
{
    // this is fugly
    switch (tabWidget->currentPageIndex()) {
    case 0: //playlist
        {
//            odebug << "playlist" << oendl;
            if ( mediaPlayerState->playlist() ) {
                return d->selectedFiles->current();
            }
            else if ( d->setDocumentUsed && d->current ) {
                return d->current;
            }
            else {
                DocLnk dl = d->files->selectedDocument();
                return &dl; //(d->files->selectedDocument());
            }
        }
        break;
    case 1: //audio
        {
//            odebug << "audioView" << oendl;
            QListIterator<DocLnk> dit( files.children() );
            for ( ; dit.current(); ++dit ) {
                if( dit.current()->name() == audioView->currentItem()->text(0) && !insanityBool) {
                    insanityBool=TRUE;
                    return dit;
                }
            }
        }
        break;
    case 2: // video
        {
            QListIterator<DocLnk> Vdit( vFiles.children() );
            for ( ; Vdit.current(); ++Vdit ) {
                if( Vdit.current()->name() == videoView->currentItem()->text(0) && !insanityBool) {
                    insanityBool=TRUE;
                    return Vdit;
                }
            }
        }
        break;
    };
    return 0;
}

bool PlayListWidget::prev()
{
    if ( mediaPlayerState->playlist() ) {
        if ( mediaPlayerState->shuffled() ) {
            const DocLnk *cur = current();
            int j = 1 + (int)(97.0 * rand() / (RAND_MAX + 1.0));
            for ( int i = 0; i < j; i++ ) {
                if ( !d->selectedFiles->next() )
                d->selectedFiles->first();
            }
            if ( cur == current() )
                if ( !d->selectedFiles->next() )
                d->selectedFiles->first();
            return TRUE;
        } else {
            if ( !d->selectedFiles->prev() ) {
                if ( mediaPlayerState->looping() ) {
                return d->selectedFiles->last();
                } else {
                return FALSE;
                }
            }
            return TRUE;
        }
    } else {
        return mediaPlayerState->looping();
    }
}

bool PlayListWidget::next()
{
    if ( mediaPlayerState->playlist() ) {
        if ( mediaPlayerState->shuffled() ) {
            return prev();
        } else {
            if ( !d->selectedFiles->next() ) {
                if ( mediaPlayerState->looping() ) {
                return d->selectedFiles->first();
                } else {
                return FALSE;
                }
            }
            return TRUE;
        }
    } else {
        return mediaPlayerState->looping();
    }
}

bool PlayListWidget::first()
{
    if ( mediaPlayerState->playlist() )
        return d->selectedFiles->first();
    else
        return mediaPlayerState->looping();
}

bool PlayListWidget::last()
{
    if ( mediaPlayerState->playlist() )
        return d->selectedFiles->last();
    else
        return mediaPlayerState->looping();
}

void PlayListWidget::saveList()
{
    writem3u();
}

void PlayListWidget::loadList( const DocLnk & lnk)
{
    QString name = lnk.name();
//    odebug << "<<<<<<<<<<<<<<<<<<<<<<<<currentList is "+name << oendl;

    if( name.length()>0) {
        setCaption("OpiePlayer: "+name);
//        odebug << "<<<<<<<<<<<<load list "+ lnk.file() << oendl;
        clearList();
        readm3u(lnk.file());
        tabWidget->setCurrentPage(0);
    }
}

void PlayListWidget::setPlaylist( bool shown )
{
    if ( shown )
        d->playListFrame->show();
    else
        d->playListFrame->hide();
}

void PlayListWidget::setView( char view )
{
    if ( view == 'l' )
        QPEApplication::showWidget( this );
    else
        hide();
}

void PlayListWidget::addSelected()
{
    DocLnk lnk;
    QString filename;

    switch (tabWidget->currentPageIndex()) {
    case 0: //playlist
        return;
        break;
    case 1: { //audio
            QListViewItemIterator it( audioView );
            for ( ; it.current(); ++it ) {
                if ( it.current()->isSelected() ) {
                    filename = it.current()->text(3);
                    lnk.setName(  QFileInfo(filename).baseName() ); //sets name
                    lnk.setFile( filename ); //sets file name
                    d->selectedFiles->addToSelection(  lnk);
                }
            }
            audioView->clearSelection();
            //    d->selectedFiles->next();
        }
        break;

    case 2: { // video
            QListViewItemIterator it( videoView );
            for ( ; it.current(); ++it ) {
                if ( it.current()->isSelected() ) {

                    filename = it.current()->text(3);
                    lnk.setName(  QFileInfo(filename).baseName() ); //sets name
                    lnk.setFile( filename ); //sets file name
                    d->selectedFiles->addToSelection(  lnk);
                }
            }
            videoView->clearSelection();
        }
        break;
    };
    //  tabWidget->setCurrentPage(0);
    writeCurrentM3u();
}

void PlayListWidget::removeSelected()
{
    d->selectedFiles->removeSelected( );
}

void PlayListWidget::playIt( QListViewItem *)
{
    //   d->setDocumentUsed = FALSE;
    //  mediaPlayerState->curPosition =0;
    //    mediaPlayerState->setPlaying(FALSE);
    mediaPlayerState->setPlaying(TRUE);
    d->selectedFiles->unSelect();
}

void PlayListWidget::addToSelection( QListViewItem *it)
{
    d->setDocumentUsed = FALSE;

    if(it) {
        switch ( tabWidget->currentPageIndex()) {
        case 0: //playlist
            return;
            break;
        };
        //      case 1: {
        DocLnk lnk;
        QString filename;

        filename=it->text(3);
        lnk.setName( fullBaseName ( QFileInfo(filename)) ); //sets name
        lnk.setFile( filename ); //sets file name
        d->selectedFiles->addToSelection(  lnk);

        if(tabWidget->currentPageIndex() == 0)
            writeCurrentM3u();
        //    tabWidget->setCurrentPage(0);

    }
}

void PlayListWidget::tabChanged(QWidget *) {
    switch ( tabWidget->currentPageIndex()) {
    case 0:
        {
            if( !tbDeletePlaylist->isHidden())
                tbDeletePlaylist->hide();
            d->tbRemoveFromList->setEnabled(TRUE);
            d->tbAddToList->setEnabled(FALSE);
        }
        break;
    case 1:
        {
            audioView->clear();
            populateAudioView();

            if( !tbDeletePlaylist->isHidden())
                tbDeletePlaylist->hide();
            d->tbRemoveFromList->setEnabled(FALSE);
            d->tbAddToList->setEnabled(TRUE);
        }
        break;
    case 2:
        {
            videoView->clear();
            populateVideoView();
            if( !tbDeletePlaylist->isHidden())
                tbDeletePlaylist->hide();
            d->tbRemoveFromList->setEnabled(FALSE);
            d->tbAddToList->setEnabled(TRUE);
        }
        break;
    case 3:
        {
            if( tbDeletePlaylist->isHidden())
                tbDeletePlaylist->show();
            playLists->reread();
        }
        break;
    };
}

void PlayListWidget::btnPlay(bool b)
{
    //    mediaPlayerState->setPlaying(b);
    switch ( tabWidget->currentPageIndex()) {
    case 0:
        {
            //            if( d->selectedFiles->current()->file().find(" ",0,TRUE) != -1
            //                if( d->selectedFiles->current()->file().find("%20",0,TRUE) != -1) {
            //                 QMessageBox::message("Note","You are trying to play\na malformed url.");
            //            } else {
            mediaPlayerState->setPlaying(b);
            insanityBool=FALSE;
            odebug << "insanity" << oendl;
            //          }
        }
        break;
    case 1:
        {
            //          d->selectedFiles->unSelect();
            addToSelection( audioView->currentItem() );
            mediaPlayerState->setPlaying( b);
            d->selectedFiles->removeSelected( );
            d->selectedFiles->unSelect();
            tabWidget->setCurrentPage(1);
            insanityBool=FALSE;
        }//          audioView->clearSelection();
        break;
    case 2:
        {

            addToSelection( videoView->currentItem() );
            mediaPlayerState->setPlaying( b);
            //          qApp->processEvents();
            d->selectedFiles->removeSelected( );
            d->selectedFiles->unSelect();
            tabWidget->setCurrentPage(2);
            insanityBool=FALSE;
        }//          videoView->clearSelection();
        break;
    };
}

void PlayListWidget::deletePlaylist()
{
    switch( QMessageBox::information( this, (tr("Remove Playlist?")),
                                        (tr("You really want to delete\nthis playlist?")),
                                        (tr("Yes")), (tr("No")), 0 )){
    case 0: // Yes clicked,
        QFile().remove(playLists->selectedDocument().file());
        QFile().remove(playLists->selectedDocument().linkFile());
        playLists->reread();
        break;
    case 1: // Cancel
        break;
    };
}

void PlayListWidget::viewPressed( int mouse, QListViewItem *, const QPoint&, int )
{
    switch (mouse) {
    case 1:
        break;
    case 2:
        {
            QPopupMenu  m;
            m.insertItem( tr( "Play" ), this, SLOT( playSelected() ));
            m.insertItem( tr( "Add to Playlist" ), this, SLOT( addSelected() ));
            m.insertSeparator();
            if( QFile(QPEApplication::qpeDir()+"lib/libopie.so").exists() )
                m.insertItem( tr( "Properties" ), this, SLOT( listDelete() ));

            m.exec( QCursor::pos() );
        }
        break;
    };
}

void PlayListWidget::playSelected()
{
    btnPlay( true);
//    d->selectedFiles->unSelect();
}

void PlayListWidget::playlistViewPressed( int mouse, QListViewItem *, const QPoint&, int)
{
    switch (mouse) {
    case 1:
        break;
    case 2:
        {
            QPopupMenu  m;
            m.insertItem( tr( "Play Selected" ), this, SLOT( playSelected() ));
            m.insertItem( tr( "Remove" ), this, SLOT( removeSelected() ));
            //       m.insertSeparator();
            //     m.insertItem( tr( "Properties" ), this, SLOT( listDelete() ));
            m.exec( QCursor::pos() );
        }
        break;
    };
}

void PlayListWidget::listDelete()
{
    Config cfg( "OpiePlayer" );
    cfg.setGroup("PlayList");
    currentPlayList = cfg.readEntry("CurrentPlaylist","");
    QString file;
    //    int noOfFiles = cfg.readNumEntry("NumberOfFiles", 0 );
    switch ( tabWidget->currentPageIndex()) {
    case 0:
        break;
    case 1:
        {
            file = audioView->currentItem()->text(0);
            QListIterator<DocLnk> Pdit( files.children() );
            for ( ; Pdit.current(); ++Pdit ) {
                if( Pdit.current()->name() == file) {
                    LnkProperties prop( Pdit.current() );
                    QPEApplication::execDialog( &prop );
                }
            }
            populateAudioView();
        }
        break;
    case 2:
        {
            //           file = videoView->selectedItem()->text(0);
            //           for ( int i = 0; i < noOfFiles; i++ ) {
            //               QString entryName;
            //               entryName.sprintf( "File%i", i + 1 );
            //               QString linkFile = cfg.readEntry( entryName );
            //                   AppLnk lnk( AppLnk(linkFile));
            //               if( lnk.name() == file ) {
            //                   LnkProperties prop( &lnk);
            // //  connect(&prop, SIGNAL(select(const AppLnk*)), this, SLOT(externalSelected(const AppLnk*)));
            //                   prop.showMaximized();
            //                   prop.exec();
            //               }
            //            }
        }
    break;
    };
}

void PlayListWidget::scanForAudio()
{
    //    odebug << "scan for audio" << oendl;
    files.detachChildren();
    QListIterator<DocLnk> sdit( files.children() );
    for ( ; sdit.current(); ++sdit ) {
        delete sdit.current();
    }
    Global::findDocuments( &files, audioMimes);
    audioScan = true;
}

void PlayListWidget::scanForVideo()
{
    //    odebug << "scan for video" << oendl;
    vFiles.detachChildren();
    QListIterator<DocLnk> sdit( vFiles.children() );
    for ( ; sdit.current(); ++sdit ) {
        delete sdit.current();
    }
    Global::findDocuments(&vFiles, "video/*");
    videoScan = true;
}

void PlayListWidget::populateAudioView()
{
    audioView->clear();
    StorageInfo storageInfo;
    const QList<FileSystem> &fs = storageInfo.fileSystems();
    if(!audioScan)
        scanForAudio();

    QListIterator<DocLnk> dit( files.children() );
    QListIterator<FileSystem> it ( fs );

    QString storage;
    for ( ; dit.current(); ++dit ) {
        for( ; it.current(); ++it ){
            const QString name = (*it)->name();
            const QString path = (*it)->path();
            if(dit.current()->file().find(path) != -1 ) storage=name;
        }

        QListViewItem * newItem;
        if ( QFile( dit.current()->file()).exists() || dit.current()->file().left(4) == "http" ) {
            long size;
            if( dit.current()->file().left(4) == "http" )
                size=0;
            else
                size = QFile( dit.current()->file() ).size();
            //            odebug << dit.current()->name() << oendl;
            newItem= /*(void)*/ new QListViewItem( audioView, dit.current()->name(),
                                QString::number(size ), storage, dit.current()->file());
            newItem->setPixmap(0, Opie::Core::OResource::loadPixmap( "opieplayer/musicfile", Opie::Core::OResource::SmallIcon ));
        }
    }
}

void PlayListWidget::populateVideoView()
{
    videoView->clear();
    StorageInfo storageInfo;
    const QList<FileSystem> &fs = storageInfo.fileSystems();

    if(!videoScan )
        scanForVideo();

    QListIterator<DocLnk> Vdit( vFiles.children() );
    QListIterator<FileSystem> it ( fs );
    videoView->clear();
    QString storage;
    for ( ; Vdit.current(); ++Vdit ) {
        for( ; it.current(); ++it ){
            const QString name = (*it)->name();
            const QString path = (*it)->path();
            if( Vdit.current()->file().find(path) != -1 ) storage=name;
        }

        QListViewItem * newItem;
        if ( QFile( Vdit.current()->file()).exists() ) {
            newItem= /*(void)*/ new QListViewItem( videoView, Vdit.current()->name(),
                   QString::number( QFile( Vdit.current()->file() ).size() ),
                                                   storage, Vdit.current()->file());
            newItem->setPixmap(0, Opie::Core::OResource::loadPixmap( "opieplayer/videofile", Opie::Core::OResource::SmallIcon ));
        }
    }
}

void PlayListWidget::openFile()
{
    QString filename, name;
    InputDialog *fileDlg;
    fileDlg = new InputDialog(this,tr("Open file or URL"),TRUE, 0);
    fileDlg->exec();
    if( fileDlg->result() == 1 ) {
        filename = fileDlg->text();
        //        odebug << "Selected filename is " + filename << oendl;
        DocLnk lnk;
        Config cfg( "OpiePlayer" );
        cfg.setGroup("PlayList");

        QString m3uFile;
        m3uFile = filename;
        if(filename.left(4) == "http") {
            if(filename.find(":",8,TRUE) != -1) { //found a port

                m3uFile = filename;
                if( m3uFile.right( 1 ).find( '/' ) == -1) {
                    m3uFile += "/";
                }
                filename = m3uFile;
            }
            if( filename.right(3) == "xml" ||
                    filename.find("rss" ) !=-1)
            {
                // readpodcast(filename );
                downloadPodcast(filename);
            }
            else {
                lnk.setName( m3uFile ); //sets name
                lnk.setFile( filename ); //sets file name
                lnk.setIcon("opieplayer2/musicfile");
                d->selectedFiles->addToSelection(  lnk );
                writeCurrentM3u();
            }
        }
        else if( filename.right( 3) == "m3u" ) {
            readm3u( filename );

        }
        else if( filename.right(3) == "pls" ) {
            readPls( filename );
        }
        else if( filename.right(3) == "xml" ||
                    filename.find("rss" ) !=-1 ) {
            readpodcast( filename );
        }
        else {
            lnk.setName( fullBaseName ( QFileInfo(filename)) ); //sets name
            lnk.setFile( filename ); //sets file name
            d->selectedFiles->addToSelection(  lnk);
            lnk.removeLinkFile();
            writeCurrentM3u();
        }
    }

    delete fileDlg;
}


/*
 writes current playlist to current m3u file */
void PlayListWidget::writeCurrentM3u()
{
//    odebug << "writing to current m3u" << oendl;
    Config cfg( "OpiePlayer" );
    cfg.setGroup("PlayList");
    currentPlayList = cfg.readEntry("CurrentPlaylist","");
    Om3u *m3uList;
    m3uList = new Om3u( currentPlayList, IO_ReadWrite | IO_Truncate );

    if( d->selectedFiles->first()) {
        do {
//            odebug << "writeCurrentM3u " +d->selectedFiles->current()->file() << oendl;
            m3uList->add( d->selectedFiles->current()->file() );
        }
        while ( d->selectedFiles->next() );
//        odebug << "<<<<<<<<<<<<>>>>>>>>>>>>>>>>>" << oendl;
        m3uList->write();
        m3uList->close();
        delete m3uList;
    }
}

  /*
 writes current playlist to m3u file */
void PlayListWidget::writem3u()
{
    InputDialog *fileDlg;
    fileDlg = new InputDialog( this, tr( "Save m3u Playlist " ), TRUE, 0);
    fileDlg->exec();
    QString name, filename, list;
    Om3u *m3uList;

    if( fileDlg->result() == 1 ) {
        name = fileDlg->text();
//        odebug << filename << oendl;
        if( name.find("/",0,true) != -1) {// assume they specify a file path
            filename = name;
            name = name.right(name.length()- name.findRev("/",-1,true) - 1 );
        }
        else //otherwise dump it somewhere noticable
            filename = QPEApplication::documentDir() + "/" + name;

        if( filename.right( 3 ) != "m3u" ) //needs filename extension
            filename += ".m3u";

        if( d->selectedFiles->first()) {
            m3uList = new Om3u(filename, IO_ReadWrite | IO_Truncate);

            do {
                m3uList->add( d->selectedFiles->current()->file());
            }
            while ( d->selectedFiles->next() );
            //    odebug << list << oendl;
            m3uList->write();
            m3uList->close();
            delete m3uList;

            DocLnk lnk;
            lnk.setFile( filename);
            lnk.setIcon("opieplayer2/playlist2");
            lnk.setName( name); //sets file name

            // odebug << filename << oendl;
            Config config( "OpiePlayer" );
            config.setGroup( "PlayList" );

            config.writeEntry("CurrentPlaylist",filename);
            currentPlayList=filename;

            if(!lnk.writeLink())
                oerr << "Writing doclink did not work" << oendl;

            setCaption(tr("OpiePlayer: ") + name);
        }
    }
    delete fileDlg;
}


void PlayListWidget::keyReleaseEvent( QKeyEvent *e)
{
    switch ( e->key() ) {
////////////////////////////// Zaurus keys
    case Key_F9: //activity
//        if(audioUI->isHidden())
//            audioUI->showMaximized();
        break;
    case Key_F10: //contacts
//        if( videoUI->isHidden())
//            videoUI->showMaximized();
        break;
    case Key_F11: //menu
        break;
    case Key_F12: //home
//        doBlank();
        break;
    case Key_F13: //mail
//        doUnblank();
        break;
    case Key_Q: //add to playlist
        addSelected();
        break;
    case Key_R: //remove from playlist
        removeSelected();
        break;
//    case Key_P: //play
//        odebug << "Play" << oendl;
//        playSelected();
//        break;
    case Key_Space:
//        playSelected(); puh
        break;
    case Key_1:
        tabWidget->setCurrentPage(0);
        break;
    case Key_2:
        tabWidget->setCurrentPage(1);
        break;
    case Key_3:
        tabWidget->setCurrentPage(2);
        break;
    case Key_4:
        tabWidget->setCurrentPage(3);
        break;
    case Key_Down:
        if ( !d->selectedFiles->next() )
            d->selectedFiles->first();

        break;
    case Key_Up:
//        if ( !d->selectedFiles->prev() )
//            d->selectedFiles->last();
        d->selectedFiles->prev();
        break;
    }
}

void PlayListWidget::keyPressEvent( QKeyEvent *)
{
//    odebug << "Key press" << oendl;
//     switch ( e->key() ) {
// ////////////////////////////// Zaurus keys
//       case Key_A: //add to playlist
//           odebug << "Add" << oendl;
//           addSelected();
//           break;
//       case Key_R: //remove from playlist
//           removeSelected();
//           break;
//       case Key_P: //play
//           odebug << "Play" << oendl;
//           playSelected();
//           break;
//       case Key_Space:
//           odebug << "Play" << oendl;
//           playSelected();
//           break;
//     }
}

void PlayListWidget::doBlank() {
    //    odebug << "do blanking" << oendl;
#ifdef QT_QWS_DEVFS
    fd=open("/dev/fb/0",O_RDWR);
#else
    fd=open("/dev/fb0",O_RDWR);
#endif
    if (fd != -1) {
        if (ioctl(fd,FBIOBLANK,1) == -1)
            oerr << "Failed blanking the screen. " << strerror(errno) << oendl;
    }
}

void PlayListWidget::doUnblank()
{
    // this crashes opieplayer with a segfault
    //      int fd;
    //       fd=open("/dev/fb0",O_RDWR);
    //    odebug << "do unblanking" << oendl;
    if (fd != -1) {
        if (ioctl(fd,FBIOBLANK,0) == -1)
            oerr << "Failed unblanking the screen. " << strerror(errno) << oendl;
        else
            close(fd);
    }
    QCopEnvelope h("QPE/System", "setBacklight(int)");
    h <<-3;// v[1]; // -3 Force on
}

void PlayListWidget::populateSkinsMenu()
{
    int item = 0;
    defaultSkinIndex = 0;
    QString skinName;
    Config cfg( "OpiePlayer" );
    cfg.setGroup("Options" );
    QString skin = cfg.readEntry( "Skin", "default" );

    QDir skinsDir( QPEApplication::qpeDir() + "pics/opieplayer2/skins" );
    skinsDir.setFilter( QDir::Dirs );
    skinsDir.setSorting(QDir::Name );
    const QFileInfoList *skinslist = skinsDir.entryInfoList();
    QFileInfoListIterator it( *skinslist );
    QFileInfo *fi;
    while ( ( fi = it.current() ) ) {
        skinName =  fi->fileName();
        //        odebug << fi->fileName() << oendl;
        if( skinName != "." &&  skinName != ".." && skinName !="CVS" )  {
            item = skinsMenu->insertItem( fi->fileName() ) ;
        }
        if( skinName == "default" ) {
            defaultSkinIndex = item;
        }
        if( skinName == skin ) {
            skinsMenu->setItemChecked( item, TRUE );
        }
        ++it;
    }
}

void PlayListWidget::skinsMenuActivated( int item )
{
    for( int i = defaultSkinIndex; i > defaultSkinIndex - static_cast<int>(skinsMenu->count()); i-- ) {
        skinsMenu->setItemChecked( i, FALSE );
    }
    skinsMenu->setItemChecked( item, TRUE );

    Config cfg( "OpiePlayer" );
    cfg.setGroup("Options");
    cfg.writeEntry("Skin", skinsMenu->text( item ) );
}

void PlayListWidget::qcopReceive(const QCString &msg, const QByteArray &data)
{
    //   odebug << "qcop message "+msg << oendl;
    QDataStream stream ( data, IO_ReadOnly );
    if ( msg == "play()" ) { //plays current selection
        btnPlay( true);
    }
    else if ( msg == "stop()" ) {
        mediaPlayerState->setPlaying( false);
    }
    else if ( msg == "togglePause()" ) {
        mediaPlayerState->togglePaused();
    }
    else if ( msg == "next()" ) { //select next in lis
        mediaPlayerState->setNext();
    }
    else if ( msg == "prev()" ) { //select previous in list
        mediaPlayerState->setPrev();
    }
    else if ( msg == "toggleLooping()" ) { //loop or not loop
        mediaPlayerState->toggleLooping();
    }
    else if ( msg == "toggleShuffled()" ) { //shuffled or not shuffled
        mediaPlayerState->toggleShuffled();
    }
    else if ( msg == "volUp()" ) { //volume more
        //       emit moreClicked();
        //       emit moreReleased();
    }
    else if ( msg == "volDown()" ) { //volume less
        //       emit lessClicked();
        //       emit lessReleased();
    }
    else if ( msg == "play(QString)" ) { //play this now
        QString file;
        stream >> file;
        setDocumentEx( (const QString &) file);
    }
    else if ( msg == "add(QString)" ) { //add to playlist
        QString file;
        stream >> file;
        QFileInfo fileInfo(file);
        DocLnk lnk;
        lnk.setName( fileInfo.baseName() ); //sets name
        lnk.setFile( file ); //sets file name
        addToSelection( lnk );
    }
    else if ( msg == "rem(QString)" ) { //remove from playlist
        QString file;
        stream >> file;
    }
    else if ( msg == "setDocument(QString)" ) { //loop or not loop
        QCopEnvelope h("QPE/Application/opieplayer", "raise()");
    }
}

/*
reads m3u and shows files/urls to playlist widget */
void PlayListWidget::readm3u( const QString &filename )
{
    //    odebug << "read m3u filename " + filename << oendl;

    Om3u *m3uList;
    QString s, name;
    m3uList = new Om3u( filename, IO_ReadOnly );
    m3uList->readM3u();
    DocLnk lnk;
    for ( QStringList::ConstIterator it = m3uList->begin(); it != m3uList->end(); ++it ) {
        s = *it;
        //          odebug << "reading "+ s << oendl;
        if(s.left(4)=="http") {
            lnk.setName( s ); //sets file name
            lnk.setIcon("opieplayer2/musicfile");

            //          if(s.right(4) != '.' || s.right(5) != '.')
            if(s.right(4) != '.' || s.right(5) != '.' )
                if( s.right(1) != "/")
                    lnk.setFile( s+"/"); //if url with no extension
                else
                    lnk.setFile( s ); //sets file name

        }
        else {
            //               if( QFileInfo( s ).exists() ) {
            lnk.setName( fullBaseName ( QFileInfo(s)));
            //                 if(s.right(4) == '.')   {//if regular file
            if(s.left(1) != "/")  {
                //            odebug << "set link "+QFileInfo(filename).dirPath()+"/"+s << oendl;
                lnk.setFile( QFileInfo(filename).dirPath()+"/"+s);
                lnk.setIcon("SoundPlayer");
            }
            else {
                //            odebug << "set link2 "+s << oendl;
                lnk.setFile( s);
                lnk.setIcon("SoundPlayer");
            }
        }
        d->selectedFiles->addToSelection( lnk );
    }
    Config config( "OpiePlayer" );
    config.setGroup( "PlayList" );

    config.writeEntry("CurrentPlaylist",filename);
    config.write();
    currentPlayList=filename;

    m3uList->close();
    delete m3uList;

    d->selectedFiles->setSelectedItem( s);
    setCaption(tr("OpiePlayer: ")+ fullBaseName ( QFileInfo(filename)));
}

/*
reads pls and adds files/urls to playlist  */
void PlayListWidget::readPls( const QString &filename )
{
    //    odebug << "pls filename is " + filename << oendl;
    Om3u *m3uList;
    QString s, name;
    m3uList = new Om3u( filename, IO_ReadOnly );
    m3uList->readPls();

    for ( QStringList::ConstIterator it = m3uList->begin(); it != m3uList->end(); ++it ) {
        s = *it;
        //        s.replace( QRegExp( "%20" )," " );
        DocLnk lnk( s );
        QFileInfo f( s );
        QString name = fullBaseName ( f);

        if( name.left( 4 ) == "http" ) {
            name = s.right( s.length() - 7);
        }
        else {
            name = s;
        }

        name = name.right( name.length() - name.findRev( "\\", -1, TRUE) - 1 );

        lnk.setName( name );
        if( s.at( s.length() - 4) == '.') {// if this is probably a file
            lnk.setFile( s );
        }
        else { //if its a url
            if( name.right( 1 ).find( '/' ) == -1) {
                s += "/";
            }
            lnk.setFile( s );
        }
        lnk.setType( "audio/x-mpegurl" );

        lnk.writeLink();
        d->selectedFiles->addToSelection( lnk );
    }

    m3uList->close();
    delete m3uList;
}

bool PlayListWidget::readpodcast( const QString &filename )
{
    QStringList latestPodCast;
    //download url
    qWarning("podcast "+filename);
    QFileInfo info(filename);
    if (info.size() > 0) {
        bool result = false;
        // qWarning("parseDoc " + feedFile.name() );
        QFile file(filename);
        QXmlInputSource source( file);
        QXmlSimpleReader reader;
        //  reader.setFeature("http://xml.org/sax/features/namespaces", true);
        //  reader.setFeature("http://xml.org/sax/features/namespace-prefixes", true);
        //  reader.setFeature("http://trolltech.com/xml/features/report-whitespace-only-CharData", false);

        reader.setContentHandler( &rssHandler);
        reader.setErrorHandler( &rssHandler);
        result = reader.parse( source);
        if (!result) {
            QMessageBox::critical(0, "Error", tr("<p>Error unable to parse file.</p>"));
//            qWarning("Error unable to parse file\n%s", handler.errorMessage.local8Bit().data());
            return false;
        }
        else {
            int size = rssHandler.getItems().size();
            qWarning( rssHandler.getChannelInfo().join("\n"));

            for(int i = 0; i < size; i++) {
                QList<QStringList> attributesList = rssHandler.getItems().at(i)->attributes;
                QStringList *sList;
                QStringList attList;
                for(sList = attributesList.first(); sList !=0; sList = attributesList.next()) {
                    for( QStringList::Iterator it = sList->begin(); it != sList->end(); ++it ) {
                        attList << (*it);
                    }
                    if(i == 0) { //this assumes that the latest is the first
                        latestPodCast << attList[2]; //this is our mp3 url
                        latestPodCast << rssHandler.getItems().at(i)->title;
                        latestPodCast << rssHandler.getItems().at(i)->description;
                        latestPodCast << rssHandler.getItems().at(i)->pubdate;
                    }
                }
            }
            QString s = latestPodCast[0];   //this is our mp3 url

//            http://www.davesipaq.com/podcast.xml
            DocLnk lnk( s );
            QFileInfo f( s );
            QString name = fullBaseName ( f);

            if( name.left( 4 ) == "http" ) {
                name = s.right( s.length() - 7);
            }
            else {
                name = s;
            }

            name = name.right( name.length() - name.findRev( "\\", -1, TRUE) - 1 );

            lnk.setName( name );
            if( s.at( s.length() - 4) == '.') {// if this is probably a file
                lnk.setFile( s );
            }
            else { //if its a url
                if( name.right( 1 ).find( '/' ) == -1) {
                    s += "/";
                }
                lnk.setFile( s );
            }
            lnk.setType( "audio/x-mpegurl" );

            lnk.writeLink();
            d->selectedFiles->addToSelection( lnk );

        }
    }
    else {
        QMessageBox::critical( 0, "Qtopia Rss", tr("<p>Sorry, could not find the requested document.</p>"));
        return false;
    }

    qWarning( latestPodCast.join("\n"));

    return true;
}

bool PlayListWidget::downloadPodcast(const QString &url)
{
    qWarning("download "+url);
    QString localFile;
    localFile = url;
    localFile = localFile.mid(7, localFile.length()-7);

    localFile = localFile.replace(QRegExp("/"), "_");
    localFile = localFile.replace(QRegExp("&"), "_");
    localFile = localFile.replace(QRegExp("="), "_");
    localFile = localFile.replace(QRegExp("\\?"), "_");
    localFile = localFile.replace(QRegExp("@"), "_");
    localFile = QDir::homeDirPath()+"/Settings/"+localFile;

#warning FIXME
    QString cmd;
    cmd = "wget ";
    cmd +=" -O ";
    cmd += localFile + " " + url;
    qWarning(cmd);
    system(cmd.latin1());
//    Process ipkg_status(QStringList()<< "wget" <<"-O" << localFile << url );
//    QString out;
//    bool r = ipkg_status.exec("",out);
//    if(r)
//        qWarning(out);
    readpodcast(localFile);
    return true;
}

