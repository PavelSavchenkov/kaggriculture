#!/bin/sh
# Builds the Local-LB / Kaggle bundle of the bc_opus agent into a submission folder:
#   main.py (kept from the folder), libopus_lb_bridge.so (baseline x86-64, glibc <= 2.28,
#   static C++ runtime), model/ (model.bin and sidecars), AGENT.toml, source/ (this
#   experiment's agent, compiler and local route-solver sources; builds inside the
#   kaggriculture repository), BUILD.txt (provenance).
# usage: scripts/package_localLB.sh <model_dir> <submission_dir> <display_name>
set -e
cd "$(dirname "$0")/.."
model=$1; out=$2; name=$3
# Toolchain: conda env kagbuild (conda-forge GCC 14.3 with the glibc 2.28 sysroot), so the
# library loads on older systems (Local-LB image: Debian bookworm glibc 2.36; Kaggle 2.35).
conda run -n kagbuild bash -c 'CXXFLAGS="" CFLAGS="" LDFLAGS="" cmake -S . -B build_kaggle -DBC_PORTABLE=ON > /dev/null && cmake --build build_kaggle -j 8 --target opus_lb_bridge > /dev/null'
if objdump -T build_kaggle/libopus_lb_bridge.so | grep -q "GLIBC_2\.\(29\|3[0-9]\)"; then echo "bridge needs glibc > 2.28"; exit 1; fi
mkdir -p "$out/model" "$out/source"
cp build_kaggle/libopus_lb_bridge.so "$out/"
cp "$model"/model.bin* "$out/model/"
rm -rf "$out/source"/*
cp -r source agent day_policy_local CMakeLists.txt "$out/source/"
cp -r opponents "$out/source/"
printf 'display_name = "%s"\nauthor = "Pavel (Kaggriculture team)"\n' "$name" > "$out/AGENT.toml"
{
    echo "experiment: experiments/v10/sep24_BC_opus"
    echo "model: $model"
    echo "built: $(date -u +%Y-%m-%dT%H:%M:%SZ) with BC_PORTABLE=ON (-march=x86-64, conda-forge GCC 14.3, glibc 2.28 sysroot, static libstdc++/libgcc)"
    echo "sha256:"
    (cd "$out" && sha256sum libopus_lb_bridge.so model/*)
} > "$out/BUILD.txt"
echo "packaged $out"
