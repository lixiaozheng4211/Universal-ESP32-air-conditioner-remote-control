#include "ac_catalog.h"

#include "remotes/gree_remotes.h"
#include "remotes/haier_remotes.h"
#include "remotes/midea_remotes.h"

namespace {

const AcCatalogNode kCatalog[] = {
    {"midea", "Midea", AcNodeKind::Brand, -1, 3, 1, nullptr},
    {"gree", "Gree", AcNodeKind::Brand, -1, 5, 2, nullptr},
    {"haier", "Haier", AcNodeKind::Brand, -1, 8, -1, nullptr},

    {kMideaStandardRemote.id, kMideaStandardRemote.name, AcNodeKind::Remote, 0,
     -1, 4, &kMideaStandardRemote},
    {kMideaRn02s13Remote.id, kMideaRn02s13Remote.name, AcNodeKind::Remote, 0,
     -1, -1, &kMideaRn02s13Remote},

    {kGreeYaw1fRemote.id, kGreeYaw1fRemote.name, AcNodeKind::Remote, 1, -1, 6,
     &kGreeYaw1fRemote},
    {kGreeYbofbRemote.id, kGreeYbofbRemote.name, AcNodeKind::Remote, 1, -1, 7,
     &kGreeYbofbRemote},
    {kGreeYx1fsfRemote.id, kGreeYx1fsfRemote.name, AcNodeKind::Remote, 1, -1,
     -1, &kGreeYx1fsfRemote},

    {kHaierAcRemote.id, kHaierAcRemote.name, AcNodeKind::Remote, 2, -1, 9,
     &kHaierAcRemote},
    {kHaierAc160Remote.id, kHaierAc160Remote.name, AcNodeKind::Remote, 2, -1,
     10, &kHaierAc160Remote},
    {kHaierAc176ARemote.id, kHaierAc176ARemote.name, AcNodeKind::Remote, 2,
     -1, 11, &kHaierAc176ARemote},
    {kHaierAc176BRemote.id, kHaierAc176BRemote.name, AcNodeKind::Remote, 2,
     -1, 12, &kHaierAc176BRemote},
    {kHaierYrw02Remote.id, kHaierYrw02Remote.name, AcNodeKind::Remote, 2, -1,
     -1, &kHaierYrw02Remote},
};

} // namespace

size_t acCatalogNodeCount() { return sizeof(kCatalog) / sizeof(kCatalog[0]); }

const AcCatalogNode &acCatalogNodeAt(size_t index) { return kCatalog[index]; }

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

uint8_t acRemoteCount() {
  uint8_t count = 0;
  for (size_t i = 0; i < acCatalogNodeCount(); ++i) {
    if (kCatalog[i].kind == AcNodeKind::Remote) {
      count++;
    }
  }
  return count;
}
