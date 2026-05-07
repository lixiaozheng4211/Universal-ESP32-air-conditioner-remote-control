#include "ac_catalog.h"

#include "remotes/aux_remotes.h"
#include "remotes/carrier_remotes.h"
#include "remotes/daikin_remotes.h"
#include "remotes/gree_remotes.h"
#include "remotes/haier_remotes.h"
#include "remotes/hitachi_remotes.h"
#include "remotes/irac_extra_remotes.h"
#include "remotes/kelon_remotes.h"
#include "remotes/lg_remotes.h"
#include "remotes/midea_remotes.h"
#include "remotes/mitsubishi_remotes.h"
#include "remotes/panasonic_remotes.h"
#include "remotes/samsung_remotes.h"
#include "remotes/sharp_remotes.h"
#include "remotes/tcl_remotes.h"
#include "remotes/toshiba_remotes.h"

namespace {

// 目录布局设计：
// - 核心品牌节点按用户界面展示顺序放在前面，保证 Qt/Android 看到的顺序稳定。
// - 后续扩展品牌追加在数组末尾，用 nextSibling 继续接到品牌链上，避免重排旧 remote id。
// - 遥控器节点通过 parent 下标指回所属品牌。
// - firstChild/nextSibling 下标组成“父子 + 兄弟链表”，逻辑上是多叉树。
// - 不使用动态链表或 vector，是为了避免 ESP32 长期运行时出现堆碎片，也方便放进只读数据区。
//
// 维护注意：
// 新增品牌或遥控器时，需要同时更新 firstChild/nextSibling/parent 下标，
// 并在 remotes/ 下声明对应 AcRemote。目录只放 IRac 已能直接发送或已有特殊后端的候选。
const AcCatalogNode kCatalog[] = {
    {"midea", "Midea", AcNodeKind::Brand, -1, 16, 1, nullptr},
    {"gree", "Gree", AcNodeKind::Brand, -1, 18, 2, nullptr},
    {"haier", "Haier", AcNodeKind::Brand, -1, 21, 3, nullptr},
    {"tcl", "TCL", AcNodeKind::Brand, -1, 26, 4, nullptr},
    {"kelon", "Kelon", AcNodeKind::Brand, -1, 27, 5, nullptr},
    {"daikin", "Daikin", AcNodeKind::Brand, -1, 28, 6, nullptr},
    {"hitachi", "Hitachi", AcNodeKind::Brand, -1, 37, 7, nullptr},
    {"panasonic", "Panasonic", AcNodeKind::Brand, -1, 44, 8, nullptr},
    {"mitsubishi_electric", "Mitsubishi Electric", AcNodeKind::Brand, -1, 51,
     9, nullptr},
    {"mitsubishi_heavy", "Mitsubishi Heavy", AcNodeKind::Brand, -1, 54, 10,
     nullptr},
    {"toshiba", "Toshiba", AcNodeKind::Brand, -1, 56, 11, nullptr},
    {"sharp", "Sharp", AcNodeKind::Brand, -1, 57, 12, nullptr},
    {"samsung", "Samsung", AcNodeKind::Brand, -1, 60, 13, nullptr},
    {"lg", "LG", AcNodeKind::Brand, -1, 61, 14, nullptr},
    {"carrier", "Carrier", AcNodeKind::Brand, -1, 66, 15, nullptr},
    {"aux", "AUX", AcNodeKind::Brand, -1, 67, 68, nullptr},

    {kMideaStandardRemote.id, kMideaStandardRemote.name, AcNodeKind::Remote, 0,
     -1, 17, &kMideaStandardRemote},
    {kMideaRn02s13Remote.id, kMideaRn02s13Remote.name, AcNodeKind::Remote, 0,
     -1, -1, &kMideaRn02s13Remote},

    {kGreeYaw1fRemote.id, kGreeYaw1fRemote.name, AcNodeKind::Remote, 1, -1, 19,
     &kGreeYaw1fRemote},
    {kGreeYbofbRemote.id, kGreeYbofbRemote.name, AcNodeKind::Remote, 1, -1, 20,
     &kGreeYbofbRemote},
    {kGreeYx1fsfRemote.id, kGreeYx1fsfRemote.name, AcNodeKind::Remote, 1, -1,
     -1, &kGreeYx1fsfRemote},

    {kHaierAcRemote.id, kHaierAcRemote.name, AcNodeKind::Remote, 2, -1, 22,
     &kHaierAcRemote},
    {kHaierAc160Remote.id, kHaierAc160Remote.name, AcNodeKind::Remote, 2, -1,
     23, &kHaierAc160Remote},
    {kHaierAc176ARemote.id, kHaierAc176ARemote.name, AcNodeKind::Remote, 2,
     -1, 24, &kHaierAc176ARemote},
    {kHaierAc176BRemote.id, kHaierAc176BRemote.name, AcNodeKind::Remote, 2,
     -1, 25, &kHaierAc176BRemote},
    {kHaierYrw02Remote.id, kHaierYrw02Remote.name, AcNodeKind::Remote, 2, -1,
     -1, &kHaierYrw02Remote},

    {kTclTac09chsdRemote.id, kTclTac09chsdRemote.name, AcNodeKind::Remote, 3,
     -1, -1, &kTclTac09chsdRemote},

    {kKelonStandardRemote.id, kKelonStandardRemote.name, AcNodeKind::Remote, 4,
     -1, -1, &kKelonStandardRemote},

    {kDaikinArc433Remote.id, kDaikinArc433Remote.name, AcNodeKind::Remote, 5,
     -1, 29, &kDaikinArc433Remote},
    {kDaikinArc477Remote.id, kDaikinArc477Remote.name, AcNodeKind::Remote, 5,
     -1, 30, &kDaikinArc477Remote},
    {kDaikin216Remote.id, kDaikin216Remote.name, AcNodeKind::Remote, 5, -1, 31,
     &kDaikin216Remote},
    {kDaikin160Remote.id, kDaikin160Remote.name, AcNodeKind::Remote, 5, -1, 32,
     &kDaikin160Remote},
    {kDaikin176Remote.id, kDaikin176Remote.name, AcNodeKind::Remote, 5, -1, 33,
     &kDaikin176Remote},
    {kDaikin128Remote.id, kDaikin128Remote.name, AcNodeKind::Remote, 5, -1, 34,
     &kDaikin128Remote},
    {kDaikin152Remote.id, kDaikin152Remote.name, AcNodeKind::Remote, 5, -1, 35,
     &kDaikin152Remote},
    {kDaikin64Remote.id, kDaikin64Remote.name, AcNodeKind::Remote, 5, -1, 36,
     &kDaikin64Remote},
    {kDaikin312Remote.id, kDaikin312Remote.name, AcNodeKind::Remote, 5, -1, -1,
     &kDaikin312Remote},

    {kHitachiAcRemote.id, kHitachiAcRemote.name, AcNodeKind::Remote, 6, -1, 38,
     &kHitachiAcRemote},
    {kHitachiAc1ARemote.id, kHitachiAc1ARemote.name, AcNodeKind::Remote, 6, -1,
     39, &kHitachiAc1ARemote},
    {kHitachiAc1BRemote.id, kHitachiAc1BRemote.name, AcNodeKind::Remote, 6, -1,
     40, &kHitachiAc1BRemote},
    {kHitachiAc264Remote.id, kHitachiAc264Remote.name, AcNodeKind::Remote, 6,
     -1, 41, &kHitachiAc264Remote},
    {kHitachiAc296Remote.id, kHitachiAc296Remote.name, AcNodeKind::Remote, 6,
     -1, 42, &kHitachiAc296Remote},
    {kHitachiAc344Remote.id, kHitachiAc344Remote.name, AcNodeKind::Remote, 6,
     -1, 43, &kHitachiAc344Remote},
    {kHitachiAc424Remote.id, kHitachiAc424Remote.name, AcNodeKind::Remote, 6,
     -1, -1, &kHitachiAc424Remote},

    {kPanasonicLkeRemote.id, kPanasonicLkeRemote.name, AcNodeKind::Remote, 7,
     -1, 45, &kPanasonicLkeRemote},
    {kPanasonicNkeRemote.id, kPanasonicNkeRemote.name, AcNodeKind::Remote, 7,
     -1, 46, &kPanasonicNkeRemote},
    {kPanasonicDkeRemote.id, kPanasonicDkeRemote.name, AcNodeKind::Remote, 7,
     -1, 47, &kPanasonicDkeRemote},
    {kPanasonicJkeRemote.id, kPanasonicJkeRemote.name, AcNodeKind::Remote, 7,
     -1, 48, &kPanasonicJkeRemote},
    {kPanasonicCkpRemote.id, kPanasonicCkpRemote.name, AcNodeKind::Remote, 7,
     -1, 49, &kPanasonicCkpRemote},
    {kPanasonicRkrRemote.id, kPanasonicRkrRemote.name, AcNodeKind::Remote, 7,
     -1, 50, &kPanasonicRkrRemote},
    {kPanasonicAc32Remote.id, kPanasonicAc32Remote.name, AcNodeKind::Remote, 7,
     -1, -1, &kPanasonicAc32Remote},

    {kMitsubishiAcRemote.id, kMitsubishiAcRemote.name, AcNodeKind::Remote, 8,
     -1, 52, &kMitsubishiAcRemote},
    {kMitsubishi112Remote.id, kMitsubishi112Remote.name, AcNodeKind::Remote, 8,
     -1, 53, &kMitsubishi112Remote},
    {kMitsubishi136Remote.id, kMitsubishi136Remote.name, AcNodeKind::Remote, 8,
     -1, -1, &kMitsubishi136Remote},

    {kMitsubishiHeavy88Remote.id, kMitsubishiHeavy88Remote.name,
     AcNodeKind::Remote, 9, -1, 55, &kMitsubishiHeavy88Remote},
    {kMitsubishiHeavy152Remote.id, kMitsubishiHeavy152Remote.name,
     AcNodeKind::Remote, 9, -1, -1, &kMitsubishiHeavy152Remote},

    {kToshibaAcRemote.id, kToshibaAcRemote.name, AcNodeKind::Remote, 10, -1,
     -1, &kToshibaAcRemote},

    {kSharpA907Remote.id, kSharpA907Remote.name, AcNodeKind::Remote, 11, -1, 58,
     &kSharpA907Remote},
    {kSharpA705Remote.id, kSharpA705Remote.name, AcNodeKind::Remote, 11, -1, 59,
     &kSharpA705Remote},
    {kSharpA903Remote.id, kSharpA903Remote.name, AcNodeKind::Remote, 11, -1, -1,
     &kSharpA903Remote},

    {kSamsungAcRemote.id, kSamsungAcRemote.name, AcNodeKind::Remote, 12, -1,
     -1, &kSamsungAcRemote},

    {kLgGe6711Remote.id, kLgGe6711Remote.name, AcNodeKind::Remote, 13, -1, 62,
     &kLgGe6711Remote},
    {kLgAkb752Remote.id, kLgAkb752Remote.name, AcNodeKind::Remote, 13, -1, 63,
     &kLgAkb752Remote},
    {kLgAkb749Remote.id, kLgAkb749Remote.name, AcNodeKind::Remote, 13, -1, 64,
     &kLgAkb749Remote},
    {kLgAkb737Remote.id, kLgAkb737Remote.name, AcNodeKind::Remote, 13, -1, 65,
     &kLgAkb737Remote},
    {kLg6711Remote.id, kLg6711Remote.name, AcNodeKind::Remote, 13, -1, -1,
     &kLg6711Remote},

    {kCarrierAc64Remote.id, kCarrierAc64Remote.name, AcNodeKind::Remote, 14,
     -1, -1, &kCarrierAc64Remote},

    {kAuxElectraRemote.id, kAuxElectraRemote.name, AcNodeKind::Remote, 15, -1,
     -1, &kAuxElectraRemote},

    {"airton", "Airton", AcNodeKind::Brand, -1, 69, 70, nullptr},
    {kAirtonStandardRemote.id, kAirtonStandardRemote.name, AcNodeKind::Remote,
     68, -1, -1, &kAirtonStandardRemote},

    {"airwell", "Airwell", AcNodeKind::Brand, -1, 71, 72, nullptr},
    {kAirwellStandardRemote.id, kAirwellStandardRemote.name,
     AcNodeKind::Remote, 70, -1, -1, &kAirwellStandardRemote},

    {"amcor", "Amcor", AcNodeKind::Brand, -1, 73, 74, nullptr},
    {kAmcorStandardRemote.id, kAmcorStandardRemote.name, AcNodeKind::Remote,
     72, -1, -1, &kAmcorStandardRemote},

    {"argo", "Argo", AcNodeKind::Brand, -1, 75, 77, nullptr},
    {kArgoWrem2Remote.id, kArgoWrem2Remote.name, AcNodeKind::Remote, 74, -1,
     76, &kArgoWrem2Remote},
    {kArgoWrem3Remote.id, kArgoWrem3Remote.name, AcNodeKind::Remote, 74, -1,
     -1, &kArgoWrem3Remote},

    {"bosch", "Bosch", AcNodeKind::Brand, -1, 78, 79, nullptr},
    {kBosch144Remote.id, kBosch144Remote.name, AcNodeKind::Remote, 77, -1, -1,
     &kBosch144Remote},

    {"coolix", "Coolix", AcNodeKind::Brand, -1, 80, 81, nullptr},
    {kCoolixStandardRemote.id, kCoolixStandardRemote.name, AcNodeKind::Remote,
     79, -1, -1, &kCoolixStandardRemote},

    {"corona", "Corona", AcNodeKind::Brand, -1, 82, 83, nullptr},
    {kCoronaAcRemote.id, kCoronaAcRemote.name, AcNodeKind::Remote, 81, -1, -1,
     &kCoronaAcRemote},

    {"delonghi", "Delonghi", AcNodeKind::Brand, -1, 84, 85, nullptr},
    {kDelonghiAcRemote.id, kDelonghiAcRemote.name, AcNodeKind::Remote, 83, -1,
     -1, &kDelonghiAcRemote},

    {"ecoclim", "Ecoclim", AcNodeKind::Brand, -1, 86, 87, nullptr},
    {kEcoclimStandardRemote.id, kEcoclimStandardRemote.name,
     AcNodeKind::Remote, 85, -1, -1, &kEcoclimStandardRemote},

    {"eurom", "Eurom", AcNodeKind::Brand, -1, 88, 89, nullptr},
    {kEuromStandardRemote.id, kEuromStandardRemote.name, AcNodeKind::Remote,
     87, -1, -1, &kEuromStandardRemote},

    {"fujitsu", "Fujitsu", AcNodeKind::Brand, -1, 90, 96, nullptr},
    {kFujitsuArRah2eRemote.id, kFujitsuArRah2eRemote.name,
     AcNodeKind::Remote, 89, -1, 91, &kFujitsuArRah2eRemote},
    {kFujitsuArDb1Remote.id, kFujitsuArDb1Remote.name, AcNodeKind::Remote, 89,
     -1, 92, &kFujitsuArDb1Remote},
    {kFujitsuArReb1eRemote.id, kFujitsuArReb1eRemote.name,
     AcNodeKind::Remote, 89, -1, 93, &kFujitsuArReb1eRemote},
    {kFujitsuArJw2Remote.id, kFujitsuArJw2Remote.name, AcNodeKind::Remote, 89,
     -1, 94, &kFujitsuArJw2Remote},
    {kFujitsuArRy4Remote.id, kFujitsuArRy4Remote.name, AcNodeKind::Remote, 89,
     -1, 95, &kFujitsuArRy4Remote},
    {kFujitsuArRew4eRemote.id, kFujitsuArRew4eRemote.name,
     AcNodeKind::Remote, 89, -1, -1, &kFujitsuArRew4eRemote},

    {"goodweather", "Goodweather", AcNodeKind::Brand, -1, 97, 98, nullptr},
    {kGoodweatherStandardRemote.id, kGoodweatherStandardRemote.name,
     AcNodeKind::Remote, 96, -1, -1, &kGoodweatherStandardRemote},

    {"kelvinator", "Kelvinator", AcNodeKind::Brand, -1, 99, 100, nullptr},
    {kKelvinatorStandardRemote.id, kKelvinatorStandardRemote.name,
     AcNodeKind::Remote, 98, -1, -1, &kKelvinatorStandardRemote},

    {"mirage", "Mirage", AcNodeKind::Brand, -1, 101, 103, nullptr},
    {kMirageKkg9ac1Remote.id, kMirageKkg9ac1Remote.name, AcNodeKind::Remote,
     100, -1, 102, &kMirageKkg9ac1Remote},
    {kMirageKkg29ac1Remote.id, kMirageKkg29ac1Remote.name, AcNodeKind::Remote,
     100, -1, -1, &kMirageKkg29ac1Remote},

    {"neoclima", "Neoclima", AcNodeKind::Brand, -1, 104, 105, nullptr},
    {kNeoclimaStandardRemote.id, kNeoclimaStandardRemote.name,
     AcNodeKind::Remote, 103, -1, -1, &kNeoclimaStandardRemote},

    {"rhoss", "Rhoss", AcNodeKind::Brand, -1, 106, 107, nullptr},
    {kRhossStandardRemote.id, kRhossStandardRemote.name, AcNodeKind::Remote,
     105, -1, -1, &kRhossStandardRemote},

    {"sanyo", "Sanyo", AcNodeKind::Brand, -1, 108, 110, nullptr},
    {kSanyoAcRemote.id, kSanyoAcRemote.name, AcNodeKind::Remote, 107, -1, 109,
     &kSanyoAcRemote},
    {kSanyoAc88Remote.id, kSanyoAc88Remote.name, AcNodeKind::Remote, 107, -1,
     -1, &kSanyoAc88Remote},

    {"teknopoint", "Teknopoint", AcNodeKind::Brand, -1, 111, 112, nullptr},
    {kTeknopointGz055be1Remote.id, kTeknopointGz055be1Remote.name,
     AcNodeKind::Remote, 110, -1, -1, &kTeknopointGz055be1Remote},

    {"technibel", "Technibel", AcNodeKind::Brand, -1, 113, 114, nullptr},
    {kTechnibelAcRemote.id, kTechnibelAcRemote.name, AcNodeKind::Remote, 112,
     -1, -1, &kTechnibelAcRemote},

    {"teco", "Teco", AcNodeKind::Brand, -1, 115, 116, nullptr},
    {kTecoStandardRemote.id, kTecoStandardRemote.name, AcNodeKind::Remote, 114,
     -1, -1, &kTecoStandardRemote},

    {"trotec", "Trotec", AcNodeKind::Brand, -1, 117, 119, nullptr},
    {kTrotecStandardRemote.id, kTrotecStandardRemote.name, AcNodeKind::Remote,
     116, -1, 118, &kTrotecStandardRemote},
    {kTrotec3550Remote.id, kTrotec3550Remote.name, AcNodeKind::Remote, 116, -1,
     -1, &kTrotec3550Remote},

    {"truma", "Truma", AcNodeKind::Brand, -1, 120, 121, nullptr},
    {kTrumaStandardRemote.id, kTrumaStandardRemote.name, AcNodeKind::Remote,
     119, -1, -1, &kTrumaStandardRemote},

    {"vestel", "Vestel", AcNodeKind::Brand, -1, 122, 123, nullptr},
    {kVestelAcRemote.id, kVestelAcRemote.name, AcNodeKind::Remote, 121, -1, -1,
     &kVestelAcRemote},

    {"voltas", "Voltas", AcNodeKind::Brand, -1, 124, 125, nullptr},
    {kVoltas122lzfRemote.id, kVoltas122lzfRemote.name, AcNodeKind::Remote, 123,
     -1, -1, &kVoltas122lzfRemote},

    {"whirlpool", "Whirlpool", AcNodeKind::Brand, -1, 126, 128, nullptr},
    {kWhirlpoolDg11j13aRemote.id, kWhirlpoolDg11j13aRemote.name,
     AcNodeKind::Remote, 125, -1, 127, &kWhirlpoolDg11j13aRemote},
    {kWhirlpoolDg11j191Remote.id, kWhirlpoolDg11j191Remote.name,
     AcNodeKind::Remote, 125, -1, -1, &kWhirlpoolDg11j191Remote},

    {"transcold", "Transcold", AcNodeKind::Brand, -1, 129, -1, nullptr},
    {kTranscoldStandardRemote.id, kTranscoldStandardRemote.name,
     AcNodeKind::Remote, 128, -1, -1, &kTranscoldStandardRemote},
};

} // 命名空间

// 目录大小由静态数组推导，避免手动维护数量常量导致漏改。
size_t acCatalogNodeCount() { return sizeof(kCatalog) / sizeof(kCatalog[0]); }

// 返回 const 引用，外部只能读取节点，不能破坏目录树下标关系。
const AcCatalogNode &acCatalogNodeAt(size_t index) { return kCatalog[index]; }

// 上位机命令使用稳定的 remote id。目录规模很小，
// 线性查找可以减少代码量，也不用维护额外映射表。
// 这里查找发生在收到 AC 命令时，不是高频循环，所以 O(n) 成本可以接受。
const AcRemote *acFindRemoteById(const String &id) {
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    const AcCatalogNode &node = kCatalog[i];
    if (node.kind == AcNodeKind::Remote && node.remote != nullptr &&
        id.equals(node.remote->id)) {
      return node.remote;
    }
  }
  return nullptr;
}

// 只统计真正可用的遥控器候选，品牌节点只负责组织结构。
// CATALOG 开头返回这个数量，方便上位机判断目录是否完整读完。
uint8_t acRemoteCount() {
  uint8_t count = 0;
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    if (kCatalog[i].kind == AcNodeKind::Remote) {
      count++;
    }
  }
  return count;
}
