%if 0%{!?rpm_device:1}
%define rpm_device %{device}
%endif
%if 0%{!?rpm_device_codename:1}
%define rpm_device_codename %{device_codename}
%endif
%if 0%{!?rpm_adaptation_device:1}
%define rpm_adaptation_device %{adaptation_device}
%endif
%if 0%{!?rpm_adaptation_device_codename:1}
%define rpm_adaptation_device_codename %{adaptation_device_codename}
%endif

%if 0%{!?patch_kernel:1}
%define patch_kernel 0
%endif

%if 0%{!?divert_flash_partition_device_info:1}
%define divert_flash_partition_device_info 0
%endif

%define _build_dir tmp
%define _common_dir %{_build_dir}/droid-compat-device

Name:          droid-compat-%{rpm_device}   
Summary:       Compatibility Layer for %{rpm_device} (on top of %{rpm_adaptation_device})
Group:         System
License:       BSD-3-Clause
Version:       0.0.1
Release:       1
Provides:      droid-compat
BuildRequires: repomd-pattern-builder

%description
%{summary}.

############
%package feature
Summary: SSU feature for %{rpm_device} compat layer
Provides: droid-compat-feature

%description feature
%{summary}.

############
%package patterns
Summary: Repository patterns for %{rpm_device} compat layer
Provides: droid-compat-patterns

%description patterns
%{summary}.

############
%package droid-config
Summary: Compatibility layer for %{rpm_device} (droid-config)
Requires: droid-config-%{rpm_adaptation_device}
Provides: droid-compat-droid-config

%description droid-config
%{summary}.

############
%package droid-config-sailfish
Summary: Compatibility layer for %{rpm_device} (droid-config-sailfish)
Requires: droid-config-%{rpm_adaptation_device}-sailfish
Provides: droid-compat-droid-config-sailfish

%description droid-config-sailfish
%{summary}.

############
%package droid-hal
Summary: Compatibility layer for %{rpm_device} (droid-hal)
Requires: droid-hal-%{rpm_adaptation_device}
Provides: droid-compat-droid-hal

%description droid-hal
%{summary}.

############
%package droid-config-flashing
Summary: Compatibility layer for %{rpm_device} (droid-config-flashing)
Requires: droid-config-%{rpm_adaptation_device}-flashing
Requires: rpm-divert
Requires: yabit
%if 0%{?dtb_package:1}
Requires: %{dtb_package}
%endif
Provides: droid-compat-droid-config-flashing

%description droid-config-flashing
%{summary}.

############
%package droid-system
Summary: Compatibility layer for %{rpm_device} (droid-system)
Requires: droid-system-%{rpm_adaptation_device}
Provides: droid-compat-droid-system

%description droid-system
%{summary}.

########################################################################

%build

PKGS="feature droid-config droid-config-sailfish droid-hal droid-config-flashing droid-system"

# Create separate build directory
rm -rf %{_build_dir}
mkdir -p %{_common_dir}
cp -arf droid-compat-device/* %{_common_dir}

# Copy common files, then overlay HA specific ones
for pkg in $PKGS; do
	[ -e droid-compat-device/$pkg ] && cp -Rav droid-compat-device/$pkg %{_build_dir}
	[ -e $pkg ] && cp -Rav $pkg %{_build_dir}
done

# Configure flash-bootimg.sh
sed \
	-i \
	's|@CMDLINE@|%{custom_cmdline}|; s|@DTB_PATH@|%{custom_dtb}|' \
	%{_build_dir}/droid-config-flashing/usr/lib/droid-compat/platform-updates/flash-bootimg.sh

# Generate file lists
for pkg in $PKGS; do
	if [ -e %{_build_dir}/$pkg ]; then
		find %{_build_dir}/$pkg ! \( -type d \) | sed -e s/"%{_build_dir}\/$pkg"/"\/"/g> %{_build_dir}/$pkg.files
	fi
done

# Build patterns

mkdir -p %{_build_dir}/pattern-includes.d
mkdir -p %{_build_dir}/patterns
mkdir -p %{_build_dir}/patterns-built

[ -e pattern-includes.d ] && cp -v pattern-includes.d/* %{_build_dir}/pattern-includes.d

for pkg in $PKGS; do
	if [ -e %{_build_dir}/$pkg ]; then
		echo " - droid-compat-%{rpm_device}-$pkg" >> %{_build_dir}/pattern-includes.d/00compat
	fi
done

sed \
	's|@DEVICE@|%{rpm_device}|; s|@ADAPTATION_DEVICE@|%{rpm_adaptation_device}|' \
	%{_common_dir}/patterns/droid-compat-@DEVICE@.yaml \
	> %{_build_dir}/patterns/droid-compat-%{rpm_device}.yaml

for include in %{_build_dir}/pattern-includes.d/*; do
	cat $include >> %{_build_dir}/patterns/droid-compat-%{rpm_device}.yaml
done

/usr/bin/repomd-pattern-builder.py --patternxml \
	-p %{_build_dir}/patterns/ -o %{_build_dir}/patterns-built \
	--version=%{version} --release=%{release}

%install

PKGS="droid-config droid-config-sailfish droid-hal droid-config-flashing droid-system"

# Copy everything into the installroot
for pkg in $PKGS; do
	if [ -e %{_build_dir}/$pkg ]; then
		cp -Rav %{_build_dir}/$pkg/* installroot
	fi
done

mkdir -p %{buildroot}/usr/share/package-groups/
cp -av %{_build_dir}/patterns-built/* %{buildroot}/usr/share/package-groups/

%files patterns
/usr/share/package-groups

%files droid-config -f %{_build_dir}/droid-config.files
%defattr(-,root,root,-)

%files droid-config-sailfish -f %{_build_dir}/droid-config-sailfish.files
%defattr(-,root,root,-)

%files droid-hal -f %{_build_dir}/droid-hal.files
%defattr(-,root,root,-)

%files droid-config-flashing -f %{_build_dir}/droid-config-flashing.files
%defattr(-,root,root,-)

%files droid-system -f %{_build_dir}/droid-system.files
%defattr(-,root,root,-)

%post droid-config-sailfish
# FIXME!
add-oneshot compat-fix-icons

%post droid-config-flashing
%if 0%{?patch_kernel:1}
# Divert flash-bootimg.sh
rpm-divert add \
	droid-compat-%{rpm_device}-droid-config-flashing \
	/var/lib/platform-updates/flash-bootimg.sh \
	/var/lib/platform-updates/flash-bootimg.sh.diverted \
	--action symlink \
	--replacement /usr/lib/droid-compat/platform-updates/flash-bootimg.sh
%endif

%if 0%{?divert_flash_partition_device_info:1}
# Divert device-info
rpm-divert add \
	droid-compat-%{rpm_device}-droid-config-flashing \
	/var/lib/flash-partition/device-info \
	/var/lib/flash-partition/device-info.diverted \
	--action symlink \
	--replacement %{custom_flash_partition_device_info_path}
%endif

rpm-divert apply --package droid-compat-%{rpm_device}-droid-config-flashing

%preun droid-config-flashing
if [ $1 -eq 0 ]; then
	# As on RPM-based systems the installation scriptlets of the upgrade
	# are executed _before_ removing the old version (thus executing this
	# postun scriplet at the end of the transaction), we are going to
	# unapply the diversions only on package removals.
	rpm-divert unapply --package droid-compat-%{rpm_device}-droid-config-flashing

%if 0%{?patch_kernel:1}
	# Divert flash-bootimg.sh
	rpm-divert remove \
		droid-compat-%{rpm_device}-droid-config-flashing \
		/var/lib/platform-updates/flash-bootimg.sh \
%endif

%if 0%{?divert_flash_partition_device_info:1}
	# Remove diversion
	rpm-divert remove \
		droid-compat-%{rpm_device}-droid-config-flashing \
		/var/lib/flash-partition/device-info
%endif

fi

%triggerin droid-config-flashing -- droid-config-%{rpm_adaptation_device}-flashing
if [ $2 -gt 1 ]; then
	# On upgrades, unapply the triggers so that when rpm will put the
	# upgraded files back in will not overwrite the diversion symlinks
	rpm-divert unapply --package droid-compat-%{rpm_device}-droid-config-flashing
fi

%triggerun droid-config-flashing -- droid-config-%{rpm_adaptation_device}-flashing
if [ $1 -gt 0 ] && [ $2 -gt 0 ]; then
	# Now that the upgrade files are in their place, it is time to re-apply
	# the diversions 
	rpm-divert apply --package droid-compat-%{rpm_device}-droid-config-flashing
fi
