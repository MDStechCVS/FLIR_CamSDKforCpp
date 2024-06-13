/***********************************************************************
 *
 * Project:   Matrix
 * $Workfile: temperature.cpp $
 *
 * Description of file:
 *  Temperature class implementation file
 *
 * Copyright:	FLIR Systems AB
 ***********************************************************************/

/*
  Inclusions
  ----------
*/


#include "stdafx.h"
#include "temperature.hpp"

/*
  Implementational considerations
  -------------------------------
*/

/*
  Definitions
  -----------
*/

CTemperature::CTemperature(tempUnit unit, bool isDiff)
{
    meUnit = unit;
    mdValue = 0;
    mbIsDiffTemp = isDiff;
}


CTemperature::CTemperature(CTemperature& otherTemp)
{
    meUnit = otherTemp.meUnit;
    mdValue = otherTemp.mdValue;
    mbIsDiffTemp = otherTemp.mbIsDiffTemp;
}


CTemperature::CTemperature(double otherTemp, tempUnit unit)
{
    Set(otherTemp, unit);
    mbIsDiffTemp = false;
}

void CTemperature::Set(double otherTemp)
{
    switch(meUnit)
    {
        case Kelvin:
            mdValue = otherTemp;
            break;
        case Celsius:
            mdValue = CtoK(otherTemp,mbIsDiffTemp);
            break;
        case Fahrenheit:
            mdValue = FtoK(otherTemp,mbIsDiffTemp);
            break;
        default:
            ASSERT(false);
            break;
    }            
}

void CTemperature::Set(double otherTemp, tempUnit unit)
{
    meUnit = unit;
    Set(otherTemp);
}

double CTemperature::Value(tempUnit getUnit)
{
    double val;
    
    switch (getUnit) {
    case Kelvin:
        val = mdValue;
        break;
        
    case Celsius:
        val = KtoC(mdValue, mbIsDiffTemp);
        break;

    case Fahrenheit:
        val = KtoF(mdValue, mbIsDiffTemp);
        break;

    case orgUnit:
        {
            if (meUnit == Kelvin)
                val = mdValue;
            else if (meUnit == Celsius)
                val = KtoC(mdValue, mbIsDiffTemp);
            else if (meUnit == Fahrenheit)
                val = KtoF(mdValue, mbIsDiffTemp);
            else
                ASSERT(FALSE);
        }
        break;
        
    default:
        ASSERT(FALSE);
    }
    
    return (val);
}


CTemperature::tempUnit CTemperature::getUnit(void)
{
    return (meUnit);
}


void CTemperature::setUnit(CTemperature::tempUnit newUnit)
{
    meUnit = newUnit;
}


bool CTemperature::getIsDiff(void)
{
    return (mbIsDiffTemp);
}


void CTemperature::setIsDiff(bool newIsDiff)
{
    mbIsDiffTemp = newIsDiff;
}


CTemperature& CTemperature::operator=(CTemperature& otherTemp)
{
    // Don't copy meUnit...
    mdValue = otherTemp.mdValue;
    mbIsDiffTemp = otherTemp.mbIsDiffTemp;
    
    return (*this);
}

CTemperature& CTemperature::operator=(const double& otherKelvinTemp)
{
    mdValue = otherKelvinTemp;

    return (*this);
}

CTemperature& CTemperature::operator=(const float& otherKelvinTemp)
{
    mdValue = otherKelvinTemp;

    return (*this);
}

double CTemperature::KtoC(double val, bool isDiffTemp)
{
    if (!isDiffTemp)
        val = val - ZERO_C;

    return (val);
}

double CTemperature::CtoK(double val, bool isDiffTemp)
{
    if (!isDiffTemp)
        val = val + ZERO_C;

    return (val);
}

double CTemperature::KtoF(double val, bool isDiffTemp)
{
    if (!isDiffTemp)
        val = val - ZERO_C;
    
    val = val * 9/5;

    if (!isDiffTemp)
        val = val + 32;

    return (val);
}

double CTemperature::FtoK(double val, bool isDiffTemp)
{
    if (!isDiffTemp)
        val = val - 32;
    
    val = val * 5 / 9;

    if (!isDiffTemp)
        val = val + ZERO_C;

    return (val);
}