
#ifndef OPIE_PROFILE_CONFIG_H
#define OPIE_PROFILE_CONFIG_H

#include <qpe/config.h>
#include <qstringlist.h>

class ProfileConfig : public Config {
public:
    ProfileConfig( const QString& prof );
    ~ProfileConfig();
    QStringList groups()const;
    void clearGroup( const QString& );
    void clearAll();

};
#endif
