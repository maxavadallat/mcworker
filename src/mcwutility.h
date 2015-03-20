#ifndef UTILITY
#define UTILITY

#include <QString>

#include "mcwinterface.h"



// Is On Same Drive
bool isOnSameDrive(const QString& aPathOne, const QString& aPathTwo);

// Get Parent Dir
QString getParentDir(const QString& aDirPath);

// Get Extension
QString getExtension(const QString& aFilePath);

// Set File Attributes
bool setAttributes(const QString& aFilePath, const int& aAttributes);

// Get File Attributes
int getAttributes(const QString& aFilePath);

// Create Dir
int createDir(const QString& aDirPath);

// Delete File
int deleteFile(const QString& aFilePath);



// Sort List


#endif // UTILITY

