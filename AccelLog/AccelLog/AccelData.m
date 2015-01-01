//
//  AccelData.m
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import "AccelData.h"

@implementation AccelData

+ (int) bytesPerObj {
	return 7;	//each AccelData from the pebble is 6 bytes
}

- (id)initWithBytes:(int8_t*)data {
	self = [super init];
	if (self) {
 		self.x = (data[0] & 0xff) | (data[1] << 8);
		self.y = (data[2] & 0xff) | (data[3] << 8);
		self.z = (data[4] & 0xff) | (data[5] << 8);
		self.didVibrate = data[6] != 0;
	}
	return self;
}

@end