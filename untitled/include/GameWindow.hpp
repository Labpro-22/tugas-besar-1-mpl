#pragma once

#include <QFrame>
#include <QMap>
#include <QWidget>

#include <vector>

#include "models/config/PropertyConfig.hpp"

class BoardWidget;
class PropertyCardWidget;
class QComboBox;

class GameWindow : public QWidget
{
public:
    explicit GameWindow(QWidget *parent = nullptr);

private:
    void loadPropertyConfigs();
    void populatePropertyPicker();
    void setSelectedProperty(int propertyId);

    BoardWidget* boardWidget = nullptr;
    QFrame* inspectorPanel = nullptr;
    PropertyCardWidget* propertyCardWidget = nullptr;
    QComboBox* propertyPicker = nullptr;
    std::vector<PropertyConfig> properties;
    QMap<int, int> pickerIndexByPropertyId;
};
