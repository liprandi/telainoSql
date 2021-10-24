#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QSystemTrayIcon>
#include "zplc.h"
#include "cycle.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    virtual void setVisible(bool visible) override;

protected:
    virtual void timerEvent(QTimerEvent *event) override;
    virtual void closeEvent(QCloseEvent *event) override;

private:
    void fillWindowDataOut(const Cycle::CisInfo& cisres, const Cycle::NextSeqInfo nxtres);
    void fillWindowDataIn(const char* buffFromPlc);

private slots:
     void setIcon(int icon);
     void iconActivated(QSystemTrayIcon::ActivationReason reason);
     void showMessage();
     void messageClicked();

private:
     void createActions();
     void createTrayIcon();

private:
    Ui::MainWindow *ui;
    QSettings m_settings;
    ZPlc m_plc;
    Cycle* m_cycle;
    QString m_plcAddress;   // plc ip address
    int m_plcRack;          // plc rack, normally 0
    int m_plcSlot;          // plc slot, normally 2
    int m_plcDbRead;        // DB to read from the plc
    int m_plcDbReadStart;   // tart byte for area to read from the plc
    int m_plcDbReadSize;    // size of the area to read from the plc
    int m_plcDbWrite;       // DB to wrinte in the plc
    int m_plcDbWriteStart;  // start byte for area to write in the plc
    int m_plcDbWriteSize;   // size of the area to write in the plc
    unsigned char *m_data;  // area to write in PLC DB
private: // tray icon managment

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    int m_iconStatus;
    QIcon m_icon[3];
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
 };
#endif // MAINWINDOW_H
