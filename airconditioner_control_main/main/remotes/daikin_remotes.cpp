#include "remotes/daikin_remotes.h"

#include "drivers/irac_backend.h"

// 大金协议会按帧长度或型号区分。
// 添加空调时每个条目都会作为独立候选逐个尝试。
const AcRemote kDaikinArc433Remote = {
    "daikin_arc433",
    "daikin",
    "Daikin ARC433",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::DAIKIN,
    kAcNoModel,
};

const AcRemote kDaikinArc477Remote = {
    "daikin_arc477",
    "daikin",
    "Daikin ARC477A1",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::DAIKIN2,
    kAcNoModel,
};

const AcRemote kDaikin216Remote = {
    "daikin_216",
    "daikin",
    "Daikin 216",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::DAIKIN216,
    kAcNoModel,
};

const AcRemote kDaikin160Remote = {
    "daikin_160",
    "daikin",
    "Daikin 160",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::DAIKIN160,
    kAcNoModel,
};

const AcRemote kDaikin176Remote = {
    "daikin_176",
    "daikin",
    "Daikin 176",
    {16, 30, true, false, true},
    &kIracRemoteClass,
    decode_type_t::DAIKIN176,
    kAcNoModel,
};

const AcRemote kDaikin128Remote = {
    "daikin_128",
    "daikin",
    "Daikin 128",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::DAIKIN128,
    kAcNoModel,
};

const AcRemote kDaikin152Remote = {
    "daikin_152",
    "daikin",
    "Daikin 152",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::DAIKIN152,
    kAcNoModel,
};

const AcRemote kDaikin64Remote = {
    "daikin_64",
    "daikin",
    "Daikin 64",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::DAIKIN64,
    kAcNoModel,
};

const AcRemote kDaikin312Remote = {
    "daikin_312",
    "daikin",
    "Daikin 312",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::DAIKIN312,
    kAcNoModel,
};
