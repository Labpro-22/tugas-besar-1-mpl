#include <QImage>
#include <QtTest>

#include "BoardWidget.hpp"
#include "GameWindow.hpp"
#include "PropertyCardWidget.hpp"

class BoardWidgetSmokeTest : public QObject
{
    Q_OBJECT

private slots:
    void createsWidget();
    void rendersOffscreen();
    void rendersPropertyCardOffscreen();
    void rendersGameWindowOffscreen();
};

void BoardWidgetSmokeTest::createsWidget()
{
    BoardWidget widget;
    QVERIFY(widget.minimumWidth() >= 700);
    QVERIFY(widget.minimumHeight() >= 700);
}

void BoardWidgetSmokeTest::rendersOffscreen()
{
    BoardWidget widget;
    widget.resize(920, 920);

    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);

    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 920);
    QCOMPARE(image.height(), 920);
}

void BoardWidgetSmokeTest::rendersPropertyCardOffscreen()
{
    PropertyCardWidget widget;
    widget.resize(360, 620);

    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    widget.render(&image);

    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 360);
    QCOMPARE(image.height(), 620);
}

void BoardWidgetSmokeTest::rendersGameWindowOffscreen()
{
    GameWindow window;
    window.resize(1320, 900);

    QImage image(window.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    window.render(&image);

    QVERIFY(!image.isNull());
    QCOMPARE(image.width(), 1320);
    QCOMPARE(image.height(), 900);
}

QTEST_MAIN(BoardWidgetSmokeTest)
#include "BoardWidgetSmokeTest.moc"
