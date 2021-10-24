#include "database.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <QDebug>

Database::Database(QObject *parent) : QObject(parent)
  ,  m_db(QSqlDatabase::addDatabase("QODBC"))
{
    m_connected = false;
}
bool Database::openDatabase(const QString& connectionString)
{
    m_db.setDatabaseName(connectionString);
    if(m_db.open())
    {
        m_connected = true;
        qDebug() << "Connection DB OK";
    }
    else
    {
        qDebug() << "Connection DB KO";
    }
    qDebug() << connectionString;
    return m_connected;
}
void Database::closeDatabase()
{
    if(m_db.open())
    {
        m_db.close();
        m_connected = false;
    }
}
bool Database::query(const QString& query)
{
    bool ret = false;
    m_result.clear();
    qDebug() << query;
    QSqlQuery q = m_db.exec(query);
    QSqlRecord r = q.record();
    while(q.next())
    {
        QMap<QString, QString> rec;
        for(int i = 0; i < r.count(); i++)
        {
            rec[r.fieldName(i)] = q.value(i).toString();
            qDebug() << r.fieldName(i) << " = " << q.value(i).toString();
            ret = true;
        }
        m_result.append(rec);
    }
    return ret;
}
