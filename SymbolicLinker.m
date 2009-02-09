/*
 *  SymbolicLinker.m
 *  SymbolicLinker
 *
 *  Created by Nick Zitzmann on Sun Mar 21 2004.
 *  Copyright (c) 2004 Nick Zitzmann. All rights reserved.
 *
 */
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "SymbolicLinker.h"
#include <stdio.h>
#include <unistd.h>
#include "MoreFinderEvents.h"
#ifdef __LP64__
#import <Cocoa/Cocoa.h>
#endif

inline OSStatus StandardAlertCF(AlertType inAlertType, CFStringRef inError, CFStringRef inExplanation, const AlertStdCFStringAlertParamRec *inAlertParam, SInt16 *outItemHit);
inline bool SLIsEqualToString(CFStringRef theString1, CFStringRef theString2);

void MakeSymbolicLinkToDesktop(CFURLRef url)
{
	FSRef desktopFolder;
	CFURLRef desktopFolderURL, destinationURL;
	CFStringRef fileName, fileNameWithSymlinkExtension;
	char sourcePath[PATH_MAX], destinationPath[PATH_MAX];
	
	// Set up the destination path...
	FSFindFolder(kUserDomain, kDesktopFolderType, false, &desktopFolder);
	desktopFolderURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &desktopFolder);
	fileName = CFURLCopyLastPathComponent(url);
	fileNameWithSymlinkExtension = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ symlink"), fileName);
	destinationURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, desktopFolderURL, fileNameWithSymlinkExtension, false);
	CFURLGetFileSystemRepresentation(destinationURL, true, (UInt8 *)destinationPath, PATH_MAX);
	CFURLGetFileSystemRepresentation(url, false, (UInt8 *)sourcePath, PATH_MAX);
	
	// Check to make sure that destPath doesn't point to something that already exists.
	if (access(destinationPath, F_OK) == 0)
	{
		int tries;
		
		for (tries = 1 ; tries < INT_MAX && access(destinationPath, F_OK) == 0 ; tries++)
		{
			// We format it like how Apple does with aliases in this case.
			//snprintf(destPath, PATH_MAX, "%s/%s symlink %d", cDesktopFolder, filename, tries);
			CFRelease(fileNameWithSymlinkExtension);
			CFRelease(destinationURL);
			fileNameWithSymlinkExtension = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ symlink %d"), fileName, tries);
			destinationURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, desktopFolderURL, fileNameWithSymlinkExtension, false);
			CFURLGetFileSystemRepresentation(destinationURL, true, (UInt8 *)destinationPath, PATH_MAX);
		}
		// In the extremely unlikely event that we tried 2^31-1 tries to get a path name, and didn't succeed, then just stop...
		if (tries == INT_MAX)
		{
			goto done;
		}
	}
	
	// Now we make the link.
	if (symlink(sourcePath, destinationPath) != 0)
	{
		CFStringRef CFMyerror = CFCopyLocalizedStringFromTableInBundle(CFSTR("Could not make the symbolic link, because the following error occurred: %d (%s)"), CFSTR("Localizable"), SLOurBundle(), "Error message");
		CFStringRef CFMyerrorFormatted = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFMyerror, errno, strerror(errno));
		SInt16 ignored;
		
		// An error occurred, so set up a standard alert box and run it...
		StandardAlertCF(kAlertCautionAlert, CFMyerrorFormatted, NULL, NULL, &ignored);
		CFRelease(CFMyerror);
		CFRelease(CFMyerrorFormatted);
	}
	else
	{
		/*static SInt32 osDesktopVersion = 0;
		
		if (osDesktopVersion == 0)
			Gestalt(gestaltSystemVersion, &osDesktopVersion);
		
		// For pre-Tiger systems, we need to send an Apple event to the Finder stating that the contents of the desktop folder have changed. We don't need to do this on Tiger systems because the Tiger Finder automatically detects the change.
		if (osDesktopVersion < 0x1040)
		{*/
			// If all went well, then let the Finder know that the file system has changed on the user's desktop.
			MoreFEUpdateItemFSRef(&desktopFolder);
		//}
	}
done:
	CFRelease(desktopFolderURL);
	CFRelease(fileName);
	CFRelease(fileNameWithSymlinkExtension);
	CFRelease(destinationURL);
}


// Here is where we make the symbolic link, given the path.
void MakeSymbolicLink(CFURLRef url)
{
	CFURLRef urlNoPathComponent = CFURLCreateCopyDeletingLastPathComponent(kCFAllocatorDefault, url);
	CFStringRef pathStringNoPathComponent = CFURLCopyFileSystemPath(urlNoPathComponent, kCFURLPOSIXPathStyle);
	char destPath[PATH_MAX], originalDestPath[PATH_MAX], pathFolder[PATH_MAX];
	
	// First check to see if we're making a link to a disk.
	if (SLIsEqualToString(pathStringNoPathComponent, CFSTR("/Volumes")) || SLIsEqualToString(pathStringNoPathComponent, CFSTR("")))
	{
		MakeSymbolicLinkToDesktop(url);
		goto done;	// clean up and exit the function
	}
	
	CFURLGetFileSystemRepresentation(url, false, (UInt8 *)originalDestPath, PATH_MAX);
	CFURLGetFileSystemRepresentation(urlNoPathComponent, false, (UInt8 *)pathFolder, PATH_MAX);
	
	// Set up the destination path...
    snprintf(destPath, PATH_MAX, "%s symlink", originalDestPath);
	
    // Check to make sure that destPath doesn't point to something that already exists.
    if (access(destPath, F_OK) == 0)
    {
        int tries;
        
        for (tries = 1 ; tries < INT_MAX && access(destPath, F_OK) == 0 ; tries++)
        {
            // We format it like how Apple does with aliases in this case.
            snprintf(destPath, PATH_MAX, "%s symlink %d", originalDestPath, tries);
        }
		// In the extremely unlikely event that we tried 2^31-1 tries to get a path name, and didn't succeed, then just stop...
		if (tries == INT_MAX)
		{
			goto done;
		}
    }
    
    // Now we make the link.
    if (symlink(originalDestPath, destPath) != 0)
    {
        if ((errno == EACCES) || (errno == EROFS))
        {
            // We get to this point if it was a "permission denied" or "read-only" error.
            // Let's try it again, but we'll make it on the desktop.
            MakeSymbolicLinkToDesktop(url);
        }
        else
        {
            CFStringRef CFMyerror = CFCopyLocalizedStringFromTableInBundle(CFSTR("Could not make the symbolic link, because the following error occurred: %d (%s)"), CFSTR("Localizable"), SLOurBundle(), "Error message");
            CFStringRef CFMyerrorFormatted = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFMyerror, errno, strerror(errno));
            SInt16 ignored;
			
            // An error occurred, so set up a standard alert box and run it...
            StandardAlertCF(kAlertCautionAlert, CFMyerrorFormatted, NULL, NULL, &ignored);
			CFRelease(CFMyerror);
            CFRelease(CFMyerrorFormatted);
        }
    }
    else
    {
		/*static long osVersion = 0;
		
		if (osVersion == 0)
			Gestalt(gestaltSystemVersion, &osVersion);
		
		// For pre-Tiger systems, we need to send an Apple event to the Finder stating that the contents of the desktop folder have changed. We don't need to do this on Tiger systems because the Tiger Finder automatically detects the change.
		if (osVersion < 0x1040)
		{*/
		// Took out the above because, as it turns out, the Tiger Finder doesn't always detect the change after all...
			FSRef pathFolderRef;
			
			// If the operation went OK, then we tell the Finder that the file system has changed.
			FSPathMakeRef((UInt8 *)pathFolder, &pathFolderRef, NULL);
			MoreFEUpdateItemFSRef(&pathFolderRef);
		//}
    }
done:
	CFRelease(urlNoPathComponent);
	CFRelease(pathStringNoPathComponent);
}

inline CFBundleRef SLOurBundle(void)
{
    CFStringRef bundleCFStringRef = CFSTR("net.comcast.home.seiryu.SymbolicLinker");
    return CFBundleGetBundleWithIdentifier(bundleCFStringRef);
}


inline OSStatus StandardAlertCF(AlertType inAlertType, CFStringRef inError, CFStringRef inExplanation, const AlertStdCFStringAlertParamRec *inAlertParam, SInt16 *outItemHit)
{
    OSStatus err = noErr;
    DialogRef outAlert;
    
#ifdef __LP64__
#pragma unused(outAlert)
	NSApplicationLoad();
	NSRunAlertPanel((NSString *)inError, @"%@", nil, nil, nil, (inExplanation ? (NSString *)inExplanation : @""));
#else
    if ((err = CreateStandardAlert(inAlertType, inError, inExplanation, inAlertParam, &outAlert)) == noErr)
    {
        err = RunStandardAlert(outAlert, NULL, outItemHit);
    }
#endif
    return err;
}

inline bool SLIsEqualToString(CFStringRef theString1, CFStringRef theString2)
{
	return (CFStringCompare(theString1, theString2, 0) == kCFCompareEqualTo);
}
