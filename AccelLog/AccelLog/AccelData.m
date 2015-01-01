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
	return 6;	// my version is just x,y,z
//	return 7;	//each AccelData from the pebble is 6 bytes
}

+ (AccelData*)fromBytes:(const UInt8 *const)data {
	return [[AccelData alloc] initWithBytes:data];
}

- (id)initWithBytes:(const UInt8 *const)data {
	self = [super init];
	if (self) {
//		self.x = (data[1] & 0xff) | (data[0] << 8);
//		self.y = (data[3] & 0xff) | (data[2] << 8);
//		self.z = (data[5] & 0xff) | (data[4] << 8);
 		self.x = (data[0] & 0xff) | (data[1] << 8);
		self.y = (data[2] & 0xff) | (data[3] << 8);
		self.z = (data[4] & 0xff) | (data[5] << 8);
//		self.didVibrate = data[6] != 0;
	}
	return self;
}

@end