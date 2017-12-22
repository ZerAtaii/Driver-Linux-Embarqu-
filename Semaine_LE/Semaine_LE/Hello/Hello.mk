HELLO_VERSION = 1.0
HELLO_SITE = $(TOPDIR)/package/Hello
HELLO_SITE_METHOD = local
HELLO_LICENSE = GPLv3+

define HELLO_BUILD_CMDS
	CC=$(TARGET_CC) $(MAKE) -C $(@D)
endef

define HELLO_INSTALL_TARGET_CMDS
	prefix=$(TARGET_DIR) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
