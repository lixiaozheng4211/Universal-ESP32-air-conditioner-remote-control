#pragma once

#include "ac_types.h"

size_t acCatalogNodeCount();
const AcCatalogNode &acCatalogNodeAt(size_t index);
const AcRemote *acFindRemoteById(const String &id);
uint8_t acRemoteCount();
