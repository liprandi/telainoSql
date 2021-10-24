#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QMap>
#include <QList>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = nullptr);
    bool openDatabase(const QString& connectionString);
    void closeDatabase();

    bool query(const QString& query);

    const QList<QMap<QString, QString>>& result()
    {
        return m_result;
    }
signals:

private:
     QSqlDatabase m_db;
     bool m_connected;
     QList<QMap<QString, QString>> m_result;
};

#endif // DATABASE_H
