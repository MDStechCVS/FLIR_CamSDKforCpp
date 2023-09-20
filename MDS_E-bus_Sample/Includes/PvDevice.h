// *****************************************************************************
//
//     Copyright (c) 2007, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#ifndef __PV_DEVICE_H__
#define __PV_DEVICE_H__

#include <PvDeviceLib.h>
#include <PvDeviceEventSink.h>
#include <PvDeviceInfo.h>
#include <PvGenParameterArray.h>


namespace PvDeviceLib
{
    class Device;

}; // namespace PvDeviceLib;


class PvDeviceEventSink;
class PvConfigurationWriter;
class PvConfigurationReader;


class PV_DEVICE_API PvDevice 
{
public:

    virtual ~PvDevice();

    static PvDevice *CreateAndConnect( const PvDeviceInfo *aDeviceInfo, PvResult *aResult );
    static PvDevice *CreateAndConnect( const PvString &aInfo, PvResult *aResult );
    static void Free( PvDevice *aDevice );

    PvDeviceType GetType() const;

	virtual PvResult Connect( const PvDeviceInfo *aDeviceInfo );
	virtual PvResult Connect( const PvString &aInfo );
	PvResult Disconnect();

    PvResult StreamEnable( uint32_t aChannel = 0 );
    PvResult StreamDisable( uint32_t aChannel = 0 );

	bool IsConnected() const;
    bool IsPleoraPowered() const;

	PvResult DumpGenICamXML( const PvString &aFilename );
	PvResult GetDefaultGenICamXMLFilename( PvString &aFilename );

	PvGenParameterArray *GetParameters();
	PvGenParameterArray *GetCommunicationParameters();

	PvResult ReadMemory( int64_t aAddress, unsigned char *aDestination, int64_t aByteCount );
	PvResult WriteMemory( int64_t aAddress, const unsigned char *aSource, int64_t aByteCount );

    PvResult WaitForMessagingChannelIdle( uint32_t aTimeout );

    PvResult RegisterEventSink( PvDeviceEventSink *aEventSink );
    PvResult UnregisterEventSink( PvDeviceEventSink *aEventSink );

    uint32_t GetHeartbeatThreadPriority() const;
    PvResult SetHeartbeatThreadPriority( uint32_t aPriority );

    uint32_t GetInterruptLinkThreadPriority() const;
    PvResult SetInterruptLinkThreadPriority( uint32_t aPriority );

    uint32_t GetInterruptQueueThreadPriority() const;
    PvResult SetInterruptQueueThreadPriority( uint32_t aPriority );

    uint32_t GetRecoveryThreadPriority() const;
    PvResult SetRecoveryThreadPriority( uint32_t aPriority );

    uint32_t GetPayloadSize();

protected:
    
	PvDevice();

    PvDeviceLib::Device *mThis;

    friend class PvConfigurationWriter;
    friend class PvConfigurationReader;

private:

	 // Not implemented
	PvDevice( const PvDevice & );
	const PvDevice &operator=( const PvDevice & );

};


#ifdef PV_INTERNAL_HEADERS
    #include <PvDeviceLib/Device.h>
#endif // PV_INTERNAL_HEADERS


#endif // __PV_DEVICE_H__


