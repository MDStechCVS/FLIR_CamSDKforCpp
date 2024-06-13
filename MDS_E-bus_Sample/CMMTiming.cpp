#include "CMMTiming.h"

CMMTiming::CMMTiming()
{
    timeBeginPeriod(1);  // MM 타이머 해상도 설정
    m_StartTime = timeGetTime();  // 시작 시간 설정
    _ftime64_s(&m_tb);  // 현재 시간 오프셋
}

CMMTiming::~CMMTiming()
{
    timeEndPeriod(1);  // MM 타이머 해상도 해제
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