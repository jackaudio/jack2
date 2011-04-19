//
//  iPhoneNetAppDelegate.m
//  iPhoneNet
//
//  Created by Stéphane  LETZ on 16/02/09.
//  Copyright Grame 2009. All rights reserved.
//

#import "iPhoneNetAppDelegate.h"

@implementation iPhoneNetAppDelegate

@synthesize window, navigationController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {    

    // Override point for customization after application launch
    // add the navigation controller's view to the window
	[window addSubview: navigationController.view];
    [window makeKeyAndVisible];
}


- (void)dealloc {
    [navigationController release];
    [window release];
    [super dealloc];
}


@end
