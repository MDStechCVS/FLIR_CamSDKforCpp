#include "CMMTiming.h"

CMMTiming::CMMTiming()
{
    timeBeginPeriod(1);  // MM Ÿ�̸� �ػ� ����
    m_StartTime = timeGetTime();  // ���� �ð� ����
    _ftime64_s(&m_tb);  // ���� �ð� ������
}

CMMTiming::~CMMTiming()
{
    timeEndPeriod(1);  // MM Ÿ�̸� �ػ� ����
}

void CMMTiming::GetLocalTime(time_t& timestamp, int& millisec, short& timeZoneBias)
{
    DWORD tm = timeGetTime() + m_tb.millitm;

    if (tm < m_StartTime)
    {
        // Wrap around - usually takes 47 days!!
        m_StartTime = timeGetTime();
        _ftime64_s(&m_tb);
    }

    // Coordinated UTC time in seconds since 1970-01-01 00:00
    timestamp = m_tb.time +
        ((tm - m_StartTime) / 1000)
        - (m_tb.timezone * 60);

    timeZoneBias = 0;

    // Correct for daylight savings
    if (m_tb.dstflag)
        timestamp += 3600;

    // Milliseconds since last second
    millisec = (tm - m_StartTime) % 1000;
}

void CMMTiming::GetUTCTime(time_t& timestamp, int& millisec, short& timeZoneBias)
{
    DWORD tm = timeGetTime() + m_tb.millitm;

    if (tm < m_StartTime)
    {
        // Wrap around - usually takes 47 days!!
        m_StartTime = timeGetTime();
        _ftime64_s(&m_tb);
    }

    // Coordinated UTC time in seconds since 1970-01-01 00:00
    timestamp = m_tb.time + ((tm - m_StartTime) / 1000);
    timeZoneBias = m_tb.timezone;

    // Correct for daylight savings
    if (m_tb.dstflag)
        timestamp += 3600;

    // Milliseconds since last second
    millisec = (tm - m_StartTime) % 1000;
}