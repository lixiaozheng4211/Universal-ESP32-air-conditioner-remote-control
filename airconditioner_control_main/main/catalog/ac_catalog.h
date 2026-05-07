#pragma once

#include "ac_types.h"

// Read-only AC catalog API. The implementation stores brands and remote
// candidates as a static tree so host software can discover available remotes.
size_t acCatalogNodeCount();
const AcCatalogNode &acCatalogNodeAt(size_t index);
const AcRemote *acFindRemoteById(const String &id);
uint8_t acRemoteCount();
