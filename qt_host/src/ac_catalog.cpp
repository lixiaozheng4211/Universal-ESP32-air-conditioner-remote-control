#include "ac_catalog.h"

QVector<AcBrand> defaultAcCatalog()
{
    return {
        {"midea", "美的", {
            {"midea_standard", "midea", "Midea standard", 17, 30, true, true, false},
            {"midea_rn02s13", "midea", "Midea RN02S13", 17, 30, true, true, true},
        }},
        {"gree", "格力", {
            {"gree_yaw1f", "gree", "Gree YAW1F", 16, 30, true, true, true},
            {"gree_ybofb", "gree", "Gree YBOFB", 16, 30, true, true, true},
            {"gree_yx1fsf", "gree", "Gree YX1FSF", 16, 30, true, true, true},
        }},
        {"haier", "海尔", {
            {"haier_ac", "haier", "Haier HSU07", 16, 30, false, true, false},
            {"haier_ac160", "haier", "Haier AC160", 16, 30, false, true, false},
            {"haier_ac176_a", "haier", "Haier AC176 V9014557-A", 16, 30, false, true, true},
            {"haier_ac176_b", "haier", "Haier AC176 V9014557-B", 16, 30, false, true, true},
            {"haier_yrw02", "haier", "Haier YR-W02", 16, 30, false, true, false},
        }},
    };
}

const AcRemote *findRemote(const QVector<AcBrand> &catalog, const QString &remoteId)
{
    for (const auto &brand : catalog) {
        for (const auto &remote : brand.remotes) {
            if (remote.id == remoteId) {
                return &remote;
            }
        }
    }
    return nullptr;
}

QString buildAcCommand(const QString &remoteId, const AcState &state)
{
    return QStringLiteral("AC remote=%1 power=%2 mode=%3 temp=%4 fan=%5 swingv=%6 swingh=%7 eco=%8")
        .arg(remoteId)
        .arg(state.power ? 1 : 0)
        .arg(state.mode)
        .arg(state.temp)
        .arg(state.fan)
        .arg(state.swingv)
        .arg(state.swingh)
        .arg(state.eco ? 1 : 0);
}

QString brandDisplayName(const AcBrand &brand)
{
    return QStringLiteral("%1 (%2)").arg(brand.name, brand.id);
}

QString defaultDeviceName(const AcRemote &remote)
{
    return QStringLiteral("%1 空调").arg(remote.name);
}
