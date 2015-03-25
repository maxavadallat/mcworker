#include <QDebug>

#include "mcwfileserverconnectionstream.h"

//==============================================================================
// Constructor
//==============================================================================
FileServerConnectionStream::FileServerConnectionStream()
    : QDataStream()
{
    //qDebug() << "FileServerConnectionStream::FileServerConnectionStream";

    // ...
}

//==============================================================================
// Constructor
//==============================================================================
FileServerConnectionStream::FileServerConnectionStream(const QByteArray& aArray)
    : QDataStream(aArray)
{
    //qDebug() << "FileServerConnectionStream::FileServerConnectionStream";

    // ...
}

//==============================================================================
// Read Chars
//==============================================================================
QDataStream& FileServerConnectionStream::operator>>(QList<QVariantMap>& aList)
{
    //qDebug() << "FileServerConnectionStream::operator >> QList<QVariantMap>& aList";

    // Init Temporary Variant Map
    QVariantMap tempMap;

    // Push This To Temp Map
    (*this) >> tempMap;

    // Add Temp Map To List
    aList << tempMap;

    return (*this);
}

//==============================================================================
// Destructor
//==============================================================================
FileServerConnectionStream::~FileServerConnectionStream()
{
    // ...

    //qDebug() << "FileServerConnectionStream::~FileServerConnectionStream";
}

