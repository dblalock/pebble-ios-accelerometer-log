//
//  AppDelegate.m
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import "AppDelegate.h"

#import <PebbleKit/PebbleKit.h>
#import "ViewController.h"

@interface AppDelegate () <PBPebbleCentralDelegate>

@end

@implementation AppDelegate

ViewController *_dataLoggingViewController;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	[PBPebbleCentral setDebugLogsEnabled:YES];
	
	uuid_t myAppUUIDbytes;
	NSUUID *myAppUUID = [[NSUUID alloc] initWithUUIDString:@"ab150ab8-3323-4ec5-a82f-d59879bd0724"];
	[myAppUUID getUUIDBytes:myAppUUIDbytes];
	[[PBPebbleCentral defaultCentral] setAppUUID:[NSData dataWithBytes:myAppUUIDbytes length:16]];
	[[PBPebbleCentral defaultCentral] setDelegate:self];
	
	self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	_dataLoggingViewController = [[ViewController alloc] initWithNibName:nil bundle:nil];
	self.window.rootViewController = [[UINavigationController alloc] initWithRootViewController:_dataLoggingViewController];
	[self.window makeKeyAndVisible];
	
	NSData* appUUID = [[PBPebbleCentral defaultCentral] appUUID];

	if (appUUID) {
		NSLog(@"app uuid not null");
		NSUUID* readUUID = [[NSUUID alloc] initWithUUIDBytes:[appUUID bytes]];
		NSLog(@"Uuid from pebble = %@", [readUUID UUIDString]);
	} else {
		NSLog(@"app uuid null");
	}
//	NSString* uuidStr = [[NSString alloc] initWithData:appUUID encoding:NSUTF8StringEncoding];
//	NSLog(@"set app uuid to %@", uuidStr);		//uh oh...this is null
	
	
	
	//SELF:pick up here; it doesn't talk to the app...
		//see what happens if we regenerate the watch app and paste in the uuid it generates
		//ALSO: we may be getting the same thing the ocean app had wherein it doens't
		//actually send the data until we end the data frame or whatever
		//
		//AHA: so it is logging and eventually sending the data; the problem is
		//that it just keeps buffering it like forever and never flushes it
		//until I quit the watch app
	
	
	//So I think the moral here is, "screw the data logging API"
	
	
	
	for (PBWatch* watch in [[PBPebbleCentral defaultCentral] connectedWatches] ) {
		NSLog(@"found connected watch: %@:", [watch name]);
	}
	
	NSLog(@"last connected watch: %@:", [[[PBPebbleCentral defaultCentral] lastConnectedWatch] name] );
	
	return YES;
}

- (void)pebbleCentral:(PBPebbleCentral*)central watchDidConnect:(PBWatch*)watch isNew:(BOOL)isNew {
	[[[UIAlertView alloc] initWithTitle:@"Connected!" message:[watch name] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] show];
	
	NSLog(@"Pebble connected: %@", [watch name]);
}

- (void)pebbleCentral:(PBPebbleCentral*)central watchDidDisconnect:(PBWatch*)watch {
	[[[UIAlertView alloc] initWithTitle:@"Disconnected!" message:[watch name] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] show];
	
	NSLog(@"Pebble disconnected: %@", [watch name]);
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
