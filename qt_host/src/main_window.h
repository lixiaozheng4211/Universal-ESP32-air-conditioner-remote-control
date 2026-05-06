#pragma once

#include "ac_catalog.h"
#include "known_ac_runner.h"
#include "ac_store.h"
#include "serial_controller.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QTreeView>
#include <QComboBox>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshPorts();
    void toggleConnection();
    void addAirConditioner();
    void startKnownDevices();
    void appendSerialLine(const QString &line);
    void updateSerialStatus(const QString &status);

private:
    void buildUi();
    void populateCatalogTree();
    void reloadKnownDevices();
    void refreshKnownTable();
    bool sendCommand(const QString &command);

    QVector<AcBrand> m_catalog;
    QVector<KnownAcDevice> m_knownDevices;

    AcStore m_store;
    SerialController m_serial;
    KnownAcRunner m_knownRunner;

    QComboBox *m_portCombo = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_addButton = nullptr;
    QTreeView *m_catalogTree = nullptr;
    QStandardItemModel *m_catalogModel = nullptr;
    QTableWidget *m_knownTable = nullptr;
    QPlainTextEdit *m_log = nullptr;
};
