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

CF_INLINE CFBundleRef SLOurBundle(void)
{
	CFStringRef bundleCFStringRef = CFSTR("net.comcast.home.seiryu.SymbolicLinker");
	return CFBundleGetBundleWithIdentifier(bundleCFStringRef);
}


CF_INLINE OSStatus StandardAlertCF(AlertType inAlertType, CFStringRef inError, CFStringRef inExplanation, const AlertStdCFStringAlertParamRec *inAlertParam, SInt16 *outItemHit)
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

CF_INLINE bool SLIsEqualToString(CFStringRef theString1, CFStringRef theString2)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wassign-enum"
	return (CFStringCompare(theString1, theString2, 0) == kCFCompareEqualTo);
#pragma clang diagnostic pop
}


void MakeSymbolicLinkToDesktop(CFURLRef url)
{
	CFURLRef desktopFolderURL, destinationURL;
	CFStringRef fileName, fileNameWithSymlinkExtension;
	char sourcePath[PATH_MAX], destinationPath[PATH_MAX];
	int tries = 1;
	
	// Set up the destination path...
	desktopFolderURL = CFBridgingRetain([[NSFileManager defaultManager] URLForDirectory:NSDesktopDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:NULL]);
	if (!desktopFolderURL)	// if, for some reason, we fail to locate the user's desktop folder, then I'd rather have us silently fail than crash
		return;
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
	
	// Now we make the link.
	while (tries < INT_MAX && symlink(sourcePath, destinationPath) != 0)
	{
		if (errno == EEXIST)	// file aleady exists; try again with a different name
		{
			CFRelease(fileNameWithSymlinkExtension);
			CFRelease(destinationURL);
			fileNameWithSymlinkExtension = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@ symlink %d"), fileName, tries);
			destinationURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, desktopFolderURL, fileNameWithSymlinkExtension, false);
			CFURLGetFileSystemRepresentation(destinationURL, true, (UInt8 *)destinationPath, PATH_MAX);
			tries++;
		}
		else
		{
			CFStringRef CFMyerror = CFCopyLocalizedStringFromTableInBundle(CFSTR("Could not make the symbolic link, because the following error occurred: %d (%s)"), CFSTR("Localizable"), SLOurBundle(), "Error message");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
			CFStringRef CFMyerrorFormatted = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFMyerror, errno, strerror(errno));
#pragma clang diagnostic pop
			SInt16 ignored;
			
			// An error occurred, so set up a standard alert box and run it...
			StandardAlertCF(kAlertCautionAlert, CFMyerrorFormatted, NULL, NULL, &ignored);
			CFRelease(CFMyerror);
			CFRelease(CFMyerrorFormatted);
		}
	}
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
	int tries = 1;
	
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
    
    // Now we make the link.
    while (tries != INT_MAX && symlink(originalDestPath, destPath) != 0)
    {
        if (errno == EACCES || errno == EROFS)
        {
            // We get to this point if it was a "permission denied" or "read-only" error.
            // Let's try it again, but we'll make it on the desktop.
            MakeSymbolicLinkToDesktop(url);
        }
		else if (errno == EEXIST)
		{
			// Something else already exists at this path. Try again with a new name, using the convention Apple uses in Finder.
			snprintf(destPath, PATH_MAX, "%s symlink %d", originalDestPath, tries);
			tries++;
		}
        else
        {
            CFStringRef CFMyerror = CFCopyLocalizedStringFromTableInBundle(CFSTR("Could not make the symbolic link, because the following error occurred: %d (%s)"), CFSTR("Localizable"), SLOurBundle(), "Error message");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
            CFStringRef CFMyerrorFormatted = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFMyerror, errno, strerror(errno));
#pragma clang diagnostic pop
            SInt16 ignored;
			
            // An error occurred, so set up a standard alert box and run it...
            StandardAlertCF(kAlertCautionAlert, CFMyerrorFormatted, NULL, NULL, &ignored);
			CFRelease(CFMyerror);
            CFRelease(CFMyerrorFormatted);
        }
    }
done:
	CFRelease(urlNoPathComponent);
	CFRelease(pathStringNoPathComponent);
}
