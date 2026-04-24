#ifndef BOARD_WIDGET_HPP
#define BOARD_WIDGET_HPP

#include <QColor>
#include <QRect>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QSet>

class QPainter;
class QMouseEvent;

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    struct PawnData {
        QString name;
        QString iconName;
        int tileIndex = 0;
        QColor accentColor;
    };

    struct BuildingData {
        int tileIndex = 0;
        int buildingLevel = 0;
    };

    struct OwnerData {
        int tileIndex = 0;
        QString ownerName;
        QColor accentColor;
        bool mortgaged = false;
    };

    explicit BoardWidget(QWidget *parent = nullptr);
    ~BoardWidget() override;

    void setActivePawnName(const QString& pawnName);
    void setPawns(const QVector<PawnData>& pawnData);
    void setBuildings(const QVector<BuildingData>& buildingData);
    void setOwners(const QVector<OwnerData>& ownerData);
    void setSelectedPropertyId(int propertyId);
    void setTileSelectionMode(const QSet<int>& validTileIndices, const QString& promptText, bool allowCancel = false);
    void clearTileSelectionMode();

signals:
    void propertySelected(int propertyId);
    void tileSelected(int tileIndex);
    void tileSelectionCanceled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    enum class TileKind {
        Property,
        Chance,
        CommunityChest,
        Railroad,
        UtilityElec,
        UtilityWater,
        IncomeTax,
        LuxuryTax,
        Festival,
        CornerGo,
        CornerJail,
        CornerFreeParking,
        CornerGoToJail
    };

    enum class EdgeSide { Bottom, Left, Top, Right, Corner };

    struct CellData {
        TileKind  kind;
        QString   price;    // e.g. "M60", "M200", "PAY M150"
        QString   name;     // display name (may have \n)
        QColor    stripColor;
        bool      hasStrip;
        QColor    accentColor;
    };

    QVector<CellData>           cells;
    QVector<PawnData>           pawns;
    QVector<BuildingData>       buildings;
    QVector<OwnerData>          owners;
    mutable QMap<QString,QPixmap> pixCache;
    QString                     activePawnName;
    int                         selectedPropertyId = 0;
    bool                        tileSelectionMode = false;
    bool                        tileSelectionAllowCancel = false;
    QSet<int>                   selectableTileIndices;
    QString                     selectionPromptText;

    QVector<CellData> createCells() const;
    QRect    boardBounds() const;
    EdgeSide sideForIndex(int idx) const;
    QRect    tileRect(int idx, const QRect &b, int cs, int es) const;
    QRect    selectionPromptRect(const QRect& centerRect) const;
    QRect    selectionCancelRect(const QRect& centerRect) const;
    bool     isInspectableTile(int idx) const;

    const QPixmap& pix(const QString &name) const;
    void drawBuildings(QPainter &p, const QRect &board, int cs, int es) const;
    void drawOwners(QPainter &p, const QRect &board, int cs, int es) const;
    void drawPawns(QPainter &p, const QRect &board, int cs, int es) const;
    void drawPawn(QPainter &p, const QRectF &r, const PawnData &pawn) const;

    void drawCell(QPainter &p, int idx, const QRect &r) const;
    void drawSelectionOverlay(QPainter& p, int idx, const QRect& r) const;
    void drawCornerGo         (QPainter &p, const QRect &r) const;
    void drawCornerJail       (QPainter &p, const QRect &r) const;
    void drawCornerFreeParking(QPainter &p, const QRect &r) const;
    void drawCornerGoToJail   (QPainter &p, const QRect &r) const;
    void drawEdgeTile         (QPainter &p, EdgeSide side, const QRect &r, const CellData &c) const;
    void drawCenter           (QPainter &p, const QRect &cr) const;

    // diamond fallback for Income Tax
    void drawDiamond(QPainter &p, const QRect &r) const;
};

#endif
