/*
                             This file is part of the Opie Project
              =.             Copyright (C) 2003-2005 by Michael 'Mickey' Lauer <mickey@Vanille.de>
            .=l.
           .>+-=
 _;:,     .>    :=|.         This program is free software; you can
.> <`_,   >  .   <=          redistribute it and/or  modify it under
:`=1 )Y*s>-.--   :           the terms of the GNU Library General Public
.="- .-=="i,     .._         License as published by the Free Software
 - .   .-<_>     .<>         Foundation; version 2 of the License.
     ._= =}       :
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

/* OPIE */
#include <opie2/onetwork.h>
#include <opie2/ostation.h>
#include <opie2/odebug.h>
using namespace Opie::Core;

/* QT */
#include <qfile.h>
#include <qtextstream.h>
#include <qapplication.h>

/* STD */
#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/sockios.h>
#define u64 __u64
#define u32 __u32
#define u16 __u16
#define u8 __u8
#include <linux/ethtool.h>

#ifndef NODEBUG
#include <opie2/odebugmapper.h>
using namespace Opie::Net::Internal;
DebugMapper* debugmapper = new DebugMapper();
#endif

/*======================================================================================
 * ONetwork
 *======================================================================================*/

namespace Opie {
namespace Net  {
ONetwork* ONetwork::_instance = 0;

ONetwork::ONetwork()
{
    odebug << "ONetwork::ONetwork()" << oendl;
    odebug << "ONetwork: This code has been compiled against Wireless Extensions V" << WIRELESS_EXT << oendl;
    synchronize();
}

void ONetwork::synchronize()
{
    // gather available interfaces by inspecting /proc/net/dev
    _interfaces.clear();
    QString str;
    QFile f( "/proc/net/dev" );
    bool hasFile = f.open( IO_ReadOnly );
    if ( !hasFile )
    {
        odebug << "ONetwork: /proc/net/dev not existing. No network devices available" << oendl;
        return;
    }
    QTextStream s( &f );
    s.readLine();
    s.readLine();
    while ( !s.atEnd() )
    {
        s >> str;
        str.truncate( str.find( ':' ) );
        odebug << "ONetwork: found interface '" << str << "'" << oendl;
        if ( str.startsWith( "wifi" ) )
        {
            odebug << "ONetwork: ignoring hostap control interface" << oendl;
            s.readLine();
            continue;
        }
        ONetworkInterface* iface = 0;
        if ( isWirelessInterface( str ) )
        {
            iface = new OWirelessNetworkInterface( this, (const char*) str );
            odebug << "ONetwork: interface '" << str << "' has Wireless Extensions" << oendl;
        }
        else
        {
            iface = new ONetworkInterface( this, (const char*) str );
        }
        _interfaces.insert( str, iface );
        s.readLine();
    }
}


short ONetwork::wirelessExtensionCompileVersion()
{
    return WIRELESS_EXT;
}


int ONetwork::count() const
{
    return _interfaces.count();
}


ONetworkInterface* ONetwork::interface( const QString& iface ) const
{
    return _interfaces[iface];
}


ONetwork* ONetwork::instance()
{
    if ( !_instance ) _instance = new ONetwork();
    return _instance;
}


ONetwork::InterfaceIterator ONetwork::iterator() const
{
    return ONetwork::InterfaceIterator( _interfaces );
}


bool ONetwork::isPresent( const char* name ) const
{
    int sfd = socket( AF_INET, SOCK_STREAM, 0 );
    struct ifreq ifr;
    memset( &ifr, 0, sizeof( struct ifreq ) );
    strncpy( (char*) &ifr.ifr_name, name, IF_NAMESIZE - 1  );
    int result = ::ioctl( sfd, SIOCGIFFLAGS, &ifr );
    return result != -1;
}


bool ONetwork::isWirelessInterface( const char* name ) const
{
    int sfd = socket( AF_INET, SOCK_STREAM, 0 );
    struct iwreq iwr;
    memset( &iwr, 0, sizeof( struct iwreq ) );
    strncpy( (char*) &iwr.ifr_name, name, IF_NAMESIZE - 1 );
    int result = ::ioctl( sfd, SIOCGIWNAME, &iwr );
    return result != -1;
}

/*======================================================================================
 * ONetworkInterface
 *======================================================================================*/

ONetworkInterface::ONetworkInterface( QObject* parent, const char* name )
                 :QObject( parent, name ),
                 _sfd( socket( AF_INET, SOCK_DGRAM, 0 ) ), _mon( 0 )
{
    odebug << "ONetworkInterface::ONetworkInterface()" << oendl;
    init();
}


struct ifreq& ONetworkInterface::ifr() const
{
    return _ifr;
}


void ONetworkInterface::init()
{
    odebug << "ONetworkInterface::init()" << oendl;
    memset( &_ifr, 0, sizeof( struct ifreq ) );

    if ( _sfd == -1 )
    {
        odebug << "ONetworkInterface::init(): Warning - can't get socket for device '" << name() << "'" << oendl;
        return;
    }
}


bool ONetworkInterface::ioctl( int call, struct ifreq& ifreq ) const
{
    #ifndef NODEBUG
    int result = ::ioctl( _sfd, call, &ifreq );
    if ( result == -1 )
        odebug << "ONetworkInterface::ioctl (" << name() << ") call '" << debugmapper->map( call )
               << "' FAILED! " << result << " (" << strerror( errno ) << ")" << oendl;
    else
        odebug << "ONetworkInterface::ioctl (" << name() << ") call '" << debugmapper->map( call )
               << "' - Status: Ok." << oendl;
    return ( result != -1 );
    #else
    return ::ioctl( _sfd, call, &ifreq ) != -1;
    #endif
}


bool ONetworkInterface::ioctl( int call ) const
{
    strncpy( _ifr.ifr_name, name(), IF_NAMESIZE );
    return ioctl( call, _ifr );
}


bool ONetworkInterface::isLoopback() const
{
    ioctl( SIOCGIFFLAGS );
    return _ifr.ifr_flags & IFF_LOOPBACK;
}


bool ONetworkInterface::setUp( bool b )
{
    ioctl( SIOCGIFFLAGS );
    if ( b ) _ifr.ifr_flags |= IFF_UP;
    else   _ifr.ifr_flags &= (~IFF_UP);
    return ioctl( SIOCSIFFLAGS );
}


bool ONetworkInterface::isUp() const
{
    ioctl( SIOCGIFFLAGS );
    return _ifr.ifr_flags & IFF_UP;
}


void ONetworkInterface::setIPV4Address( const QHostAddress& addr )
{
    struct sockaddr_in *sa = (struct sockaddr_in *) &_ifr.ifr_addr;
    sa->sin_family = AF_INET;
    sa->sin_port = 0;
    sa->sin_addr.s_addr = htonl( addr.ip4Addr() );
    ioctl( SIOCSIFADDR );
}


OHostAddress ONetworkInterface::ipV4Address() const
{
    struct sockaddr_in* sa = (struct sockaddr_in *) &_ifr.ifr_addr;
    return ioctl( SIOCGIFADDR ) ? OHostAddress( ntohl( sa->sin_addr.s_addr ) ) : OHostAddress();
}


void ONetworkInterface::setMacAddress( const OMacAddress& addr )
{
    _ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy( &_ifr.ifr_hwaddr.sa_data, addr.native(), 6 );
    ioctl( SIOCSIFHWADDR );
}


OMacAddress ONetworkInterface::macAddress() const
{
    if ( ioctl( SIOCGIFHWADDR ) )
    {
        return OMacAddress( _ifr );
    }
    else
    {
        return OMacAddress::unknown;
    }
}


void ONetworkInterface::setIPV4Netmask( const QHostAddress& addr )
{
    struct sockaddr_in *sa = (struct sockaddr_in *) &_ifr.ifr_addr;
    sa->sin_family = AF_INET;
    sa->sin_port = 0;
    sa->sin_addr.s_addr = htonl( addr.ip4Addr() );
    ioctl( SIOCSIFNETMASK );
}


OHostAddress ONetworkInterface::ipV4Netmask() const
{
    struct sockaddr_in* sa = (struct sockaddr_in *) &_ifr.ifr_addr;
    return ioctl( SIOCGIFNETMASK ) ? OHostAddress( ntohl( sa->sin_addr.s_addr ) ) : OHostAddress();
}


int ONetworkInterface::dataLinkType() const
{
    if ( ioctl( SIOCGIFHWADDR ) )
    {
        return _ifr.ifr_hwaddr.sa_family;
    }
    else
    {
        return -1;
    }
}


void ONetworkInterface::setMonitoring( OMonitoringInterface* m )
{
    _mon = m;
    odebug << "ONetwork::setMonitoring(): Installed monitoring driver '" << m->name() << "' on interface '" << name() << "'" << oendl;
}


OMonitoringInterface* ONetworkInterface::monitoring() const
{
    return _mon;
}


ONetworkInterface::~ONetworkInterface()
{
    odebug << "ONetworkInterface::~ONetworkInterface()" << oendl;
    if ( _sfd != -1 ) ::close( _sfd );
}


bool ONetworkInterface::setPromiscuousMode( bool b )
{
    ioctl( SIOCGIFFLAGS );
    if ( b ) _ifr.ifr_flags |= IFF_PROMISC;
    else   _ifr.ifr_flags &= (~IFF_PROMISC);
    return ioctl( SIOCSIFFLAGS );
}


bool ONetworkInterface::promiscuousMode() const
{
    ioctl( SIOCGIFFLAGS );
    return _ifr.ifr_flags & IFF_PROMISC;
}


bool ONetworkInterface::isWireless() const
{
    return ioctl( SIOCGIWNAME );
}


ONetworkInterfaceDriverInfo ONetworkInterface::driverInfo() const
{
        struct ethtool_drvinfo info;
        info.cmd = ETHTOOL_GDRVINFO;
        _ifr.ifr_data = (caddr_t) &info;
        return ioctl( SIOCETHTOOL ) ? ONetworkInterfaceDriverInfo( info.driver, info.version, info.fw_version, info.bus_info) : ONetworkInterfaceDriverInfo();
}

/*======================================================================================
 * OChannelHopper
 *======================================================================================*/

OChannelHopper::OChannelHopper( OWirelessNetworkInterface* iface )
               :QObject( 0, "Mickey's funky hopper" ),
               _iface( iface ), _interval( 0 ), _tid( 0 ), d( 0 )
{
    int _maxChannel = iface->channels();
    // generate fancy hopping sequence honoring the device capabilities
    if ( _maxChannel >=  1 ) _channels.append(  1 );
    if ( _maxChannel >=  7 ) _channels.append(  7 );
    if ( _maxChannel >= 13 ) _channels.append( 13 );
    if ( _maxChannel >=  2 ) _channels.append(  2 );
    if ( _maxChannel >=  8 ) _channels.append(  8 );
    if ( _maxChannel >=  3 ) _channels.append(  3 );
    if ( _maxChannel >= 14 ) _channels.append( 14 );
    if ( _maxChannel >=  9 ) _channels.append(  9 );
    if ( _maxChannel >=  4 ) _channels.append(  4 );
    if ( _maxChannel >= 10 ) _channels.append( 10 );
    if ( _maxChannel >=  5 ) _channels.append(  5 );
    if ( _maxChannel >= 11 ) _channels.append( 11 );
    if ( _maxChannel >=  6 ) _channels.append(  6 );
    if ( _maxChannel >= 12 ) _channels.append( 12 );
    //FIXME: Add 802.11a/g channels
    _channel = _channels.begin();
}


OChannelHopper::~OChannelHopper()
{
}


bool OChannelHopper::isActive() const
{
    return _tid;
}


int OChannelHopper::channel() const
{
    return *_channel;
}


void OChannelHopper::timerEvent( QTimerEvent* )
{
    _iface->setChannel( *_channel );
    emit( hopped( *_channel ) );
    odebug << "OChannelHopper::timerEvent(): set channel " << *_channel << " on interface '" << _iface->name() << "'" << oendl;
    if ( ++_channel == _channels.end() ) _channel = _channels.begin();
}


void OChannelHopper::setInterval( int interval )
{
    if ( interval == _interval )
        return;

    if ( _interval )
        killTimer( _tid );

    _tid = 0;
    _interval = interval;

    if ( _interval )
    {
        _tid = startTimer( interval );
    }
}


int OChannelHopper::interval() const
{
    return _interval;
}


/*======================================================================================
 * OWirelessNetworkInterface
 *======================================================================================*/

OWirelessNetworkInterface::OWirelessNetworkInterface( QObject* parent, const char* name )
                         :ONetworkInterface( parent, name ), _hopper( 0 )
{
    odebug << "OWirelessNetworkInterface::OWirelessNetworkInterface()" << oendl;
    init();
}


OWirelessNetworkInterface::~OWirelessNetworkInterface()
{
}


struct iwreq& OWirelessNetworkInterface::iwr() const
{
    return _iwr;
}


void OWirelessNetworkInterface::init()
{
    odebug << "OWirelessNetworkInterface::init()" << oendl;
    memset( &_iwr, 0, sizeof( struct iwreq ) );
    buildInformation();
    buildPrivateList();
    dumpInformation();
}


bool OWirelessNetworkInterface::isAssociated() const
{
    //FIXME: handle different modes
    return !(associatedAP() == OMacAddress::unknown);
}


void OWirelessNetworkInterface::setAssociatedAP( const OMacAddress& mac ) const
{
    _iwr.u.ap_addr.sa_family = ARPHRD_ETHER;
    ::memcpy(_iwr.u.ap_addr.sa_data, mac.native(), ETH_ALEN);
    wioctl( SIOCSIWAP );
}


OMacAddress OWirelessNetworkInterface::associatedAP() const
{
    if ( ioctl( SIOCGIWAP ) )
        return (const unsigned char*) &_ifr.ifr_hwaddr.sa_data[0];
    else
        return OMacAddress::unknown;
}


void OWirelessNetworkInterface::buildInformation()
{
    //ML: If you listen carefully enough, you can hear lots of WLAN drivers suck
    //ML: The HostAP drivers need more than sizeof struct_iw range to complete
    //ML: SIOCGIWRANGE otherwise they fail with "Invalid Argument Length".
    //ML: The Wlan-NG drivers on the otherside fail (segfault!) if you allocate
    //ML: _too much_ space. This is damn shitty crap *sigh*
    //ML: We allocate a large memory region in RAM and check whether the
    //ML: driver pollutes this extra space. The complaint will be made on stdout,
    //ML: so please forward this...

    struct iwreq wrq;
    int len = sizeof( struct iw_range )*2;
    char buffer[len];
    memset( buffer, 0, len );
    memcpy( wrq.ifr_name, name(), IFNAMSIZ);
    wrq.u.data.pointer = (caddr_t) buffer;
    wrq.u.data.length = sizeof buffer;
    wrq.u.data.flags = 0;

    int result = ::ioctl( _sfd, SIOCGIWRANGE, &wrq );
    if ( result == -1 )
    {
        owarn << "OWirelessNetworkInterface::buildInformation(): SIOCGIWRANGE failed (" << strerror( errno ) << ") - retrying with smaller buffer..." << oendl;
        wrq.u.data.length = sizeof( struct iw_range );
        result = ::ioctl( _sfd, SIOCGIWRANGE, &wrq );
    }

    if ( result == -1 )
    {
        owarn << "OWirelessNetworkInterface::buildInformation(): Can't get driver information (" << strerror( errno ) << ") - using default values." << oendl;
        _channels.insert( 2412,  1 ); // 2.412 GHz
        _channels.insert( 2417,  2 ); // 2.417 GHz
        _channels.insert( 2422,  3 ); // 2.422 GHz
        _channels.insert( 2427,  4 ); // 2.427 GHz
        _channels.insert( 2432,  5 ); // 2.432 GHz
        _channels.insert( 2437,  6 ); // 2.437 GHz
        _channels.insert( 2442,  7 ); // 2.442 GHz
        _channels.insert( 2447,  8 ); // 2.447 GHz
        _channels.insert( 2452,  9 ); // 2.452 GHz
        _channels.insert( 2457, 10 ); // 2.457 GHz
        _channels.insert( 2462, 11 ); // 2.462 GHz

        memset( &_range, 0, sizeof( struct iw_range ) );
    }
    else
    {
        // <check if the driver overwrites stuff>
        int max = 0;
        for ( int r = sizeof( struct iw_range ); r < len; r++ )
            if (buffer[r] != 0)
                max = r;
        if (max > 0)
        {
            owarn << "OWirelessNetworkInterface::buildInformation(): Driver for wireless interface '" << name()
                  << "' sucks! It overwrote the buffer end with at least " << max - sizeof( struct iw_range ) << " bytes!" << oendl;
        }
        // </check if the driver overwrites stuff>

        struct iw_range range;
        memcpy( &range, buffer, sizeof range );

        odebug << "OWirelessNetworkInterface::buildInformation(): Interface reported to have " << (int) range.num_frequency << " channels." << oendl;
        for ( int i = 0; i < range.num_frequency; ++i )
        {
            int freq = (int) ( double( range.freq[i].m ) * pow( 10.0, range.freq[i].e ) / 1000000.0 );
            odebug << "OWirelessNetworkInterface::buildInformation: Adding frequency " << freq << " as channel " << i+1 << oendl;
            _channels.insert( freq, i+1 );
        }
    }

    memcpy( &_range, buffer, sizeof( struct iw_range ) );
    odebug << "OWirelessNetworkInterface::buildInformation(): Information block constructed." << oendl;
}


short OWirelessNetworkInterface::wirelessExtensionDriverVersion() const
{
    return _range.we_version_compiled;
}


void OWirelessNetworkInterface::buildPrivateList()
{
    odebug << "OWirelessNetworkInterface::buildPrivateList()" << oendl;

    struct iw_priv_args priv[IW_MAX_PRIV_DEF];

    _iwr.u.data.pointer = (char*) &priv;
    _iwr.u.data.length = IW_MAX_PRIV_DEF; // length in terms of number of (sizeof iw_priv_args), not (sizeof iw_priv_args) itself
    _iwr.u.data.flags = 0;

    if ( !wioctl( SIOCGIWPRIV ) )
    {
        owarn << "OWirelessNetworkInterface::buildPrivateList(): Can't get private ioctl information (" << strerror( errno ) << ")." << oendl;
        return;
    }

    for ( int i = 0; i < _iwr.u.data.length; ++i )
    {
        new OPrivateIOCTL( this, priv[i].name, priv[i].cmd, priv[i].get_args, priv[i].set_args );
    }
    odebug << "OWirelessNetworkInterface::buildPrivateList(): Private ioctl list constructed." << oendl;
}


void OWirelessNetworkInterface::dumpInformation() const
{
    odebug << "OWirelessNetworkInterface::() -------------- dumping information block ----------------" << oendl;

    odebug << " - driver's idea of maximum throughput is " << _range.throughput
           << " bps = " << ( _range.throughput / 8 ) << " byte/s = " << ( _range.throughput / 8 / 1024 )
           << " Kb/s = " << QString().sprintf("%f.2", float( _range.throughput ) / 8.0 / 1024.0 / 1024.0 )
           << " Mb/s" << oendl;

    odebug << " - driver for '" << name() << "' (V" << _range.we_version_source
           << ") has been compiled against WE V" << _range.we_version_compiled << oendl;

    if ( _range.we_version_compiled != WIRELESS_EXT )
    {
        owarn << "Version mismatch! WE_DRIVER = " << _range.we_version_compiled << " and WE_OPIENET = " << WIRELESS_EXT << oendl;
    }

    odebug << "OWirelessNetworkInterface::() ---------------------------------------------------------" << oendl;
}


int OWirelessNetworkInterface::channel() const
{
    //FIXME: When monitoring enabled, then use it
    //FIXME: to gather the current RF channel
    //FIXME: Until then, get active channel from hopper.
    if ( _hopper && _hopper->isActive() )
        return _hopper->channel();

    if ( !wioctl( SIOCGIWFREQ ) )
    {
        return -1;
    }
    else
    {
        return _channels[ static_cast<int>(double( _iwr.u.freq.m ) * pow( 10.0, _iwr.u.freq.e ) / 1000000) ];
    }
}


void OWirelessNetworkInterface::setChannel( int c ) const
{
    if ( !c )
    {
        oerr << "OWirelessNetworkInterface::setChannel( 0 ) called - fix your application!" << oendl;
        return;
    }

    if ( !_mon )
    {
        memset( &_iwr, 0, sizeof( struct iwreq ) );
        _iwr.u.freq.m = c;
        _iwr.u.freq.e = 0;
        wioctl( SIOCSIWFREQ );
    }
    else
    {
        _mon->setChannel( c );
    }
}


double OWirelessNetworkInterface::frequency() const
{
    if ( !wioctl( SIOCGIWFREQ ) )
    {
        return -1.0;
    }
    else
    {
        return double( _iwr.u.freq.m ) * pow( 10.0, _iwr.u.freq.e ) / 1000000000.0;
    }
}


int OWirelessNetworkInterface::channels() const
{
    return _channels.count();
}


void OWirelessNetworkInterface::setChannelHopping( int interval )
{
    if ( !_hopper ) _hopper = new OChannelHopper( this );
    _hopper->setInterval( interval );
    //FIXME: When and by whom will the channel hopper be deleted?
    //TODO: rely on QObject hierarchy
}


int OWirelessNetworkInterface::channelHopping() const
{
    return _hopper->interval();
}


OChannelHopper* OWirelessNetworkInterface::channelHopper() const
{
    return _hopper;
}


void OWirelessNetworkInterface::commit() const
{
    wioctl( SIOCSIWCOMMIT );
}


void OWirelessNetworkInterface::setMode( const QString& newMode )
{
    #ifdef FINALIZE
    QString currentMode = mode();
    if ( currentMode == newMode ) return;
    #endif

    odebug << "OWirelessNetworkInterface::setMode(): trying to set mode " << newMode << oendl;

    _iwr.u.mode = stringToMode( newMode );

    if ( _iwr.u.mode != IW_MODE_MONITOR )
    {
        // IWR.U.MODE WIRD DURCH ABFRAGE DES MODE HIER PLATTGEMACHT!!!!!!!!!!!!!!!!!!!!! DEPP!
        _iwr.u.mode = stringToMode( newMode );
        wioctl( SIOCSIWMODE );

        // special iwpriv fallback for monitor mode (check if we're really out of monitor mode now)

        if ( mode() == "monitor" )
        {
            odebug << "OWirelessNetworkInterface::setMode(): SIOCSIWMODE not sufficient - trying fallback to iwpriv..." << oendl;
            if ( _mon )
                _mon->setEnabled( false );
            else
                odebug << "ONetwork(): can't switch monitor mode without installed monitoring interface" << oendl;
        }

    }
    else    // special iwpriv fallback for monitor mode
    {
        if ( wioctl( SIOCSIWMODE ) )
        {
            odebug << "OWirelessNetworkInterface::setMode(): IW_MODE_MONITOR ok" << oendl;
        }
        else
        {
            odebug << "OWirelessNetworkInterface::setMode(): SIOCSIWMODE not working - trying fallback to iwpriv..." << oendl;

            if ( _mon )
                _mon->setEnabled( true );
            else
                odebug << "ONetwork(): can't switch monitor mode without installed monitoring interface" << oendl;
        }
    }
}


QString OWirelessNetworkInterface::mode() const
{
    memset( &_iwr, 0, sizeof( struct iwreq ) );

    if ( !wioctl( SIOCGIWMODE ) )
    {
        return "<unknown>";
    }

    odebug << "OWirelessNetworkInterface::setMode(): WE's idea of current mode seems to be " << modeToString( _iwr.u.mode ) << oendl;

    // legacy compatible monitor mode check

    if ( dataLinkType() == ARPHRD_IEEE80211 || dataLinkType() == 802 )
    {
        return "monitor";
    }
    else
    {
        return modeToString( _iwr.u.mode );
    }
}

void OWirelessNetworkInterface::setNickName( const QString& nickname )
{
    _iwr.u.essid.pointer = const_cast<char*>( (const char*) nickname );
    _iwr.u.essid.length = nickname.length();
    wioctl( SIOCSIWNICKN );
}


QString OWirelessNetworkInterface::nickName() const
{
    char str[IW_ESSID_MAX_SIZE];
    _iwr.u.data.pointer = &str[0];
    _iwr.u.data.length = IW_ESSID_MAX_SIZE;
    if ( !wioctl( SIOCGIWNICKN ) )
    {
        return "<unknown>";
    }
    else
    {
        str[_iwr.u.data.length] = '\0'; // some drivers don't zero-terminate the string
        return str;
    }
}


void OWirelessNetworkInterface::setPrivate( const QString& call, int numargs, ... )
{
    OPrivateIOCTL* priv = static_cast<OPrivateIOCTL*>( child( (const char*) call ) );
    if ( !priv )
    {
        owarn << "OWirelessNetworkInterface::setPrivate(): interface '" << name()
              << "' does not support private ioctl '" << call << "'" << oendl;
        return;
    }
    if ( priv->numberSetArgs() != numargs )
    {
        owarn << "OWirelessNetworkInterface::setPrivate(): parameter count not matching. '"
              << call << "' expects " << priv->numberSetArgs() << ", but got " << numargs << oendl;
        return;
    }

    odebug << "OWirelessNetworkInterface::setPrivate(): about to call '" << call << "' on interface '" << name() << "'" << oendl;
    memset( &_iwr, 0, sizeof _iwr );
    va_list argp;
    va_start( argp, numargs );
    for ( int i = 0; i < numargs; ++i )
    {
        priv->setParameter( i, va_arg( argp, int ) );
    }
    va_end( argp );
    priv->invoke();
}


void OWirelessNetworkInterface::getPrivate( const QString&  )
{
    oerr << "OWirelessNetworkInterface::getPrivate() is not implemented yet." << oendl;
}


bool OWirelessNetworkInterface::hasPrivate( const QString& call )
{
    return child( call.local8Bit() );
}


QString OWirelessNetworkInterface::SSID() const
{
    char str[IW_ESSID_MAX_SIZE];
    _iwr.u.essid.pointer = &str[0];
    _iwr.u.essid.length = IW_ESSID_MAX_SIZE;
    if ( !wioctl( SIOCGIWESSID ) )
    {
        return "<unknown>";
    }
    else
    {
        str[_iwr.u.essid.length] = '\0'; // some drivers don't zero-terminate the string
        return str;
    }
}


void OWirelessNetworkInterface::setSSID( const QString& ssid )
{
    _iwr.u.essid.pointer = const_cast<char*>( (const char*) ssid );
    _iwr.u.essid.length = ssid.length()+1; // zero byte
    wioctl( SIOCSIWESSID );
}


OStationList* OWirelessNetworkInterface::scanNetwork()
{
    _iwr.u.param.flags = IW_SCAN_DEFAULT;
    _iwr.u.param.value = 0;
    if ( !wioctl( SIOCSIWSCAN ) )
    {
        return 0;
    }

    OStationList* stations = new OStationList();

    int timeout = 10000000;

    odebug << "ONetworkInterface::scanNetwork() - scan started." << oendl;

    bool results = false;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000; // initial timeout ~ 250ms
    char buffer[IW_SCAN_MAX_DATA];

    while ( !results && timeout > 0 )
    {
        timeout -= tv.tv_usec;
        select( 0, 0, 0, 0, &tv );

        _iwr.u.data.pointer = &buffer[0];
        _iwr.u.data.flags = 0;
        _iwr.u.data.length = sizeof buffer;
        if ( wioctl( SIOCGIWSCAN ) )
        {
            results = true;
            continue;
        }
        else if ( errno == EAGAIN)
        {
            odebug << "ONetworkInterface::scanNetwork() - scan in progress..." << oendl;
            if ( qApp )
            {
                qApp->processEvents( 100 );
                continue;
            }
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            continue;
        }
    }

    odebug << "ONetworkInterface::scanNetwork() - scan finished." << oendl;

    if ( results )
    {
        odebug << " - result length = " << _iwr.u.data.length << oendl;
        if ( !_iwr.u.data.length )
        {
            odebug << " - no results (empty neighbourhood)" << oendl;
            return stations;
        }

        odebug << " - results are in!" << oendl;
        dumpBytes( (const unsigned char*) &buffer[0], _iwr.u.data.length );

        // parse results
        struct iw_event iwe;
        struct iw_stream_descr stream;
        unsigned int cmd_index = 0, event_type = 0, event_len = 0;
        char *pointer;

        const char standard_ioctl_hdr[] = {
                IW_HEADER_TYPE_NULL,    /* SIOCSIWCOMMIT */
                IW_HEADER_TYPE_CHAR,    /* SIOCGIWNAME */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWNWID */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWNWID */
                IW_HEADER_TYPE_FREQ,    /* SIOCSIWFREQ */
                IW_HEADER_TYPE_FREQ,    /* SIOCGIWFREQ */
                IW_HEADER_TYPE_UINT,    /* SIOCSIWMODE */
                IW_HEADER_TYPE_UINT,    /* SIOCGIWMODE */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWSENS */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWSENS */
                IW_HEADER_TYPE_NULL,    /* SIOCSIWRANGE */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWRANGE */
                IW_HEADER_TYPE_NULL,    /* SIOCSIWPRIV */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWPRIV */
                IW_HEADER_TYPE_NULL,    /* SIOCSIWSTATS */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWSTATS */
                IW_HEADER_TYPE_POINT,   /* SIOCSIWSPY */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWSPY */
                IW_HEADER_TYPE_POINT,   /* SIOCSIWTHRSPY */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWTHRSPY */
                IW_HEADER_TYPE_ADDR,    /* SIOCSIWAP */
                IW_HEADER_TYPE_ADDR,    /* SIOCGIWAP */
                IW_HEADER_TYPE_NULL,    /* -- hole -- */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWAPLIST */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWSCAN */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWSCAN */
                IW_HEADER_TYPE_POINT,   /* SIOCSIWESSID */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWESSID */
                IW_HEADER_TYPE_POINT,   /* SIOCSIWNICKN */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWNICKN */
                IW_HEADER_TYPE_NULL,    /* -- hole -- */
                IW_HEADER_TYPE_NULL,    /* -- hole -- */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWRATE */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWRATE */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWRTS */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWRTS */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWFRAG */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWFRAG */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWTXPOW */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWTXPOW */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWRETRY */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWRETRY */
                IW_HEADER_TYPE_POINT,   /* SIOCSIWENCODE */
                IW_HEADER_TYPE_POINT,   /* SIOCGIWENCODE */
                IW_HEADER_TYPE_PARAM,   /* SIOCSIWPOWER */
                IW_HEADER_TYPE_PARAM,   /* SIOCGIWPOWER */
        };

        const char standard_event_hdr[] = {
                IW_HEADER_TYPE_ADDR,    /* IWEVTXDROP */
                IW_HEADER_TYPE_QUAL,    /* IWEVQUAL */
                IW_HEADER_TYPE_POINT,   /* IWEVCUSTOM */
                IW_HEADER_TYPE_ADDR,    /* IWEVREGISTERED */
                IW_HEADER_TYPE_ADDR,    /* IWEVEXPIRED */
                IW_HEADER_TYPE_POINT,   /* IWEVGENIE */
                IW_HEADER_TYPE_POINT,   /* IWEVMICHAELMICFAILURE */
                IW_HEADER_TYPE_POINT,   /* IWEVASSOCREQIE */
                IW_HEADER_TYPE_POINT,   /* IWEVASSOCRESPIE */
                IW_HEADER_TYPE_POINT,   /* IWEVPMKIDCAND */
        };


        const int event_type_size[] = {
                IW_EV_LCP_LEN,          /* IW_HEADER_TYPE_NULL */
                0,
                IW_EV_CHAR_LEN,         /* IW_HEADER_TYPE_CHAR */
                0,
                IW_EV_UINT_LEN,         /* IW_HEADER_TYPE_UINT */
                IW_EV_FREQ_LEN,         /* IW_HEADER_TYPE_FREQ */
                IW_EV_ADDR_LEN,         /* IW_HEADER_TYPE_ADDR */
                0,
                IW_EV_POINT_LEN,        /* Without variable payload */
                IW_EV_PARAM_LEN,        /* IW_HEADER_TYPE_PARAM */
                IW_EV_QUAL_LEN,         /* IW_HEADER_TYPE_QUAL */
        };


        //Initialize the stream
        memset( &stream, 0, sizeof(struct iw_stream_descr) );
        stream.current = buffer;
        stream.end = buffer + _iwr.u.data.length;

        do
        {
            if ((stream.current + IW_EV_LCP_LEN) > stream.end)
                break;
            memcpy((char *) &iwe, stream.current, IW_EV_LCP_LEN);

            if (iwe.len <= IW_EV_LCP_LEN) //If yes, it is an invalid event
                break;
            if (iwe.cmd <= SIOCIWLAST) {
                cmd_index = iwe.cmd - SIOCIWFIRST;

                if(cmd_index < sizeof(standard_ioctl_hdr))
                    event_type = standard_ioctl_hdr[cmd_index];
            }
            else {
                cmd_index = iwe.cmd - IWEVFIRST;

                if(cmd_index < sizeof(standard_event_hdr))
                    event_type = standard_event_hdr[cmd_index];
            }

            /* Unknown events -> event_type=0 => IW_EV_LCP_LEN */
            event_len = event_type_size[event_type];

            /* Fixup for later version of WE */
            if((_range.we_version_compiled > 18) && (event_type == IW_HEADER_TYPE_POINT))
                event_len -= IW_EV_POINT_OFF;

            /* Check if we know about this event */
            if(event_len <= IW_EV_LCP_LEN) {
                /* Skip to next event */
                stream.current += iwe.len;
                continue;
            }

            event_len -= IW_EV_LCP_LEN;

            /* Set pointer on data */
            if(stream.value != NULL)
                pointer = stream.value;                    /* Next value in event */
            else
                pointer = stream.current + IW_EV_LCP_LEN;  /* First value in event */

            if((pointer + event_len) > stream.end) {
                /* Go to next event */
                stream.current += iwe.len;
                break;
            }

            /* Fixup for later version of WE */
            if((_range.we_version_compiled > 18) && (event_type == IW_HEADER_TYPE_POINT))
                memcpy((char *) &iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pointer, event_len);
            else
                memcpy((char *) &iwe + IW_EV_LCP_LEN, pointer, event_len);

            /* Skip event in the stream */
            pointer += event_len;

            /* Special processing for iw_point events */
            if(event_type == IW_HEADER_TYPE_POINT) {
            /* Check the length of the payload */

                if((iwe.len - (event_len + IW_EV_LCP_LEN)) > 0)
                    /* Set pointer on variable part (warning : non aligned) */
                    iwe.u.data.pointer = pointer;
                else
                    /* No data */
                    iwe.u.data.pointer = NULL;
                    /* Go to next event */
                    stream.current += iwe.len;
            }

            else {
                /* Is there more value in the event ? */
                if((pointer + event_len) <= (stream.current + iwe.len))
                    /* Go to next value */
                     stream.value = pointer;
                else {
                    /* Go to next event */
                    stream.value = NULL;
                    stream.current += iwe.len;
                }
            }

            struct iw_event *we = &iwe;
            //------
            odebug << " - reading next event... cmd=" << we->cmd << ", len=" << we->len << oendl;
            switch (we->cmd)
            {
                case SIOCGIWAP:
                {
                    odebug << "SIOCGIWAP" << oendl;
                    stations->append( new OStation() );
                    stations->last()->macAddress = (const unsigned char*) &we->u.ap_addr.sa_data[0];
                    break;
                }
                case SIOCGIWMODE:
                {
                    odebug << "SIOCGIWMODE" << oendl;
                    stations->last()->type = modeToString( we->u.mode );
                    break;
                }
                case SIOCGIWFREQ:
                {
                    odebug << "SIOCGIWFREQ" << oendl;
                    if ( we->u.freq.m > 1000 )
                        stations->last()->channel = _channels[ static_cast<int>(double( we->u.freq.m ) * pow( 10.0, we->u.freq.e ) / 1000000) ];
                    else
                        stations->last()->channel = static_cast<int>(((double) we->u.freq.m) * pow( 10.0, we->u.freq.e ));
                    break;
                }
                case SIOCGIWESSID:
                {
                    odebug << "SIOCGIWESSID" << oendl;
                    we->u.essid.length = '\0'; // make sure it is zero terminated
                    stations->last()->ssid = static_cast<const char*> (we->u.essid.pointer);
                    odebug << "ESSID: " << stations->last()->ssid << oendl;
                    break;
                }
                case IWEVQUAL:
                {
                    odebug << "IWEVQUAL" << oendl;
                    stations->last()->level = static_cast<int>(we->u.qual.level);
                    break;             /* Quality part of statistics (scan) */
                }
                case SIOCGIWENCODE:
                {
                    odebug << "SIOCGIWENCODE" << oendl;
                    stations->last()->encrypted = !(we->u.data.flags & IW_ENCODE_DISABLED);
                    break;
                }

                case SIOCGIWRATE:
                {
                    odebug << "SIOCGIWRATE" << oendl;
                    stations->last()->rates.append(we->u.bitrate.value);
                    break;
                }
                case SIOCGIWSENS: odebug << "SIOCGIWSENS" << oendl; break;
                case IWEVTXDROP: odebug << "IWEVTXDROP" << oendl; break;         /* Packet dropped to excessive retry */
                case IWEVCUSTOM: odebug << "IWEVCUSTOM" << oendl; break;         /* Driver specific ascii string */
                case IWEVREGISTERED: odebug << "IWEVREGISTERED" << oendl; break; /* Discovered a new node (AP mode) */
                case IWEVEXPIRED: odebug << "IWEVEXPIRED" << oendl; break;       /* Expired a node (AP mode) */
                default: odebug << "unhandled event" << oendl;
            }

        } while (true);
    }
    else
    {
        odebug << " - no results (timeout) :(" << oendl;
    }
    return stations;
}


int OWirelessNetworkInterface::signalStrength() const
{
    iw_statistics stat;
    ::memset( &stat, 0, sizeof stat );
    _iwr.u.data.pointer = (char*) &stat;
    _iwr.u.data.flags = 0;
    _iwr.u.data.length = sizeof stat;

    if ( !wioctl( SIOCGIWSTATS ) )
    {
        return -1;
    }

    int max = _range.max_qual.qual;
    int cur = stat.qual.qual;
//    int lev = stat.qual.level; //FIXME: Do something with them?
//    int noi = stat.qual.noise; //FIXME: Do something with them?


    return max != 0 ? cur*100/max: -1;
}


bool OWirelessNetworkInterface::wioctl( int call, struct iwreq& iwreq ) const
{
    #ifndef NODEBUG
    int result = ::ioctl( _sfd, call, &iwreq );

    if ( result == -1 )
        odebug << "ONetworkInterface::wioctl (" << name() << ") call '"
               << debugmapper->map( call ) << "' FAILED! " << result << " (" << strerror( errno ) << ")" << oendl;
    else
        odebug << "ONetworkInterface::wioctl (" << name() << ") call '"
               << debugmapper->map( call ) << "' - Status: Ok." << oendl;

    return ( result != -1 );
    #else
    return ::ioctl( _sfd, call, &iwreq ) != -1;
    #endif
}


bool OWirelessNetworkInterface::wioctl( int call ) const
{
    strncpy( _iwr.ifr_name, name(), IF_NAMESIZE - 1 );
    return wioctl( call, _iwr );
}


/*======================================================================================
 * OMonitoringInterface
 *======================================================================================*/

OMonitoringInterface::OMonitoringInterface( ONetworkInterface* iface, bool prismHeader )
                      :_if( static_cast<OWirelessNetworkInterface*>( iface ) ), _prismHeader( prismHeader ), d( 0 )
{
}


OMonitoringInterface::~OMonitoringInterface()
{
}


void OMonitoringInterface::setChannel( int c )
{
    // use standard WE channel switching protocol
    memset( &_if->_iwr, 0, sizeof( struct iwreq ) );
    _if->_iwr.u.freq.m = c;
    _if->_iwr.u.freq.e = 0;
    _if->wioctl( SIOCSIWFREQ );
}


void OMonitoringInterface::setEnabled( bool  )
{
}


/*======================================================================================
 * OCiscoMonitoringInterface
 *======================================================================================*/

OCiscoMonitoringInterface::OCiscoMonitoringInterface( ONetworkInterface* iface, bool prismHeader )
                           :OMonitoringInterface( iface, prismHeader )
{
    iface->setMonitoring( this );
}


OCiscoMonitoringInterface::~OCiscoMonitoringInterface()
{
}


void OCiscoMonitoringInterface::setEnabled( bool /*b*/ )
{
    QString fname;
    fname.sprintf( "/proc/driver/aironet/%s", (const char*) _if->name() );
    QFile f( fname );
    if ( !f.exists() ) return;

    if ( f.open( IO_WriteOnly ) )
    {
        QTextStream s( &f );
        s << "Mode: r";
        s << "Mode: y";
        s << "XmitPower: 1";
    }

    // flushing and closing will be done automatically when f goes out of scope
}


QString OCiscoMonitoringInterface::name() const
{
    return "cisco";
}


void OCiscoMonitoringInterface::setChannel( int )
{
    // cisco devices automatically switch channels when in monitor mode
}


/*======================================================================================
 * OWlanNGMonitoringInterface
 *======================================================================================*/


OWlanNGMonitoringInterface::OWlanNGMonitoringInterface( ONetworkInterface* iface, bool prismHeader )
                           :OMonitoringInterface( iface, prismHeader )
{
    iface->setMonitoring( this );
}


OWlanNGMonitoringInterface::~OWlanNGMonitoringInterface()
{
}


void OWlanNGMonitoringInterface::setEnabled( bool b )
{
    //FIXME: do nothing if its already in the same mode

    QString enable = b ? "true" : "false";
    QString prism = _prismHeader ? "true" : "false";
    QString cmd;
    cmd.sprintf( "$(which wlanctl-ng) %s lnxreq_wlansniff channel=%d enable=%s prismheader=%s",
                 (const char*) _if->name(), 1, (const char*) enable, (const char*) prism );
    system( cmd );
}


QString OWlanNGMonitoringInterface::name() const
{
    return "wlan-ng";
}


void OWlanNGMonitoringInterface::setChannel( int c )
{
    //NOTE: Older wlan-ng drivers automatically hopped channels while lnxreq_wlansniff=true. Newer ones don't.

    QString enable = "true"; //_if->monitorMode() ? "true" : "false";
    QString prism = _prismHeader ? "true" : "false";
    QString cmd;
    cmd.sprintf( "$(which wlanctl-ng) %s lnxreq_wlansniff channel=%d enable=%s prismheader=%s",
                 (const char*) _if->name(), c, (const char*) enable, (const char*) prism );
    system( cmd );
}


/*======================================================================================
 * OHostAPMonitoringInterface
 *======================================================================================*/

OHostAPMonitoringInterface::OHostAPMonitoringInterface( ONetworkInterface* iface, bool prismHeader )
                           :OMonitoringInterface( iface, prismHeader )
{
    iface->setMonitoring( this );
}

OHostAPMonitoringInterface::~OHostAPMonitoringInterface()
{
}

void OHostAPMonitoringInterface::setEnabled( bool b )
{
    int monitorCode = _prismHeader ? 1 : 2;
    if ( b )
    {
        _if->setPrivate( "monitor", 1, monitorCode );
    }
    else
    {
        _if->setPrivate( "monitor", 1, 0 );
    }
}


QString OHostAPMonitoringInterface::name() const
{
    return "hostap";
}


/*======================================================================================
 * OOrinocoNetworkInterface
 *======================================================================================*/

OOrinocoMonitoringInterface::OOrinocoMonitoringInterface( ONetworkInterface* iface, bool prismHeader )
                           :OMonitoringInterface( iface, prismHeader )
{
    iface->setMonitoring( this );
}


OOrinocoMonitoringInterface::~OOrinocoMonitoringInterface()
{
}


void OOrinocoMonitoringInterface::setChannel( int c )
{
    if ( !_if->hasPrivate( "monitor" ) )
    {
        this->OMonitoringInterface::setChannel( c );
    }
    else
    {
        int monitorCode = _prismHeader ? 1 : 2;
        _if->setPrivate( "monitor", 2, monitorCode, c );
    }
}


void OOrinocoMonitoringInterface::setEnabled( bool b )
{
    if ( b )
    {
        setChannel( 1 );
    }
    else
    {
        _if->setPrivate( "monitor", 2, 0, 0 );
    }
}


QString OOrinocoMonitoringInterface::name() const
{
    return "orinoco";
}

}
}
