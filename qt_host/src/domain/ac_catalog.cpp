#include "ac_catalog.h"

#include <QHash>
#include <QRegularExpression>

namespace {

AcRemote remote(const QString& id, const QString& brandId, const QString& name,
                int minTemp = 16, int maxTemp = 30, bool supportsFan = true,
                bool supportsSwingV = true, bool supportsSwingH = false) {
  return {id, brandId, name, minTemp, maxTemp, supportsFan, supportsSwingV,
          supportsSwingH};
}

QString localizedBrandName(const QString& id, const QString& fallback) {
  static const QHash<QString, QString> names = {
      {"midea", QStringLiteral("美的")},
      {"gree", QStringLiteral("格力")},
      {"haier", QStringLiteral("海尔")},
      {"tcl", QStringLiteral("TCL")},
      {"kelon", QStringLiteral("科龙")},
      {"daikin", QStringLiteral("大金")},
      {"hitachi", QStringLiteral("日立")},
      {"panasonic", QStringLiteral("松下")},
      {"mitsubishi_electric", QStringLiteral("三菱电机")},
      {"mitsubishi_heavy", QStringLiteral("三菱重工")},
      {"toshiba", QStringLiteral("东芝")},
      {"sharp", QStringLiteral("夏普")},
      {"samsung", QStringLiteral("三星")},
      {"lg", QStringLiteral("LG")},
      {"carrier", QStringLiteral("开利")},
      {"aux", QStringLiteral("奥克斯")},
  };
  return names.value(id, fallback.isEmpty() ? id : fallback);
}

QString catalogField(const QString& line, const QString& key) {
  const QRegularExpression re(
      QStringLiteral("(?:^|\\s)%1=(\"[^\"]*\"|\\S+)").arg(key));
  const auto match = re.match(line);
  if (!match.hasMatch()) {
    return {};
  }

  QString value = match.captured(1);
  if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"')) {
    value = value.mid(1, value.size() - 2);
  }
  return value;
}

bool catalogBool(const QString& line, const QString& key,
                 bool defaultValue = false) {
  const QString value = catalogField(line, key);
  if (value.isEmpty()) {
    return defaultValue;
  }
  return value == QStringLiteral("1") || value == QStringLiteral("true") ||
         value == QStringLiteral("on");
}

void parseTempRange(const QString& line, int* minTemp, int* maxTemp) {
  const QString range = catalogField(line, QStringLiteral("temp"));
  const QStringList parts = range.split('-');
  if (parts.size() != 2) {
    return;
  }

  bool minOk = false;
  bool maxOk = false;
  const int parsedMin = parts[0].toInt(&minOk);
  const int parsedMax = parts[1].toInt(&maxOk);
  if (minOk && maxOk && parsedMin <= parsedMax) {
    *minTemp = parsedMin;
    *maxTemp = parsedMax;
  }
}

}  // namespace

QVector<AcBrand> defaultAcCatalog() {
  // 这里是 Qt 侧当前内置的遥控器目录。
  // 注意：remote.id 必须和 ESP32 固件里的 AcRemote.id 完全一致，否则固件会返回
  // UNKNOWN_REMOTE。连接 ESP32 后会优先用 CATALOG 刷新这里的 fallback 目录。
  return {
      {"midea",
       "美的",
       {
           remote("midea_standard", "midea", "Midea standard", 17, 30, true,
                  true, false),
           remote("midea_rn02s13", "midea", "Midea RN02S13", 17, 30, true,
                  true, true),
       }},
      {"gree",
       "格力",
       {
           remote("gree_yaw1f", "gree", "Gree YAW1F", 16, 30, true, true,
                  true),
           remote("gree_ybofb", "gree", "Gree YBOFB", 16, 30, true, true,
                  true),
           remote("gree_yx1fsf", "gree", "Gree YX1FSF", 16, 30, true, true,
                  true),
       }},
      {"haier",
       "海尔",
       {
           remote("haier_ac", "haier", "Haier HSU07", 16, 30, true, true,
                  false),
           remote("haier_ac160", "haier", "Haier AC160", 16, 30, true, true,
                  false),
           remote("haier_ac176_a", "haier", "Haier AC176 V9014557-A", 16, 30,
                  true, true, true),
           remote("haier_ac176_b", "haier", "Haier AC176 V9014557-B", 16, 30,
                  true, true, true),
           remote("haier_yrw02", "haier", "Haier YR-W02", 16, 30, true, true,
                  false),
       }},
      {"tcl", "TCL", {remote("tcl_tac09chsd", "tcl", "TCL TAC09CHSD", 16, 30,
                              true, true, true)}},
      {"kelon", "科龙", {remote("kelon_standard", "kelon", "Kelon standard")}},
      {"daikin",
       "大金",
       {
           remote("daikin_arc433", "daikin", "Daikin ARC433", 16, 30, true,
                  true, true),
           remote("daikin_arc477", "daikin", "Daikin ARC477A1", 16, 30, true,
                  true, true),
           remote("daikin_216", "daikin", "Daikin 216", 16, 30, true, true,
                  true),
           remote("daikin_160", "daikin", "Daikin 160"),
           remote("daikin_176", "daikin", "Daikin 176", 16, 30, true, false,
                  true),
           remote("daikin_128", "daikin", "Daikin 128"),
           remote("daikin_152", "daikin", "Daikin 152"),
           remote("daikin_64", "daikin", "Daikin 64"),
           remote("daikin_312", "daikin", "Daikin 312", 16, 30, true, true,
                  true),
       }},
      {"hitachi",
       "日立",
       {
           remote("hitachi_ac", "hitachi", "Hitachi AC", 16, 30, true, true,
                  true),
           remote("hitachi_ac1_a", "hitachi", "Hitachi AC1 R-LT0541-HTA A",
                  16, 30, true, true, true),
           remote("hitachi_ac1_b", "hitachi", "Hitachi AC1 R-LT0541-HTA B",
                  16, 30, true, true, true),
           remote("hitachi_ac264", "hitachi", "Hitachi AC264", 16, 30, true,
                  false, false),
           remote("hitachi_ac296", "hitachi", "Hitachi AC296", 16, 30, true,
                  false, false),
           remote("hitachi_ac344", "hitachi", "Hitachi AC344", 16, 30, true,
                  true, true),
           remote("hitachi_ac424", "hitachi", "Hitachi AC424"),
       }},
      {"panasonic",
       "松下",
       {
           remote("panasonic_lke", "panasonic", "Panasonic LKE", 16, 30, true,
                  true, true),
           remote("panasonic_nke", "panasonic", "Panasonic NKE", 16, 30, true,
                  true, true),
           remote("panasonic_dke", "panasonic", "Panasonic DKE/PKR", 16, 30,
                  true, true, true),
           remote("panasonic_jke", "panasonic", "Panasonic JKE", 16, 30, true,
                  true, true),
           remote("panasonic_ckp", "panasonic", "Panasonic CKP", 16, 30, true,
                  true, true),
           remote("panasonic_rkr", "panasonic", "Panasonic RKR", 16, 30, true,
                  true, true),
           remote("panasonic_ac32", "panasonic", "Panasonic AC32", 16, 30,
                  true, true, true),
       }},
      {"mitsubishi_electric",
       "三菱电机",
       {
           remote("mitsubishi_ac", "mitsubishi_electric",
                  "Mitsubishi Electric AC", 16, 30, true, true, true),
           remote("mitsubishi112", "mitsubishi_electric",
                  "Mitsubishi Electric 112", 16, 30, true, true, true),
           remote("mitsubishi136", "mitsubishi_electric",
                  "Mitsubishi Electric 136"),
       }},
      {"mitsubishi_heavy",
       "三菱重工",
       {
           remote("mitsubishi_heavy_88", "mitsubishi_heavy",
                  "Mitsubishi Heavy 88", 16, 30, true, true, true),
           remote("mitsubishi_heavy_152", "mitsubishi_heavy",
                  "Mitsubishi Heavy 152", 16, 30, true, true, true),
       }},
      {"toshiba", "东芝", {remote("toshiba_ac", "toshiba", "Toshiba AC")}},
      {"sharp",
       "夏普",
       {
           remote("sharp_a907", "sharp", "Sharp A907"),
           remote("sharp_a705", "sharp", "Sharp A705"),
           remote("sharp_a903", "sharp", "Sharp A903"),
       }},
      {"samsung",
       "三星",
       {remote("samsung_ac", "samsung", "Samsung AC", 16, 30, true, true,
               true)}},
      {"lg",
       "LG",
       {
           remote("lg_ge6711", "lg", "LG GE6711AR2853M", 16, 30, true, true,
                  true),
           remote("lg_akb752", "lg", "LG AKB75215403", 16, 30, true, true,
                  true),
           remote("lg_akb749", "lg", "LG AKB74955603", 16, 30, true, true,
                  true),
           remote("lg_akb737", "lg", "LG AKB73757604", 16, 30, true, true,
                  true),
           remote("lg_6711a20083v", "lg", "LG 6711A20083V"),
       }},
      {"carrier", "开利", {remote("carrier_ac64", "carrier", "Carrier AC64")}},
      {"aux",
       "奥克斯",
       {remote("aux_electra", "aux", "AUX YKR-T/011", 16, 32, true, true,
               true)}},
  };
}

QVector<AcBrand> catalogFromCatalogLines(const QStringList& lines) {
  QVector<AcBrand> catalog;
  QHash<QString, int> brandIndexById;

  for (const QString& line : lines) {
    if (line.startsWith(QStringLiteral("CAT BRAND "))) {
      const QString id = catalogField(line, QStringLiteral("id"));
      if (id.isEmpty() || brandIndexById.contains(id)) {
        continue;
      }
      const QString rawName = catalogField(line, QStringLiteral("name"));
      AcBrand brand;
      brand.id = id;
      brand.name = localizedBrandName(id, rawName);
      brandIndexById.insert(id, catalog.size());
      catalog.push_back(brand);
      continue;
    }

    if (!line.startsWith(QStringLiteral("CAT REMOTE "))) {
      continue;
    }

    const QString id = catalogField(line, QStringLiteral("id"));
    const QString brandId = catalogField(line, QStringLiteral("brand"));
    if (id.isEmpty() || brandId.isEmpty()) {
      continue;
    }

    if (!brandIndexById.contains(brandId)) {
      AcBrand brand;
      brand.id = brandId;
      brand.name = localizedBrandName(brandId, brandId);
      brandIndexById.insert(brandId, catalog.size());
      catalog.push_back(brand);
    }

    int minTemp = 16;
    int maxTemp = 30;
    parseTempRange(line, &minTemp, &maxTemp);

    AcRemote parsed;
    parsed.id = id;
    parsed.brandId = brandId;
    parsed.name = catalogField(line, QStringLiteral("name"));
    if (parsed.name.isEmpty()) {
      parsed.name = id;
    }
    parsed.minTemp = minTemp;
    parsed.maxTemp = maxTemp;
    parsed.supportsFan = catalogBool(line, QStringLiteral("fan"), true);
    parsed.supportsSwingV = catalogBool(line, QStringLiteral("swingv"));
    parsed.supportsSwingH = catalogBool(line, QStringLiteral("swingh"));
    catalog[brandIndexById.value(brandId)].remotes.push_back(parsed);
  }

  return catalog;
}

const AcRemote* findRemote(const QVector<AcBrand>& catalog,
                           const QString& remoteId) {
  // 目录规模很小，线性查找更直观；如果后续扩展到大量遥控器，再换成 QHash。
  for (const auto& brand : catalog) {
    for (const auto& remote : brand.remotes) {
      if (remote.id == remoteId) {
        return &remote;
      }
    }
  }
  return nullptr;
}

QString buildAcCommand(const QString& remoteId, const AcState& state) {
  // 不带 action 的 AC 命令表示“完整状态同步”。
  // 添加空调流程故意使用完整状态：先开机，再改温度，方便用户判断候选遥控器是否匹配。
  return QStringLiteral(
             "AC remote=%1 power=%2 mode=%3 temp=%4 fan=%5 swingv=%6 swingh=%7")
      .arg(remoteId)
      .arg(state.power ? 1 : 0)
      .arg(state.mode)
      .arg(state.temp)
      .arg(state.fan)
      .arg(state.swingv)
      .arg(state.swingh);
}

QString buildAcActionCommand(const QString& remoteId, const QString& action,
                             const AcState& state) {
  // 带 action 的 AC 命令表示“只发送某个动作”。
  // 仍然带上完整状态字段，是为了让固件能校验温度范围、模式等，也方便日志排查。
  return QStringLiteral(
             "AC remote=%1 action=%2 power=%3 mode=%4 temp=%5 fan=%6 "
             "swingv=%7 swingh=%8")
      .arg(remoteId)
      .arg(action)
      .arg(state.power ? 1 : 0)
      .arg(state.mode)
      .arg(state.temp)
      .arg(state.fan)
      .arg(state.swingv)
      .arg(state.swingh);
}

QString brandDisplayName(const AcBrand& brand) {
  return QStringLiteral("%1 (%2, %3个候选)")
      .arg(brand.name, brand.id)
      .arg(brand.remotes.size());
}

QString defaultDeviceName(const AcRemote& remote) {
  return QStringLiteral("%1 空调").arg(remote.name);
}
