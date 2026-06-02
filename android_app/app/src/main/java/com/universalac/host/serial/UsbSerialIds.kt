package com.universalac.host.serial

object UsbSerialIds {
    const val QINHENG_VENDOR_ID = 0x1A86
    const val CH34X_PRODUCT_7522 = 0x7522

    fun supportsCh34x(vendorId: Int, productId: Int): Boolean {
        return vendorId == QINHENG_VENDOR_ID && productId == CH34X_PRODUCT_7522
    }
}
