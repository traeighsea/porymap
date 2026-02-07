#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <QString>
#include <QLineEdit>
#include <QColorSpace>
#include <string>
#include <sstream>

namespace Util {
    void numericalModeSort(QStringList &list);
    int roundUpToMultiple(int numToRound, int multiple);
    QString toDefineCase(QString input);
    QString toHexString(uint32_t value, int minLength = 0);
    QString toHtmlParagraph(const QString &text);
    QString stripPrefix(const QString &s, const QString &prefix);
    Qt::Orientations getOrientation(bool xflip, bool yflip);
    QString replaceExtension(const QString &path, const QString &newExtension);
    void setErrorStylesheet(QLineEdit *lineEdit, bool isError);
    QString toStylesheetString(const QFont &font);
    void show(QWidget *w);
    QColorSpace toColorSpace(int colorSpaceInt);
    QString mkpath(const QString& dirPath);

    std::string replaceFileExtension(const std::string& path, const std::string& extension);
    QString replaceFileExtension(const QString& path, const std::string& extension);
    std::string getFileExtensionFromPath(const QString& path);
    bool hasExtension(const QString& path, const std::string& extension);

    // Careful not to send this into the json!
    template <typename T>
    std::string intToHexStd(T num)
    {
        std::stringstream stream;
        stream << "0x" 
                << std::setfill ('0') << std::setw(sizeof(T)*2) 
                << std::hex << num;
        return stream.str();
    }
    // Usually the conversion you want, since we use QStrings often
    template <typename T>
    QString intToHex(T num)
    {
        return QString::fromStdString(intToHexStd<T>(num));
    }
    template <typename T>
    T hexToInt(std::string str)
    {
        T retVal;
        std::stringstream stream;
        stream << std::hex << str;
        stream >> retVal;
        return retVal;
    }
    template <typename T>
    T hexToInt(QString str)
    {
        std::string stdStr = str.toStdString();
        return hexToInt<T>(stdStr);
    }
}

#endif // UTILITY_H
