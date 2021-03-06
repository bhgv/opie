/*
                            This file is part of the Opie Project

                             Copyright (c)  2002 Max Reiss <harlekin@handhelds.org>
                             Copyright (c)  2002 L. Potter <ljp@llornkcor.com>
                             Copyright (c)  2002 Holger Freyther <zecke@handhelds.org>
              =.
            .=l.
           .>+-=
 _;:,     .>    :=|.         This program is free software; you can
.> <`_,   >  .   <=          redistribute it and/or  modify it under
:`=1 )Y*s>-.--   :           the terms of the GNU General Public
.="- .-=="i,     .._         License as published by the Free Software
 - .   .-<_>     .<>         Foundation; either version 2 of the License,
     ._= =}       :          or (at your option) any later version.
    .%`+i>       _;_.
    .i_,=:_.      -<s.       This program is distributed in the hope that
     +  .  -:.       =       it will be useful,  but WITHOUT ANY WARRANTY;
    : ..    .:,     . . .    without even the implied warranty of
    =_        +     =;=|`    MERCHANTABILITY or FITNESS FOR A
  _.=:.       :    :=>`:     PARTICULAR PURPOSE. See the GNU
..}^=.=       =       ;      Library General Public License for more
++=   -.     .`     .:       details.
 :     =  ...= . :.=-
 -.   .:....=;==+<;          You should have received a copy of the GNU
  -_. . .   )=.  =           Library General Public License along with
    --        :-=`           this library; see the file COPYING.LIB.
                             If not, write to the Free Software Foundation,
                             Inc., 59 Temple Place - Suite 330,
                             Boston, MA 02111-1307, USA.

*/

#ifndef AUDIO_WIDGET_H
#define AUDIO_WIDGET_H

#include <qlineedit.h>

#include <opie2/oticker.h>

#include "mediawidget.h"

class QPixmap;

class AudioWidget : public MediaWidget {
    Q_OBJECT
public:
    AudioWidget( PlayListWidget &playList, MediaPlayerState &mediaPlayerState, QWidget* parent=0, const char* name=0 );
    ~AudioWidget();
    void setTickerText( const QString &text ) { songInfo.setText( text ); }

    static MediaWidget::GUIInfo guiInfo();

public slots:
    void updateSlider( long, long );
    void sliderPressed( );
    void sliderReleased( );
    void setLooping( bool b) { setToggleButton( Loop, b ); }
    void setPosition( long );
    void setSeekable( bool );

public:
    virtual void setLength( long );
    virtual void setPlaying( bool b) { setToggleButton( Play, b ); }
    virtual void setDisplayType( MediaPlayerState::DisplayType displayType );

    virtual void loadSkin();

signals:
    void sliderMoved(long);

protected:
    void doBlank();
    void doUnblank();
    void resizeEvent( QResizeEvent *re );
    void timerEvent( QTimerEvent *event );
    void keyReleaseEvent( QKeyEvent *e);
private slots:
    void skipFor();
    void skipBack();
    void stopSkip();
private:
    int skipDirection;
    QString skin;

    Opie::Ui::OTicker  songInfo;
    QSlider slider;
    QLineEdit time;
    bool isStreaming : 1;
    bool audioSliderBeingMoved : 1;
};


#endif // AUDIO_WIDGET_H

