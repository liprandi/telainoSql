#include <QMessageBox>
#include <QCloseEvent>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings("cam.ini", QSettings::IniFormat)
    , m_cycle(nullptr)
    , m_data(nullptr)
    , m_iconStatus(0)
{
    m_icon[0] = QIcon(":/icon/cam");
    m_icon[1] = QIcon(":/icon/camWarning");
    m_icon[2] = QIcon(":/icon/camFault");
    createActions();
    createTrayIcon();

    m_plcAddress = m_settings.value("plc/address", "129.100.2.1").toString();
    m_plcRack = m_settings.value("plc/rack", 0).toInt();
    m_plcSlot = m_settings.value("plc/slot", 2).toInt();
    m_plcDbRead = m_settings.value("plc/db_read", 1201).toInt();
    m_plcDbReadStart = m_settings.value("plc/db_read_start", 0).toInt();
    m_plcDbReadSize = m_settings.value("plc/db_read_size", 128).toInt();
    m_plcDbWrite = m_settings.value("plc/db_write", 1200).toInt();
    m_plcDbWriteStart = m_settings.value("plc/db_write_start", 0).toInt();
    m_plcDbWriteSize = m_settings.value("plc/db_write_size", 128).toInt();
    m_plc.setAddress(m_plcAddress, m_plcRack, m_plcSlot);
    m_data =  new unsigned char[m_plcDbWriteSize];
    memset(m_data, 0, m_plcDbWriteSize);
    m_cycle =  new Cycle(&m_settings);
    ui->setupUi(this);

    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::messageClicked);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);

    setIcon(m_iconStatus);
    trayIcon->show();
    startTimer(500ms);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_cycle;
    m_settings.setValue("plc/address", m_plcAddress);
    m_settings.setValue("plc/rack", m_plcRack);
    m_settings.setValue("plc/slot", m_plcSlot);
    m_settings.setValue("plc/db_read", m_plcDbRead);
    m_settings.setValue("plc/db_read_start", m_plcDbReadStart);
    m_settings.setValue("plc/db_read_size", m_plcDbReadSize);
    m_settings.setValue("plc/db_write", m_plcDbWrite);
    m_settings.setValue("plc/db_write_start", m_plcDbWriteStart);
    m_settings.setValue("plc/db_write_size", m_plcDbWriteSize);
    delete[] m_data;
}
// tray icon managment
void MainWindow::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    maximizeAction->setEnabled(!isMaximized());
    restoreAction->setEnabled(isMaximized() || !visible);
    QMainWindow::setVisible(visible);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(trayIcon->isVisible())
    {
        showMessage();
        hide();
        event->ignore();
    }
}
void MainWindow::setIcon(int icon)
{
    if(icon < 0 || icon > 2)
        return;
    m_iconStatus = icon;
    trayIcon->setIcon(m_icon[m_iconStatus]);
    setWindowIcon(m_icon[m_iconStatus]);
    QString msg;
    switch(m_iconStatus)
    {
        case 1:
            msg = tr("Warning operation");
            break;
        case 2:
            msg = tr("Fault operation");
            break;
        default:
            msg = tr("Normal operation mode");
            break;
    }

    trayIcon->setToolTip(msg);
}
void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if(isHidden())
            show();
        else
            hide();
        break;
    case QSystemTrayIcon::MiddleClick:
        showMessage();
        break;
    default:
        ;
    }
}
void MainWindow::showMessage()
{
    QSystemTrayIcon::MessageIcon msgIcon = QSystemTrayIcon::MessageIcon(m_iconStatus + 1);

    if(m_iconStatus >= 0)
    {
        trayIcon->showMessage(tr("Telaino"), tr("Program in running"), msgIcon, 15 * 1000);
    }
}
void MainWindow::messageClicked()
{
}
void MainWindow::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if(m_data && m_plcDbRead > 0 && m_plcDbReadSize > 0)
    {
        char* buff = m_plc.getData(2);
        if(!buff)
        {
            m_plc.setAreaIn(2, m_plcDbRead, m_plcDbReadStart, m_plcDbReadSize, 500ms);
        }
        else
        {
            QByteArray cis(&buff[2], 8);
            m_cycle->setCis(cis);
            Cycle::CisInfo cisres;
            if(m_cycle->getCisData(cisres))
            {
                Cycle::NextSeqInfo nxtres;
                if(m_cycle->getNextData(nxtres))
                {
                    // cis
                    memmove(&m_data[2], cisres.cis, 8);
                    // seqnum
                    m_data[10] = ((cisres.seqnum >> 24L) & 0x000000ffL);
                    m_data[11] = ((cisres.seqnum >> 16L) & 0x000000ffL);
                    m_data[12] = ((cisres.seqnum >> 8L)  & 0x000000ffL);
                    m_data[13] = (cisres.seqnum & 0x000000ffL);
                    // model
                    m_data[14] = ((cisres.model >> 8) & 0x00ff);
                    m_data[15] = (cisres.model & 0x00ff);
                    // tt
                    memmove(&m_data[16], cisres.tt, 4);
                    // eco
                    memmove(&m_data[20], cisres.eco, 4);
                    for(int i = 0; i < 10; i++)
                    {
                        m_data[30 + i * 4] = ((nxtres.seqnum[i] >> 24L) & 0x000000ffL);
                        m_data[31 + i * 4] = ((nxtres.seqnum[i] >> 16L) & 0x000000ffL);
                        m_data[32 + i * 4] = ((nxtres.seqnum[i] >> 8L) & 0x000000ffL);
                        m_data[33 + i * 4] = (nxtres.seqnum[i] & 0x000000ffL);
                    }
                    fillWindowDataOut(cisres, nxtres);
                }
            }
            m_data[0] = buff[0];
            m_data[1] = buff[1];
            m_plc.writeData(m_plcDbWrite, m_plcDbWriteStart, m_plcDbWriteSize, m_data);
            fillWindowDataIn(buff);
        }
    }
}
void MainWindow::fillWindowDataOut(const Cycle::CisInfo& cisres, const Cycle::NextSeqInfo nxtres)
{
    char tmp[10];

    QLineEdit* lnxt[] = {ui->leDbNxt1, ui->leDbNxt2, ui->leDbNxt3, ui->leDbNxt4, ui->leDbNxt5,
                 ui->leDbNxt6, ui->leDbNxt7, ui->leDbNxt8, ui->leDbNxt9, ui->leDbNxt10};
    memmove(tmp, cisres.cis, 8);
    tmp[8] = 0;
    ui->leDbCis->setText(QString(tmp));
    memmove(tmp, cisres.tt, 4);
    tmp[4] = 0;
    ui->leDbTT->setText(QString(tmp));
    memmove(tmp, cisres.eco, 4);
    tmp[4] = 0;
    ui->leDbEco->setText(QString(tmp));
    ui->leDbSeqnum->setText(QString("%1").arg(cisres.seqnum));
    ui->leDbModel->setText(QString("%1").arg(cisres.model));
    for(int i = 0; i < 10; i++)
    {
        lnxt[i]->setVisible(nxtres.seqnum[i] > 0);
        lnxt[i]->setText(QString("%1").arg(nxtres.seqnum[i]));
    }
}
void MainWindow::fillWindowDataIn(const char* buffFromPlc)
{
    unsigned short alive = buffFromPlc[0] * 0x100 + buffFromPlc[1];
    ui->leAlive->setText(QString("%1").arg(alive));
    char tmp[10];
    memmove(tmp, &buffFromPlc[2], 8);
    tmp[8] = 0;
    ui->leCis->setText(QString(tmp));
}
