// *****************************************************************************
//
//     Copyright (c) 2011, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#ifndef __PVSERIALBRIDGEMANAGERWND_H__
#define __PVSERIALBRIDGEMANAGERWND_H__


#include <PvGUILib.h>
#include <PvWnd.h>

#include <PvSerialBridge.h>
#include <PvGenParameterArray.h>
#include <PvPropertyList.h>
#include <PvStringList.h>


typedef enum 
{
    PvSerialBridgeTypeInvalid = -1,
    PvSerialBridgeTypeNone = 0,
    PvSerialBridgeTypeSerialCOMPort = 1,
    PvSerialBridgeTypeCameraLinkDLL = 2,
    PvSerialBridgeTypeCLProtocol = 3,

} PvSerialBridgeType;


class PV_GUI_API PvSerialBridgeManagerWnd : public PvWnd
{
public:

	PvSerialBridgeManagerWnd();
	virtual ~PvSerialBridgeManagerWnd();

    PvResult SetDevice( IPvDeviceAdapter *aDevice );

    uint32_t GetPortCount() const;
    PvString GetPortName( uint32_t aIndex ) const;
    PvGenParameterArray *GetGenParameterArray( uint32_t aIndex );
    PvString GetGenParameterArrayName( uint32_t aIndex );

    virtual void OnParameterArrayCreated( PvGenParameterArray *aArray, const PvString &aName );
    virtual void OnParameterArrayDeleted( PvGenParameterArray *aArray );

    PvResult Save( PvPropertyList &aPropertyList );
    PvResult Load( PvPropertyList &aPropertyList, PvStringList &aErrors );

    PvResult Recover();

protected:

private:

    // Not implemented
	PvSerialBridgeManagerWnd( const PvSerialBridgeManagerWnd & );
	const PvSerialBridgeManagerWnd &operator=( const PvSerialBridgeManagerWnd & );

};


#endif // __PVSERIALBRIDGEMANAGERWND_H__

