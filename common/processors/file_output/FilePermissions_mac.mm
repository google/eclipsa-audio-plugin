/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>

#include "FilePermissions.h"

static bool isProcessSandboxed() {
  return [[NSProcessInfo processInfo].environment objectForKey:@"APP_SANDBOX_CONTAINER_ID"] != nil;
}

std::string createSecurityScopedBookmark(const std::string& path) {
  @autoreleasepool {
    NSURL* url =
        [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
    if (!url || !isProcessSandboxed()) return {};

    NSError* error = nil;
    NSData* bookmark =
        [url bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
            includingResourceValuesForKeys:nil
                             relativeToURL:nil
                                     error:&error];
    if (!bookmark) return {};

    NSString* base64 = [bookmark base64EncodedStringWithOptions:0];
    return base64 ? std::string([base64 UTF8String]) : std::string{};
  }
}

void* startSecurityScopedAccess(const std::string& bookmarkBase64) {
  if (bookmarkBase64.empty() || !isProcessSandboxed()) return nullptr;
  @autoreleasepool {
    NSData* data = [[NSData alloc]
        initWithBase64EncodedString:[NSString
                                        stringWithUTF8String:bookmarkBase64
                                                                 .c_str()]
                            options:0];
    if (!data) return nullptr;

    BOOL stale = NO;
    NSError* error = nil;
    NSURL* url = [NSURL
        URLByResolvingBookmarkData:data
                           options:NSURLBookmarkResolutionWithSecurityScope
                     relativeToURL:nil
               bookmarkDataIsStale:&stale
                             error:&error];
    if (!url) return nullptr;

    if (![url startAccessingSecurityScopedResource]) return nullptr;
    return (__bridge_retained void*)url;
  }
}

void stopSecurityScopedAccess(void* handle) {
  if (!handle) return;
  @autoreleasepool {
    NSURL* url = (__bridge_transfer NSURL*)handle;
    [url stopAccessingSecurityScopedResource];
  }
}
