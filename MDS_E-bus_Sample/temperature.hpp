/***********************************************************************
 *
 * Project:   Matrix
 * $Workfile: temperature.hpp $
 *
 * Description of file:
 *  Temperature class declaration
 *
 * Copyright:	FLIR Systems AB
 ***********************************************************************/
#ifndef _TEMPERATURE_HPP
#define _TEMPERATURE_HPP

/*
  Inclusions, assumed and actual
  ------------------------------
*/

/*
  Implementational considerations
  -------------------------------
*/

/*
  Definitions
  -----------
*/
#ifndef ZERO_C
#define ZERO_C   273.15
#endif

class CTemperature {    
    // Exported methods/types
public:
    typedef enum {
        Kelvin = 1,
        Celsius,
        Fahrenheit,
        orgUnit       // Interface method use only, i.e. Value() below
    } tempUnit;

    CTemperature(tempUnit unit = Kelvin, bool isDiff = false);


    // Copy constructor
    CTemperature(CTemperature& otherTemp);

    CTemperature(double otherTemp, tempUnit unit = Kelvin);

    // Assignment Operation
    operator const double() const { return mdValue; }
    
    // Explicit result unit
    double Value(tempUnit getUnit = orgUnit);
    void Set(double otherTemp, tempUnit unit);
    void Set(double otherTemp);


    tempUnit getUnit(void);

    void setUnit(tempUnit newUnit);

    bool getIsDiff(void);

    void setIsDiff(bool newIsDiff);
    
    CTemperature& operator=(CTemperature& otherTemp);

    CTemperature& operator=(const double& otherKelvinTemp);

    CTemperature& operator=(const float& otherKelvinTemp);    

    // More operators to come...
    
    // Private data    
protected:
    double mdValue;
    tempUnit meUnit;
    bool mbIsDiffTemp;

    inline double KtoC(double val, bool isDiffTemp);
    
    inline double KtoF(double val, bool isDiffTemp);

    inline double CtoK(double val, bool isDiffTemp);
    
    inline double FtoK(double val, bool isDiffTemp);
};

#endif // _TEMPERATURE_HPP