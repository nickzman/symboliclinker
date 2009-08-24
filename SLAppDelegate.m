//
//  SLAppDelegate.m
//  SymbolicLinker
//
//  Created by Nick Zitzmann on 8/23/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

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
	[NSTimer scheduledTimerWithTimeInterval:10.0 target:NSApp selector:@selector(terminate:) userInfo:nil repeats:NO];	// stay resident for a while, then self-destruct
#endif
}


- (void)makeSymbolicLink:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error
{
	NSURL *fileURL = [NSURL URLFromPasteboard:pboard];
	
	if (fileURL)
		MakeSymbolicLink((CFURLRef)fileURL);
}

@end
