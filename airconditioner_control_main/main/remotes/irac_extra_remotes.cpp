#include "remotes/irac_extra_remotes.h"

#include "drivers/irac_backend.h"

// 这里的品牌大多是 IRremoteESP8266 已经抽象进 IRac 的长尾协议。
// 固件只保存“候选遥控器描述”，真正的编码仍由第三方库完成，
// 后续如果某个品牌需要特殊按键码，再拆出独立后端即可。

const AcRemote kAirtonStandardRemote = {
    "airton_standard",
    "airton",
    "Airton standard",
    {16, 31, true, true, false},
    &kIracRemoteClass,
    decode_type_t::AIRTON,
    kAcNoModel,
};

const AcRemote kAirwellStandardRemote = {
    "airwell_standard",
    "airwell",
    "Airwell standard",
    {16, 30, true, false, false},
    &kIracRemoteClass,
    decode_type_t::AIRWELL,
    kAcNoModel,
};

const AcRemote kAmcorStandardRemote = {
    "amcor_standard",
    "amcor",
    "Amcor standard",
    {12, 32, true, false, false},
    &kIracRemoteClass,
    decode_type_t::AMCOR,
    kAcNoModel,
};

const AcRemote kArgoWrem2Remote = {
    "argo_wrem2",
    "argo",
    "Argo SAC WREM2",
    {10, 32, true, true, false},
    &kIracRemoteClass,
    decode_type_t::ARGO,
    static_cast<int16_t>(argo_ac_remote_model_t::SAC_WREM2),
};

const AcRemote kArgoWrem3Remote = {
    "argo_wrem3",
    "argo",
    "Argo SAC WREM3",
    {10, 32, true, true, false},
    &kIracRemoteClass,
    decode_type_t::ARGO,
    static_cast<int16_t>(argo_ac_remote_model_t::SAC_WREM3),
};

const AcRemote kBosch144Remote = {
    "bosch_144",
    "bosch",
    "Bosch 144-bit",
    {16, 30, true, false, false},
    &kIracRemoteClass,
    decode_type_t::BOSCH144,
    kAcNoModel,
};

const AcRemote kCoolixStandardRemote = {
    "coolix_standard",
    "coolix",
    "Coolix standard",
    {17, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::COOLIX,
    kAcNoModel,
};

const AcRemote kCoronaAcRemote = {
    "corona_ac",
    "corona",
    "Corona AC",
    {17, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::CORONA_AC,
    kAcNoModel,
};

const AcRemote kDelonghiAcRemote = {
    "delonghi_ac",
    "delonghi",
    "Delonghi AC",
    {18, 32, true, false, false},
    &kIracRemoteClass,
    decode_type_t::DELONGHI_AC,
    kAcNoModel,
};

const AcRemote kEcoclimStandardRemote = {
    "ecoclim_standard",
    "ecoclim",
    "Ecoclim standard",
    {5, 36, true, false, false},
    &kIracRemoteClass,
    decode_type_t::ECOCLIM,
    kAcNoModel,
};

const AcRemote kEuromStandardRemote = {
    "eurom_standard",
    "eurom",
    "Eurom standard",
    {16, 32, true, true, false},
    &kIracRemoteClass,
    decode_type_t::EUROM,
    kAcNoModel,
};

// 富士通不同遥控器的帧格式和摆风能力差异较大，
// 所以按库里的 model 枚举拆成多个候选，添加空调时逐个试。
const AcRemote kFujitsuArRah2eRemote = {
    "fujitsu_arrah2e",
    "fujitsu",
    "Fujitsu AR-RAH2E",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARRAH2E),
};

const AcRemote kFujitsuArDb1Remote = {
    "fujitsu_ardb1",
    "fujitsu",
    "Fujitsu AR-DB1",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARDB1),
};

const AcRemote kFujitsuArReb1eRemote = {
    "fujitsu_arreb1e",
    "fujitsu",
    "Fujitsu AR-REB1E",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARREB1E),
};

const AcRemote kFujitsuArJw2Remote = {
    "fujitsu_arjw2",
    "fujitsu",
    "Fujitsu AR-JW2",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARJW2),
};

const AcRemote kFujitsuArRy4Remote = {
    "fujitsu_arry4",
    "fujitsu",
    "Fujitsu AR-RY4",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARRY4),
};

const AcRemote kFujitsuArRew4eRemote = {
    "fujitsu_arrew4e",
    "fujitsu",
    "Fujitsu AR-REW4E",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::FUJITSU_AC,
    static_cast<int16_t>(fujitsu_ac_remote_model_t::ARREW4E),
};

const AcRemote kGoodweatherStandardRemote = {
    "goodweather_standard",
    "goodweather",
    "Goodweather standard",
    {16, 31, true, true, false},
    &kIracRemoteClass,
    decode_type_t::GOODWEATHER,
    kAcNoModel,
};

const AcRemote kKelvinatorStandardRemote = {
    "kelvinator_standard",
    "kelvinator",
    "Kelvinator standard",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::KELVINATOR,
    kAcNoModel,
};

const AcRemote kMirageKkg9ac1Remote = {
    "mirage_kkg9ac1",
    "mirage",
    "Mirage KKG9A-C1",
    {16, 32, true, true, false},
    &kIracRemoteClass,
    decode_type_t::MIRAGE,
    static_cast<int16_t>(mirage_ac_remote_model_t::KKG9AC1),
};

const AcRemote kMirageKkg29ac1Remote = {
    "mirage_kkg29ac1",
    "mirage",
    "Mirage KKG29A-C1",
    {16, 32, true, true, true},
    &kIracRemoteClass,
    decode_type_t::MIRAGE,
    static_cast<int16_t>(mirage_ac_remote_model_t::KKG29AC1),
};

const AcRemote kNeoclimaStandardRemote = {
    "neoclima_standard",
    "neoclima",
    "Neoclima standard",
    {16, 32, true, true, true},
    &kIracRemoteClass,
    decode_type_t::NEOCLIMA,
    kAcNoModel,
};

const AcRemote kRhossStandardRemote = {
    "rhoss_standard",
    "rhoss",
    "Rhoss standard",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::RHOSS,
    kAcNoModel,
};

const AcRemote kSanyoAcRemote = {
    "sanyo_ac",
    "sanyo",
    "Sanyo AC",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::SANYO_AC,
    kAcNoModel,
};

const AcRemote kSanyoAc88Remote = {
    "sanyo_ac88",
    "sanyo",
    "Sanyo AC88",
    {10, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::SANYO_AC88,
    kAcNoModel,
};

const AcRemote kTeknopointGz055be1Remote = {
    "teknopoint_gz055be1",
    "teknopoint",
    "Teknopoint GZ055BE1",
    {16, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::TEKNOPOINT,
    static_cast<int16_t>(tcl_ac_remote_model_t::GZ055BE1),
};

const AcRemote kTechnibelAcRemote = {
    "technibel_ac",
    "technibel",
    "Technibel AC",
    {16, 31, true, true, false},
    &kIracRemoteClass,
    decode_type_t::TECHNIBEL_AC,
    kAcNoModel,
};

const AcRemote kTecoStandardRemote = {
    "teco_standard",
    "teco",
    "Teco standard",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::TECO,
    kAcNoModel,
};

const AcRemote kTrotecStandardRemote = {
    "trotec_standard",
    "trotec",
    "Trotec standard",
    {18, 32, true, false, false},
    &kIracRemoteClass,
    decode_type_t::TROTEC,
    kAcNoModel,
};

const AcRemote kTrotec3550Remote = {
    "trotec_3550",
    "trotec",
    "Trotec 3550",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::TROTEC_3550,
    kAcNoModel,
};

const AcRemote kTrumaStandardRemote = {
    "truma_standard",
    "truma",
    "Truma standard",
    {16, 31, true, false, false},
    &kIracRemoteClass,
    decode_type_t::TRUMA,
    kAcNoModel,
};

const AcRemote kVestelAcRemote = {
    "vestel_ac",
    "vestel",
    "Vestel AC",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::VESTEL_AC,
    kAcNoModel,
};

const AcRemote kVoltas122lzfRemote = {
    "voltas_122lzf",
    "voltas",
    "Voltas 122LZF",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::VOLTAS,
    static_cast<int16_t>(voltas_ac_remote_model_t::kVoltas122LZF),
};

const AcRemote kWhirlpoolDg11j13aRemote = {
    "whirlpool_dg11j13a",
    "whirlpool",
    "Whirlpool DG11J13A",
    {18, 32, true, true, false},
    &kIracRemoteClass,
    decode_type_t::WHIRLPOOL_AC,
    static_cast<int16_t>(whirlpool_ac_remote_model_t::DG11J13A),
};

const AcRemote kWhirlpoolDg11j191Remote = {
    "whirlpool_dg11j191",
    "whirlpool",
    "Whirlpool DG11J191",
    {16, 30, true, true, false},
    &kIracRemoteClass,
    decode_type_t::WHIRLPOOL_AC,
    static_cast<int16_t>(whirlpool_ac_remote_model_t::DG11J191),
};

const AcRemote kTranscoldStandardRemote = {
    "transcold_standard",
    "transcold",
    "Transcold standard",
    {18, 30, true, true, true},
    &kIracRemoteClass,
    decode_type_t::TRANSCOLD,
    kAcNoModel,
};
