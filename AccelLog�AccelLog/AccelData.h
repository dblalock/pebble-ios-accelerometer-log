//
//  AccelData.h
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import <Foundation/Foundation.h>

#define BYTES_PER_VALUE 1

@interface AccelData : NSObject {

}

+ (int) bytesPerObj;
+ (int64_t) currentTimeStamp;
+ (AccelData*)fromUInt8s:(const UInt8 *const)data;
+ (AccelData*)fromInt8s:(const SInt8 *const)data;
- (id)initWithBytes:(const UInt8 *const)data timestamp:(int64_t)time;

#if BYTES_PER_VALUE == 2
@property int16_t x;
@property int16_t y;
@property int16_t z;
#else
@property int8_t x;
@property int8_t y;
@property int8_t z;
#endif
@property int64_t timestamp;
//@property BOOL didVibrate;

@end
