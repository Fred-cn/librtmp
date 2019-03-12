include $(TOPDIR)/build_rules/rules.mk
PKG_NAME:=rtmpsdk
PKG_SRC:=$(PKG_NAME)
PKG_VERSION:=1.0.0.22
PKG_BUILD_DIR:=$(BUILD_BASE_DIR)/$(PKG_NAME)
PKG_SKIP:=$(PKG_BUILD_DIR)/.built
PKG_PATCHES:=patches

EXTRA_CFLAGS:=-I$(PKG_BUILD_DIR) \
		-I$(PKG_BUILD_DIR)/CommonLib \
		-I$(PKG_BUILD_DIR)/AACEncoder/include \
		-I$(PKG_BUILD_DIR)/srs_librtmp/core \
		-I$(PKG_BUILD_DIR)/srs_librtmp/kernel \
		-I$(PKG_BUILD_DIR)/srs_librtmp/libs \
		-I$(PKG_BUILD_DIR)/srs_librtmp/protocol \
		-DBOOST_ASIO_DISABLE_STD_CHRONO -Wl,--version-script=$(PKG_BUILD_DIR)/export.map

EXTRA_LDFALGS:=-lboost_system-mt -lboost_chrono-mt -lboost_date_time-mt \
		-lboost_filesystem-mt -lboost_thread-mt

all: 

ifneq ($(wildcard $(PKG_SKIP)),)
	@$(call MESSAGE,"Finished!", $(PKG_BUILD_DIR))
else
	make clean
	make prepare
	make compile
	make install_staging
	make install
endif


prepare:
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)

compile:
	$(MAKE) -C $(PKG_BUILD_DIR) \
	CC="$(TARGET_CC)" \
	CXX="$(TARGET_CXX)" \
	AR="$(TARGET_AR)" \
	CFLAGS="$(TARGET_CPPFLAGS) $(TARGET_CXXFLAGS) $(EXTRA_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS) $(EXTRA_LDFALGS)"

install_staging:
	#$(INSTALL_DIR) $(BUILD_STAGING_DIR)/usr/include
	$(INSTALL_DIR) $(BUILD_STAGING_DIR)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/librtmpsdk.so $(BUILD_STAGING_DIR)/usr/lib/
	
install:
	$(INSTALL_DIR) $(BUILD_ROOTFS_DIR)/usr/dmbox/lib/app
	$(CP) $(PKG_BUILD_DIR)/librtmpsdk.so $(PKG_BUILD_DIR)/librtmpsdk.$(PKG_VERSION).so
	$(LN) librtmpsdk.$(PKG_VERSION).so $(PKG_BUILD_DIR)/librtmpsdk.so
	$(CP) $(PKG_BUILD_DIR)/*.so* $(BUILD_ROOTFS_DIR)/usr/dmbox/lib/app

	@touch ${PKG_SKIP};

clean:
	rm $(PKG_BUILD_DIR) -rf
