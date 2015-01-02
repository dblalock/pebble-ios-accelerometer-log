//
//  ViewController.m
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import "ViewController.h"

#import <PebbleKit/PebbleKit.h>
#import "AccelData.h"
#import "FileUtils.h"

//#define POLL

@interface ViewController () <PBDataLoggingServiceDelegate>

@end

@implementation ViewController {
	UITextView *_textView;
	NSTimer *_pollTimer;
}

- (void) pollForData:(NSTimer*)timer {
#ifdef POLL
	NSLog(@"polling for data...");
	[[[PBPebbleCentral defaultCentral] dataLoggingService] pollForData];
#endif
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self) {
		[[[PBPebbleCentral defaultCentral] dataLoggingService] setDelegate:self];
	}
	return self;
}

// create a text view programmatically to save us the trouble of
// creating a real storyboard
- (void)viewDidLoad {
	[super viewDidLoad];
	_textView = [[UITextView alloc] initWithFrame:self.view.bounds];
	_textView.text = @"Log:\n-------------------\n";
	_textView.editable = NO;
	_textView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
	_textView.autoresizesSubviews = YES;
	[self.view addSubview:_textView];
	
	_pollTimer = [NSTimer scheduledTimerWithTimeInterval:30.0
												  target:self
												selector:@selector(pollForData:)
												userInfo:nil repeats:YES];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

//=================================================================
#pragma mark Logging Stuff
//=================================================================

int64_t currentTimeMs() {
	return [@(floor([NSDate timeIntervalSinceReferenceDate] * 1000)) longLongValue];
}

-(void) writeSamples:(const SInt8 *const)array
	   numberOfItems:(uint)len {
	NSString* fileName = [NSString stringWithFormat:@"%lld.csv",
						  currentTimeMs()];
	for (unsigned int i = 0; i < len; i++) {
		NSString* line = [NSString stringWithFormat:@"%hhd\n", array[i]];
		[FileUtils appendString:line toFile:fileName];
	}
}

static int rxs = 0;
static long totalBytes = 0;
- (BOOL) dataLoggingService:(PBDataLoggingService *)service
			  hasByteArrays:(const UInt8 *const)bytes
			  numberOfItems:(UInt16)numberOfItems
	  forDataLoggingSession:(PBDataLoggingSessionMetadata *)session {
	
	totalBytes += numberOfItems;
	NSString* msg = [NSString stringWithFormat:@"Received %u bytes, tx #%d", numberOfItems, ++rxs];
	msg = [msg stringByAppendingString:[NSString stringWithFormat:@"\ntotal bytes ever: %ld", totalBytes]];
	NSLog(@"%@", msg);
	[_textView setText:msg];
	
	if (numberOfItems >= 3) {
		AccelData* data = [AccelData fromUInt8s:bytes];
		NSString* str = [NSString stringWithFormat:@"\nx,y,z = %hhd, %hhd %hhd",
						 data.x, data.y, data.z];
		NSLog(@"%@",str);
		NSLog(@"total bytes: %ld", totalBytes);
		[_textView setText:[[_textView text] stringByAppendingString:str]];
	}
	return YES;
}

- (BOOL) dataLoggingService:(PBDataLoggingService *)service
				  hasSInt8s:(const SInt8 *const)array
			  numberOfItems:(UInt16)numberOfItems
	  forDataLoggingSession:(PBDataLoggingSessionMetadata *)session {

	totalBytes += numberOfItems;
	NSString* msg = [NSString stringWithFormat:@"Received %u ints, tx #%d", numberOfItems, ++rxs];
	msg = [msg stringByAppendingString:[NSString stringWithFormat:@"\ntotal bytes ever: %ld", totalBytes]];
	NSLog(@"%@", msg);
	[_textView setText:msg];

	if (numberOfItems >= 3) {
		AccelData* data = [AccelData fromInt8s:array];
		NSString* str = [NSString stringWithFormat:@"\nx,y,z = %hhd, %hhd %hhd",
						 data.x, data.y, data.z];
		NSLog(@"%@", str);
		NSLog(@"total bytes: %ld", totalBytes);
		[_textView setText:[[_textView text] stringByAppendingString:str]];
	}
	return YES;
}

- (void)dataLoggingService:(PBDataLoggingService *)service sessionDidFinish:(PBDataLoggingSessionMetadata *)sessionMetadata {
	NSLog(@"Session finished.");
}

@end
