#!/bin/sh
# Assembles a ready-to-run agent folder (Local-LB or Kaggle) from this handoff:
#   <out>/main.py, <out>/AGENT.toml, <out>/libopus_lb_bridge.so, <out>/model/ (v12 weights + sidecars),
#   <out>/submission.tar.gz (the Kaggle archive: main.py, the bridge, model/), <out>/BUILD.txt.
# The bridge is built from ../experiment with BC_PORTABLE=ON in conda env kagbuild (conda-forge
# GCC 14 against a glibc 2.28 sysroot, static libstdc++), as every shipped agent was: the Local-LB
# image has glibc 2.36 and Kaggle 2.35, so a bridge built against a newer system glibc may not load.
# Create the env once: conda create -n kagbuild -c conda-forge gxx_linux-64=14 sysroot_linux-64=2.28 cmake make
# usage: agent/package.sh <out_dir> [sidecar dir, default agent/model]   (env: BUILD_DIR)
set -e
here=$(cd "$(dirname "$0")" && pwd)
handoff=$(dirname "$here")
out=$1; sidecars=$(cd "${2:-$here/model}" && pwd)
build=${BUILD_DIR:-$(dirname "$handoff")/work/pipeline_sep25_build_portable}
conda run -n kagbuild bash -c "CXXFLAGS='' CFLAGS='' LDFLAGS='' cmake -S '$handoff/experiment' -B '$build' -DBC_PORTABLE=ON -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build '$build' -j 8 --target opus_lb_bridge > /dev/null"
if objdump -T "$build/libopus_lb_bridge.so" | grep -q "GLIBC_2\.\(29\|3[0-9]\)"; then echo "bridge needs glibc > 2.28"; exit 1; fi
mkdir -p "$out/model"
cp "$here/main.py" "$here/AGENT.toml" "$build/libopus_lb_bridge.so" "$out/"
cp "$handoff/weights/v12_cond/model.bin" "$sidecars"/model.bin.* "$out/model/"
(cd "$out" && tar --owner=0 --group=0 --mtime=@0 -czf submission.tar.gz main.py libopus_lb_bridge.so model)
{
    echo "handoff: pipeline_sep25_handoff, sidecars: $sidecars"
    echo "built: $(date -u +%Y-%m-%dT%H:%M:%SZ) with BC_PORTABLE=ON (conda env kagbuild)"
    echo "sha256:"
    (cd "$out" && sha256sum main.py libopus_lb_bridge.so model/*)
} > "$out/BUILD.txt"
echo "packaged $out"
