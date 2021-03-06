/*
 * datebookpluginconfig.cpp
 *
 * copyright   : (c) 2002,2003,2004 by Maximilian Rei�
 * email       : harlekin@handhelds.org
 *
 */
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "datebookpluginconfig.h"

#include <qpe/config.h>

#include <qlayout.h>

DatebookPluginConfig::DatebookPluginConfig( QWidget* parent, const char* name)
    : TodayConfigWidget( parent, name ) {

    QVBoxLayout *layout = new QVBoxLayout( this );

    m_gui = new DatebookPluginConfigBase( this );

    layout->addWidget( m_gui );

    readConfig();
}

void DatebookPluginConfig::readConfig() {
    Config cfg( "todaydatebookplugin" );
    cfg.setGroup( "config" );

    m_max_lines_meet = cfg.readNumEntry( "maxlinesmeet", 5 );
    m_gui->SpinBox1->setValue( m_max_lines_meet );
    m_show_location = cfg.readNumEntry( "showlocation", 1 );
    m_gui->CheckBox1->setChecked( m_show_location );
    m_show_notes = cfg.readNumEntry( "shownotes", 0 );
    m_gui->CheckBox2->setChecked( m_show_notes );
    m_only_later = cfg.readNumEntry( "onlylater", 1 );
    m_gui->CheckBox4->setChecked( cfg.readNumEntry( "timeextraline", 1 ) );
    m_gui->CheckBox3->setChecked( m_only_later );
    m_more_days = cfg.readNumEntry( "moredays", 0 );
    m_gui->SpinBox2->setValue( m_more_days );
    m_maxCharClip = cfg.readNumEntry( "maxcharclip" , 38 );
    m_gui->SpinBox3->setValue( m_maxCharClip );
}


void DatebookPluginConfig::writeConfig() {
    Config cfg( "todaydatebookplugin" );
    cfg.setGroup( "config" );

    m_max_lines_meet = m_gui->SpinBox1->value();
    cfg.writeEntry( "maxlinesmeet",  m_max_lines_meet);
    m_show_location = m_gui->CheckBox1->isChecked();
    cfg.writeEntry( "showlocation", m_show_location);
    m_show_notes = m_gui->CheckBox2->isChecked();
    cfg.writeEntry( "shownotes", m_show_notes );
    m_only_later  = m_gui->CheckBox3->isChecked();
    cfg.writeEntry( "timeextraline",  m_gui->CheckBox4->isChecked() );
    cfg.writeEntry( "onlylater", m_only_later );
    m_more_days = m_gui->SpinBox2->value();
    cfg.writeEntry( "moredays", m_more_days );
    m_maxCharClip = m_gui->SpinBox3->value();
    cfg.writeEntry( "maxcharclip", m_maxCharClip );
    cfg.write();
}

DatebookPluginConfig::~DatebookPluginConfig() {
}
