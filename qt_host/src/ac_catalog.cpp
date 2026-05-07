#include "ac_catalog.h"

QVector<AcBrand> defaultAcCatalog() {
  // 这里是 Qt 侧当前内置的遥控器目录。
  // 注意：remote.id 必须和 ESP32 固件里的 AcRemote.id 完全一致，否则固件会返回
  // UNKNOWN_REMOTE。以后如果遥控器越来越多，更推荐让 Qt 用 CATALOG 命令从固件读取。
  return {
      {"midea",
       "美的",
       {
           {"midea_standard", "midea", "Midea standard", 17, 30, true, true,
            false},
           {"midea_rn02s13", "midea", "Midea RN02S13", 17, 30, true, true,
            true},
       }},
      {"gree",
       "格力",
       {
           {"gree_yaw1f", "gree", "Gree YAW1F", 16, 30, true, true, true},
           {"gree_ybofb", "gree", "Gree YBOFB", 16, 30, true, true, true},
           {"gree_yx1fsf", "gree", "Gree YX1FSF", 16, 30, true, true, true},
       }},
      {"haier",
       "海尔",
       {
           {"haier_ac", "haier", "Haier HSU07", 16, 30, false, true, false},
           {"haier_ac160", "haier", "Haier AC160", 16, 30, false, true, false},
           {"haier_ac176_a", "haier", "Haier AC176 V9014557-A", 16, 30, false,
            true, true},
           {"haier_ac176_b", "haier", "Haier AC176 V9014557-B", 16, 30, false,
            true, true},
           {"haier_yrw02", "haier", "Haier YR-W02", 16, 30, false, true, false},
       }},
  };
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
             "AC remote=%1 power=%2 mode=%3 temp=%4 fan=%5 swingv=%6 swingh=%7 "
             "eco=%8")
      .arg(remoteId)
      .arg(state.power ? 1 : 0)
      .arg(state.mode)
      .arg(state.temp)
      .arg(state.fan)
      .arg(state.swingv)
      .arg(state.swingh)
      .arg(state.eco ? 1 : 0);
}

QString buildAcActionCommand(const QString& remoteId, const QString& action,
                             const AcState& state) {
  // 带 action 的 AC 命令表示“只发送某个动作”。
  // 仍然带上完整状态字段，是为了让固件能校验温度范围、模式等，也方便日志排查。
  return QStringLiteral(
             "AC remote=%1 action=%2 power=%3 mode=%4 temp=%5 fan=%6 "
             "swingv=%7 swingh=%8 eco=%9")
      .arg(remoteId)
      .arg(action)
      .arg(state.power ? 1 : 0)
      .arg(state.mode)
      .arg(state.temp)
      .arg(state.fan)
      .arg(state.swingv)
      .arg(state.swingh)
      .arg(state.eco ? 1 : 0);
}

QString brandDisplayName(const AcBrand& brand) {
  return QStringLiteral("%1 (%2)").arg(brand.name, brand.id);
}

QString defaultDeviceName(const AcRemote& remote) {
  return QStringLiteral("%1 空调").arg(remote.name);
}
