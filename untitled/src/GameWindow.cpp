#include "GameWindow.hpp"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QVector>

#include <exception>

#include "BoardWidget.hpp"
#include "MonopolyUiShared.hpp"
#include "PropertyCardWidget.hpp"
#include "models/config/ConfigData.hpp"
#include "utils/ConfigLoader.hpp"

GameWindow::GameWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gameWindow"));

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(18);

    boardWidget = new BoardWidget(this);
    rootLayout->addWidget(boardWidget, 1);

    inspectorPanel = new QFrame(this);
    inspectorPanel->setObjectName(QStringLiteral("propertyInspector"));
    inspectorPanel->setMinimumWidth(380);
    inspectorPanel->setMaximumWidth(430);
    inspectorPanel->hide();

    auto *inspectorLayout = new QVBoxLayout(inspectorPanel);
    inspectorLayout->setContentsMargins(20, 20, 20, 20);
    inspectorLayout->setSpacing(14);

    auto *eyebrow = new QLabel(QStringLiteral("PROPERTY CARD"), inspectorPanel);
    eyebrow->setObjectName(QStringLiteral("eyebrow"));
    inspectorLayout->addWidget(eyebrow);

    auto *title = new QLabel(QStringLiteral("Property Card (Figma Rebuild)"), inspectorPanel);
    title->setObjectName(QStringLiteral("inspectorTitle"));
    title->setWordWrap(true);
    inspectorLayout->addWidget(title);

    auto *hint = new QLabel(
        QStringLiteral("Versi ini memprioritaskan Property Card (street) sesuai config/property.txt. Klik tile board atau pilih dari daftar."),
        inspectorPanel
    );
    hint->setObjectName(QStringLiteral("inspectorHint"));
    hint->setWordWrap(true);
    inspectorLayout->addWidget(hint);

    propertyPicker = new QComboBox(inspectorPanel);
    propertyPicker->setObjectName(QStringLiteral("propertyPicker"));
    inspectorLayout->addWidget(propertyPicker);

    propertyCardWidget = new PropertyCardWidget(inspectorPanel);
    propertyCardWidget->hide();
    inspectorLayout->addWidget(propertyCardWidget, 1);

    rootLayout->addWidget(inspectorPanel);

    setStyleSheet(
        "#gameWindow {"
        "  background: #05090d;"
        "}"
        "#propertyInspector {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "      stop:0 #d1eefb, stop:1 #9ed0eb);"
        "  border: 2px solid #6ea5c4;"
        "  border-radius: 24px;"
        "}"
        "#eyebrow {"
        "  color: #244d66;"
        "  font: 700 12pt 'Inter';"
        "  letter-spacing: 1px;"
        "}"
        "#inspectorTitle {"
        "  color: #10202c;"
        "  font: 900 17pt 'Inter';"
        "}"
        "#inspectorHint {"
        "  color: #315469;"
        "  font: 500 10pt 'Inter';"
        "}"
        "#propertyPicker {"
        "  min-height: 40px;"
        "  padding: 8px 12px;"
        "  border: 1px solid #6b98b1;"
        "  border-radius: 14px;"
        "  background: rgba(255, 255, 255, 0.88);"
        "  color: #10202c;"
        "  font: 600 10pt 'Inter';"
        "}"
    );

    loadPropertyConfigs();
    populatePropertyPicker();

    QVector<BoardWidget::PawnData> demoPawns;
    demoPawns.append({QStringLiteral("ITB"), QStringLiteral("ITB.png"), 0, QColor(237, 87, 69), false});
    demoPawns.append({QStringLiteral("UGM"), QStringLiteral("UGM.avif"), 10, QColor(52, 152, 219), false});
    demoPawns.append({QStringLiteral("UI"), QStringLiteral("UI.jpg"), 20, QColor(46, 204, 113), false});
    boardWidget->setPawns(demoPawns);

    connect(boardWidget, &BoardWidget::propertySelected, this, [this](int propertyId) {
        setSelectedProperty(propertyId);
    });

    connect(propertyPicker, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) {
            return;
        }

        setSelectedProperty(propertyPicker->itemData(index).toInt());
    });

}

void GameWindow::loadPropertyConfigs()
{
    const QString configDir = MonopolyUi::findConfigDirectory();
    if (configDir.isEmpty()) {
        return;
    }

    try {
        ConfigLoader loader(configDir.toStdString());
        const ConfigData configData = loader.loadAll();
        properties = configData.getPropertyConfigs();
        propertyCardWidget->setConfigData(configData);
    } catch (const std::exception&) {
        properties.clear();
    }
}

void GameWindow::populatePropertyPicker()
{
    propertyPicker->clear();
    pickerIndexByPropertyId.clear();

    if (properties.empty()) {
        propertyPicker->addItem(QStringLiteral("Config property tidak ditemukan"));
        propertyPicker->setEnabled(false);
        return;
    }

    propertyPicker->setEnabled(true);

    int index = 0;
    for (const PropertyConfig& property : properties) {
        if (property.getPropertyType() != PropertyType::STREET) {
            continue;
        }

        const QString label = QStringLiteral("%1. %2 (%3)")
            .arg(property.getId())
            .arg(MonopolyUi::formatTileName(property.getName()).replace('\n', ' '))
            .arg(QString::fromStdString(property.getCode()));
        propertyPicker->addItem(label, property.getId());
        pickerIndexByPropertyId.insert(property.getId(), index);
        ++index;
    }

    if (index == 0) {
        propertyPicker->clear();
        propertyPicker->addItem(QStringLiteral("Belum ada data STREET di property.txt"));
        propertyPicker->setEnabled(false);
    }
}

void GameWindow::setSelectedProperty(int propertyId)
{
    boardWidget->setSelectedPropertyId(propertyId);

    if (!pickerIndexByPropertyId.contains(propertyId)) {
        propertyCardWidget->hide();
        inspectorPanel->hide();
        return;
    }

    inspectorPanel->show();
    propertyCardWidget->show();
    propertyCardWidget->setSelectedProperty(propertyId);
    boardWidget->setSelectedPropertyId(propertyId);

    const int comboIndex = pickerIndexByPropertyId.value(propertyId, -1);
    if (comboIndex >= 0 && propertyPicker->currentIndex() != comboIndex) {
        propertyPicker->blockSignals(true);
        propertyPicker->setCurrentIndex(comboIndex);
        propertyPicker->blockSignals(false);
    }
}
