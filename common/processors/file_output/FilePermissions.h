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

#pragma once
#include <string>

// Creates a security-scoped bookmark for the file at the given path and
// returns it as a base64-encoded string. Must be called immediately after
// the user selects a file via NSSavePanel/NSOpenPanel while the Powerbox
// access grant is still active. Returns an empty string on failure or on
// non-Apple platforms.
std::string createSecurityScopedBookmark(const std::string& path);

// Resolves a base64-encoded security-scoped bookmark and starts sandbox
// access for the referenced file. Returns an opaque handle that must be
// passed to stopSecurityScopedAccess when writing is complete. Returns
// nullptr on failure, when the process is not sandboxed, or on non-Apple
// platforms.
void* startSecurityScopedAccess(const std::string& bookmarkBase64);

// Stops sandbox access for a handle returned by startSecurityScopedAccess
// and releases the associated resource. Safe to call with nullptr.
void stopSecurityScopedAccess(void* handle);
