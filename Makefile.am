AUTOMAKE_OPTIONS = foreign
SUBDIRS = src
DIST_SUBDIRS = src

if ENABLE_STANDALONE
CLEANFILES = flite jtalk
else
CLEANFILES = package/navigation.wgt flite jtalk
endif

if ENABLE_STANDALONE
bin_SCRIPTS = flite jtalk
else
bin_SCRIPTS = flite jtalk package
endif

if ENABLE_STANDALONE
flite: flite.in
	sed -e 's,[@]datadir[@],$(datadir),g' $(srcdir)/flite.in > flite
	chmod +x flite

jtalk: jtalk.in
	sed -e 's,[@]datadir[@],$(datadir),g' -e 's,[@]exec_prefix[@],$(exec_prefix),g' $(srcdir)/jtalk.in > jtalk
	chmod +x jtalk
else
flite: flite.in
	sed -e 's,[@]datadir[@],$(datadir),g' $(srcdir)/flite_agl.in > flite
	chmod +x flite

jtalk: jtalk.in
	sed -e 's,[@]datadir[@],$(datadir),g' -e 's,[@]exec_prefix[@],$(exec_prefix),g' $(srcdir)/jtalk_agl.in > jtalk
	chmod +x jtalk
endif

package:src/navi jtalk flite
	rm -rf package/
	mkdir -p package/root
	mkdir -p package/root/lib
	cp src/.libs/navi package/root/
	cp flite package/root/
	cp jtalk package/root/
	cp src/.libs/libnavicore.so.0 package/root/lib/
	cp $(srcdir)/agl/config.xml package/root/
	cp $(srcdir)/agl/HMI_AppLauncher_Navigation_Active-01.png package/root/navigation.png
	wgtpkg-pack -f -o package/navigation.wgt package/root
