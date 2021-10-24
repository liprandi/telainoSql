#include "zplc.h"

ZPlc::ZPlc(QObject *parent):
    QThread()
{
    m_run = false;
    m_quit = false;
}

ZPlc::~ZPlc()
{
    m_quit = true;
    while(m_run)
        msleep(100);
}

ZPlc::In::In(int id, int dbNumber, int start, int size, std::chrono::duration<long long, std::milli> msec):
    m_id(id)
  , m_db(dbNumber)
  , m_start(start)
  , m_size(size)
  , m_time(msec)
{
    m_last =  std::chrono::steady_clock::now();
    m_last -= m_time;
    m_buffer = new char[size];
}

ZPlc::In::~In()
{
    delete [] m_buffer;
    m_buffer = nullptr;
    m_db = 0;
    m_start = 0;
    m_size = 0;
    m_time = m_time.zero();
}
ZPlc::Out::Out(int dbNumber, int start, int size, void* buffer):
    m_db(dbNumber)
  , m_start(start)
  , m_size(size)
{
    m_buffer = new char[size];
    memmove(m_buffer, buffer, m_size);
}

ZPlc::Out::~Out()
{
    delete [] m_buffer;
    m_buffer = nullptr;
    m_db = 0;
    m_start = 0;
    m_size = 0;
}

void ZPlc::setAddress(const QString& address, int rack, int slot)
{
    m_address = address;
    m_rack = rack;
    m_slot = slot;
    start();
}

void ZPlc::setAreaIn(int id, int dbNumber, int start, int size, std::chrono::duration<long long, std::milli> msec)
{
    const std::lock_guard<std::mutex> lock(m_mutexRead);
    m_in.push_back(new In(id, dbNumber, start, size, msec));
}

void ZPlc::writeData(int dbNumber, int start, int size, void* buffer)
{
    const std::lock_guard<std::mutex> lock(m_mutexWrite);
    m_out.push_back(new Out(dbNumber, start, size, buffer));
}

char* ZPlc::getData(int id)
{
    char* buff = nullptr;
    const std::lock_guard<std::mutex> lock(m_mutexRead);
    for(auto i: m_in)
    {
        if(i->m_id == id)
        {
            buff = i->m_buffer;
            break;
        }
    }
    return buff;
}

void ZPlc::run()
{
    bool ok = true;
    m_run = true;
    m_s7 = Cli_Create();
    while(!m_quit)
    {
        ok = true;
        if(!Cli_ConnectTo(m_s7, m_address.toLocal8Bit().data(), m_rack, m_slot))
        {
            Cli_GetOrderCode(m_s7, &m_orderCode);
            Cli_GetCpuInfo(m_s7, &m_cpuInfo);
            while(!m_quit && ok)
            {
                // read data
                {
                    auto t = std::chrono::steady_clock::now();
                    const std::lock_guard<std::mutex> lock(m_mutexRead);
                    for(auto i: m_in)
                    {
                        auto diff = t - i->m_last;
                        if(diff > i->m_time)
                        {
                            i->m_last = t;
                            if(Cli_DBRead(m_s7, i->m_db, i->m_start, i->m_size, i->m_buffer))
                                ok = false;
                        }
                    }
                }
                // write data
                {
                    if(m_mutexWrite.try_lock())
                    {
                        while(m_out.size() > 0)
                        {
                            auto i = m_out.front();
                            if(Cli_DBWrite(m_s7, i->m_db, i->m_start, i->m_size, i->m_buffer))
                                ok = false;
                            m_out.pop_front();
                        }
                        m_mutexWrite.unlock();
                    }
                }
                msleep(100);
            }
            Cli_Disconnect(m_s7);
        }
        else
            sleep(3);
    }
    m_run = false;
    m_quit = false;
}
