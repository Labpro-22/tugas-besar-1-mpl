#include "MonopolyUiShared.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace {

QString findUpwardDirectory(
    const QString& startPath,
    const QString& directoryName,
    const QStringList& requiredFiles
)
{
    QDir cursor(startPath);
    if (!cursor.exists()) {
        return {};
    }

    for (int depth = 0; depth < 8; ++depth) {
        const QString candidate = cursor.absoluteFilePath(directoryName);
        if (QFileInfo::exists(candidate) && QFileInfo(candidate).isDir()) {
            bool allFilesExist = true;
            for (const QString& requiredFile : requiredFiles) {
                if (!QFileInfo::exists(candidate + '/' + requiredFile)) {
                    allFilesExist = false;
                    break;
                }
            }

            if (allFilesExist) {
                return QDir::cleanPath(candidate);
            }
        }

        if (!cursor.cdUp()) {
            break;
        }
    }

    return {};
}

}  // namespace

namespace MonopolyUi {

QString formatCurrency(int amount)
{
    return QStringLiteral("M%1").arg(amount);
}

QString formatTileName(const std::string& rawName)
{
    QString name = QString::fromStdString(rawName);
    name.replace('_', ' ');

    if (name.startsWith(QStringLiteral("STASIUN "))) {
        name.replace(QStringLiteral("STASIUN "), QStringLiteral("STASIUN\n"));
    } else if (name == QStringLiteral("IBU KOTA NUSANTARA")) {
        name = QStringLiteral("IBU KOTA\nNUSANTARA");
    }

    return name;
}

QString findConfigDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("config"),
            {
                QStringLiteral("property.txt"),
                QStringLiteral("railroad.txt"),
                QStringLiteral("utility.txt")
            }
        );

        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString findImagesDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("images"),
            {QStringLiteral("Chance.png")}
        );

        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString findPionDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("pion"),
            {QStringLiteral("ITB.png")}
        );

        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QColor colorFromGroup(ColorGroup colorGroup, const QColor& fallback)
{
    switch (colorGroup) {
    case ColorGroup::COKLAT: return QColor(149, 84, 54);
    case ColorGroup::BIRU_MUDA: return QColor(169, 220, 244);
    case ColorGroup::MERAH_MUDA: return QColor(217, 58, 150);
    case ColorGroup::ORANGE: return QColor(247, 148, 29);
    case ColorGroup::MERAH: return QColor(237, 28, 36);
    case ColorGroup::KUNING: return QColor(254, 242, 0);
    case ColorGroup::HIJAU: return QColor(31, 178, 90);
    case ColorGroup::BIRU_TUA: return QColor(0, 114, 187);
    case ColorGroup::ABU_ABU: return QColor(170, 170, 170);
    case ColorGroup::DEFAULT: return fallback;
    }

    return fallback;
}

}  // namespace MonopolyUi
