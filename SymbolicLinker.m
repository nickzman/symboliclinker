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
#ifdef USE_COCOA
#import <Cocoa/Cocoa.h>
#else
#include "MoreFinderEvents.h"
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
	if (SLIsEqualToString(fileName, CFSTR("/")))	// true if the user is making a symlink to the boot volume
	{
		CFRelease(fileName);
		fileName = CFURLCopyFileSystemPath(url, kCFURLHFSPathStyle);	// use CoreFoundation to figure out the boot volume's name
	}
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
		// If all went well, then let the Finder know that the file system has changed on the user's desktop.
#ifndef USE_COCOA
		MoreFEUpdateItemFSRef(&desktopFolder);
#endif
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
	if (SLIsEqualToString(pathStringNoPathComponent, CFSTR("/Volumes")) || SLIsEqualToString(pathStringNoPathComponent, CFSTR("")) || SLIsEqualToString(pathStringNoPathComponent, CFSTR("/..")) || SLIsEqualToString(pathStringNoPathComponent, CFSTR("/")))
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
#ifndef USE_COCOA
		FSRef pathFolderRef;
		
		// If the operation went OK, then we tell the Finder that the file system has changed.
		FSPathMakeRef((UInt8 *)pathFolder, &pathFolderRef, NULL);
		MoreFEUpdateItemFSRef(&pathFolderRef);
#endif
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
	
#ifdef USE_COCOA
	CFUserNotificationDisplayAlert(0.0, kCFUserNotificationPlainAlertLevel, NULL, NULL, NULL, (inError ? inError : CFSTR("")), inExplanation, NULL, NULL, NULL, NULL);
#else
    DialogRef outAlert;
	
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
