#ifndef FILESERVERCONNECTIONSTREAM_H
#define FILESERVERCONNECTIONSTREAM_H

#include <QDataStream>
#include <QList>
#include <QVariantMap>

//==============================================================================
// File Server Connection Stream Class
//==============================================================================
class FileServerConnectionStream : public QDataStream
{
public:
    // Constructor
    FileServerConnectionStream();
    // Constructor
    explicit FileServerConnectionStream(const QByteArray& aArray);

    // Destructor
    virtual ~FileServerConnectionStream();

public:

    // Operator >>
    QDataStream& operator>>(QList<QVariantMap>& aList);
};

#endif // FILESERVERCONNECTIONSTREAM_H
