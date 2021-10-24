#include "cycle.h"
#include <QFile>

Cycle::Cycle(QSettings *settings, QObject *parent) : QThread(parent)
  , m_settings(settings)
  , m_cisCurrent("00000000")
  , m_cisChecked(m_cisCurrent)
{
    QFile conn("connection.txt");
    if(conn.open(QFile::ReadOnly))
    {
        m_dbConnection = conn.readAll();
        m_dbConnection.remove(QChar('\r'));
        m_dbConnection.remove(QChar('\n'));
        conn.close();
    }
    QFile cis("cisquery.sql");
    if(cis.open(QFile::ReadOnly))
    {
        m_cisnumQuery = cis.readAll();
        cis.close();
    }
    QFile seq("seqquery.sql");
    if(seq.open(QFile::ReadOnly))
    {
        m_seqnumQuery = seq.readAll();
        seq.close();
    }
    start();
}
Cycle::~Cycle()
{
    m_quit = true;
    while(m_run)
        msleep(100);
}

void Cycle::run()
{
    m_run = true;

    m_db.openDatabase(m_dbConnection);

    while(!m_quit)
    {
        m_cisMutex.lock();
        if(m_cisChecked.compare(m_cisCurrent) != 0)
        {
            if(readCisInfo())
                readSeqInfo();

            m_cisChecked = m_cisCurrent;
        }
        m_cisMutex.unlock();
        msleep(500);
    }
    m_db.closeDatabase();
    m_run = false;
    m_quit = false;
}
bool Cycle::readCisInfo()
{
    QString cis7 = m_cisCurrent.left(7);
    QString query =  QString(m_cisnumQuery).arg(cis7);
    if(m_db.query(query))
    {
        m_data.cis[0] = 0;
        auto res = m_db.result();
        for(auto& i: res)
        {
            QString c = i["CISNUM"];
            if(!c.compare(cis7))
            {
                if(m_cisCurrent.length() >= 8)
                    memmove(m_data.cis, m_cisCurrent.toLatin1().data(), 8);
                m_data.seqnum = i["SEQNUM"].toULong();
                m_data.model = i["MODCOD"].toUShort();
                if(!i["VAL_L1"].compare("TT"))
                {
                    memset(m_data.tt, ' ', 4);
                    int l = i["VAL_L2"].length();
                    if(l > 4)
                        l = 4;
                    memmove(m_data.tt, i["VAL_L2"].toLatin1().data(), l);
                }
                else if(!i["VAL_L1"].compare("ECO"))
                {
                    memset(m_data.eco, ' ', 4);
                    int l = i["VAL_L2"].length();
                    if(l > 4)
                        l = 4;
                    memmove(m_data.eco, i["VAL_L2"].toLatin1().data(), l);
                }
            }
        }
    }
    return m_data.seqnum > 0;
}
void Cycle::readSeqInfo()
{
    memset(m_next.seqnum, 0L, sizeof(m_next.seqnum));
    QString query =  QString(m_seqnumQuery).arg(m_data.seqnum);
    if(m_db.query(query))
    {
        auto res = m_db.result();
        int cnt = 0;
        unsigned long seqnum = 0;
        unsigned long prev_seqnum = 0;
        for(auto& i: res)
        {
            seqnum = i["SEQNUM"].toULong();
            if(seqnum != 0 && seqnum == prev_seqnum)
            {
                m_next.seqnum[cnt++] = seqnum;
                prev_seqnum = 0;
            }
            else if(seqnum > 0)
            {
                prev_seqnum = seqnum;
            }
        }
    }
}
