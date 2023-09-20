// *****************************************************************************
//
//     Copyright (c) 2011, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#ifndef __PVGENBROWSERTABBEDWND_H__
#define __PVGENBROWSERTABBEDWND_H__


#include <PvGUILib.h>
#include <PvWnd.h>
#include <PvGenParameterArray.h>
#include <PvPropertyList.h>


class GenBrowserWndBase;


class PV_GUI_API PvGenBrowserTabbedWnd : public PvWnd
{
public:

	PvGenBrowserTabbedWnd();
	virtual ~PvGenBrowserTabbedWnd();

    void Reset();
    PvResult AddGenParameterArray( PvGenParameterArray *aArray, const PvString &aName, uint32_t *aIndex = NULL );
    PvResult RemoveGenParameterArray( uint32_t aIndex );

    uint32_t GetParameterArrayCount() const;
    const PvGenParameterArray *GetGenParameterArray( uint32_t aIndex ) const;
    PvResult GetParameterArrayName( uint32_t aIndex, PvString &aName ) const;
    PvResult SetParameterArrayName( uint32_t aIndex, const PvString &aName );

	virtual bool IsParameterDisplayed( PvGenParameterArray *aArray, PvGenParameter *aParameter );

    PvResult SetVisibility( PvGenVisibility aVisibility );
    PvGenVisibility GetVisibility();

    PvResult Save( PvPropertyList &aPropertyList );
    PvResult Load( PvPropertyList &aPropertyList );

    void Refresh();
    void Refresh( uint32_t aIndex );

protected:

private:

    // Not implemented
	PvGenBrowserTabbedWnd( const PvGenBrowserTabbedWnd & );
	const PvGenBrowserTabbedWnd &operator=( const PvGenBrowserTabbedWnd & );

};


#endif // __PVGENBROWSERTABBEDWND_H__

