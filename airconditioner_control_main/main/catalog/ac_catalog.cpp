#include "ac_catalog.h"

#include "remotes/carrier_remotes.h"
#include "remotes/daikin_remotes.h"
#include "remotes/gree_remotes.h"
#include "remotes/haier_remotes.h"
#include "remotes/hitachi_remotes.h"
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

// Catalog layout:
// - brand nodes are listed first in the user-facing order.
// - remote nodes follow and point back to their brand by parent index.
// - firstChild/nextSibling indexes form a multi-child tree without heap memory.
// When adding a brand or remote, update these indexes together with the
// corresponding AcRemote declaration in remotes/.
const AcCatalogNode kCatalog[] = {
    {"midea", "Midea", AcNodeKind::Brand, -1, 15, 1, nullptr},
    {"gree", "Gree", AcNodeKind::Brand, -1, 17, 2, nullptr},
    {"haier", "Haier", AcNodeKind::Brand, -1, 20, 3, nullptr},
    {"tcl", "TCL", AcNodeKind::Brand, -1, 25, 4, nullptr},
    {"kelon", "Kelon", AcNodeKind::Brand, -1, 26, 5, nullptr},
    {"daikin", "Daikin", AcNodeKind::Brand, -1, 27, 6, nullptr},
    {"hitachi", "Hitachi", AcNodeKind::Brand, -1, 36, 7, nullptr},
    {"panasonic", "Panasonic", AcNodeKind::Brand, -1, 43, 8, nullptr},
    {"mitsubishi_electric", "Mitsubishi Electric", AcNodeKind::Brand, -1, 50,
     9, nullptr},
    {"mitsubishi_heavy", "Mitsubishi Heavy", AcNodeKind::Brand, -1, 53, 10,
     nullptr},
    {"toshiba", "Toshiba", AcNodeKind::Brand, -1, 55, 11, nullptr},
    {"sharp", "Sharp", AcNodeKind::Brand, -1, 56, 12, nullptr},
    {"samsung", "Samsung", AcNodeKind::Brand, -1, 59, 13, nullptr},
    {"lg", "LG", AcNodeKind::Brand, -1, 60, 14, nullptr},
    {"carrier", "Carrier", AcNodeKind::Brand, -1, 65, -1, nullptr},

    {kMideaStandardRemote.id, kMideaStandardRemote.name, AcNodeKind::Remote, 0,
     -1, 16, &kMideaStandardRemote},
    {kMideaRn02s13Remote.id, kMideaRn02s13Remote.name, AcNodeKind::Remote, 0,
     -1, -1, &kMideaRn02s13Remote},

    {kGreeYaw1fRemote.id, kGreeYaw1fRemote.name, AcNodeKind::Remote, 1, -1, 18,
     &kGreeYaw1fRemote},
    {kGreeYbofbRemote.id, kGreeYbofbRemote.name, AcNodeKind::Remote, 1, -1, 19,
     &kGreeYbofbRemote},
    {kGreeYx1fsfRemote.id, kGreeYx1fsfRemote.name, AcNodeKind::Remote, 1, -1,
     -1, &kGreeYx1fsfRemote},

    {kHaierAcRemote.id, kHaierAcRemote.name, AcNodeKind::Remote, 2, -1, 21,
     &kHaierAcRemote},
    {kHaierAc160Remote.id, kHaierAc160Remote.name, AcNodeKind::Remote, 2, -1,
     22, &kHaierAc160Remote},
    {kHaierAc176ARemote.id, kHaierAc176ARemote.name, AcNodeKind::Remote, 2,
     -1, 23, &kHaierAc176ARemote},
    {kHaierAc176BRemote.id, kHaierAc176BRemote.name, AcNodeKind::Remote, 2,
     -1, 24, &kHaierAc176BRemote},
    {kHaierYrw02Remote.id, kHaierYrw02Remote.name, AcNodeKind::Remote, 2, -1,
     -1, &kHaierYrw02Remote},

    {kTclTac09chsdRemote.id, kTclTac09chsdRemote.name, AcNodeKind::Remote, 3,
     -1, -1, &kTclTac09chsdRemote},

    {kKelonStandardRemote.id, kKelonStandardRemote.name, AcNodeKind::Remote, 4,
     -1, -1, &kKelonStandardRemote},

    {kDaikinArc433Remote.id, kDaikinArc433Remote.name, AcNodeKind::Remote, 5,
     -1, 28, &kDaikinArc433Remote},
    {kDaikinArc477Remote.id, kDaikinArc477Remote.name, AcNodeKind::Remote, 5,
     -1, 29, &kDaikinArc477Remote},
    {kDaikin216Remote.id, kDaikin216Remote.name, AcNodeKind::Remote, 5, -1, 30,
     &kDaikin216Remote},
    {kDaikin160Remote.id, kDaikin160Remote.name, AcNodeKind::Remote, 5, -1, 31,
     &kDaikin160Remote},
    {kDaikin176Remote.id, kDaikin176Remote.name, AcNodeKind::Remote, 5, -1, 32,
     &kDaikin176Remote},
    {kDaikin128Remote.id, kDaikin128Remote.name, AcNodeKind::Remote, 5, -1, 33,
     &kDaikin128Remote},
    {kDaikin152Remote.id, kDaikin152Remote.name, AcNodeKind::Remote, 5, -1, 34,
     &kDaikin152Remote},
    {kDaikin64Remote.id, kDaikin64Remote.name, AcNodeKind::Remote, 5, -1, 35,
     &kDaikin64Remote},
    {kDaikin312Remote.id, kDaikin312Remote.name, AcNodeKind::Remote, 5, -1, -1,
     &kDaikin312Remote},

    {kHitachiAcRemote.id, kHitachiAcRemote.name, AcNodeKind::Remote, 6, -1, 37,
     &kHitachiAcRemote},
    {kHitachiAc1ARemote.id, kHitachiAc1ARemote.name, AcNodeKind::Remote, 6, -1,
     38, &kHitachiAc1ARemote},
    {kHitachiAc1BRemote.id, kHitachiAc1BRemote.name, AcNodeKind::Remote, 6, -1,
     39, &kHitachiAc1BRemote},
    {kHitachiAc264Remote.id, kHitachiAc264Remote.name, AcNodeKind::Remote, 6,
     -1, 40, &kHitachiAc264Remote},
    {kHitachiAc296Remote.id, kHitachiAc296Remote.name, AcNodeKind::Remote, 6,
     -1, 41, &kHitachiAc296Remote},
    {kHitachiAc344Remote.id, kHitachiAc344Remote.name, AcNodeKind::Remote, 6,
     -1, 42, &kHitachiAc344Remote},
    {kHitachiAc424Remote.id, kHitachiAc424Remote.name, AcNodeKind::Remote, 6,
     -1, -1, &kHitachiAc424Remote},

    {kPanasonicLkeRemote.id, kPanasonicLkeRemote.name, AcNodeKind::Remote, 7,
     -1, 44, &kPanasonicLkeRemote},
    {kPanasonicNkeRemote.id, kPanasonicNkeRemote.name, AcNodeKind::Remote, 7,
     -1, 45, &kPanasonicNkeRemote},
    {kPanasonicDkeRemote.id, kPanasonicDkeRemote.name, AcNodeKind::Remote, 7,
     -1, 46, &kPanasonicDkeRemote},
    {kPanasonicJkeRemote.id, kPanasonicJkeRemote.name, AcNodeKind::Remote, 7,
     -1, 47, &kPanasonicJkeRemote},
    {kPanasonicCkpRemote.id, kPanasonicCkpRemote.name, AcNodeKind::Remote, 7,
     -1, 48, &kPanasonicCkpRemote},
    {kPanasonicRkrRemote.id, kPanasonicRkrRemote.name, AcNodeKind::Remote, 7,
     -1, 49, &kPanasonicRkrRemote},
    {kPanasonicAc32Remote.id, kPanasonicAc32Remote.name, AcNodeKind::Remote, 7,
     -1, -1, &kPanasonicAc32Remote},

    {kMitsubishiAcRemote.id, kMitsubishiAcRemote.name, AcNodeKind::Remote, 8,
     -1, 51, &kMitsubishiAcRemote},
    {kMitsubishi112Remote.id, kMitsubishi112Remote.name, AcNodeKind::Remote, 8,
     -1, 52, &kMitsubishi112Remote},
    {kMitsubishi136Remote.id, kMitsubishi136Remote.name, AcNodeKind::Remote, 8,
     -1, -1, &kMitsubishi136Remote},

    {kMitsubishiHeavy88Remote.id, kMitsubishiHeavy88Remote.name,
     AcNodeKind::Remote, 9, -1, 54, &kMitsubishiHeavy88Remote},
    {kMitsubishiHeavy152Remote.id, kMitsubishiHeavy152Remote.name,
     AcNodeKind::Remote, 9, -1, -1, &kMitsubishiHeavy152Remote},

    {kToshibaAcRemote.id, kToshibaAcRemote.name, AcNodeKind::Remote, 10, -1,
     -1, &kToshibaAcRemote},

    {kSharpA907Remote.id, kSharpA907Remote.name, AcNodeKind::Remote, 11, -1, 57,
     &kSharpA907Remote},
    {kSharpA705Remote.id, kSharpA705Remote.name, AcNodeKind::Remote, 11, -1, 58,
     &kSharpA705Remote},
    {kSharpA903Remote.id, kSharpA903Remote.name, AcNodeKind::Remote, 11, -1, -1,
     &kSharpA903Remote},

    {kSamsungAcRemote.id, kSamsungAcRemote.name, AcNodeKind::Remote, 12, -1,
     -1, &kSamsungAcRemote},

    {kLgGe6711Remote.id, kLgGe6711Remote.name, AcNodeKind::Remote, 13, -1, 61,
     &kLgGe6711Remote},
    {kLgAkb752Remote.id, kLgAkb752Remote.name, AcNodeKind::Remote, 13, -1, 62,
     &kLgAkb752Remote},
    {kLgAkb749Remote.id, kLgAkb749Remote.name, AcNodeKind::Remote, 13, -1, 63,
     &kLgAkb749Remote},
    {kLgAkb737Remote.id, kLgAkb737Remote.name, AcNodeKind::Remote, 13, -1, 64,
     &kLgAkb737Remote},
    {kLg6711Remote.id, kLg6711Remote.name, AcNodeKind::Remote, 13, -1, -1,
     &kLg6711Remote},

    {kCarrierAc64Remote.id, kCarrierAc64Remote.name, AcNodeKind::Remote, 14,
     -1, -1, &kCarrierAc64Remote},
};

} // namespace

size_t acCatalogNodeCount() { return sizeof(kCatalog) / sizeof(kCatalog[0]); }

const AcCatalogNode &acCatalogNodeAt(size_t index) { return kCatalog[index]; }

// Host commands use stable remote ids, so lookup is linear over the small
// catalog. This keeps code size low and avoids maintaining a separate map.
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

// Count only usable remote candidates; brand nodes are structure only.
uint8_t acRemoteCount() {
  uint8_t count = 0;
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    if (kCatalog[i].kind == AcNodeKind::Remote) {
      count++;
    }
  }
  return count;
}
