#ifndef ZPLC_H
#define ZPLC_H

#include <QThread>
#include <mutex>
#include <list>
#include <chrono>
#include "snap7.h"

using namespace std::chrono_literals;

class ZPlc: public QThread
{
    Q_OBJECT
public:
    class In
    {
    public:
        In(int id, int dbNumber, int start, int size, std::chrono::duration<__int64, std::milli> msec);
        ~In();
    private:
        int m_id;
        char* m_buffer;
        int m_db;
        int m_start;
        int m_size;
        std::chrono::duration<__int64, std::milli> m_time;
        std::chrono::time_point<std::chrono::steady_clock> m_last;
        friend class ZPlc;
    };
    class Out
    {
    public:
        Out(int dbNumber, int start, int size, void* buffer);
        ~Out();
    private:
        char* m_buffer;
        int m_db;
        int m_start;
        int m_size;
        friend class ZPlc;
    };
public:
    ZPlc(QObject *parent = nullptr);
    virtual ~ZPlc();

    void setAddress(const QString& address, int rack, int slot);
    void setAreaIn(int id, int dbNumber, int start, int size, std::chrono::duration<long long, std::milli> msec);
    void writeData(int dbNumber, int start, int size, void* buffer);
    char* getData(int id);
protected:
    virtual void run() override;

private:
    bool m_run;
    bool m_quit;
    S7Object m_s7;
    TS7OrderCode m_orderCode;
    TS7CpuInfo m_cpuInfo;
    std::list<In*> m_in;
    std::list<Out*> m_out;

    QString m_address;
    int m_rack;
    int m_slot;
    std::mutex m_mutexRead;
    std::mutex m_mutexWrite;
};

#endif // ZPLC_H
