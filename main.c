// =============================================================================
//	main.c for SymbolicLinker
//	Descended from Apple's SampleCMPlugIn source code
//	"SymbolicLinker" is Copyright 2003 Nick Zitzmann
//	"SampleCMPlugIn" is Copyright 2003 Apple Computer, Inc.
//	This project is not sponsored, promoted, or endorsed by Apple
// =============================================================================

// Uncomment ONE of these lines
//#define DEBUGSTR(s) DebugStr(s)
#define DEBUGSTR(s)

// =============================================================================

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <Carbon/Carbon.h>
/*#include <stdio.h>
#include <unistd.h>
#include "MoreFinderEvents.h"*/
#include "SymbolicLinker.h"

// =============================================================================
//	typedefs, structs, consts, enums, etc.
// =============================================================================

#define kSymbolicLinkerFactoryID	( CFUUIDGetConstantUUIDWithBytes( NULL,		\
                                                                   0xC5, 0x2C, 0x25, 0x66, 0x4F, 0xB5, 0x11, 0xD5,	\
                                                                   0xFF, 0x01, 0x00, 0x30, 0x65, 0xB3, 0x00, 0xBC))
// "C52C2566-4FB5-11D5-FF01-003065B300BC"

/*enum
{
    keyContextualMenuCommandID				=	'cmcd',
    keyContextualMenuSubmenu				=	'cmsb'
};*/

// The layout for an instance of SampleCMPlugInType.
typedef struct SampleCMPlugInType
{
    ContextualMenuInterfaceStruct	*cmInterface;
    CFUUIDRef						factoryID;
    UInt32							refCount;
} SampleCMPlugInType;

// =============================================================================
//	local function prototypes
// =============================================================================
/*static void MakeSymbolicLinkToDesktop(unsigned char *path);
static void MakeSymbolicLink(unsigned char *path);
inline CFBundleRef SLOurBundle(void);
inline OSStatus StandardAlertCF(AlertType inAlertType, CFStringRef inError, CFStringRef inExplanation, const AlertStdCFStringAlertParamRec *inAlertParam, SInt16 *outItemHit);*/

// =============================================================================
//	interface function prototypes
// =============================================================================
static HRESULT SLPlugInQueryInterface(void* thisInstance,REFIID iid,LPVOID* ppv);
static ULONG SLPlugInAddRef(void *thisInstance);
static ULONG SLPlugInRelease(void *thisInstance);

static OSStatus SLPlugInExamineContext(void* thisInstance,const AEDesc* inContext,AEDescList* outCommandPairs);
static OSStatus SLPlugInHandleSelection(void* thisInstance,AEDesc* inContext,SInt32 inCommandID);
static void SLPlugInPostMenuCleanup( void *thisInstance );

#pragma mark -

// =============================================================================
//	static (local) global variables
// =============================================================================

static SInt32	gNumCommandIDs = 0;

// =============================================================================
//	local function implementations
// =============================================================================

// -----------------------------------------------------------------------------
//	DeallocSLPlugInType
// -----------------------------------------------------------------------------
//	Utility function that deallocates the instance when
//	the refCount goes to zero.
//
static void DeallocSLPlugInType( SampleCMPlugInType* thisInstance )
{
    CFUUIDRef	theFactoryID = thisInstance->factoryID;

    DEBUGSTR("\p|DeallocSampleCMPlugInType-I-Debug;g");

    free( thisInstance );
    if ( theFactoryID )
    {
        CFPlugInRemoveInstanceForFactory( theFactoryID );
        CFRelease( theFactoryID );
    }
}

// -----------------------------------------------------------------------------
//	testInterfaceFtbl	definition
// -----------------------------------------------------------------------------
//	The TestInterface function table.
//
static ContextualMenuInterfaceStruct testInterfaceFtbl =
{
    // Required padding for COM
    NULL,

    // These three are the required COM functions
    SLPlugInQueryInterface,
    SLPlugInAddRef,
    SLPlugInRelease,

    // Interface implementation
    SLPlugInExamineContext,
    SLPlugInHandleSelection,
    SLPlugInPostMenuCleanup
};


// -----------------------------------------------------------------------------
//	AllocSLPlugInType
// -----------------------------------------------------------------------------
//	Utility function that allocates a new instance.
//
static SampleCMPlugInType* AllocSLPlugInType(CFUUIDRef		inFactoryID )
{
    // Allocate memory for the new instance.
    SampleCMPlugInType *theNewInstance;

    DEBUGSTR("\p|AllocSampleCMPlugInType-I-Debug;g");

    theNewInstance = (SampleCMPlugInType*) malloc(sizeof( SampleCMPlugInType ) );

    // Point to the function table
    theNewInstance->cmInterface = &testInterfaceFtbl;

    // Retain and keep an open instance refcount<
    // for each factory.
    theNewInstance->factoryID = CFRetain( inFactoryID );
    CFPlugInAddInstanceForFactory( inFactoryID );

    // This function returns the IUnknown interface
    // so set the refCount to one.
    theNewInstance->refCount = 1;
    return theNewInstance;
}

// -----------------------------------------------------------------------------
//	SLPlugInFactory
// -----------------------------------------------------------------------------
//	Implementation of the factory function for this type.
//
extern void* SLPlugInFactory(CFAllocatorRef allocator,CFUUIDRef typeID );
void* SLPlugInFactory(CFAllocatorRef allocator,CFUUIDRef typeID )
{
#pragma unused (allocator)

    DEBUGSTR("\p|SampleCMPlugInFactory-I-Debug;g");

    // If correct type is being requested, allocate an
    // instance of TestType and return the IUnknown interface.
    if ( CFEqual( typeID, kContextualMenuTypeID ) )
    {
        SampleCMPlugInType *result;
        result = AllocSLPlugInType(kSymbolicLinkerFactoryID);
        return result;
    }
    else
    {
        // If the requested type is incorrect, return NULL.
        return NULL;
    }
}

// -----------------------------------------------------------------------------
//	AddCommandToAEDescList
// -----------------------------------------------------------------------------
static OSStatus AddCommandToAEDescList(CFStringRef	inCommandCFStringRef,
                                       SInt32		inCommandID,
                                       AEDescList*	ioCommandList)
{
    OSStatus anErr = noErr;
    AERecord theCommandRecord = { typeNull, NULL };
    /*CFStringRef tCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault,NULL,
                                                        CFSTR("%@ - %ld (0x%.8lX)"),inCommandCFStringRef,inCommandID,inCommandID);
    CFIndex length = CFStringGetLength(tCFStringRef);
    const UniChar* dataPtr = CFStringGetCharactersPtr(tCFStringRef);*/
    CFIndex length = CFStringGetLength(inCommandCFStringRef);
    const UniChar* dataPtr = CFStringGetCharactersPtr(inCommandCFStringRef);
    const UniChar* tempPtr = nil;

    //	printf("\nSampleCMPlugIn->AddCommandToAEDescList: Trying to add an item." );

    if (dataPtr == NULL)
    {
        tempPtr = (UniChar*) NewPtr(length * sizeof(UniChar));
        if (nil == tempPtr)
            goto AddCommandToAEDescList_fail;

        //CFStringGetCharacters(tCFStringRef, CFRangeMake(0,length), (UniChar*) tempPtr);
        CFStringGetCharacters(inCommandCFStringRef, CFRangeMake(0,length), (UniChar*) tempPtr);
        dataPtr = tempPtr;
    }
    if (nil == dataPtr)
        goto AddCommandToAEDescList_fail;

    // create an apple event record for our command
    anErr = AECreateList( NULL, 0, true, &theCommandRecord );
    require_noerr( anErr, AddCommandToAEDescList_fail );

    // stick the command text into the aerecord
    anErr = AEPutKeyPtr( &theCommandRecord, keyAEName, typeUnicodeText, dataPtr, length * sizeof(UniChar));
    require_noerr( anErr, AddCommandToAEDescList_fail );

    // stick the command ID into the AERecord
    anErr = AEPutKeyPtr( &theCommandRecord, keyContextualMenuCommandID,
                         typeSInt32, &inCommandID, sizeof( inCommandID ) );
    require_noerr( anErr, AddCommandToAEDescList_fail );

    // stick this record into the list of commands that we are
    // passing back to the CMM
    anErr = AEPutDesc(ioCommandList, 			// the list we're putting our command into
                      0, 						// stick this command onto the end of our list
                      &theCommandRecord );	// the command I'm putting into the list

AddCommandToAEDescList_fail:
        // clean up after ourself; dispose of the AERecord
        AEDisposeDesc( &theCommandRecord );

    if (nil != tempPtr)
        DisposePtr((Ptr) tempPtr);

    return anErr;

} // AddCommandToAEDescList

// -----------------------------------------------------------------------------
//	HandleFileSubmenu
// -----------------------------------------------------------------------------
static OSStatus HandleFileSubmenu(const AEDesc* inContext,const SInt32 inCommandID)
{
    long	index,numItems;
    OSStatus	anErr = noErr;

    //printf("\nSymbolicLinker->HandleFileSubmenu(inContext: 0x%lx, inCommandID: 0x%lx)",(SInt32) inContext,(SInt32) inCommandID );

    anErr = AECountItems(inContext,&numItems);
    if (noErr == anErr)
    {
        for (index = 1;index <= numItems;index++)
        {
            AEKeyword	theAEKeyword;
            AEDesc		theAEDesc;

            anErr = AEGetNthDesc(inContext,index,typeAlias,&theAEKeyword,&theAEDesc);
            if (noErr != anErr) continue;

            if (theAEDesc.descriptorType == typeAlias)
            {
                FSRef tFSRef;
                Size dataSize = AEGetDescDataSize(&theAEDesc);
                AliasHandle tAliasHdl = (AliasHandle) NewHandle(dataSize);

                if (nil != tAliasHdl)
                {
                    Boolean wasChanged;

                    anErr = AEGetDescData(&theAEDesc,*tAliasHdl,dataSize);
                    if (noErr == anErr)
                    {
                        anErr = FSResolveAlias(NULL,tAliasHdl,&tFSRef,&wasChanged);
                        if (noErr == anErr)
                        {
							CFURLRef tURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &tFSRef);
							
							// At this point we have a URL that points back to the file that was selected in the Finder. Now we can go to work...
							MakeSymbolicLink(tURL);
							CFRelease(tURL);
                        }
                    }
                    DisposeHandle((Handle) tAliasHdl);
                }
            }
            AEDisposeDesc(&theAEDesc);
        }
    }

    return anErr;
} // HandleFileSubmenu

// =============================================================================
//	interface function implementations
// =============================================================================

// -----------------------------------------------------------------------------
//	SLPlugInQueryInterface
// -----------------------------------------------------------------------------
//	Implementation of the IUnknown QueryInterface function.
//
static HRESULT SLPlugInQueryInterface(void* thisInstance,REFIID iid,LPVOID* ppv)
{
    // Create a CoreFoundation UUIDRef for the requested interface.
    CFUUIDRef	interfaceID = CFUUIDCreateFromUUIDBytes( NULL, iid );

    DEBUGSTR("\p|SampleCMPlugInQueryInterface-I-Debug;g");

    // Test the requested ID against the valid interfaces.
    if ( CFEqual( interfaceID, kContextualMenuInterfaceID ) )
    {
        // If the TestInterface was requested, bump the ref count,
        // set the ppv parameter equal to the instance, and
        // return good status.
        //		((SampleCMPlugInType*) thisInstance )->cmInterface->AddRef(thisInstance );
        SLPlugInAddRef(thisInstance);

        *ppv = thisInstance;
        CFRelease( interfaceID );
        return S_OK;
    }
    else if ( CFEqual( interfaceID, IUnknownUUID ) )
    {
        // If the IUnknown interface was requested, same as above.
        //		( ( SampleCMPlugInType* ) thisInstance )->cmInterface->AddRef(thisInstance );
        SLPlugInAddRef(thisInstance);

        *ppv = thisInstance;
        CFRelease( interfaceID );
        return S_OK;
    }
    else
    {
        // Requested interface unknown, bail with error.
        *ppv = NULL;
        CFRelease( interfaceID );
        return E_NOINTERFACE;
    }
}

// -----------------------------------------------------------------------------
//	SLPlugInAddRef
// -----------------------------------------------------------------------------
//	Implementation of reference counting for this type. Whenever an interface
//	is requested, bump the refCount for the instance. NOTE: returning the
//	refcount is a convention but is not required so don't rely on it.
//
static ULONG SLPlugInAddRef( void *thisInstance )
{
    DEBUGSTR("\p|SampleCMPlugInAddRef-I-Debug;g");

    ( ( SampleCMPlugInType* ) thisInstance )->refCount += 1;
    return ( ( SampleCMPlugInType* ) thisInstance)->refCount;
}

// -----------------------------------------------------------------------------
//      SLPlugInRelease
// -----------------------------------------------------------------------------
//	When an interface is released, decrement the refCount.
//	If the refCount goes to zero, deallocate the instance.
//
static ULONG SLPlugInRelease( void *thisInstance )
{
    DEBUGSTR("\p|SampleCMPlugInRelease-I-Debug;g");

    ((SampleCMPlugInType*) thisInstance )->refCount -= 1;
    if (((SampleCMPlugInType*) thisInstance)->refCount == 0)
    {
        DeallocSLPlugInType((SampleCMPlugInType*) thisInstance);
        return 0;
    }
    else
    {
        return ((SampleCMPlugInType*) thisInstance)->refCount;
    }
}

// -----------------------------------------------------------------------------
//	SLPlugInExamineContext
// -----------------------------------------------------------------------------
//	The implementation of the ExamineContext test interface function.
//

static OSStatus SLPlugInExamineContext(	void*				thisInstance,
                                              const AEDesc*		inContext,
                                              AEDescList*			outCommandPairs)
{
    //CFStringRef bundleCFStringRef = CFSTR("net.comcast.home.seiryu.SymbolicLinker");
    //CFBundleRef myCFBundleRef = CFBundleGetBundleWithIdentifier(bundleCFStringRef);
    //CFStringRef makeSymLinkText = CFSTR("Make Symbolic Link");
    CFStringRef makeSymLinkText = CFCopyLocalizedStringFromTableInBundle(CFSTR("Make Symbolic Link"), CFSTR("Localizable"), SLOurBundle(), "Localized title of the symbolic link plugin (for Leopard & earlier users)");

    DEBUGSTR("\p|SampleCMPlugInExamineContext-I-Debug;g");

    // Sequence the command ids
    gNumCommandIDs = 0;

    /*printf("\nSymbolicLinker->SLPlugInExamineContext(instance: 0x%x, inContext: 0x%x, outCommandPairs: 0x%x)",
           ( unsigned ) thisInstance,
           ( const unsigned ) inContext,
           ( unsigned ) outCommandPairs );*/

    // this is a quick sample that looks for text in the context descriptor

    // make sure the descriptor isn't null
    if ( inContext != NULL )
    {
        AEDesc theAEDesc = { typeNull, NULL };
        /*CFStringRef	tempCFStringRef;
        Str32 tStr32;*/

        //AddCommandToAEDescList( CFSTR("-"), ++gNumCommandIDs,outCommandPairs );

        /*if (NULL != myCFBundleRef)
        {
            tempCFStringRef = CFBundleGetValueForInfoDictionaryKey(myCFBundleRef, CFSTR("CFBundleName"));
            tempCFStringRef = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("Inside '%@'"), tempCFStringRef);
            if (NULL != tempCFStringRef)
            {
                AddCommandToAEDescList( tempCFStringRef, ++gNumCommandIDs,outCommandPairs );
                CFRelease(tempCFStringRef);
            }
        }*/

        // tell the raw type of the descriptor
        /*sprintf((Ptr) tStr32,"raw type: '%4.4s'",(Ptr) &inContext->descriptorType);
        tempCFStringRef = CFStringCreateWithCString(kCFAllocatorDefault,(Ptr) tStr32,kCFStringEncodingASCII);
        AddCommandToAEDescList( tempCFStringRef, ++gNumCommandIDs,outCommandPairs );
        CFRelease(tempCFStringRef);*/

        // We only care about lists and aliases. Ignore everything else.
        if (inContext->descriptorType == typeAEList)
            AddCommandToAEDescList(makeSymLinkText, ++gNumCommandIDs, outCommandPairs);
        else if (inContext->descriptorType == typeAlias)
        {
            if ( AECoerceDesc( inContext, typeAEList, &theAEDesc ) == noErr )
            {
                AddCommandToAEDescList(makeSymLinkText, ++gNumCommandIDs, outCommandPairs);
                AEDisposeDesc( &theAEDesc );
            }
            else
            {
                printf("\nSLPlugInExamineContext: Unable to coerce to list." );
                // add a text only command to our command list
                AddCommandToAEDescList( CFSTR("Can't Coerce alias to list."), ++gNumCommandIDs,outCommandPairs );
            }
        }

        // Just for kicks, lets create a submenu for our plugin
        //CreateSampleSubmenu( outCommandPairs );
    }
    else
    {
        printf("\nSLPlugInExamineContext: Someone sent me a NULL descriptor!" );
        // we have a null descriptor
        AddCommandToAEDescList( CFSTR("NULL Descriptor"), ++gNumCommandIDs,outCommandPairs );
    }

    return noErr;
}

// -----------------------------------------------------------------------------
//	SLPlugInHandleSelection
// -----------------------------------------------------------------------------
//	The implementation of the HandleSelection test interface function.
//
static OSStatus SLPlugInHandleSelection(void*				thisInstance,
                                              AEDesc*				inContext,
                                              SInt32				inCommandID )
{
    DEBUGSTR("\p|SampleCMPlugInHandleSelection-I-Debug;g");

    /*printf("\nSymbolicLinker->SLPlugInHandleSelection(instance: 0x%lx, inContext: 0x%lx, inCommandID: 0x%lx)",
           ( SInt32 ) thisInstance,
           ( SInt32 ) inContext,
           ( SInt32 ) inCommandID );*/

    // Sequence the command ids
    gNumCommandIDs = 0;

    // this is a quick sample that looks for text in the context descriptor

    // make sure the descriptor isn't null
    if ( inContext != NULL )
    {
        AEDesc theAEDesc = { typeNull, NULL };

        if (inContext->descriptorType == typeAEList)
            HandleFileSubmenu(inContext,inCommandID);
        else if (inContext->descriptorType == typeAlias)
        {
            if ( AECoerceDesc( inContext, typeAEList, &theAEDesc ) == noErr )
            {
                HandleFileSubmenu(&theAEDesc,inCommandID);
                AEDisposeDesc( &theAEDesc );
            }
            else
            {
                printf("\nSLPlugInHandleSelection: Unable to coerce to list." );
            }
        }

        // handle the submenu for our plugin
        //HandleSampleSubmenu(inContext,inCommandID);
    }
    else
    {
        printf("\nSLPlugInHandleSelection: Someone sent me a NULL descriptor!" );
        // we have a null descriptor
        //		AddCommandToAEDescList( CFSTR("NULL Descriptor"), ++gNumCommandIDs,outCommandPairs );
        ++gNumCommandIDs;
    }

    return noErr;
}

// -----------------------------------------------------------------------------
//	SLPlugInPostMenuCleanup
// -----------------------------------------------------------------------------
//	The implementation of the PostMenuCleanup test interface function.
//
static void SLPlugInPostMenuCleanup( void *thisInstance )
{
    DEBUGSTR("\p|SampleCMPlugInPostMenuCleanup-I-Debug;g");

    //printf("\nSymbolicLinker->SLPlugInPostMenuCleanup(instance: 0x%x)", ( unsigned ) thisInstance );

    // No need to clean up.  We are a tidy folk.
}
