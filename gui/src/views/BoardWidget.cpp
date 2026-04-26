#include "views/BoardWidget.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygon>
#include <QRegularExpression>
#include <QStringList>
#include <QtGui/QFontMetrics>
#include <QtMath>

#include <algorithm>
#include <exception>
#include <map>

#include "utils/UiCommon.hpp"
#include "models/Enums.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/PropertyConfig.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  PALETTE  (exact Figma hex values)
// ─────────────────────────────────────────────────────────────────────────────
namespace Pal {
    const QColor bg       (250, 247, 255);
    const QColor frame    (255, 255, 255);
    const QColor line     (  0,   0,   0);
    const QColor ink      ( 55,  65,  81);
    const QColor white    (255, 252, 250);

    // property strips
    const QColor brown    (186, 156, 240);
    const QColor sky      ( 98, 153, 238);
    const QColor pink     (154,  84, 236);
    const QColor orange   (255, 127, 120);
    const QColor red      (255,  76,  34);
    const QColor yellow   (255, 211,  28);
    const QColor green    ( 16, 174,  91);
    const QColor darkBlue ( 82,  76, 255);

    // special
    const QColor jailOr   (255,  76,  34);
    const QColor chestBlue(106, 154, 244);
    const QColor chanceBg (255,  76,  34);
    const QColor logoRed  ( 82,  76, 255);
}

namespace {
QString formatCurrency(int amount)
{
    return QString::number(amount);
}

QString priceText(const QString& amount)
{
    if (amount.isEmpty()) {
        return {};
    }
    QString cleaned = amount;
    cleaned.remove(QStringLiteral("M"));
    cleaned.replace(QStringLiteral("BAYAR"), QStringLiteral("PAY"));
    return cleaned.startsWith(QStringLiteral("PAY"))
        ? cleaned.simplified()
        : QStringLiteral("PRICE %1").arg(cleaned.simplified());
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

QString findUpwardDirectory(const QString& startPath, const QString& directoryName, const QStringList& requiredFiles)
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

QString findImagesDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("images"),
            {QStringLiteral("chance_icon.png")}
        );

        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString findHouseHotelsDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("househotels"),
            {QStringLiteral("Property 1=1 House.png")}
        );

        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString buildingAssetName(int buildingLevel)
{
    if (buildingLevel >= 5) {
        return QStringLiteral("Property 1=Hotel.png");
    }

    switch (buildingLevel) {
    case 1: return QStringLiteral("Property 1=1 House.png");
    case 2: return QStringLiteral("Property 1=2 House.png");
    case 3: return QStringLiteral("Property 1=3 House.png");
    case 4: return QStringLiteral("Property 1=4 House.png");
    default: return {};
    }
}

QColor colorFromGroup(ColorGroup colorGroup, const QColor& fallback)
{
    switch (colorGroup) {
    case ColorGroup::COKLAT: return Pal::brown;
    case ColorGroup::BIRU_MUDA: return Pal::sky;
    case ColorGroup::MERAH_MUDA: return Pal::pink;
    case ColorGroup::ORANGE: return Pal::orange;
    case ColorGroup::MERAH: return Pal::red;
    case ColorGroup::KUNING: return Pal::yellow;
    case ColorGroup::HIJAU: return Pal::green;
    case ColorGroup::BIRU_TUA: return Pal::darkBlue;
    default: return fallback;
    }
}

QString initialsForName(const QString& name)
{
    const QStringList parts = name.split(QRegularExpression(QStringLiteral("[_\\s]+")), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return name.left(2).toUpper();
    }
    if (parts.size() == 1) {
        return parts.front().left(2).toUpper();
    }
    return (parts.front().left(1) + parts.back().left(1)).toUpper();
}

QFont fittedTextFont(
    const QString& family,
    int desiredPointSize,
    int minimumPointSize,
    int weight,
    const QString& text,
    const QRect& rect,
    int flags
)
{
    for (int pointSize = desiredPointSize; pointSize > minimumPointSize; --pointSize) {
        QFont font(family, pointSize, weight);
        const QRect bounds = QFontMetrics(font).boundingRect(rect, flags, text);
        if (bounds.width() <= rect.width() && bounds.height() <= rect.height()) {
            return font;
        }
    }

    return QFont(family, minimumPointSize, weight);
}
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────
BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent), cells(createCells())
{
    setMinimumSize(560, 560);
    setAutoFillBackground(false);
}
BoardWidget::~BoardWidget() = default;

void BoardWidget::setActivePawnName(const QString& pawnName)
{
    if (activePawnName == pawnName) {
        return;
    }

    activePawnName = pawnName;
    update();
}

void BoardWidget::setSelectedPropertyId(int propertyId)
{
    if (selectedPropertyId == propertyId) {
        return;
    }

    selectedPropertyId = propertyId;
    update();
}

void BoardWidget::setTileSelectionMode(const QSet<int>& validTileIndices, const QString& promptText, bool allowCancel)
{
    tileSelectionMode = true;
    tileSelectionAllowCancel = allowCancel;
    selectableTileIndices = validTileIndices;
    selectionPromptText = promptText;
    setCursor(Qt::PointingHandCursor);
    update();
}

void BoardWidget::clearTileSelectionMode()
{
    if (!tileSelectionMode && selectableTileIndices.isEmpty() && selectionPromptText.isEmpty() && !tileSelectionAllowCancel) {
        return;
    }

    tileSelectionMode = false;
    tileSelectionAllowCancel = false;
    selectableTileIndices.clear();
    selectionPromptText.clear();
    unsetCursor();
    update();
}

int BoardWidget::tileCount() const
{
    return cells.size();
}

void BoardWidget::setConfigData(const ConfigData& config)
{
    configData = &config;
    cells = createCells();
    update();
}

int BoardWidget::tileIndexForPropertyId(int propertyId) const
{
    for (int index = 0; index < cells.size(); ++index) {
        if (cells[index].propertyId == propertyId) {
            return index;
        }
    }
    return propertyId - 1;
}

int BoardWidget::propertyIdForTileIndex(int tileIndex) const
{
    if (tileIndex < 0 || tileIndex >= cells.size()) {
        return 0;
    }
    return cells[tileIndex].propertyId;
}

void BoardWidget::setPawns(const QVector<PawnData>& pawnData)
{
    pawns = pawnData;
    update();
}

void BoardWidget::setBuildings(const QVector<BuildingData>& buildingData)
{
    buildings = buildingData;
    update();
}

void BoardWidget::setOwners(const QVector<OwnerData>& ownerData)
{
    owners = ownerData;
    update();
}

// ─────────────────────────────────────────────────────────────────────────────
//  PIXMAP LOADER  — tries multiple locations for the images/ folder
// ─────────────────────────────────────────────────────────────────────────────
const QPixmap& BoardWidget::pix(const QString &name) const
{
    if (pixCache.contains(name))
        return pixCache.find(name).value();

    QPixmap pm;

    const QStringList searchDirs = {findImagesDirectory(), findHouseHotelsDirectory()};
    for (const QString& directory : searchDirs) {
        if (directory.isEmpty()) {
            continue;
        }

        const QString path = directory + '/' + name;
        if (QFileInfo::exists(path) && pm.load(path)) {
            break;
        }
    }

    pixCache.insert(name, pm);          // mutable: safe in const method
    return pixCache.find(name).value();
}

void BoardWidget::drawPawn(QPainter &p, const QRectF &r, const PawnData &pawn) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    const bool isActivePawn = pawn.name == activePawnName;

    QRectF shadowRect = r.translated(2, 3);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(143, 168, 216, 80));
    p.drawEllipse(shadowRect);

    if (isActivePawn) {
        p.setBrush(QColor(pawn.accentColor.red(), pawn.accentColor.green(), pawn.accentColor.blue(), 80));
        p.drawEllipse(r.adjusted(-6, -6, 6, 6));
    }

    p.setBrush(pawn.accentColor.isValid() ? pawn.accentColor : QColor(230, 230, 230));
    p.setPen(QPen(isActivePawn ? QColor(255, 245, 175) : Pal::line, isActivePawn ? 2.6 : 1.2));
    p.drawEllipse(r);

    const QRectF iconRect = r.adjusted(r.width() * 0.16, r.height() * 0.16, -r.width() * 0.16, -r.height() * 0.16);
    const QPixmap &icon = pix(pawn.iconName);

    if (!icon.isNull()) {
        p.setClipPath(QPainterPath());
        p.save();
        QPainterPath clip;
        clip.addEllipse(iconRect);
        p.setClipPath(clip);
        const QSize scaledSize = icon.size().scaled(iconRect.size().toSize(), Qt::KeepAspectRatio);
        const QRect target(
            int(iconRect.center().x() - scaledSize.width() / 2.0),
            int(iconRect.center().y() - scaledSize.height() / 2.0),
            scaledSize.width(),
            scaledSize.height()
        );
        p.drawPixmap(target, icon);
        p.restore();
    } else {
        QFont font(QStringLiteral("Trebuchet MS"), qMax(6, int(r.width() * 0.30)));
        font.setBold(true);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(iconRect, Qt::AlignCenter, initialsForName(pawn.name));
    }

    p.setPen(QPen(isActivePawn ? Qt::white : Pal::line, isActivePawn ? 1.8 : 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);
    p.restore();
}

void BoardWidget::drawPawns(QPainter &p, const QRect &board, int cs, int es) const
{
    if (pawns.isEmpty()) {
        return;
    }

    QMap<int, QVector<const PawnData*>> grouped;
    for (const PawnData &pawn : pawns) {
        int tileIndex = pawn.tileIndex;
        if (tileIndex < 0 || tileIndex >= cells.size()) {
            tileIndex = 0;
        }
        grouped[tileIndex].append(&pawn);
    }

    const qreal tokenSize = qMax<qreal>(18.0, qMin(cs, es) * 0.26);

    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        const int tileIndex = it.key();
        const QVector<const PawnData*> &items = it.value();
        const QRect tile = tileRect(tileIndex, board, cs, es);

        QVector<QPointF> centers;
        centers.reserve(items.size());

        const qreal left = tile.left();
        const qreal top = tile.top();
        const qreal right = tile.right();
        const qreal bottom = tile.bottom();
        const qreal midX = tile.center().x();
        const qreal midY = tile.center().y();

        if (items.size() == 1) {
            centers.append(QPointF(midX, midY));
        } else if (items.size() == 2) {
            centers.append(QPointF(midX - tokenSize * 0.35, midY - tokenSize * 0.18));
            centers.append(QPointF(midX + tokenSize * 0.35, midY + tokenSize * 0.18));
        } else if (items.size() == 3) {
            centers.append(QPointF(midX - tokenSize * 0.35, midY - tokenSize * 0.28));
            centers.append(QPointF(midX + tokenSize * 0.35, midY - tokenSize * 0.28));
            centers.append(QPointF(midX, midY + tokenSize * 0.30));
        } else {
            centers.append(QPointF(left + tokenSize * 0.9, top + tokenSize * 0.9));
            centers.append(QPointF(right - tokenSize * 0.9, top + tokenSize * 0.9));
            centers.append(QPointF(left + tokenSize * 0.9, bottom - tokenSize * 0.9));
            centers.append(QPointF(right - tokenSize * 0.9, bottom - tokenSize * 0.9));
        }

        for (int i = 0; i < items.size(); ++i) {
            const qreal currentTokenSize = items[i]->name == activePawnName ? tokenSize * 1.15 : tokenSize;
            const QRectF tokenRect(
                centers[qMin(i, centers.size() - 1)].x() - currentTokenSize / 2.0,
                centers[qMin(i, centers.size() - 1)].y() - currentTokenSize / 2.0,
                currentTokenSize,
                currentTokenSize
            );
            drawPawn(p, tokenRect, *items[i]);
        }
    }
}

// helper: draw pixmap centred + scaled inside rect, with aspect-ratio kept
static void drawPix(QPainter &p, const QPixmap &pm, const QRect &r)
{
    if (pm.isNull()) return;
    QSize scaled = pm.size().scaled(r.size(), Qt::KeepAspectRatio);
    QRect dest(r.center().x() - scaled.width()/2,
               r.center().y() - scaled.height()/2,
               scaled.width(), scaled.height());
    p.drawPixmap(dest, pm);
}

void BoardWidget::drawBuildings(QPainter &p, const QRect &board, int cs, int es) const
{
    if (buildings.isEmpty()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    for (const BuildingData& building : buildings) {
        if (building.tileIndex < 0 || building.tileIndex >= cells.size() || building.buildingLevel <= 0) {
            continue;
        }
        if (cells[building.tileIndex].kind != TileKind::Property) {
            continue;
        }

        const QRect tile = tileRect(building.tileIndex, board, cs, es);
        const int size = qMax(20, int(qMin(tile.width(), tile.height()) * 0.38));
        QRect iconRect;

        switch (sideForIndex(building.tileIndex)) {
        case EdgeSide::Bottom:
            iconRect = QRect(tile.center().x() - size / 2, tile.top() + tile.height() / 6, size, size);
            break;
        case EdgeSide::Top:
            iconRect = QRect(tile.center().x() - size / 2, tile.bottom() - tile.height() / 6 - size, size, size);
            break;
        case EdgeSide::Left:
            iconRect = QRect(tile.right() - tile.width() / 6 - size, tile.center().y() - size / 2, size, size);
            break;
        case EdgeSide::Right:
            iconRect = QRect(tile.left() + tile.width() / 6, tile.center().y() - size / 2, size, size);
            break;
        case EdgeSide::Corner:
            continue;
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(143, 168, 216, 52));
        p.drawEllipse(iconRect.adjusted(2, size - size / 5, -2, 3));

        p.setBrush(Qt::white);
        p.setPen(QPen(QColor(42, 55, 72), qMax(1, size / 16)));
        p.drawRoundedRect(iconRect, 3, 3);

        QFont font(QStringLiteral("Trebuchet MS"), qMax(7, size / 3), QFont::Black);
        p.setFont(font);
        p.setPen(QColor(42, 55, 72));
        p.drawText(iconRect, Qt::AlignCenter, building.buildingLevel >= 5 ? QStringLiteral("H") : QString::number(building.buildingLevel));
    }

    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  TILE DATA  (index 0=GO at bottom-right, clockwise)
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::drawOwners(QPainter &p, const QRect &board, int cs, int es) const
{
    if (owners.isEmpty()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    for (const OwnerData& owner : owners) {
        if (owner.tileIndex < 0 || owner.tileIndex >= cells.size()) {
            continue;
        }
        if (!isInspectableTile(owner.tileIndex) || owner.ownerName.isEmpty()) {
            continue;
        }

        const QRect tile = tileRect(owner.tileIndex, board, cs, es);
        const int badgeHeight = qMax(14, qMin(tile.width(), tile.height()) / 5);
        const int badgeWidth = qMax(28, qMin(tile.width() - 8, badgeHeight * 3));
        const QRect badge(tile.left() + 4, tile.bottom() - badgeHeight - 4, badgeWidth, badgeHeight);

        const QColor accent = owner.accentColor.isValid() ? owner.accentColor : QColor(40, 40, 40);
        p.setPen(QPen(Pal::line, 1));
        p.setBrush(accent);
        p.drawRoundedRect(badge, 4, 4);

        QFont font(QStringLiteral("Trebuchet MS"), qMax(6, badgeHeight / 2), QFont::Black);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(badge, Qt::AlignCenter, initialsForName(owner.ownerName));

        if (owner.mortgaged) {
            const QRect mortgageBadge(badge.right() + 3, badge.top(), badgeHeight, badgeHeight);
            p.setPen(QPen(Pal::line, 1));
            p.setBrush(QColor(126, 116, 196));
            p.drawRoundedRect(mortgageBadge, 4, 4);
            p.setPen(Qt::white);
            p.drawText(mortgageBadge, Qt::AlignCenter, QStringLiteral("M"));
        }

        if (owner.festivalDuration > 0 && owner.festivalMultiplier > 1) {
            const int festivalWidth = qMax(badgeHeight + 8, badgeHeight * 2);
            int festivalLeft = badge.right() + 3 + (owner.mortgaged ? badgeHeight + 3 : 0);
            int festivalTop = badge.top();
            if (festivalLeft + festivalWidth > tile.right() - 4) {
                festivalLeft = badge.left();
                festivalTop = badge.top() - badgeHeight - 3;
            }

            const QRect festivalBadge(festivalLeft, festivalTop, festivalWidth, badgeHeight);
            p.setPen(QPen(Pal::line, 1));
            p.setBrush(QColor(255, 191, 54));
            p.drawRoundedRect(festivalBadge, 4, 4);

            QFont festivalFont(QStringLiteral("Trebuchet MS"), qMax(5, badgeHeight / 2), QFont::Black);
            p.setFont(festivalFont);
            p.setPen(QColor(38, 30, 0));
            p.drawText(
                festivalBadge,
                Qt::AlignCenter,
                QStringLiteral("x%1").arg(owner.festivalMultiplier));
        }
    }

    p.restore();
}

QVector<BoardWidget::CellData> BoardWidget::createCells() const
{
    QVector<CellData> d(40);

    // Shorthands
    auto prop = [&](int i,const QString &price,const QString &name,const QColor &strip){
        d[i]={TileKind::Property, price, name, strip, true, strip, i + 1};
    };
    auto spec = [&](int i,TileKind k,const QString &price,const QString &name,
                    const QColor &accent=Pal::line){
        d[i]={k, price, name, {}, false, accent, 0};
    };

    // ── BOTTOM (indices 0–9, right→left) ──
    spec( 0, TileKind::CornerGo,          "",          "Petak Mulai");
    prop( 1, "M60",   "GARUT",            Pal::brown);
    spec( 2, TileKind::CommunityChest,    "",          "DANA\nUMUM",    Pal::darkBlue);
    prop( 3, "M60",   "TASIKMALAYA",      Pal::brown);
    spec( 4, TileKind::IncomeTax,         "PAY M150",  "PPH",           Pal::line);
    spec( 5, TileKind::Railroad,          "M200",      "STASIUN\nGAMBIR");
    prop( 6, "M100",  "BOGOR",            Pal::sky);
    spec( 7, TileKind::Festival,          "",          "FESTIVAL",      Pal::pink);
    prop( 8, "M100",  "DEPOK",            Pal::sky);
    prop( 9, "M120",  "BEKASI",           Pal::sky);

    // ── LEFT (indices 10–19, bottom→top) ──
    spec(10, TileKind::CornerJail,        "",          "PENJARA");
    prop(11, "M140",  "MAGELANG",         Pal::pink);
    spec(12, TileKind::UtilityElec,       "M150",      "PLN",           Pal::line);
    prop(13, "M140",  "SOLO",             Pal::pink);
    prop(14, "M160",  "YOGYAKARTA",       Pal::pink);
    spec(15, TileKind::Railroad,          "M200",      "STASIUN\nBANDUNG");
    prop(16, "M180",  "MALANG",           Pal::orange);
    spec(17, TileKind::CommunityChest,    "",          "DANA\nUMUM",    Pal::darkBlue);
    prop(18, "M180",  "SEMARANG",         Pal::orange);
    prop(19, "M200",  "SURABAYA",         Pal::orange);

    // ── TOP (indices 20–29, left→right) ──
    spec(20, TileKind::CornerFreeParking, "",          "BEBAS PARKIR");
    prop(21, "M220",  "MAKASSAR",         Pal::red);
    spec(22, TileKind::Chance,            "",          "KESEMPATAN",    Pal::darkBlue);
    prop(23, "M240",  "BALIKPAPAN",       Pal::red);
    prop(24, "M260",  "MANADO",           Pal::red);
    spec(25, TileKind::Railroad,          "M200",      "STASIUN\nTUGU");
    prop(26, "M260",  "PALEMBANG",        Pal::yellow);
    prop(27, "M260",  "PEKANBARU",        Pal::yellow);
    spec(28, TileKind::UtilityWater,      "M150",      "PAM",           Pal::line);
    prop(29, "M280",  "MEDAN",            Pal::yellow);

    // ── RIGHT (indices 30–39, top→bottom) ──
    spec(30, TileKind::CornerGoToJail,    "",          "PERGI KE\nPENJARA");
    prop(31, "M300",  "BANDUNG",          Pal::green);
    prop(32, "M300",  "DENPASAR",         Pal::green);
    spec(33, TileKind::Festival,          "",          "FESTIVAL",      Pal::pink);
    prop(34, "M320",  "MATARAM",          Pal::green);
    spec(35, TileKind::Railroad,          "M200",      "STASIUN\nGUBENG");
    spec(36, TileKind::Chance,            "",          "KESEMPATAN",    Pal::orange);
    prop(37, "M350",  "JAKARTA",          Pal::darkBlue);
    spec(38, TileKind::LuxuryTax,         "M150",      "PPNBM",         Pal::line);
    prop(39, "M400",  "IKN",              Pal::darkBlue);

    if (configData != nullptr) {
        try {
            const ConfigData& config = *configData;

            auto propertyCell = [](const PropertyConfig& property) {
                CellData cell;
                cell.name = formatTileName(property.getName());
                cell.stripColor = colorFromGroup(property.getColorGroup(), Pal::line);
                cell.accentColor = cell.stripColor;
                cell.hasStrip = property.getPropertyType() == PropertyType::STREET;
                cell.propertyId = property.getId();
                cell.price = property.getBuyPrice() > 0 ? formatCurrency(property.getBuyPrice()) : QString();

                switch (property.getPropertyType()) {
                case PropertyType::STREET:
                    cell.kind = TileKind::Property;
                    break;
                case PropertyType::RAILROAD:
                    cell.kind = TileKind::Railroad;
                    cell.stripColor = QColor();
                    cell.accentColor = Pal::line;
                    break;
                case PropertyType::UTILITY:
                    cell.kind = (property.getCode() == "PLN") ? TileKind::UtilityElec : TileKind::UtilityWater;
                    cell.stripColor = QColor();
                    cell.accentColor = Pal::line;
                    break;
                }

                return cell;
            };

            auto actionCell = [&config](const ActionTileConfig& action) {
                CellData cell;
                cell.name = formatTileName(action.getName());
                cell.stripColor = QColor();
                cell.hasStrip = false;
                cell.accentColor = colorFromGroup(action.getColorGroup(), Pal::line);
                cell.propertyId = 0;
                cell.price = QString();

                const QString code = QString::fromStdString(action.getCode());
                const std::string& tileType = action.getTileType();
                if (tileType == "KARTU") {
                    cell.kind = code == QStringLiteral("DNU") ? TileKind::CommunityChest : TileKind::Chance;
                } else if (tileType == "PAJAK") {
                    if (code == QStringLiteral("PPH")) {
                        cell.kind = TileKind::IncomeTax;
                        cell.price = QStringLiteral("BAYAR %1").arg(formatCurrency(config.getTaxConfig().getPphFlat()));
                    } else {
                        cell.kind = TileKind::LuxuryTax;
                        cell.price = QStringLiteral("BAYAR %1").arg(formatCurrency(config.getTaxConfig().getPbmFlat()));
                    }
                } else if (tileType == "FESTIVAL") {
                    cell.kind = TileKind::Festival;
                    cell.accentColor = Pal::pink;
                } else if (tileType == "SPESIAL") {
                    if (code == QStringLiteral("GO")) {
                        cell.kind = TileKind::CornerGo;
                        cell.price = config.getSpecialConfig().getGoSalary() > 0
                            ? formatCurrency(config.getSpecialConfig().getGoSalary())
                            : QString();
                    } else if (code == QStringLiteral("PEN")) {
                        cell.kind = TileKind::CornerJail;
                    } else if (code == QStringLiteral("PPJ")) {
                        cell.kind = TileKind::CornerGoToJail;
                    } else {
                        cell.kind = TileKind::CornerFreeParking;
                    }
                } else {
                    cell.kind = TileKind::Festival;
                }

                return cell;
            };

            std::map<int, const PropertyConfig*> propertyById;
            std::map<std::string, const PropertyConfig*> propertyByCode;
            for (const PropertyConfig& property : config.getPropertyConfigs()) {
                propertyById[property.getId()] = &property;
                propertyByCode[property.getCode()] = &property;
            }

            std::map<int, const ActionTileConfig*> actionById;
            std::map<std::string, const ActionTileConfig*> actionByCode;
            for (const ActionTileConfig& action : config.getActionTileConfigs()) {
                actionById[action.getId()] = &action;
                if (actionByCode.find(action.getCode()) == actionByCode.end()) {
                    actionByCode[action.getCode()] = &action;
                }
            }

            QVector<CellData> configured;
            const std::vector<std::string>& layoutCodes = config.getBoardLayoutCodes();
            if (!layoutCodes.empty()) {
                configured.reserve(static_cast<int>(layoutCodes.size()));
                for (const std::string& code : layoutCodes) {
                    const auto propertyIt = propertyByCode.find(code);
                    if (propertyIt != propertyByCode.end()) {
                        configured.push_back(propertyCell(*propertyIt->second));
                        continue;
                    }

                    const auto actionIt = actionByCode.find(code);
                    if (actionIt != actionByCode.end()) {
                        configured.push_back(actionCell(*actionIt->second));
                    }
                }
            } else {
                int boardSize = 0;
                for (const auto& entry : propertyById) {
                    boardSize = std::max(boardSize, entry.first);
                }
                for (const auto& entry : actionById) {
                    boardSize = std::max(boardSize, entry.first);
                }

                configured.resize(boardSize);
                for (int id = 1; id <= boardSize; ++id) {
                    const auto actionIt = actionById.find(id);
                    if (actionIt != actionById.end()) {
                        configured[id - 1] = actionCell(*actionIt->second);
                        continue;
                    }

                    const auto propertyIt = propertyById.find(id);
                    if (propertyIt != propertyById.end()) {
                        configured[id - 1] = propertyCell(*propertyIt->second);
                    }
                }
            }

            if (configured.size() >= 20 && configured.size() <= 60 && configured.size() % 4 == 0) {
                return configured;
            }
        } catch (const std::exception&) {
            // Keep fallback static board data when config files are unavailable.
        }
    }

    return d;
}

// ─────────────────────────────────────────────────────────────────────────────
//  GEOMETRY
// ─────────────────────────────────────────────────────────────────────────────
QRect BoardWidget::boardBounds() const
{
    const int margin = qMax(2, qMin(width(), height()) / 220);
    const int s = qMax(0, qMin(width(), height()) - 2 * margin);
    return { (width() - s) / 2, (height() - s) / 2, s, s };
}
BoardWidget::EdgeSide BoardWidget::sideForIndex(int i) const
{
    const int sideTileCount = qMax(1, cells.size() / 4);
    if (i % sideTileCount == 0) return EdgeSide::Corner;
    if (i < sideTileCount) return EdgeSide::Bottom;
    if (i < 2 * sideTileCount) return EdgeSide::Left;
    if (i < 3 * sideTileCount) return EdgeSide::Top;
    return EdgeSide::Right;
}
QRect BoardWidget::tileRect(int i, const QRect &b, int cs, int es) const
{
    int x=b.x(), y=b.y();
    const int sideTileCount = qMax(1, cells.size() / 4);
    const int edgeSlots = qMax(1, sideTileCount - 1);
    if (i == 0) return {x + cs + edgeSlots * es, y + cs + edgeSlots * es, cs, cs};
    if (i >= 1 && i < sideTileCount) return {x + cs + (sideTileCount - i - 1) * es, y + cs + edgeSlots * es, es, cs};
    if (i == sideTileCount) return {x, y + cs + edgeSlots * es, cs, cs};
    if (i > sideTileCount && i < 2 * sideTileCount) return {x, y + cs + (2 * sideTileCount - i - 1) * es, cs, es};
    if (i == 2 * sideTileCount) return {x, y, cs, cs};
    if (i > 2 * sideTileCount && i < 3 * sideTileCount) return {x + cs + (i - 2 * sideTileCount - 1) * es, y, es, cs};
    if (i == 3 * sideTileCount) return {x + cs + edgeSlots * es, y, cs, cs};
    return {x + cs + edgeSlots * es, y + cs + (i - 3 * sideTileCount - 1) * es, cs, es};
}

QRect BoardWidget::selectionPromptRect(const QRect& centerRect) const
{
    return centerRect.adjusted(
        centerRect.width() / 8,
        centerRect.height() / 3,
        -centerRect.width() / 8,
        -centerRect.height() / 3);
}

QRect BoardWidget::selectionCancelRect(const QRect& centerRect) const
{
    const QRect prompt = selectionPromptRect(centerRect);
    const int size = qMax(26, qMin(prompt.width(), prompt.height()) / 6);
    return QRect(prompt.right() - size - 10, prompt.top() + 10, size, size);
}

bool BoardWidget::isInspectableTile(int idx) const
{
    if (idx < 0 || idx >= cells.size()) {
        return false;
    }
    if (cells[idx].propertyId <= 0) {
        return false;
    }

    switch (cells[idx].kind) {
    case TileKind::Property:
    case TileKind::Railroad:
    case TileKind::UtilityElec:
    case TileKind::UtilityWater:
        return true;
    default:
        return false;
    }
}

void BoardWidget::mousePressEvent(QMouseEvent *event)
{
    const QRect board = boardBounds();
    const int cs = qMax(74, int(board.width() * 0.132));
    const int edgeSlots = qMax(1, cells.size() / 4 - 1);
    const int es = (board.width() - 2 * cs) / edgeSlots;
    const QRect cr(board.x() + cs, board.y() + cs, es * edgeSlots, es * edgeSlots);

    if (tileSelectionMode && tileSelectionAllowCancel && selectionCancelRect(cr).contains(event->pos())) {
        emit tileSelectionCanceled();
        return;
    }

    for (int index = 0; index < cells.size(); ++index) {
        if (!tileRect(index, board, cs, es).contains(event->pos())) {
            continue;
        }

        if (tileSelectionMode) {
            if (selectableTileIndices.contains(index)) {
                setSelectedPropertyId(cells[index].propertyId);
                emit tileSelected(index);
            }
            break;
        }

        if (isInspectableTile(index)) {
            const int propertyId = cells[index].propertyId;
            setSelectedPropertyId(propertyId);
            emit propertySelected(propertyId);
        }
        break;
    }

    QWidget::mousePressEvent(event);
}

// ─────────────────────────────────────────────────────────────────────────────
//  HELPER: draw diamond (Income Tax fallback)
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::drawDiamond(QPainter &p, const QRect &r) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Pal::line, 2));
    p.setBrush(QColor(180,180,180));
    int cx=r.center().x(), cy=r.center().y();
    int rx=qMin(r.width(),r.height())/2-3;
    QPolygon pl;
    pl << QPoint(cx,cy-rx) << QPoint(cx+rx,cy) << QPoint(cx,cy+rx) << QPoint(cx-rx,cy);
    p.drawPolygon(pl);
    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  CORNER TILES
// ─────────────────────────────────────────────────────────────────────────────

// ── GO (bottom-right) ──
void BoardWidget::drawCornerGo(QPainter &p, const QRect &r) const
{
    p.save();
    p.fillRect(r, Pal::jailOr);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(r);

    int W=r.width(), H=r.height();

    // "DAPAT ... GAJI SAAT LEWAT" – tiny black text top-left area
    QFont tiny("Arial", qMax(7, W/13), QFont::Bold);
    p.setFont(tiny);
    p.setPen(Qt::white);
    const QString goSalary = (!cells.isEmpty() && !cells[0].price.isEmpty()) ? cells[0].price : QStringLiteral("M200");
    p.save();
    p.translate(r.center());
    p.rotate(-45);
    p.drawText(QRect(-W/2, -H/2 + H/10, W, H/4), Qt::AlignCenter,
               QStringLiteral("DAPAT %1").arg(goSalary));

    // Red arrow pointing left (pointing toward tile 1)
    // Arrow at bottom half, pointing left
    {
        p.setPen(QPen(QColor(255, 196, 148), qMax(2, W / 32), Qt::SolidLine, Qt::SquareCap));
        const int arrowY = H / 3;
        for (int i = 0; i < 5; ++i) {
            const int x = -W / 2 + W / 8 + i * W / 12;
            p.drawLine(QPoint(x + W / 16, arrowY), QPoint(x, arrowY + W / 16));
            p.drawLine(QPoint(x + W / 16, arrowY), QPoint(x, arrowY - W / 16));
        }
    }

    // "GO" — big red bold text in top-right quadrant
    QFont goF("Arial Black", qMax(24, W*2/5));
    p.setFont(goF);
    p.drawText(QRect(-W/2, -H/8, W, H/2), Qt::AlignCenter, "GO");

    p.restore();

    p.restore();
}

// ── JAIL (bottom-left) ──
void BoardWidget::drawCornerJail(QPainter &p, const QRect &r) const
{
    p.save();
    p.fillRect(r, Pal::white);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(r);

    int W=r.width(), H=r.height();

    // "HANYA / MAMPIR" vertical text on right strip
    {
        QFont sf("Arial", qMax(5, W / 22), QFont::Bold);
        p.save();
        p.translate(r.right() - W / 6, r.center().y());
        p.rotate(-90);
        p.setFont(sf);
        p.setPen(Pal::ink);
        p.drawText(QRect(-H / 2 + 6, -W / 9, H - 12, W / 4), Qt::AlignCenter | Qt::TextWordWrap, "HANYA\nMAMPIR");
        p.restore();
    }

    // Orange jail box (diagonal) with "DI / LAPAS"
    int boxS = int(qMin(W,H)*0.54);
    QRect jailBox(r.left()+4, r.top()+4, boxS, boxS);
    p.fillRect(jailBox, Pal::jailOr);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(jailBox);

    // "DI LAPAS" text in jail box
    {
        QFont jf("Arial", qMax(6, boxS/8), QFont::Bold);
        p.setFont(jf);
        p.setPen(Qt::white);
        p.save();
        p.translate(jailBox.center());
        p.rotate(-35);
        p.drawText(QRect(-boxS/2,-boxS/2,boxS,boxS), Qt::AlignCenter, "DI\nLAPAS");
        p.restore();
    }

    // In Jail image inside orange box
    const QPixmap &inJailPm = pix("in_jail_corner.png");
    if (!inJailPm.isNull()) {
        QRect imgR = jailBox.adjusted(4,4,-4,-4);
        drawPix(p, inJailPm, imgR);
    }

    p.restore();
}

// ── FREE PARKING (top-left) ──
void BoardWidget::drawCornerFreeParking(QPainter &p, const QRect &r) const
{
    p.save();
    p.fillRect(r, Pal::white);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(r);

    int W=r.width(), H=r.height();

    // "BEBAS" top-left, "PARKIR" bottom-right (rotated 180 because top-left corner faces down-right)
    QFont lf("Arial", qMax(7, W/13), QFont::Bold);
    p.setFont(lf);
    p.setPen(Pal::ink);
    p.save();
    p.translate(r.center());
    p.rotate(135);
    p.drawText(QRect(-W/2,-H/2,W,H/3), Qt::AlignCenter, "BEBAS");
    p.drawText(QRect(-W/2,H/6,W,H/3), Qt::AlignCenter, "PARKIR");
    p.restore();

    // Free Parking image (car)
    const QPixmap &pm = pix("free_parking_corner.png");
    QRect imgR(r.left()+W/6, r.top()+H/6, W*2/3, H*2/3);
    drawPix(p, pm, imgR);

    p.restore();
}

// ── GO TO JAIL (top-right) ──
void BoardWidget::drawCornerGoToJail(QPainter &p, const QRect &r) const
{
    p.save();
    p.fillRect(r, Pal::white);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(r);

    int W=r.width(), H=r.height();

    // "GO TO" at top, "JAIL" at bottom (rotated 225 = facing left+down)
    QFont lf("Arial", qMax(7, W/13), QFont::Bold);
    p.setFont(lf);
    p.setPen(Pal::ink);
    p.save();
    p.translate(r.center());
    p.rotate(225);
    p.drawText(QRect(-W/2, -H/2, W, H/3), Qt::AlignCenter, "PERGI KE");
    p.drawText(QRect(-W/2, H/6,  W, H/3), Qt::AlignCenter, "PENJARA");
    p.restore();

    // Go To Jail image (cop)
    const QPixmap &pm = pix("go_to_jail_corner.png");
    QRect imgR(r.left()+W/6, r.top()+H/6, W*2/3, H*2/3);
    drawPix(p, pm, imgR);

    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  EDGE TILE
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::drawEdgeTile(QPainter &p, EdgeSide side,
                               const QRect &r, const CellData &c) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    // Rotate so "top" of local canvas always faces board interior
    p.translate(r.center());
    int angle = 0;
    if (side == EdgeSide::Left)  angle =  90;
    if (side == EdgeSide::Top)   angle = 180;
    if (side == EdgeSide::Right) angle = -90;
    p.rotate(angle);

    bool horiz = (side==EdgeSide::Bottom || side==EdgeSide::Top);
    int W = horiz ? r.width()  : r.height();
    int H = horiz ? r.height() : r.width();
    QRect lr(-W/2, -H/2, W, H);

    // Base fill
    p.fillRect(lr, Pal::white);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(lr);

    // ── Property strip (TOP, facing board center) ──
    const int stripH = qMax(18, H/4); // slightly thicker strip
    if (c.hasStrip) {
        QRect strip(lr.left(), lr.top(), W, stripH);
        p.fillRect(strip, c.stripColor);
        p.setPen(QPen(Pal::line, 2));
        p.drawRect(strip);
    }

    // ── Calculate content zone below strip ──
    int topY  = lr.top() + (c.hasStrip ? stripH : 0);
    QRect content(lr.left()+4, topY+4, W-8, H - (c.hasStrip ? stripH : 0) - 8);

    // Font sizes are fitted to the available tile text box so long city names stay readable.
    QFont nameFont("Arial", qMax(7, W/9), QFont::Bold);
    QFont priceFont("Arial", qMax(5, W/18), QFont::Bold);

    if (c.hasStrip) {
        // ── PROPERTY TILE layout (matches reference):
        //    [NAME  centered right below the strip]
        //    [price at bottom]
        // ─────────────────────────────────────────
        QRect nameR(content.left(), content.top() + 4, content.width(), content.height()*55/100);
        nameFont = fittedTextFont(
            QStringLiteral("Arial"),
            qMax(6, W / 11),
            4,
            QFont::Bold,
            c.name,
            nameR,
            Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap);
        p.setFont(nameFont);
        p.setPen(Pal::ink);
        p.drawText(nameR, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, c.name);

        if (!c.price.isEmpty()) {
            QRect priceR(content.left(), content.bottom() - (content.height()/4), content.width(), content.height()/4);
            priceFont = fittedTextFont(
                QStringLiteral("Arial"),
                qMax(5, W / 18),
                5,
                QFont::Bold,
                priceText(c.price),
                priceR,
                Qt::AlignHCenter | Qt::AlignBottom);
            p.setFont(priceFont);
            p.setPen(Pal::ink);
            p.drawText(priceR, Qt::AlignHCenter | Qt::AlignBottom, priceText(c.price));
        }

    } else {
        // ── SPECIAL TILE layout (matches reference):
        //    [NAME at top]
        //    [ICON big, centered]
        //    [PRICE at bottom]
        // ─────────────────────────────────────────
        const int nameSectionH = content.height() * 25 / 100;
        const int priceSectionH = content.height() * 20 / 100;
        const int iconSectionH  = content.height() - nameSectionH - priceSectionH;

        // Name top
        QRect nameR(content.left(), content.top() + 4, content.width(), nameSectionH);
        nameFont = fittedTextFont(
            QStringLiteral("Arial"),
            qMax(6, W / 11),
            4,
            QFont::Bold,
            c.name,
            nameR,
            Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap);
        p.setFont(nameFont);
        p.setPen(Pal::ink);
        p.drawText(nameR, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, c.name);

        // Icon centre – fill most of middle section
        int iconPad = content.width() / 8;
        QRect iconR(content.left() + iconPad,
                    content.top() + nameSectionH,
                    content.width() - 2*iconPad,
                    iconSectionH);

        QString imgName;
        bool usePng = true;
        switch (c.kind) {
        case TileKind::Railroad:       imgName = "railroad_icon.png";          break;
        case TileKind::CommunityChest: imgName = "community_chest_icon.png";   break;
        case TileKind::Chance:         imgName = "chance_icon.png";            break;
        case TileKind::UtilityElec:    imgName = "electric_company_icon.png";  break;
        case TileKind::UtilityWater:   imgName = "waterworks_icon.png";        break;
        case TileKind::LuxuryTax:      imgName = "luxury_tax_icon.png";        break;
        case TileKind::Festival:       imgName = "festival_icon.png";          break;
        case TileKind::IncomeTax:      usePng  = false;                        break;
        default:                       usePng  = false;                        break;
        }

        if (usePng) {
            drawPix(p, pix(imgName), iconR);
        } else if (c.kind == TileKind::IncomeTax) {
            drawDiamond(p, iconR);
        }

        // Price bottom
        if (!c.price.isEmpty()) {
            QRect priceR(content.left(), content.bottom() - priceSectionH, content.width(), priceSectionH);
            priceFont = fittedTextFont(
                QStringLiteral("Arial"),
                qMax(5, W / 18),
                5,
                QFont::Bold,
                priceText(c.price),
                priceR,
                Qt::AlignHCenter | Qt::AlignBottom);
            p.setFont(priceFont);
            p.setPen(Pal::ink);
            p.drawText(priceR, Qt::AlignHCenter | Qt::AlignBottom, priceText(c.price));
        }
    }

    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  CENTER AREA
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::drawCenter(QPainter &p, const QRect &cr) const
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(cr, Pal::bg);

    // ── Dana Umum card (top-left, tilted -35°) ──
    {
        // Dana Umum card – top-left of center, tilted -35°, SMALLER
        int cw = cr.width()*4/12, ch = cr.height()*3/12;
        QPoint center(cr.left() + cr.width()*28/100,
                      cr.top()  + cr.height()*28/100);
        p.save();
        p.translate(center);
        p.rotate(-35);
        QRect card(-cw/2, -ch/2, cw, ch);
        p.setPen(QPen(Pal::chestBlue, qMax(3, cr.width() / 115)));
        p.setBrush(Qt::NoBrush);
        p.drawRect(card);
        const QRect iconRect(
            card.left() + card.width() * 34 / 100,
            card.top() + card.height() * 18 / 100,
            card.width() * 32 / 100,
            card.height() * 32 / 100
        );
        drawPix(p, pix("community_chest_center_card.png"), iconRect);
        QFont cardFont("Arial", qMax(8, cr.width() / 55), QFont::Bold);
        p.setFont(cardFont);
        p.setPen(Pal::ink);
        p.drawText(card.adjusted(8, card.height()*62/100, -8, -8), Qt::AlignCenter, "DANA UMUM");
        p.restore();
    }

    // ── Kesempatan card (bottom-right, tilted -35°, SMALLER) ──
    {
        int cw = cr.width()*4/12, ch = cr.height()*3/12;
        QPoint center(cr.left() + cr.width()*72/100,
                      cr.top()  + cr.height()*72/100);
        p.save();
        p.translate(center);
        p.rotate(-35);
        QRect card(-cw/2, -ch/2, cw, ch);
        p.setPen(QPen(Pal::chanceBg, qMax(3, cr.width() / 110)));
        p.setBrush(Qt::NoBrush);
        p.drawRect(card);
        QFont cardFont("Arial", qMax(8, cr.width() / 55), QFont::Bold);
        p.setFont(cardFont);
        p.setPen(Pal::ink);
        p.drawText(card.adjusted(8, 8, -8, -card.height()*62/100), Qt::AlignCenter, "KESEMPATAN");
        QFont qFont("Arial Black", qMax(18, cr.width() / 16));
        p.setFont(qFont);
        p.setPen(Pal::chanceBg);
        p.drawText(card.adjusted(0, card.height()/5, 0, 0), Qt::AlignCenter, "?");
        p.restore();
    }

    {
        p.save();
        p.translate(cr.center());
        p.rotate(-35);

        const qreal s = cr.width() / 100.0;
        p.setPen(QPen(Pal::line, qMax<qreal>(1.2, s * 0.22), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(-16*s, 16*s, 20*s, 20*s), 205 * 16, 205 * 16);
        p.drawLine(QPointF(-27*s, 33*s), QPointF(-18*s, 23*s));
        p.drawLine(QPointF(-27*s, 33*s), QPointF(-22*s, 41*s));

        p.setBrush(Pal::pink);
        p.setPen(QPen(Pal::line, qMax<qreal>(2.0, s * 0.35)));
        p.drawPie(QRectF(-28*s, 3*s, 33*s, 33*s), 25 * 16, 245 * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(Pal::green);
        p.drawRoundedRect(QRectF(22*s, -1*s, 11*s, 28*s), 6*s, 6*s);
        p.drawRoundedRect(QRectF(31*s, -1*s, 11*s, 28*s), 6*s, 6*s);
        p.drawRoundedRect(QRectF(40*s, -1*s, 11*s, 28*s), 6*s, 6*s);

        p.setBrush(QColor(255, 76, 34));
        p.drawRoundedRect(QRectF(-33*s, -15*s, 11*s, 22*s), 6*s, 6*s);
        p.setBrush(QColor(35, 181, 232));
        p.drawEllipse(QRectF(-18*s, -14*s, 12*s, 12*s));
        p.setBrush(QColor(155, 82, 236));
        p.drawEllipse(QRectF(-24*s, -5*s, 13*s, 13*s));
        p.setBrush(QColor(21, 206, 126));
        p.drawEllipse(QRectF(-12*s, 2*s, 12*s, 12*s));

        p.restore();
    }

    // ── NIMONSPOLI diagonal banner (smaller, won't clip) ──
    {
        double logoW = cr.width() * 0.90;
        double logoH = cr.height()* 0.18;
        p.save();
        p.translate(cr.center());
        p.rotate(-38);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(143, 168, 216, 54));
        p.drawRect(QRectF(-logoW/2+7, -logoH/2+7, logoW, logoH));

        p.setBrush(Pal::logoRed);
        p.setPen(QPen(Pal::line, 2));
        p.drawRect(QRectF(-logoW/2, -logoH/2, logoW, logoH));

        p.setPen(QPen(Pal::line, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(-logoW/2+8, -logoH/2+8, logoW-16, logoH-16));

        p.setBrush(Pal::white);
        const qreal handle = qMax<qreal>(7.0, logoH * 0.14);
        const QVector<QPointF> handles = {
            QPointF(-logoW/2+8, -logoH/2+8), QPointF(logoW/2-8, -logoH/2+8),
            QPointF(-logoW/2+8, logoH/2-8), QPointF(logoW/2-8, logoH/2-8)
        };
        for (const QPointF& pt : handles) {
            QRectF h(pt.x() - handle/2, pt.y() - handle/2, handle, handle);
            p.drawRect(h);
        }

        // Text – sized to fit banner height
        int fontSize = qMax(10, int(logoH * 0.40));
        QFont lf("Arial Black", fontSize);
        lf.setBold(true);
        p.setFont(lf);
        p.setPen(Qt::white);
        p.drawText(QRectF(-logoW/2, -logoH/2, logoW, logoH), Qt::AlignCenter, "NIMONSPOLI");

        p.restore();
    }

    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  DRAW CELL (dispatcher)
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::drawCell(QPainter &p, int idx, const QRect &r) const
{
    const CellData &c  = cells[idx];
    const EdgeSide side = sideForIndex(idx);

    if (side == EdgeSide::Corner) {
        switch (c.kind) {
        case TileKind::CornerGo: drawCornerGo(p, r); break;
        case TileKind::CornerJail: drawCornerJail(p, r); break;
        case TileKind::CornerFreeParking: drawCornerFreeParking(p, r); break;
        case TileKind::CornerGoToJail: drawCornerGoToJail(p, r); break;
        default: drawEdgeTile(p, side, r, c); break;
        }
    } else {
        drawEdgeTile(p, side, r, c);
    }

    if (selectedPropertyId == c.propertyId && isInspectableTile(idx)) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 242, 0), 4));
        p.drawRect(r.adjusted(4, 4, -4, -4));
        p.setPen(QPen(QColor(0, 114, 187), 2));
        p.drawRect(r.adjusted(8, 8, -8, -8));
        p.restore();
    }
}

void BoardWidget::drawSelectionOverlay(QPainter& p, int idx, const QRect& r) const
{
    if (!tileSelectionMode) {
        return;
    }

    const bool selectable = selectableTileIndices.contains(idx);
    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    if (!selectable) {
        p.fillRect(r, QColor(70, 74, 78, 138));
    } else {
        p.setBrush(QColor(255, 242, 0, 54));
        p.setPen(QPen(QColor(18, 98, 197), 3));
        p.drawRect(r.adjusted(4, 4, -4, -4));
    }

    p.restore();
}

// ─────────────────────────────────────────────────────────────────────────────
//  paintEvent
// ─────────────────────────────────────────────────────────────────────────────
void BoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Blue outer frame
    p.fillRect(rect(), Pal::frame);

    const QRect board = boardBounds();
    // Corner tile ~13.8% of board; edge tiles fill the rest on each side.
    const int cs = qMax(74, int(board.width() * 0.132));
    const int edgeSlots = qMax(1, cells.size() / 4 - 1);
    const int es = (board.width() - 2*cs) / edgeSlots;

    // Board background; perimeter cells draw the visible outside border.
    p.fillRect(board, Pal::bg);

    // Center
    const QRect cr(board.x()+cs, board.y()+cs, es*edgeSlots, es*edgeSlots);
    drawCenter(p, cr);

    for (int i = 0; i < cells.size(); ++i)
        drawCell(p, i, tileRect(i, board, cs, es));

    drawOwners(p, board, cs, es);
    drawBuildings(p, board, cs, es);

    for (int i = 0; i < cells.size(); ++i)
        drawSelectionOverlay(p, i, tileRect(i, board, cs, es));

    drawPawns(p, board, cs, es);

    if (tileSelectionMode && !selectionPromptText.isEmpty()) {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        QRect promptRect = selectionPromptRect(cr);
        p.setPen(QPen(Pal::line, 2));
        p.setBrush(QColor(255, 254, 248, 236));
        p.drawRect(promptRect);

        QRect textRect = promptRect.adjusted(16, 12, -16, -12);
        if (tileSelectionAllowCancel) {
            const QRect cancelRect = selectionCancelRect(cr);
            textRect.setRight(cancelRect.left() - 8);
            p.setBrush(QColor(126, 116, 196));
            p.setPen(Qt::NoPen);
            p.drawRect(cancelRect);
            p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(cancelRect.topLeft() + QPoint(8, 8), cancelRect.bottomRight() - QPoint(8, 8));
            p.drawLine(cancelRect.topRight() + QPoint(-8, 8), cancelRect.bottomLeft() + QPoint(8, -8));
        }

        QFont titleFont(QStringLiteral("Trebuchet MS"), qMax(10, cr.width() / 36), QFont::Black);
        p.setFont(titleFont);
        p.setPen(Pal::ink);
        p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, selectionPromptText.toUpper());
        p.restore();
    }
}
