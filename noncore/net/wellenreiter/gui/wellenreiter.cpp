/**********************************************************************
** Copyright (C) 2002 Michael 'Mickey' Lauer.  All rights reserved.
**
** This file is part of Opie Environment.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
***********************************************************************/

// Opie

#ifdef QWS
#include <opie/odevice.h>
using namespace Opie;
#endif

#ifdef QWS
#include <opie2/oapplication.h>
#else
#include <qapplication.h>
#endif
#include <opie2/onetwork.h>
#include <opie2/opcap.h>

// Qt

#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qsocketnotifier.h>

// Standard

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

// Local

#include "wellenreiter.h"
#include "scanlist.h"
#include "logwindow.h"
#include "hexwindow.h"
#include "configwindow.h"

#include "manufacturers.h"

Wellenreiter::Wellenreiter( QWidget* parent )
    : WellenreiterBase( parent, 0, 0 ),
      sniffing( false ), iface( 0 ), manufacturerdb( 0 ), configwindow( 0 )
{

    //
    // construct manufacturer database
    //

    QString manufile;
    #ifdef QWS
    manufile.sprintf( "%s/share/wellenreiter/manufacturers.dat", (const char*) QPEApplication::qpeDir() );
    #else
    manufile.sprintf( "/usr/local/share/wellenreiter/manufacturers.dat" );
    #endif
    manufacturerdb = new ManufacturerDB( manufile );

    logwindow->log( "(i) Wellenreiter has been started." );

    //
    // detect operating system
    //

    #ifdef QWS
    QString sys;
    sys.sprintf( "(i) Running on '%s'.", (const char*) ODevice::inst()->systemString() );
    _system = ODevice::inst()->system();
    logwindow->log( sys );
    #endif

    // setup GUI
    netview->setColumnWidthMode( 1, QListView::Manual );

    if ( manufacturerdb )
        netview->setManufacturerDB( manufacturerdb );

    pcap = new OPacketCapturer();

}

Wellenreiter::~Wellenreiter()
{
    // no need to delete child widgets, Qt does it all for us

    delete manufacturerdb;
    delete pcap;
}

void Wellenreiter::setConfigWindow( WellenreiterConfigWindow* cw )
{
    configwindow = cw;
}

void Wellenreiter::receivePacket(OPacket* p)
{
    logwindow->log( "(d) Received data from daemon" );
    //TODO

    // check if we received a beacon frame
    // static_cast is justified here
    OWaveLanManagementPacket* beacon = static_cast<OWaveLanManagementPacket*>( p->child( "802.11 Management" ) );
    if ( !beacon ) return;
    QString type;

    //FIXME: Can stations in ESS mode can be distinguished from APs?
    //FIXME: Apparently yes, but not by listening to beacons, because
    //FIXME: they simply don't send beacons in infrastructure mode.
    //FIXME: so we also have to listen to data packets

    if ( beacon->canIBSS() )
        type = "adhoc";
    else
        type = "managed";

    OWaveLanManagementSSID* ssid = static_cast<OWaveLanManagementSSID*>( p->child( "802.11 SSID" ) );
    QString essid = ssid ? ssid->ID() : QString("<unknown>");
    OWaveLanManagementDS* ds = static_cast<OWaveLanManagementDS*>( p->child( "802.11 DS" ) );
    int channel = ds ? ds->channel() : -1;

    OWaveLanPacket* header = static_cast<OWaveLanPacket*>( p->child( "802.11" ) );
    netView()->addNewItem( type, essid, header->macAddress2().toString(), header->usesWep(), channel, 0 );
}

void Wellenreiter::startStopClicked()
{
    if ( sniffing )
    {
        disconnect( SIGNAL( receivedPacket(OPacket*) ), this, SLOT( receivePacket(OPacket*) ) );

        iface->setChannelHopping(); // stop hopping channels
        pcap->close();
        sniffing = false;
        #ifdef QWS
        oApp->setTitle();
        #else
        qApp->mainWidget()->setCaption( "Wellenreiter II" );
        #endif

        // get interface name from config window
        const QString& interface = configwindow->interfaceName->currentText();
        ONetwork* net = ONetwork::instance();
        iface = static_cast<OWirelessNetworkInterface*>(net->interface( interface ));

        // switch off monitor mode
        iface->setMonitorMode( false );
        // switch off promisc flag
        iface->setPromiscuousMode( false );

        //TODO: Display "please wait..." (use owait?)

        /*

        QString cmdline;
        cmdline.sprintf( "ifdown %s; sleep 1; ifup %s", (const char*) interface, (const char*) interface, (const char*) interface );
        system( cmdline ); //FIXME: Use OProcess

        */

        // message the user

        //QMessageBox::information( this, "Wellenreiter II", "Your wireless card\nshould now be usable again." );
    }

    else
    {
        // get configuration from config window

        const QString& interface = configwindow->interfaceName->currentText();
        const int cardtype = configwindow->daemonDeviceType();
        const int interval = configwindow->daemonHopInterval();

        if ( ( interface == "" ) || ( cardtype == 0 ) )
        {
            QMessageBox::information( this, "Wellenreiter II", "Your device is not\nproperly configured. Please reconfigure!" );
            return;
        }

        // configure device

        ONetwork* net = ONetwork::instance();
        iface = static_cast<OWirelessNetworkInterface*>(net->interface( interface ));

        // set monitor mode

        switch ( cardtype )
        {
            case 1: iface->setMonitoring( new OCiscoMonitoringInterface( iface ) ); break;
            case 2: iface->setMonitoring( new OWlanNGMonitoringInterface( iface ) ); break;
            case 3: iface->setMonitoring( new OHostAPMonitoringInterface( iface ) ); break;
            case 4: iface->setMonitoring( new OOrinocoMonitoringInterface( iface ) ); break;
            default: assert( 0 ); // shouldn't happen
        }

        iface->setMonitorMode( true );

        // open pcap and start sniffing
        pcap->open( interface );

        if ( !pcap->isOpen() )
        {
            QMessageBox::warning( this, "Wellenreiter II", "Can't open packet capturer:\n" + QString(strerror( errno ) ));
            return;
        }

        // set capturer to non-blocking mode
        pcap->setBlocking( false );

        // start channel hopper
        iface->setChannelHopping( 1000 ); //use interval from config window

        // connect
        connect( pcap, SIGNAL( receivedPacket(OPacket*) ), this, SLOT( receivePacket(OPacket*) ) );

        logwindow->log( "(i) Daemon has been started." );
        #ifdef QWS
        oApp->setTitle( "Scanning ..." );
        #else
        qApp->mainWidget()->setCaption( "Wellenreiter II / Scanning ..." );
        #endif
        sniffing = true;

    }
}
