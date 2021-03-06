
#ifndef BLUEBASE_H
#define BLUEBASE_H

#include <qvariant.h>
#include <qwidget.h>
#include <qscrollview.h>
#include <qsplitter.h>
#include <qlist.h>
#include <qpixmap.h>

#include "bluetoothbase.h"

#include "btserviceitem.h"
#include "btdeviceitem.h"

#include "popuphelper.h"

#include "bticonloader.h"
#include "forwarder.h"

#include <opie2/obluetoothdevicerecord.h>
#include <manager.h>

class QVBox;
class QHBoxLayout;
class QGridLayout;
class QFrame;
class QLabel;
class QPushButton;
class QTabWidget;
class QCheckBox;
class BTConnectionItem;
namespace Opie {
namespace Bluez {
    class OBluetooth;
    class OBluetoothInterface;
    class OBluetoothDevice;
    class DeviceHandlerPool;
    class DeviceRecord;
}
}

using namespace Opie::Bluez;

namespace OpieTooth {

    class BlueBase : public BluetoothBase {
        Q_OBJECT

    public:
        BlueBase( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
        ~BlueBase();

        static QString appName() { return QString::fromLatin1("bluetooth-manager"); }

    protected:


    private slots:
        void startScan();


    private:
        bool find( const DeviceRecord& device );
        void readConfig();
        void writeConfig();
        void readSavedDevices();
        void writeSavedDevices();
        void initGui();
        void updateStatus();
        void connectInterface(const OBluetoothInterface *);
        void removeDevice( const QString &bdaddr );
        void pairDevice( const QString &bdaddr );
        void updateDeviceActive( BTDeviceItem * item );
        BTDeviceItem *findDeviceItem( const QString &bdaddr );

        DeviceHandlerPool *m_devHandlerPool;
        PopupHelper *m_popHelper;
        Manager *m_localDevice;
        OBluetooth *m_bluetooth;
        QStringList m_servicesDevices;

        QString m_defaultPasskey;

        QPixmap m_offPix;
        QPixmap m_onPix;
        QPixmap m_findPix;

        BTIconLoader *m_iconLoader;
        SerialForwarder* forwarder;

        bool m_loadedDevices;

    private slots:
        void addSearchedDevices( const QValueList<DeviceRecord> &newDevices );
        void addServicesToDevices();
        void addServicesToDevice( BTDeviceItem *item );
        void addConnectedDevices();
        void addConnectedDevices( ConnectionState::ValueList );
        void startServiceActionClicked( QListViewItem *item );
        void startServiceActionHold( QListViewItem *, const QPoint &, int );
        void applyConfigChanges();
        void doForward();
        void doShowPasskey(bool);
        void forwardExit(Opie::Core::OProcess* proc);
        void editServices();
        void addSignalStrength();
        void addSignalStrength( const QString& mac, const QString& strengh );
        void rfcommDialog();
        void showEvent(QShowEvent *);
        void interfacePropertyChanged(const QString&);
        void devicePropertyChanged(OBluetoothDevice *, const QString&);
        void defaultInterfaceChanged( OBluetoothInterface *);
        void deviceFound( OBluetoothDevice *dev, bool newDevice );
        void servicesFound( OBluetoothDevice *dev );
        void deviceHandlerSuccess(DeviceHandler*,const QString&,const QString&);
        void deviceHandlerFailure(DeviceHandler*,const QString&,const QString&,const QString&);
    };

}

#endif
