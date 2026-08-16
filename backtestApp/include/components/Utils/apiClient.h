#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

// Network layer only: talks to the local (PoC) REST API and returns raw
// bytes / JSON. Deliberately knows nothing about Cereal or OHLCBar -
// deserialization is handled by BinarySerializer.
class ApiClient
{
public:
    // GET /files - returns the list of available .bin file names.
    // Sets ok=false and errorMessage on failure (network error, bad JSON, HTTP error).
    static QStringList listFiles(bool& ok, QString& errorMessage);

    // GET /files/<filename> - returns the raw binary content of the file.
    // Sets ok=false and errorMessage on failure.
    static QByteArray downloadFile(const QString& filename, bool& ok, QString& errorMessage);
};
