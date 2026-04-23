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
#include <QtMath>

#include <exception>

#include "utils/UiCommon.hpp"
#include "models/Enums.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/PropertyConfig.hpp"
#include "utils/ConfigLoader.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  PALETTE  (exact Figma hex values)
// ─────────────────────────────────────────────────────────────────────────────
namespace Pal {
    const QColor bg       (205, 230, 208);   // #CDE6D0
    const QColor frame    (  0, 114, 187);   // #0072BB blue outer
    const QColor line     (  0,   0,   0);
    const QColor white    (255, 255, 255);

    // property strips
    const QColor brown    (149,  84,  54);   // #955436
    const QColor sky      (169, 220, 244);   // #A9DCF4
    const QColor pink     (217,  58, 150);   // #D93A96
    const QColor orange   (247, 148,  29);   // #F7941D
    const QColor red      (237,  28,  36);   // #ED1C24
    const QColor yellow   (254, 242,   0);   // #FEF200
    const QColor green    ( 31, 178,  90);   // #1FB25A
    const QColor darkBlue (  0, 114, 187);   // #0072BB

    // special
    const QColor jailOr   (247, 148,  29);
    const QColor chestBlue(170, 224, 250);
    const QColor chanceBg (247, 148,  29);
    const QColor logoRed  (241,  28,  36);
}

namespace {
QString formatCurrency(int amount)
{
    return QStringLiteral("₩%1").arg(amount);
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

QString findConfigDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList startPaths = {QDir::currentPath(), appDir};

    for (const QString& startPath : startPaths) {
        const QString found = findUpwardDirectory(
            startPath,
            QStringLiteral("config"),
            {QStringLiteral("property.txt"), QStringLiteral("tax.txt"), QStringLiteral("special.txt")}
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
            {QStringLiteral("chance_icon.png")}
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
}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
//  CONSTRUCTION
// ─────────────────────────────────────────────────────────────────────────────
BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent), cells(createCells())
{
    setMinimumSize(620, 620);
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

void BoardWidget::setPawns(const QVector<PawnData>& pawnData)
{
    pawns = pawnData;
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

    const QStringList searchDirs = {findImagesDirectory()};
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
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawEllipse(shadowRect);

    if (isActivePawn) {
        p.setBrush(QColor(pawn.accentColor.red(), pawn.accentColor.green(), pawn.accentColor.blue(), 80));
        p.drawEllipse(r.adjusted(-6, -6, 6, 6));
    }

    p.setBrush(pawn.accentColor.isValid() ? pawn.accentColor : QColor(230, 230, 230));
    p.setPen(QPen(isActivePawn ? QColor(255, 245, 175) : Qt::black, isActivePawn ? 2.6 : 1.2));
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

    p.setPen(QPen(isActivePawn ? Qt::white : Qt::black, isActivePawn ? 1.8 : 1.0));
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

// ─────────────────────────────────────────────────────────────────────────────
//  TILE DATA  (index 0=GO at bottom-right, clockwise)
// ─────────────────────────────────────────────────────────────────────────────
QVector<BoardWidget::CellData> BoardWidget::createCells() const
{
    QVector<CellData> d(40);

    // Shorthands
    auto prop = [&](int i,const QString &price,const QString &name,const QColor &strip){
        d[i]={TileKind::Property, price, name, strip, true, strip};
    };
    auto spec = [&](int i,TileKind k,const QString &price,const QString &name,
                    const QColor &accent=Pal::line){
        d[i]={k, price, name, {}, false, accent};
    };

    // ── BOTTOM (indices 0–9, right→left) ──
    spec( 0, TileKind::CornerGo,          "",          "Petak Mulai");
    prop( 1, "₩60",   "GARUT",            Pal::brown);
    spec( 2, TileKind::CommunityChest,    "",          "DANA\nUMUM",    Pal::darkBlue);
    prop( 3, "₩60",   "TASIKMALAYA",      Pal::brown);
    spec( 4, TileKind::IncomeTax,         "PAY ₩150",  "PPH",           Pal::line);
    spec( 5, TileKind::Railroad,          "₩200",      "STASIUN\nGAMBIR");
    prop( 6, "₩100",  "BOGOR",            Pal::sky);
    spec( 7, TileKind::Festival,          "",          "FESTIVAL",      Pal::pink);
    prop( 8, "₩100",  "DEPOK",            Pal::sky);
    prop( 9, "₩120",  "BEKASI",           Pal::sky);

    // ── LEFT (indices 10–19, bottom→top) ──
    spec(10, TileKind::CornerJail,        "",          "PENJARA");
    prop(11, "₩140",  "MAGELANG",         Pal::pink);
    spec(12, TileKind::UtilityElec,       "₩150",      "PLN",           Pal::line);
    prop(13, "₩140",  "SOLO",             Pal::pink);
    prop(14, "₩160",  "YOGYAKARTA",       Pal::pink);
    spec(15, TileKind::Railroad,          "₩200",      "STASIUN\nBANDUNG");
    prop(16, "₩180",  "MALANG",           Pal::orange);
    spec(17, TileKind::CommunityChest,    "",          "DANA\nUMUM",    Pal::darkBlue);
    prop(18, "₩180",  "SEMARANG",         Pal::orange);
    prop(19, "₩200",  "SURABAYA",         Pal::orange);

    // ── TOP (indices 20–29, left→right) ──
    spec(20, TileKind::CornerFreeParking, "",          "BEBAS PARKIR");
    prop(21, "₩220",  "MAKASSAR",         Pal::red);
    spec(22, TileKind::Chance,            "",          "KESEMPATAN",    Pal::darkBlue);
    prop(23, "₩240",  "BALIKPAPAN",       Pal::red);
    prop(24, "₩260",  "MANADO",           Pal::red);
    spec(25, TileKind::Railroad,          "₩200",      "STASIUN\nTUGU");
    prop(26, "₩260",  "PALEMBANG",        Pal::yellow);
    prop(27, "₩260",  "PEKANBARU",        Pal::yellow);
    spec(28, TileKind::UtilityWater,      "₩150",      "PAM",           Pal::line);
    prop(29, "₩280",  "MEDAN",            Pal::yellow);

    // ── RIGHT (indices 30–39, top→bottom) ──
    spec(30, TileKind::CornerGoToJail,    "",          "PERGI KE\nPENJARA");
    prop(31, "₩300",  "BANDUNG",          Pal::green);
    prop(32, "₩300",  "DENPASAR",         Pal::green);
    spec(33, TileKind::Festival,          "",          "FESTIVAL",      Pal::pink);
    prop(34, "₩320",  "MATARAM",          Pal::green);
    spec(35, TileKind::Railroad,          "₩200",      "STASIUN\nGUBENG");
    spec(36, TileKind::Chance,            "",          "KESEMPATAN",    Pal::orange);
    prop(37, "₩350",  "JAKARTA",          Pal::darkBlue);
    spec(38, TileKind::LuxuryTax,         "₩150",      "PPNBM",         Pal::line);
    prop(39, "₩400",  "IKN",              Pal::darkBlue);

    const QString configDir = findConfigDirectory();
    if (!configDir.isEmpty()) {
        try {
            ConfigLoader loader(configDir.toStdString());
            const ConfigData config = loader.loadAll();

            const TaxConfig& taxConfig = config.getTaxConfig();
            const SpecialConfig& specialConfig = config.getSpecialConfig();

            if (specialConfig.getGoSalary() > 0) {
                d[0].price = formatCurrency(specialConfig.getGoSalary());
            }

            if (taxConfig.getPphFlat() > 0) {
                d[4].price = QStringLiteral("BAYAR %1").arg(formatCurrency(taxConfig.getPphFlat()));
            }

            if (taxConfig.getPbmFlat() > 0) {
                d[38].price = QStringLiteral("BAYAR %1").arg(formatCurrency(taxConfig.getPbmFlat()));
            }

            for (const PropertyConfig& property : config.getPropertyConfigs()) {
                const int index = property.getId() - 1;
                if (index < 0 || index >= d.size()) {
                    continue;
                }

                CellData& cell = d[index];
                cell.name = formatTileName(property.getName());

                switch (property.getPropertyType()) {
                case PropertyType::STREET:
                    cell.kind = TileKind::Property;
                    cell.hasStrip = true;
                    cell.stripColor = colorFromGroup(property.getColorGroup(), cell.stripColor);
                    cell.accentColor = cell.stripColor;
                    if (property.getBuyPrice() > 0) {
                        cell.price = formatCurrency(property.getBuyPrice());
                    }
                    break;
                case PropertyType::RAILROAD:
                    cell.kind = TileKind::Railroad;
                    cell.hasStrip = false;
                    cell.stripColor = QColor();
                    cell.accentColor = Pal::line;
                    cell.price = property.getBuyPrice() > 0 ? formatCurrency(property.getBuyPrice()) : QString();
                    break;
                case PropertyType::UTILITY:
                    cell.kind = (property.getCode() == "PLN") ? TileKind::UtilityElec : TileKind::UtilityWater;
                    cell.hasStrip = false;
                    cell.stripColor = QColor();
                    cell.accentColor = Pal::line;
                    cell.price = property.getBuyPrice() > 0 ? formatCurrency(property.getBuyPrice()) : QString();
                    break;
                }
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
    const int margin = 10;
    const int s = qMax(0, qMin(width(), height()) - 2 * margin);
    return { (width() - s) / 2, (height() - s) / 2, s, s };
}
BoardWidget::EdgeSide BoardWidget::sideForIndex(int i) const
{
    if (i%10==0) return EdgeSide::Corner;
    if (i< 10)   return EdgeSide::Bottom;
    if (i< 20)   return EdgeSide::Left;
    if (i< 30)   return EdgeSide::Top;
    return EdgeSide::Right;
}
QRect BoardWidget::tileRect(int i, const QRect &b, int cs, int es) const
{
    int x=b.x(), y=b.y();
    if (i==0)              return {x+cs+9*es, y+cs+9*es, cs, cs};
    if (i>=1  && i<=9)    return {x+cs+(9-i)*es, y+cs+9*es, es, cs};
    if (i==10)             return {x, y+cs+9*es, cs, cs};
    if (i>=11 && i<=19)   return {x, y+cs+(19-i)*es, cs, es};
    if (i==20)             return {x, y, cs, cs};
    if (i>=21 && i<=29)   return {x+cs+(i-21)*es, y, es, cs};
    if (i==30)             return {x+cs+9*es, y, cs, cs};
    return                        {x+cs+9*es, y+cs+(i-31)*es, cs, es};
}

bool BoardWidget::isInspectableTile(int idx) const
{
    if (idx < 0 || idx >= cells.size()) {
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
    const int cs = qMax(88, int(board.width() * 0.138));
    const int es = (board.width() - 2 * cs) / 9;

    for (int index = 0; index < 40; ++index) {
        if (!tileRect(index, board, cs, es).contains(event->pos())) {
            continue;
        }

        if (isInspectableTile(index)) {
            const int propertyId = index + 1;
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
    p.fillRect(r, Pal::white);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(r);

    int W=r.width(), H=r.height();

    // "DAPAT ... GAJI SAAT LEWAT" – tiny black text top-left area
    QFont tiny("Arial", qMax(6, W/17), QFont::Bold);
    p.setFont(tiny);
    p.setPen(Pal::line);
    const QString goSalary = (!cells.isEmpty() && !cells[0].price.isEmpty()) ? cells[0].price : QStringLiteral("₩200");
    p.drawText(r.adjusted(4,4,-W/2,-H/2), Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,
               QStringLiteral("DAPAT\n%1 GAJI\nSAAT LEWAT").arg(goSalary));

    // Red arrow pointing left (pointing toward tile 1)
    // Arrow at bottom half, pointing left
    {
        int ax  = r.right() - W/6;
        int ay  = r.bottom() - H/4;
        int aw  = W/2, ah = H/7;
        QPolygon arr;
        arr << QPoint(ax,      ay - ah/2)
            << QPoint(ax-aw,   ay - ah/2)
            << QPoint(ax-aw,   ay - ah)
            << QPoint(ax-aw - ah, ay)
            << QPoint(ax-aw,   ay + ah)
            << QPoint(ax-aw,   ay + ah/2)
            << QPoint(ax,      ay + ah/2);
        p.setPen(Qt::NoPen);
        p.setBrush(Pal::red);
        p.drawPolygon(arr);
    }

    // "GO" — big red bold text in top-right quadrant
    QFont goF("Arial Black", qMax(22, W*2/5));
    goF.setBold(true);
    p.setFont(goF);
    p.setPen(Pal::red);
    QRect goR(r.left()+W/2, r.top()+4, W/2-4, H/2-4);
    p.drawText(goR, Qt::AlignCenter, "GO");

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
        QFont sf("Arial", qMax(5,W/18), QFont::Bold);
        p.save();
        p.translate(r.right()-W/8, r.center().y());
        p.rotate(-90);
        p.setFont(sf);
        p.setPen(Pal::line);
        p.drawText(QRect(-H/2+4, -W/8+2, H-8, W/4), Qt::AlignCenter|Qt::TextWordWrap, "HANYA\nMAMPIR");
        p.restore();
    }

    // Orange jail box (diagonal) with "DI / LAPAS"
    int boxS = int(qMin(W,H)*0.62);
    QRect jailBox(r.left()+4, r.top()+4, boxS, boxS);
    p.fillRect(jailBox, Pal::jailOr);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(jailBox);

    // "DI LAPAS" text in jail box
    {
        QFont jf("Arial", qMax(6, boxS/8), QFont::Bold);
        p.setFont(jf);
        p.setPen(Pal::line);
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
    QFont lf("Arial", qMax(6, W/14), QFont::Bold);
    p.setFont(lf);
    p.setPen(Pal::red);
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
    QFont lf("Arial", qMax(6, W/14), QFont::Bold);
    p.setFont(lf);
    p.setPen(Pal::darkBlue);
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
    p.fillRect(lr, Pal::bg);
    p.setPen(QPen(Pal::line, 3));
    p.drawRect(lr);

    // ── Property strip (TOP, facing board center) ──
    const int stripH = qMax(18, H/4); // slightly thicker strip
    if (c.hasStrip) {
        QRect strip(lr.left(), lr.top(), W, stripH);
        p.fillRect(strip, c.stripColor);
        p.setPen(QPen(Pal::line, 3));
        p.drawRect(strip);
    }

    // ── Calculate content zone below strip ──
    int topY  = lr.top() + (c.hasStrip ? stripH : 0);
    QRect content(lr.left()+2, topY+2, W-4, H - (c.hasStrip ? stripH : 0) - 4);

    // Font sizes based on tile width (fonts not bold as per reference screenshot)
    QFont nameFont("Inter", qMax(5, W/11)); 
    QFont priceFont("Inter", qMax(5, W/11));

    if (c.hasStrip) {
        // ── PROPERTY TILE layout (matches reference):
        //    [NAME  centered right below the strip]
        //    [price at bottom]
        // ─────────────────────────────────────────
        QRect nameR(content.left(), content.top() + 4, content.width(), content.height()/2);
        p.setFont(nameFont);
        p.setPen(Pal::line);
        p.drawText(nameR, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, c.name);

        if (!c.price.isEmpty()) {
            p.setFont(priceFont);
            p.setPen(Pal::line);
            QRect priceR(content.left(), content.bottom() - (content.height()/3), content.width(), content.height()/3);
            p.drawText(priceR, Qt::AlignHCenter | Qt::AlignBottom, c.price);
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
        p.setFont(nameFont);
        p.setPen(Pal::line);
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
            p.setFont(priceFont);
            p.setPen(Pal::line);
            p.drawText(priceR, Qt::AlignHCenter | Qt::AlignBottom, c.price);
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
        p.setPen(QPen(QColor(28,82,132), 2, Qt::DashLine));
        p.setBrush(Pal::chestBlue);
        p.drawRect(card);
        drawPix(p, pix("community_chest_center_card.png"), card.adjusted(4,4,-4,-4));
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
        p.setPen(QPen(QColor(120,70,10), 2, Qt::DashLine));
        p.setBrush(Pal::chanceBg);
        p.drawRect(card);
        drawPix(p, pix("chance_center_card.png"), card.adjusted(4,4,-4,-4));
        p.restore();
    }

    // ── NIMONSPOLI diagonal banner (smaller, won't clip) ──
    {
        double logoW = cr.width() * 0.72;
        double logoH = cr.height()* 0.13;
        p.save();
        p.translate(cr.center());
        p.rotate(-38);

        // Drop shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0,0,0,55));
        p.drawRoundedRect(QRectF(-logoW/2+6, -logoH/2+6, logoW, logoH), 10, 10);

        // Red banner
        p.setBrush(Pal::logoRed);
        p.setPen(QPen(Pal::white, 5));
        p.drawRoundedRect(QRectF(-logoW/2, -logoH/2, logoW, logoH), 10, 10);

        // Inner black outline
        p.setPen(QPen(Pal::line, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(-logoW/2+4, -logoH/2+4, logoW-8, logoH-8), 7, 7);

        // Text – sized to fit banner height
        int fontSize = qMax(10, int(logoH * 0.48));
        QFont lf("Arial Black", fontSize);
        lf.setBold(true);
        lf.setLetterSpacing(QFont::PercentageSpacing, 108);
        p.setFont(lf);
        // shadow
        p.setPen(QColor(0,0,0,80));
        p.drawText(QRectF(-logoW/2+3, -logoH/2+3, logoW, logoH), Qt::AlignCenter, "NIMONSPOLI");
        // white text
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
        // Base fill + border
        p.fillRect(r, Pal::white);
        p.setPen(QPen(Pal::line, 3));
        p.drawRect(r);

        switch (idx) {
        case  0: drawCornerGo(p, r);          break;
        case 10: drawCornerJail(p, r);         break;
        case 20: drawCornerFreeParking(p, r);  break;
        case 30: drawCornerGoToJail(p, r);     break;
        }
    } else {
        // Tile outer border (fallback)
        p.fillRect(r, Pal::bg);
        p.setPen(QPen(Pal::line, 3));
        p.drawRect(r);
        drawEdgeTile(p, side, r, c);
    }

    if (selectedPropertyId == idx + 1 && isInspectableTile(idx)) {
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
    // Corner tile ~13.8% of board, edge tiles fill the rest in 9 equal slots
    const int cs = qMax(88, int(board.width() * 0.138));
    const int es = (board.width() - 2*cs) / 9;

    // Board background
    p.fillRect(board, Pal::bg);
    p.setPen(QPen(Pal::line, 3));
    p.drawRect(board);

    // Center
    const QRect cr(board.x()+cs, board.y()+cs, es*9, es*9);
    drawCenter(p, cr);

    // All 40 tiles
    for (int i = 0; i < 40; ++i)
        drawCell(p, i, tileRect(i, board, cs, es));

    drawPawns(p, board, cs, es);

    // Redraw outer board border on top
    p.setPen(QPen(Pal::line, 3));
    p.setBrush(Qt::NoBrush);
    p.drawRect(board);
    p.setPen(QPen(Pal::line, 2));
    p.drawRect(cr);
}
