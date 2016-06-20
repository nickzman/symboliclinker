//
//  SLAppDelegate.m
//  SymbolicLinker
//
//  Created by Nick Zitzmann on 8/23/09.
//  Copyright 2009 Nick Zitzmann. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import "SLAppDelegate.h"
#import "SymbolicLinker.h"

int main(int argc, char **argv)
{
	return NSApplicationMain(argc, (const char **)argv);
}


@implementation SLAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	NSString *dummy = NSLocalizedStringFromTable(@"Make Symbolic Link", @"ServicesMenu", @"Localized title of the symbolic link service (for Snow Leopard & later users)");	// this is here just so genstrings will pick up the localized service name
	
#pragma unused(dummy)
	NSUpdateDynamicServices();	// force a reload of the user's services
	[NSApp setServicesProvider:self];	// this class will provide the services
#ifndef DEBUG
	[NSTimer scheduledTimerWithTimeInterval:5.0 target:NSApp selector:@selector(terminate:) userInfo:nil repeats:NO];	// stay resident for a while, then self-destruct
#endif
}


- (void)makeSymbolicLink:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error
{
	NSArray *filenames = [pboard propertyListForType:NSFilenamesPboardType];	// I thought these old pboard types were supposed to be deprecated in favor of UTIs, but this is the only way we can handle multiple files at once
	
	for (NSString *filename in filenames)
	{
		NSURL *fileURL = [NSURL fileURLWithPath:filename];
		
		if (fileURL)
			MakeSymbolicLink((__bridge CFURLRef)fileURL);
	}
}

@end
