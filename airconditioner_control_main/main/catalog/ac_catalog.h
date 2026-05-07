#pragma once

#include "ac_types.h"

// 只读空调目录 API。
// 目录是固件侧的“遥控器数据库”：上位机通过 CATALOG 读它，
// 添加空调流程也按这个顺序逐个试候选遥控器。
// 对外只暴露只读访问函数，是为了避免运行时改目录导致下标关系失效。
size_t acCatalogNodeCount();
const AcCatalogNode &acCatalogNodeAt(size_t index);
const AcRemote *acFindRemoteById(const String &id);
uint8_t acRemoteCount();
