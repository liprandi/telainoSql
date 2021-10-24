#ifndef CYCLE_H
#define CYCLE_H

#include <QThread>
#include <QSettings>
#include <mutex>

#include "database.h"

class Cycle : public QThread
{
    Q_OBJECT
public:
    struct CisInfo
    {
        unsigned char cis[8];
        unsigned long seqnum;
        unsigned short model;
        unsigned char tt[4];
        unsigned char eco[4];
    };

    struct NextSeqInfo
    {
        unsigned long seqnum[10];
    };
public:
    explicit Cycle(QSettings* settings, QObject *parent = nullptr);
    virtual ~Cycle();

    void setCis(const QByteArray& cis)
    {
        if(m_cisMutex.try_lock())
        {
            m_cisCurrent = cis;
            m_cisMutex.unlock();
        }
    }
    bool getCisData(CisInfo& data)
    {
        bool ok = false;
        if(m_cisMutex.try_lock())
        {
            if(m_data.cis[0] >= '0' && m_data.cis[0] <= '9')
            {
                data = m_data;
                m_data.cis[0] = 0;
                ok = true;
            }
            m_cisMutex.unlock();
        }
        return ok;
    }
    bool getNextData(NextSeqInfo& nxt)
    {
        bool ok = false;
        if(m_cisMutex.try_lock())
        {
            if(m_next.seqnum[0] > 0)
            {
                nxt = m_next;
                m_next.seqnum[0] = 0;
            }
            m_cisMutex.unlock();
            ok = true;
        }
        return ok;
    }
private:
    bool readCisInfo();
    void readSeqInfo();
protected:
    virtual void run() override;
private:
    bool m_run;
    bool m_quit;
    Database m_db;
    CisInfo m_data;
    NextSeqInfo m_next;
    QString m_dbConnection;     // for example {SQL Server Native Client 11.0}
    QSettings* m_settings;  // setting used in application
    QString m_cisCurrent;   // cis to examine
    QString m_cisChecked;   // cis examined
    QString m_cisnumQuery;     // query used with CISNUM number
    QString m_seqnumQuery;  // query used with SEQNUM number
    std::mutex m_cisMutex;
};

#endif // CYCLE_H
