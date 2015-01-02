//
//  AccelData.m
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import "AccelData.h"

@implementation AccelData

// used to parse a long array into a bunch of readings
+ (int) bytesPerObj {
	return 3 * BYTES_PER_VALUE;
}

+ (int64_t) currentTimeStamp {
	return [@(floor([NSDate timeIntervalSinceReferenceDate] * 1000)) longLongValue];
}

+ (AccelData*)fromUInt8s:(const UInt8 *const)data {
	return [[AccelData alloc] initWithBytes:data timestamp:-1];
}

+ (AccelData*)fromInt8s:(const SInt8 *const)data {
	return [[AccelData alloc] initWithBytes: ((const UInt8 *)data) timestamp:-1];
}

- (id)initWithBytes:(const UInt8 *const)data timestamp:(int64_t)time {
	self = [super init];
	if (self) {
#if BYTES_PER_VALUE == 2
 		self.x = (data[0] & 0xff) | (data[1] << 8);
		self.y = (data[2] & 0xff) | (data[3] << 8);
		self.z = (data[4] & 0xff) | (data[5] << 8);
#else
		self.x = data[0];
		self.y = data[1];
		self.z = data[2];
#endif
		if (time > 0) {
			self.timestamp = time;
		} else {
			self.timestamp = [AccelData currentTimeStamp];
		}
	}
	return self;
}

@end