//
//  AccelData.h
//  AccelLog
//
//  Created by DB on 10/19/14.
//  Copyright (c) 2014 DB. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AccelData : NSObject {

}

+ (int) bytesPerObj;
+ (AccelData*)fromBytes:(const UInt8 *const)data;
- (id)initWithBytes:(const UInt8 *const)data;

@property int16_t x;
@property int16_t y;
@property int16_t z;
//@property int64_t timestamp;
//@property BOOL didVibrate;

@end
