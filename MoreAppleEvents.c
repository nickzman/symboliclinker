/*
	File:		MoreAppleEvents.cp

	Contains:	Apple Event Manager utilities.

	DRI:		George Warner

	Copyright:	Copyright (c) 2000-2001 by Apple Computer, Inc., All Rights Reserved.

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Change History (most recent first):

$Log: MoreAppleEvents.cp,v $
Revision 1.21  2002/03/08 23:51:00  geowar
More cleanup.
Fixed memory leak in MoreAETellSelfToSetCFStringRefProperty

Revision 1.20  2002/03/07 20:31:51  geowar
General clean up.
Added recovery code to MoreAESendEventReturnPString.
New API: MoreAECreateAEDescFromCFString.

Revision 1.19  2002/02/19 18:54:57  geowar
Written by: => DRI:

Revision 1.18  2002/01/16 19:11:06  geowar
err => anErr, (anErr ?= noErr) => (noErr ?= anErr)
Added MoreAESendEventReturnAEDesc, MoreAESendEventReturnAEDescList,
MoreAETellSelfToSetCFStringRefProperty,  & MoreAEGetCFStringFromDescriptor routines.

Revision 1.17  2001/11/07 15:50:50  eskimo1
Tidy up headers, add CVS logs, update copyright.


        <16>     21/9/01    Quinn   Changes for CWPro7 Mach-O build.
        <15>     8/28/01    gaw     CodeBert (error -> pError, theAppleEvent -> pAppleEvent, etc.)
        <14>     15/2/01    Quinn   MoreAECreateAppleEventTargetID is not supported for Carbon
                                    builds because all of its required declarations are defined
                                    CALL_NOT_IN_CARBON. Also some minor fixes prompted by gcc
                                    warnings.
        <13>     26/5/00    Quinn   Eliminate bogus consts detected by MPW's SC compiler.
        <12>     4/26/00    gaw     Fix bug, swapped creator & file type parameters
        <11>     27/3/00    Quinn   Remove MoreAEDeleteItemFromRecord.  It's functionality is
                                    covered by AEDeleteKeyDesc.
        <10>     20/3/00    Quinn   Added routines to deal with "missing value".  Added
                                    MoreAECopyDescriptorDataToHandle.  Added
                                    MoreAEDeleteItemFromRecord.
         <9>      3/9/00    gaw     Y2K!
         <8>      3/9/00    gaw     API changes for MoreAppleEvents
         <7>      3/9/00    GW      Intergrating AppleEvent Helper code. First Check In
         <6>      6/3/00    Quinn   Added a bunch of trivial wrapper routines.  George may come
                                    along and change all these soon, but I needed them for MoreOSL.
         <5>      1/3/00    Quinn   Change the signature for AEGetDescData to match the version we
                                    actually shipped.
         <4>     2/15/99    PCG     add AEGetDescDataSize for non-Carbon clients
         <3>     1/29/99    PCG     add AEGetDescData
         <2>    11/11/98    PCG     fix header
         <1>    11/10/98    PCG     first big re-org at behest of Quinn

	Old Change History (most recent first):

         <2>    10/11/98    Quinn   Convert "MorePrefix.h" to "MoreSetup.h".
         <2>     6/16/98    PCG     CreateProcessTarget works with nil PSN
         <1>     6/16/98    PCG     initial checkin
*/

//	Conditionals to setup the build environment the way we like it.
#include "MoreSetup.h"

/*#if !MORE_FRAMEWORK_INCLUDES
// **********	Universal Headers		****************************************
	#include <AERegistry.h>
	#include <AEHelpers.h>
	#include <AEObjects.h>
	#include <AEPackObject.h>
	#include <ASRegistry.h>
	//#include <FinderRegistry.h>
	#include <Gestalt.h>
#endif*/
#include <Carbon/Carbon.h>

//**********	Project Headers			****************************************
#include "MoreAppleEvents.h"
#include "MoreAEObjects.h"
#include "MoreProcesses.h"
#include "MoreMemory.h"

//**********	Private Definitions		****************************************
enum {
	kFinderFileType			= 'FNDR',
	kFinderCreatorType		= 'MACS',
	kFinderProcessType		= 'FNDR',
	kFinderProcessSignature	= 'MACS'
};
static AEIdleUPP gAEIdleUPP = nil;
//*******************************************************************************
#pragma mark ==> Create Target Descriptors for AEvents ¥
/********************************************************************************
	Create and return an AEDesc for the process target with the specified PSN.
	If no PSN is supplied the use the current process

	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	____________
*/
pascal OSErr MoreAECreateProcessTarget(ProcessSerialNumber* pPSN, AEDesc* pAppleEvent)
{
	ProcessSerialNumber self;

	if (!pPSN)
	{
		pPSN = &self;

		self.lowLongOfPSN		= kNoProcess;
		self.highLongOfPSN		= kCurrentProcess;
	}

	return AECreateDesc (typeProcessSerialNumber,pPSN,sizeof(*pPSN),pAppleEvent);
}	// MoreAECreateProcessTarget
//*******************************************************************************
#pragma mark ==> Create AEvents ¥
/********************************************************************************
	Create and return an AppleEvent of the given class and ID. The event will be
	targeted at the current process, with an AEAddressDesc of type
	typeProcessSerialNumber.

	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	____________
*/
pascal OSErr MoreAECreateAppleEventSelfTarget(
					AEEventClass pAEEventClass,
					AEEventID pAEEventID,
					AppleEvent* pAppleEvent)
{
	OSErr	anErr = noErr;
	
	ProcessSerialNumber		selfPSN = {0, kCurrentProcess};
	
	anErr = MoreAECreateAppleEventProcessTarget( &selfPSN, pAEEventClass, pAEEventID, pAppleEvent );
	
	return ( anErr );
}//end MoreAECreateAppleEventSelfTarget

/********************************************************************************
	Create and return an AppleEvent of the given class and ID. The event will be
	targeted at the process specified by the target type and creator codes, 
	with an AEAddressDesc of type typeProcessSerialNumber.

	pType			==>		The file type of the process to be found.
	pCreator		==>		The creator type of the process to be found.
	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	procNotFound	Ð600	No eligible process with specified descriptor
	____________
*/
pascal	OSErr	MoreAECreateAppleEventSignatureTarget(
						OSType pType,
						OSType pCreator,
						AEEventClass pAEEventClass,
						AEEventID pAEEventID,
						AppleEvent* pAppleEvent )
{
	OSErr	anErr = noErr;
	
	ProcessSerialNumber		psn = {kNoProcess, kNoProcess};
	
	// <12> bug fix, pCreator & pType parameters swapped.
	anErr = MoreProcFindProcessBySignature( pCreator, pType, &psn );
	if ( noErr == anErr )
	{
		anErr = MoreAECreateAppleEventProcessTarget( &psn, pAEEventClass, pAEEventID, pAppleEvent );
	}
	return anErr;
}//end MoreAECreateAppleEventSignatureTarget

/********************************************************************************
	Create and return an AppleEvent of the given class and ID. The event will be
	targeted at the application with the specific creator.

	psnPtr			==>		Pointer to the PSN to target the event with.
	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	procNotFound	Ð600	No eligible process with specified descriptor
	____________
*/
pascal OSStatus MoreAECreateAppleEventCreatorTarget(
							const AEEventClass pAEEventClass,
							const AEEventID pAEEventID,
							const OSType pCreator,
							AppleEvent* pAppleEvent)
{
	OSStatus 	anErr;
	AEDesc 		targetDesc;
	
	MoreAssertQ(pAppleEvent != nil);

	MoreAENullDesc(&targetDesc);
	
	anErr = AECreateDesc(typeApplSignature, &pCreator, sizeof(pCreator), &targetDesc);
	if (noErr == anErr)
		anErr = AECreateAppleEvent(pAEEventClass, pAEEventID, &targetDesc, 
									kAutoGenerateReturnID, kAnyTransactionID, pAppleEvent);
	MoreAEDisposeDesc(&targetDesc);
	
	return anErr;
}//end MoreAECreateAppleEventCreatorTarget

/********************************************************************************
	Create and return an AppleEvent of the given class and ID. The event will be
	targeted with the provided PSN.

	psnPtr			==>		Pointer to the PSN to target the event with.
	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	procNotFound	Ð600	No eligible process with specified descriptor
	____________
*/
pascal	OSErr	MoreAECreateAppleEventProcessTarget(
						const ProcessSerialNumberPtr psnPtr,
						AEEventClass pAEEventClass,
						AEEventID pAEEventID,
						AppleEvent* pAppleEvent )
{
	OSErr	anErr = noErr;
	AEDesc	targetAppDesc = {typeNull,nil};
	
	anErr = AECreateDesc (typeProcessSerialNumber, psnPtr, sizeof( ProcessSerialNumber ), &targetAppDesc);

	if ( noErr == anErr )
	{
		anErr = AECreateAppleEvent( pAEEventClass, pAEEventID, &targetAppDesc,
									kAutoGenerateReturnID, kAnyTransactionID, pAppleEvent);
	}
	
	MoreAEDisposeDesc( &targetAppDesc );
	
	return anErr;
}//end MoreAECreateAppleEventProcessTarget

/********************************************************************************
	Create and return an AppleEvent of the given class and ID. The event will be
	targeted with the provided TargetID.

	pTargetID		==>		Pointer to the TargetID to target the event with.
	pAEEventClass	==>		The class of the event to be created.
	pAEEventID		==>		The ID of the event to be created.
	pAppleEvent		==>		Pointer to an AppleEvent where the
							event record will be returned.
					<==		The Apple event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	memFullErr		-108	Not enough room in heap zone	
	procNotFound	Ð600	No eligible process with specified descriptor
	____________
*/
#if CALL_NOT_IN_CARBON

// See comment in header file.

pascal	OSErr	MoreAECreateAppleEventTargetID(
					const TargetID* pTargetID,
			  		AEEventClass pAEEventClass,
			  		AEEventID pAEEventID,
					AppleEvent* pAppleEvent )
{
	OSErr	anErr = noErr;
	AEDesc	targetAppDesc = {typeNull,nil};
	
	anErr = AECreateDesc (typeTargetID, pTargetID, sizeof( TargetID ), &targetAppDesc);

	if ( noErr == anErr )
	{
		anErr = AECreateAppleEvent( pAEEventClass, pAEEventID, &targetAppDesc,
									kAutoGenerateReturnID, kAnyTransactionID, pAppleEvent);
	}
	
	MoreAEDisposeDesc( &targetAppDesc );
	
	return anErr;
}//end MoreAECreateAppleEventTargetID

#endif

#pragma mark ==> Send AppleEvents ¥
#if 0
//¥	De-appreciated! Don't use! Use one of the more specific routines (w/idle proc) below.
pascal OSErr MoreAESendAppleEvent (const AppleEvent* pAppleEvent, AppleEvent* pAEReply)
{
	OSErr anErr = noErr;

	AESendMode aeSendMode = kAEAlwaysInteract | kAECanSwitchLayer;

	if (pAEReply)
	{
		aeSendMode |= kAEWaitReply;

		MoreAENullDesc(pAEReply);
	}

	anErr = AESend (pAppleEvent, pAEReply, aeSendMode, kAENormalPriority, kAEDefaultTimeout, nil, nil);

	return anErr;
}//end MoreAESendAppleEvent
#endif 0

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Will wait for a reply if an idle function is provided, but no result will be returned.

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	
	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal	OSErr	MoreAESendEventNoReturnValue(
					const AEIdleUPP pIdleProcUPP,
					const AppleEvent* pAppleEvent )
{
	OSErr		anErr = noErr;
	AppleEvent	theReply = {typeNull,nil};
	AESendMode	sendMode;

	if (nil == pIdleProcUPP)
		sendMode = kAENoReply;
	else
		sendMode = kAEWaitReply;

	anErr = AESend( pAppleEvent, &theReply, sendMode, kAENormalPriority, kNoTimeOut, pIdleProcUPP, nil );
	if ((noErr == anErr) && (kAEWaitReply == sendMode))
		anErr = MoreAEGetHandlerError(&theReply);

	MoreAEDisposeDesc( &theReply );
	
	return anErr;
}//end MoreAESendEventNoReturnValue

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Return the direct object as a AEDesc of pAEDescType

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	pDescType		==>		The type of value returned by the event.
	pAEDescList		<==		The value returned by the event.

	RESULT CODES
	____________
	noErr			   0	No error	
	paramErr		 -50	No idle function provided

	and any other error that can be returned by AESend
	or the handler in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendEventReturnAEDesc(
						const AEIdleUPP		pIdleProcUPP,
						const AppleEvent	*pAppleEvent,
						const DescType		pDescType,
						AEDesc				*pAEDesc)
{
	OSErr anErr = noErr;

	//	No idle function is an error, since we are expected to return a value
	if (pIdleProcUPP == nil)
		anErr = paramErr;
	else
	{
		AppleEvent theReply = {typeNull,nil};
		AESendMode sendMode = kAEWaitReply;

		anErr = AESend(pAppleEvent, &theReply, sendMode, kAENormalPriority, kNoTimeOut, pIdleProcUPP, nil);
		//	[ Don't dispose of the event, it's not ours ]
		if (noErr == anErr)
		{
			anErr = MoreAEGetHandlerError(&theReply);

			if (!anErr && theReply.descriptorType != typeNull)
			{
				anErr = AEGetParamDesc(&theReply, keyDirectObject, pDescType, pAEDesc);
			}
			MoreAEDisposeDesc(&theReply);
		}
	}
	return anErr;
}	// MoreAESendEventReturnAEDesc

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Return the direct object as a AEDescList

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	pAEDescList		<==		The value returned by the event.

	RESULT CODES
	____________
	noErr			   0	No error	
	paramErr		 -50	No idle function provided

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendEventReturnAEDescList(
					const AEIdleUPP pIdleProcUPP,
					const AppleEvent* pAppleEvent,
					AEDescList* pAEDescList)
{
	OSErr anErr = noErr;

	//	No idle function is an error, since we are expected to return a value
	if (pIdleProcUPP == nil)
		anErr = paramErr;
	else
	{
		AppleEvent theReply = {typeNull,nil};
		AESendMode sendMode = kAEWaitReply;

		anErr = AESend(pAppleEvent, &theReply, sendMode, kAENormalPriority, kNoTimeOut, pIdleProcUPP, nil);
		//	[ Don't dispose of the event, it's not ours ]
		if (noErr == anErr)
		{
			anErr = MoreAEGetHandlerError(&theReply);

			if (!anErr && theReply.descriptorType != typeNull)
			{
				anErr = AEGetParamDesc(&theReply, keyDirectObject, typeAEList, pAEDescList);
			}
			MoreAEDisposeDesc(&theReply);
		}
	}
	return anErr;
}	// MoreAESendEventReturnAEDescList

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Return data (at pDataPtr) of type pDesiredType

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	theValue		<==		The value returned by the event.

	RESULT CODES
	____________
	noErr			   0	No error	
	paramErr		 -50	No idle function provided

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendEventReturnData(
						const AEIdleUPP		pIdleProcUPP,
						const AppleEvent	*pAppleEvent,
						DescType			pDesiredType,
						DescType*			pActualType,
						void*		 		pDataPtr,
						Size				pMaximumSize,
						Size 				*pActualSize)
{
	OSErr anErr = noErr;

	//	No idle function is an error, since we are expected to return a value
	if (pIdleProcUPP == nil)
		anErr = paramErr;
	else
	{
		AppleEvent theReply = {typeNull,nil};
		AESendMode sendMode = kAEWaitReply;

		anErr = AESend(pAppleEvent, &theReply, sendMode, kAENormalPriority, kNoTimeOut, pIdleProcUPP, nil);
		//	[ Don't dispose of the event, it's not ours ]
		if (noErr == anErr)
		{
			anErr = MoreAEGetHandlerError(&theReply);

			if (!anErr && theReply.descriptorType != typeNull)
			{
				anErr = AEGetParamPtr(&theReply, keyDirectObject, pDesiredType,
							pActualType, pDataPtr, pMaximumSize, pActualSize);
			}
			MoreAEDisposeDesc(&theReply);
		}
	}
	return anErr;
}	// MoreAESendEventReturnData

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Return a SInt16 (typeSmallInteger).

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	theValue		<==		The value returned by the event.
	
	RESULT CODES
	____________
	noErr			   0	No error	
	paramErr		 -50	No idle function provided
	
	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendEventReturnSInt16(
					const AEIdleUPP pIdleProcUPP,
					const AppleEvent* pAppleEvent,
					SInt16* pValue)
{
	DescType			actualType;
	Size 				actualSize;

	return MoreAESendEventReturnData(pIdleProcUPP,pAppleEvent,typeSInt16,
				&actualType,pValue,sizeof(SInt16),&actualSize);
}	// MoreAESendEventReturnSInt16

/********************************************************************************
	Send the provided AppleEvent using the provided idle function.
	Returns a PString.

	pIdleProcUPP	==>		The idle function to use when sending the event.
	pAppleEvent		==>		The event to be sent.
	pStr255			<==		The value returned by the event.

	RESULT CODES
	____________
	noErr			   0	No error	
	paramErr		 -50	No idle function provided

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/

pascal OSErr MoreAESendEventReturnPString(
					const AEIdleUPP pIdleProcUPP,
					const AppleEvent* pAppleEvent,
					Str255 pStr255)
{
	DescType			actualType;
	Size 				actualSize;
	OSErr				anErr;

	anErr = MoreAESendEventReturnData(pIdleProcUPP,pAppleEvent,typePString,
				&actualType,pStr255,sizeof(Str255),&actualSize);

	if (errAECoercionFail == anErr)
	{
		anErr =  MoreAESendEventReturnData(pIdleProcUPP,pAppleEvent,typeChar,
			&actualType,(Ptr) &pStr255[1],sizeof(Str255),&actualSize);
		if (actualSize < 256)
			pStr255[0] = actualSize;
		else
			anErr = errAECoercionFail;
	}
	return anErr;
}	// MoreAESendEventReturnPString
#pragma mark ==> Functions for talking to ourselfs

/********************************************************************************
	Send an AppleEvent of the specified Class & ID to myself using the 
	default idle function.

	pEventID	==>		The event to be sent.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendToSelfNoReturnValue(
					const AEEventClass pEventClass,
					const AEEventID pEventID)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(pEventClass,pEventID,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pSelection, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);		//	Always dispose of objects as soon as you are done (helps avoid leaks)
			if (noErr == anErr)
				anErr = MoreAESendEventNoReturnValue(gAEIdleUPP, &tAppleEvent);
		}
		MoreAEDisposeDesc(&tAppleEvent);			// always dispose of AEDescs when you are finished with them
	}
	return anErr;
}	// MoreAESendToSelfNoReturnValue

/********************************************************************************
	Send an AppleEvent of the specified Class & ID to myself using the 
	default idle function. Wait for a reply and extract a SInt16 result.

	pEventClass		==>		The event class to be sent.
	pEventID		==>		The event ID to be sent.
	pValue			<==		Where the return SInt16 will be stored.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAESendToSelfReturnSInt16(
					const AEEventClass pEventClass,
					const AEEventID pEventID,
					SInt16* pValue)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(pEventClass,pEventID,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pSelection, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);	//	Always dispose of objects as soon as you are done (helps avoid leaks)
			if (noErr == anErr)
				anErr = MoreAESendEventReturnSInt16(gAEIdleUPP, &tAppleEvent, pValue);
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	return anErr;
}//end MoreAESendToSelfReturnSInt16

/********************************************************************************
	Send a get data (kAEGetData) AppleEvent to myself using the 
	default idle function. Wait for a reply and extract a SInt16 result.

	pPropType		==>		The property type.
	pValue			<==		Where the resulting SInt16 will be stored.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAETellSelfToGetSInt16Property(const DescType pPropType,SInt16* pValue)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(kAECoreSuite,kAEGetData,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pPropType, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);	//	Always dispose of objects as soon as you are done (helps avoid leaks)
			if (noErr == anErr)
				anErr = MoreAESendEventReturnSInt16(gAEIdleUPP, &tAppleEvent, pValue);
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	return anErr;
}//end MoreAETellSelfToGetSInt16Property

/********************************************************************************
	Send a get data (kAEGetData) AppleEvent to myself using the 
	default idle function. Wait for a reply and extract a Str255 result.

	pPropType		==>		The property type.
	pValue			<==		Where the resulting Str255 will be stored.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAETellSelfToGetStr255Property(const DescType pPropType,Str255 pValue)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(kAECoreSuite,kAEGetData,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pPropType, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);	//	Always dispose of objects as soon as you are done (helps avoid leaks)
			if (noErr == anErr)
				anErr = MoreAESendEventReturnPString(gAEIdleUPP, &tAppleEvent, pValue);
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	return anErr;
}//end MoreAETellSelfToGetStr255Property

/********************************************************************************
	Send a set data (kAESetData) AppleEvent to myself with a SInt16 parameter
	and using the default idle function.

	pPropType		==>		The property type.
	pValue			==>		The SInt16 value to be set.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAETellSelfToSetSInt16Property(const DescType pPropType,SInt16 pValue)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(kAECoreSuite,kAESetData,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pPropType, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);	//	Always dispose of objects as soon as you are done (helps avoid leaks)
			if (noErr == anErr)
			{
				anErr = AEPutParamPtr(&tAppleEvent, keyAEData, typeSInt16, &pValue, sizeof(SInt16));
				if (noErr == anErr)
					anErr = MoreAESendEventNoReturnValue(gAEIdleUPP, &tAppleEvent);
			}
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	return anErr;
}//end MoreAETellSelfToSetSInt16Property

/********************************************************************************
	Send a set data (kAESetData) AppleEvent to myself with a Pascal string
	parameter and using the default idle function.

	pEventID			==>		The event to be sent.
	pValue				==>		The Str255 to be sent.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAETellSelfToSetStr255Property(const DescType pPropType,Str255 pValue)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	OSErr anErr = noErr;

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(kAECoreSuite,kAESetData,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pPropType, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);
			if (noErr == anErr)
			{
				anErr = AEPutParamPtr(&tAppleEvent, keyAEData, typePString, pValue, pValue[0] + 1);
				if (noErr == anErr)
					anErr = MoreAESendEventNoReturnValue(gAEIdleUPP, &tAppleEvent);
			}
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	return anErr;
} // MoreAETellSelfToSetStr255Property

/********************************************************************************
	Send a set data (kAESetData) AppleEvent to myself with a CFStringRef
	parameter and using the default idle function.

	pEventID			==>		The event to be sent.
	pValue				==>		The CFString to be sent.

	RESULT CODES
	____________
	noErr			   0	No error	

	and any other error that can be returned by AESend or the handler
	in the application that gets the event.
	____________
*/
pascal OSErr MoreAETellSelfToSetCFStringRefProperty(
				const DescType pPropType,
				const CFStringRef pCFStringRef)
{
	AppleEvent tAppleEvent = {typeNull,nil};	//	If you always init AEDescs, it's always safe to dispose of them.
	CFIndex length = CFStringGetLength(pCFStringRef);
    const UniChar* dataPtr = CFStringGetCharactersPtr(pCFStringRef);
	const UniChar* tempPtr = nil;
	OSErr anErr = noErr;

    if (dataPtr == NULL)
	{
		tempPtr = (UniChar*) NewPtr(length * sizeof(UniChar));

		if (nil == tempPtr) return memFullErr;

		CFStringGetCharacters(pCFStringRef, CFRangeMake(0,length), (UniChar*) tempPtr);
		dataPtr = tempPtr;
	}

	if (nil == gAEIdleUPP)
		gAEIdleUPP = NewAEIdleUPP(MoreAESimpleIdleFunction);

	anErr = MoreAECreateAppleEventSelfTarget(kAECoreSuite,kAESetData,&tAppleEvent);
	if (noErr == anErr)
	{
		AEDesc containerObj = {typeNull,nil};	// start with the null (application) container
		AEDesc propertyObject = {typeNull,nil};

		anErr = MoreAEOCreatePropertyObject(pPropType, &containerObj, &propertyObject);
		if (noErr == anErr)
		{
			anErr = AEPutParamDesc(&tAppleEvent, keyDirectObject, &propertyObject);
			MoreAEDisposeDesc(&propertyObject);
			if (noErr == anErr)
			{
				anErr = AEPutParamPtr(&tAppleEvent, keyAEData, typeUnicodeText, dataPtr, length * sizeof(UniChar));
				if (noErr == anErr)
					anErr = MoreAESendEventNoReturnValue(gAEIdleUPP, &tAppleEvent);
			}
		}
		MoreAEDisposeDesc(&tAppleEvent);	// always dispose of AEDescs when you are finished with them
	}
	if (nil != tempPtr)
		DisposePtr((Ptr) tempPtr);

	return anErr;
}	// MoreAETellSelfToSetCFStringRefProperty
//*******************************************************************************
#pragma mark ==> Misc. AE utility functions ¥
//*******************************************************************************
// Appends each of the items in pSourceList to the pDestList.
pascal OSStatus MoreAEAppendListToList(const AEDescList* pSourceList, AEDescList* pDestList)
{
	OSStatus  anErr;
	AEKeyword junkKeyword;
	long      listCount;
	SInt32    listIndex;
	AEDesc	  thisValue;

	MoreAssertQ(pSourceList != nil);
	MoreAssertQ(pDestList   != nil);

	anErr = AECountItems(pSourceList, &listCount);
	if (noErr == anErr) {
		for (listIndex = 1; listIndex <= listCount; listIndex++) {
			MoreAENullDesc(&thisValue);
			
			anErr = AEGetNthDesc(pSourceList, listIndex, typeWildCard, &junkKeyword, &thisValue);
			if (noErr == anErr) {
				anErr = AEPutDesc(pDestList, 0, &thisValue);
			}
			
			MoreAEDisposeDesc(&thisValue);
			if (noErr != anErr) {
				break;
			}
		}
	}

	return anErr;
}//end MoreAEAppendListToList

//*******************************************************************************
// This routine takes a result descriptor and an error.
// If there is a result to add to the reply it makes sure the reply isn't
// NULL itself then adds the error to the reply depending on the error
// and the type of result.
pascal OSErr MoreAEMoreAESetReplyErrorNumber (OSErr pOSErr, AppleEvent* pAEReply)
{
	OSErr anErr = noErr;

	if (pAEReply->dataHandle)
	{
		if (!MoreAssert (pAEReply->descriptorType == typeAppleEvent))
			anErr = paramErr;
		else
			anErr = AEPutParamPtr (pAEReply,keyErrorNumber,typeSInt16,&pOSErr,sizeof(pOSErr));
	}

	return anErr;
}//end MoreAEMoreAESetReplyErrorNumber
//*******************************************************************************
// This routine takes a result descriptor, a reply descriptor and an error.
// If there is a result to add to the reply it makes sure the reply isn't
// NULL itself then adds the result to the reply depending on the error
// and the type of result.

pascal OSErr MoreAEAddResultToReply(const AEDesc* pResult, AEDesc* pAEReply, const OSErr pError)
{
	OSErr	anErr;

	// Check that the pAEReply is not NULL and there is a result to put in it  
	if (typeNull == pAEReply->descriptorType || typeNull == pResult->descriptorType)
		return (pError);
	
	if (noErr == pError)
		anErr = AEPutParamDesc(pAEReply, keyDirectObject, pResult);
	else
	{
		switch (pResult->descriptorType)
		{
			case typeSInt32:
				anErr = AEPutParamDesc(pAEReply, keyErrorNumber, pResult);
				break;
				
			case typeChar:
				anErr = AEPutParamDesc(pAEReply, keyErrorString, pResult);
				break;
				
			default:
				anErr = errAETypeError;
		}
		
		if (noErr == anErr)
			anErr = pError;		// Don't loose that error
	}
	return (anErr);
}//end MoreAEAddResultToReply
//*******************************************************************************
//	Name:		MoreAEGotRequiredParams
//	Function:	Checks that all parameters defined as 'required' have been read

pascal OSErr	MoreAEGotRequiredParams(const AppleEvent* pAppleEventPtr)
{
	DescType	returnedType;
	Size		actualSize;
	OSErr		anErr;
	
	// look for the keyMissedKeywordAttr, just to see if it's there
	
	anErr = AEGetAttributePtr(pAppleEventPtr, keyMissedKeywordAttr, typeWildCard,
												&returnedType, NULL, 0, &actualSize);
	
	switch (anErr)
	{
		case errAEDescNotFound:		// attribute not there means we
			anErr = noErr;			// got all required parameters.
			break;
			
		case noErr:					// attribute there means missed
			anErr = errAEParamMissed;	// at least one parameter.
			break;
			
		// default:		pass on unexpected error in looking for the attribute
	}
	return (anErr);
} // GotReqiredParams

/********************************************************************************
	Takes a reply event checks it for any errors that may have been returned
	by the event handler. A simple function, in that it only returns the error
	number. You can often also extract an error string and three other error
	parameters from a reply event.
	
	Also see:
		IM:IAC for details about returned error strings.
		AppleScript developer release notes for info on the other error parameters.
	
	pAEReply	==>		The reply event to be checked.

	RESULT CODES
	____________
	noErr				    0	No error	
	????					??	Pretty much any error, depending on what the
								event handler returns for it's errors.
*/
pascal	OSErr	MoreAEGetHandlerError(const AppleEvent* pAEReply)
{
	OSErr		anErr = noErr;
	OSErr		handlerErr;
	
	DescType	actualType;
	long		actualSize;
	
	if ( pAEReply->descriptorType != typeNull )	// there's a reply, so there may be an error
	{
		OSErr	getErrErr = noErr;
		
		getErrErr = AEGetParamPtr( pAEReply, keyErrorNumber, typeSInt16, &actualType,
									&handlerErr, sizeof( OSErr ), &actualSize );
		
		if ( getErrErr != errAEDescNotFound )	// found an errorNumber parameter
		{
			anErr = handlerErr;					// so return it's value
		}
	}
	return anErr;
}//end MoreAEGetHandlerError

/********************************************************************************
	Get the class and ID from an AppleEvent.

	pAppleEvent			==>		The event to get the class and ID from.
	pAEEventClass	   output:	The event's class.
	pAEEventID		   output:	The event's ID.
	
	RESULT CODES
	____________
	noErr					    0	No error	
	memFullErr				 -108	Not enough room in heap zone	
	errAEDescNotFound 		-1701	Descriptor record was not found	
	errAENotAEDesc			-1704	Not a valid descriptor record	
	errAEReplyNotArrived	-1718	Reply has not yet arrived	
*/	
pascal OSErr MoreAEExtractClassAndID(
					const AppleEvent* pAppleEvent,
					AEEventClass* pAEEventClass,
					AEEventID* pAEEventID )
{
	DescType	actualType;
	Size		actualSize;
	OSErr		anErr;

	anErr = AEGetAttributePtr( pAppleEvent, keyEventClassAttr, typeType, &actualType,
								pAEEventClass, sizeof( pAEEventClass ), &actualSize );
	if ( noErr == anErr )
	{
		anErr = AEGetAttributePtr( pAppleEvent, keyEventIDAttr, typeType, &actualType,
									pAEEventID, sizeof( pAEEventID ), &actualSize );
	}
	return ( anErr );
}//end ExtractClassAndID

/********************************************************************************
	A very simple idle function. It simply ignors any event it receives,
	returns 30 for the sleep time and nil for the mouse region.
	
	Your application should supply an idle function that handles any events it
	might receive. This is especially important if your application has any windows.
	
	Also see:
		IM:IAC for details about idle functions.
		Pending Update Perils technote for more about handling low-level events.
*/	
pascal	Boolean	MoreAESimpleIdleFunction(
	EventRecord* event,
	long* sleepTime,
	RgnHandle* mouseRgn )
{
#pragma unused( event )
	*sleepTime = 30;
	*mouseRgn = nil;
	
	return ( false );
}//end MoreAESimpleIdleFunction

/********************************************************************************
	Is the Apple Event Manager present.
	
	RESULT CODES
	____________
	true	The Apple Event Manager is present
	false	It isn't
*/
pascal Boolean MoreAEHasAppleEvents(void)
{
	static	long		gHasAppleEvents = kFlagNotSet;
	
	if ( gHasAppleEvents == kFlagNotSet )
	{
		SInt32	response;
		
		if ( Gestalt( gestaltAppleEventsAttr, &response ) == noErr )
			gHasAppleEvents = ( response & (1L << gestaltAppleEventsPresent) ) != 0;
	}
	return gHasAppleEvents;
}//end MoreAEHasAppleEvents
//*******************************************************************************
// Did this AppleEvent come from the Finder?
pascal OSErr MoreAEIsSenderFinder (const AppleEvent* pAppleEvent, Boolean* pIsFinder)
{
	OSErr					anErr = noErr;
	DescType				actualType;
	ProcessSerialNumber		senderPSN;
	Size					actualSize;

	if (!MoreAssert (pAppleEvent && pIsFinder))							return paramErr;
	if (!MoreAssert (pAppleEvent->descriptorType == typeAppleEvent))	return paramErr;
	if (!MoreAssert (pAppleEvent->dataHandle))							return paramErr;

	anErr = AEGetAttributePtr (pAppleEvent, keyAddressAttr, typeProcessSerialNumber, &actualType,
		(Ptr) &senderPSN, sizeof (senderPSN), &actualSize);
	if (MoreAssert (noErr == anErr))
	{
		if (!MoreAssert (actualType == typeProcessSerialNumber))
			anErr = paramErr;
		else if (!MoreAssert (actualSize == sizeof (senderPSN)))
			anErr = paramErr;
		else
		{
			ProcessInfoRec processInfo;

			if (!(anErr = MoreProcGetProcessInformation (&senderPSN,&processInfo)))
			{
				*pIsFinder = (	processInfo.processSignature == kFinderProcessSignature && 
								processInfo.processType == kFinderProcessType);
			}
		}		
	}

	return anErr;
}//end MoreAEIsSenderFinder
#pragma mark ==> AEDesc Constructor & Destructor ¥
//*******************************************************************************
// Initialises desc to the null descriptor (typeNull, nil).
pascal void MoreAENullDesc(AEDesc* desc)
{
	MoreAssertQ(desc != nil);

	desc->descriptorType = typeNull;
	desc->dataHandle     = nil;
}//end MoreAENullDesc

//*******************************************************************************
// Disposes of desc and initialises it to the null descriptor.
pascal void MoreAEDisposeDesc(AEDesc* desc)
{
	OSStatus junk;

	MoreAssertQ(desc != nil);
	
	junk = AEDisposeDesc(desc);
	MoreAssertQ(junk == noErr);

	MoreAENullDesc(desc);
}//end MoreAEDisposeDesc

//*******************************************************************************
#pragma mark ==> AEDesc Data Accessors for 68K ¥
//*******************************************************************************
// These routines are only implemented in PreCarbon.o for PowerPC
// So for 68K we had to write our own versions

#if (!ACCESSOR_CALLS_ARE_FUNCTIONS && TARGET_CPU_68K)

pascal Size AEGetDescDataSize(	const AEDesc* pAEDesc)
{
	return GetHandleSize(pAEDesc->dataHandle);
}//end AEGetDescDataSize

pascal OSErr AEGetDescData(	const AEDesc*	pAEDesc,
				void*			pDataPtr,
				Size 			pMaxSize)
{
	Size copySize = AEGetDescDataSize(pAEDesc);

	if ((nil == pAEDesc) || (nil == pDataPtr))
		return paramErr;

	if (pMaxSize < copySize)
		copySize = pMaxSize;

	BlockMoveData(*pAEDesc->dataHandle,pDataPtr,copySize);

	return noErr;
}//end AEGetDescData
#endif (!ACCESSOR_CALLS_ARE_FUNCTIONS && TARGET_CPU_68K)
//*******************************************************************************
#pragma mark ==> Get Data From Descriptors ¥

/********************************************************************************
	This is the generic routine that all the other's use instead of
	duplicating all this code unnecessarily.

	pAEDesc			==>		The descriptor we want the data from
	pDestPtr		<==		Where we want to store the data from this desc.
	pMaxSize		==>		The maxium amount of data we can store.
	pActualSize		<==		The actual amount of data stored.

	RESULT CODES
	____________
	noErr			   0	No error	
	____________
*/
pascal void MoreAEGetRawDataFromDescriptor(
					const AEDesc* pAEDesc,
					Ptr     pDestPtr,
					Size    pMaxSize,
					Size*	pActualSize)
{
	Size copySize;

	if (pAEDesc->dataHandle) 
	{
		*pActualSize = AEGetDescDataSize(pAEDesc);

		if (*pActualSize < pMaxSize)
			copySize = *pActualSize;
		else
			copySize = pMaxSize;

		 AEGetDescData(pAEDesc,pDestPtr,copySize);
	}
	else
		*pActualSize = 0;
}//end MoreAEGetRawDataFromDescriptor

/********************************************************************************
	Extract a CFString from a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pCFStringRef	<==		Where we want to store the pascal string

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/
pascal OSErr MoreAEGetCFStringFromDescriptor(const AEDesc* pAEDesc, CFStringRef* pCFStringRef)
{
	//AEDesc      resultDesc = {typeNull, NULL};
	AEDesc		uniAEDesc = {typeNull, NULL};
	OSErr		anErr;

	if (nil == pCFStringRef)
		return paramErr;

	anErr = AECoerceDesc(pAEDesc, typeUnicodeText, &uniAEDesc);
	if (noErr == anErr)
	{
		if (typeUnicodeText == uniAEDesc.descriptorType)
		{
	        Size bufSize = AEGetDescDataSize(&uniAEDesc);
	        Ptr buf = NewPtr(bufSize);

	        if ((noErr == (anErr = MemError())) && (NULL != buf))
	        {
	            anErr = AEGetDescData(&uniAEDesc, buf, bufSize);
	            if (noErr == anErr)
	                *pCFStringRef = CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*) buf, bufSize / sizeof(UniChar));

	            DisposePtr(buf);
	        }
		}
		MoreAEDisposeDesc(&uniAEDesc);
	}
	return (anErr);
}//end MoreAEGetCFStringFromDescriptor

/********************************************************************************
	Extract a pascal string a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pStringPtr	<==		Where we want to store the pascal string

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/
pascal OSErr MoreAEGetPStringFromDescriptor(const AEDesc* pAEDesc, StringPtr pStringPtr)
{
	Size         stringSize;
	AEDesc       resultDesc = {typeNull, NULL};
	OSErr        anErr;
	
	anErr = AECoerceDesc(pAEDesc, typeChar, &resultDesc);
	if (noErr != anErr) goto done;
	
	pStringPtr[0] = 0;
	
	MoreAEGetRawDataFromDescriptor(&resultDesc, (Ptr) &pStringPtr[1], 255, &stringSize);
	if (stringSize <= 255) 
		pStringPtr[0] = stringSize;
	else
		anErr = errAECoercionFail;

done:		
	MoreAEDisposeDesc(&resultDesc);
		
	return(anErr);
}//end MoreAEGetPStringFromDescriptor

/********************************************************************************
	Extract a C string from the descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pStringPtr	<==		Where we want to store the C string

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/
pascal OSErr MoreAEGetCStringFromDescriptor(const AEDesc* pAEDesc, char* pCharPtr)
{
	Size         stringSize;
	AEDesc       resultDesc = {typeNull, NULL};
	OSErr        anErr;
	
	anErr = AECoerceDesc(pAEDesc, typeChar, &resultDesc);
	if (noErr == anErr)
	{
		MoreAEGetRawDataFromDescriptor(&resultDesc, pCharPtr, 1024, &stringSize);
		pCharPtr[stringSize] = 0;
	}
	MoreAEDisposeDesc(&resultDesc);
		
	return (anErr);
}//end MoreAEGetCStringFromDescriptor

/********************************************************************************
	Extract a short from a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pStringPtr	<==		Where we want to store the short.

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/

pascal OSErr MoreAEGetShortFromDescriptor(const AEDesc* pAEDesc, SInt16* pResult)
{
	OSErr   myErr;
	Size    intSize;
	AEDesc  resultDesc;

	*pResult = 0;

	myErr = AECoerceDesc(pAEDesc,typeSInt16,&resultDesc);

	if (myErr==noErr) 
	{
		MoreAEGetRawDataFromDescriptor(&resultDesc,(Ptr) pResult,2,&intSize);
		if (intSize>2) 
			myErr = errAECoercionFail;
	}

	MoreAEDisposeDesc(&resultDesc);

	return(myErr);
}//end MoreAEGetShortFromDescriptor

/********************************************************************************
	Extract a Boolean from a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pStringPtr	<==		Where we want to store the boolean.

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/

pascal OSErr MoreAEGetBooleanFromDescriptor(const AEDesc* pAEDesc,Boolean* pResult)
{
	AEDesc  resultDesc;
	Size    boolSize;
	OSErr   myErr;

	*pResult = false;
	myErr = AECoerceDesc(pAEDesc,typeBoolean,&resultDesc);

	if (myErr==noErr) 
	{
		MoreAEGetRawDataFromDescriptor(&resultDesc,(Ptr)pResult,
			sizeof(Boolean),&boolSize);
		if (sizeof(boolSize) > sizeof(Boolean)) 
			myErr = errAECoercionFail;
	}

	MoreAEDisposeDesc(&resultDesc);

	return(myErr);
}//end MoreAEGetBooleanFromDescriptor

/********************************************************************************
	Extract a long from a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pStringPtr	<==		Where we want to store the long.

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/

pascal OSErr MoreAEGetLongFromDescriptor(const AEDesc* pAEDesc, long* pResult)
{
	OSErr   myErr;
	Size    intSize;
	AEDesc  resultDesc;

	*pResult = 0;
	myErr = AECoerceDesc(pAEDesc,typeSInt32,&resultDesc);

	if (myErr==noErr) 
	{
		MoreAEGetRawDataFromDescriptor(&resultDesc,(Ptr)pResult,4,&intSize);
		if (intSize>4) 
			myErr = errAECoercionFail;
	}

	MoreAEDisposeDesc(&resultDesc);

	return(myErr);
} /*MoreAEGetLongIntFromDescriptor*/

/********************************************************************************
	Extract a OSType from a descriptor.

  pAEDesc		==>		The descriptor we want the data from
  pResult		<==		Where we want to store the OSType.

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/

pascal OSErr MoreAEGetOSTypeFromDescriptor(const AEDesc* pAEDesc,OSType* pResult)
{
	return (MoreAEGetLongFromDescriptor(pAEDesc,(long*) pResult));
}//end MoreAEGetOSTypeFromDescriptor

/********************************************************************************
	Creates an AEDesc from a CFString.

  pCFStringRef			==>			The CFString we want the data from
  pAEDesc		<==		Where we want to store the AEDesc

  RESULT CODES
  ____________
  noErr			   0	No error	
  ____________
*/
pascal OSErr MoreAECreateAEDescFromCFString(const CFStringRef pCFStringRef,AEDesc* pAEDesc)
{
	OSErr		anErr = paramErr;

	if ((nil != pCFStringRef) && (nil != pAEDesc))
	{
		CFIndex length = CFStringGetLength(pCFStringRef);
		Size	dataSize = length * sizeof(UniChar);
		UniChar* data = (UniChar*) NewPtr(dataSize);

		if (nil == data)
		{
			anErr = MemError();
			if (noErr == anErr)
				anErr = memFullErr;
		}
		else
		{
			CFStringGetCharacters(pCFStringRef,CFRangeMake(0,length),data);
			anErr = AECreateDesc(typeUnicodeText,data,dataSize,pAEDesc);
			DisposePtr((Ptr) data);
		}
	}
	return (anErr);
}//end MoreAECreateAEDescFromCFString

//*******************************************************************************
// This routine creates a new handle and puts the contents of the desc
// in that handle.  CarbonÕs opaque AEDescÕs means that we need this
// functionality a lot.
pascal OSStatus MoreAECopyDescriptorDataToHandle(const AEDesc* desc, Handle* descData)
{
	OSStatus anErr;
	OSStatus junk;
	
	MoreAssertQ(desc      != nil);
	MoreAssertQ(descData  != nil);
	MoreAssertQ(*descData == nil);
	
	*descData = NewHandle(AEGetDescDataSize(desc));
	anErr = MoreMemError(*descData);
	if (noErr == anErr) {
		HLock(*descData);
		junk = AEGetDescData(desc, **descData, AEGetDescDataSize(desc));
		MoreAssertQ(junk == noErr);
		HUnlock(*descData);
	}
	return anErr;
}//end MoreAECopyDescriptorDataToHandle

//*******************************************************************************
// This routine returns true if and only if desc is the "missing value" value.
pascal Boolean MoreAEIsMissingValue(const AEDesc* desc)
{
	DescType missing;
	
	return (desc->descriptorType == typeType)
			&& (AEGetDescDataSize(desc) == sizeof(missing))
			&& (AEGetDescData(desc, &missing, sizeof(missing)) == noErr)
			&& (missing == cMissingValue);
}//end MoreAEIsMissingValue

//*******************************************************************************
// This routine creates a descriptor that represents the missing value.
pascal OSStatus MoreAECreateMissingValue(AEDesc* desc)
{
	const static DescType missingValue = cMissingValue;
	
	return AECreateDesc(typeType, &missingValue, sizeof(missingValue), desc);
}//end MoreAECreateMissingValue
