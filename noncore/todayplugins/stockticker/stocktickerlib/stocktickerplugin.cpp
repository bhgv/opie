/*
 * stocktickerplugin.cpp
 *
 * copyright   : (c) 2002 by L.J. Potter
 * email       : llornkcor@handhelds.org
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


#include "stocktickerplugin.h"
#include "stocktickerpluginwidget.h"
#include "stocktickerconfig.h"

StockTickerPlugin::StockTickerPlugin() {
}

StockTickerPlugin::~StockTickerPlugin() {
}

QString StockTickerPlugin::pluginName() const {
    return QObject::tr( "StockTicker plugin" );
}

double StockTickerPlugin::versionNumber() const {
    return 0.6;
}

QString StockTickerPlugin::pixmapNameWidget() const {
    return "stockticker/stockticker";
}

QWidget* StockTickerPlugin::widget( QWidget * wid ) {
    return new StockTickerPluginWidget( wid, "StockTicker " );
}

QString StockTickerPlugin::pixmapNameConfig() const {
    return "stockticker/stockticker";
}

TodayConfigWidget* StockTickerPlugin::configWidget( QWidget* wid ) {
    return new StocktickerPluginConfig( wid , "Stockticker Config" );
}

QString StockTickerPlugin::appName() const {
    return "stockticker";
}

bool StockTickerPlugin::excludeFromRefresh() const {

return true;
}

