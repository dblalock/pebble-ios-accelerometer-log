//
//  ViewController.m
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import "ViewController.h"
#import <PebbleKit/PebbleKit.h>

@interface ViewController () <PBDataLoggingServiceDelegate>

@end

@implementation ViewController {
	UITextView *_textView;
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
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

static int txs = 0;
- (BOOL) dataLoggingService:(PBDataLoggingService *)service hasByteArrays:(const UInt8 *const)bytes numberOfItems:(UInt16)numberOfItems forDataLoggingSession:(PBDataLoggingSessionMetadata *)session {
	NSString* msg = [NSString stringWithFormat:@"Received %u bytes, tx %d", numberOfItems, txs++];
	NSLog(@"%@", msg);
	[_textView setText:msg];
	if (numberOfItems >= 6) {
		NSString* str = [NSString stringWithFormat:@"x,y,z = %hd, %hd %hd",
						 (short)bytes[0], (short)bytes[2], (short)bytes[4]];
		NSLog(@"%@",str);
	}
	return YES;
}

- (void)dataLoggingService:(PBDataLoggingService *)service sessionDidFinish:(PBDataLoggingSessionMetadata *)sessionMetadata {
	NSLog(@"Session finished.");
}

@end
