#pragma once

#include "global.h"
// MM 타이머를 위한 클래스
class CMMTiming
{
public:
    CMMTiming();
    virtual ~CMMTiming();

    void GetLocalTime(time_t& timestamp, int& millisec, short& timeZoneBias);
    void GetUTCTime(time_t& timestamp, int& millisec, short& timeZoneBias);

protected:
    DWORD m_StartTime;
    __timeb64 m_tb;
};
