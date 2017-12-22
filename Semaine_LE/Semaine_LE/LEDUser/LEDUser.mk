LEDUSER_VERSION = 1.0
LEDUSER_SITE = $(TOPDIR)/package/LEDUser/driver
LEDUSER_SITE_METHOD = local
LEDUSER_LICENSE = GPLv3+

LEDUSER_DEPENDENCIES = linux

define LEDUSER_BUILD_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D)
endef

define LEDUSER_INSTALL_TARGET_CMDS
	$(MAKE) -C $(LINUX_DIR) $(LINUX_MAKE_FLAGS) M=$(@D) modules_install
endef

$(eval $(generic-package))
