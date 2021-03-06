/*
 * addressplugin.h
 *
 * copyright   : (c) 2003 by Stefan Eilers
 * email       : eilers.stefan@epost.de
 *
 * This implementation was derived from the todolist plugin implementation
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

#ifndef ADDRESSBOOK_PLUGIN_H
#define ADDRESSBOOK_PLUGIN_H

#include "addresspluginwidget.h"

#include <opie2/oclickablelabel.h>
#include <opie2/todayplugininterface.h>

#include <qstring.h>
#include <qwidget.h>

class AddressBookPlugin : public TodayPluginObject {

public:
    AddressBookPlugin();
    ~AddressBookPlugin();

    QString pluginName()  const;
    double versionNumber() const;
    QString pixmapNameWidget() const;
    QWidget* widget(QWidget *);
    QString pixmapNameConfig() const;
    TodayConfigWidget* configWidget(QWidget *);
    QString appName() const;
    bool excludeFromRefresh() const;
    void refresh();
    void reinitialize();

 private:
    AddressBookPluginWidget* m_abWidget;
};

#endif
