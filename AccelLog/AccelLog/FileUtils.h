//
//  FileUtils.h
//  cB-OLP425
//
//  Created by DB on 3/25/14.
//  Copyright (c) 2014 connectBlue. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface FileUtils : NSObject

+(NSString*) docsDirectory;
+(NSString*) getFullFileName:(NSString*)fileName;

+(NSString*) wrapStr:(const char*) str;
+(void) ensureFileExists:(NSString*)fileName;

+(void) writeString:(NSString*)str toFile:(NSString*)fileName;
+(void) writeUTF8String:(const char*)str toFile:(const char*)fileName;

+(void) appendString:(NSString*)str toFile:(NSString*)fileName;
+(void) appendUTF8String:(const char*)str toFile:(const char*)fileName;

+(NSString*) readStringFromFile:(NSString*)fileName;


@end
