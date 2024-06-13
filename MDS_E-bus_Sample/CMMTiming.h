#pragma once

#include "global.h"
// MM Ÿ�̸Ӹ� ���� Ŭ����
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
