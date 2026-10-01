#!/bin/bash
# Judge-ready agent folder: the Local-LB snapshot's m19-fc2 folder with source/dc11/<files> replaced from <dc11 dir>, the bridge rebuilt
# from standalone/ (kagbuild compiler, as the shipped package), and model/ = <model dir> (cp -rL). Writes into a fresh folder only.
# usage: mkbridge.sh <name> <dc11 dir> "<file ...>" <model dir>
set -e
cd "$(dirname "$0")"
name=$1; dc=$(readlink -f $2); files=$3; model=$(readlink -f $4)
S=/home/pavel/Programming/kaggriculture/experiments/v10/sep24_BC_opus/data/localLB_d6157df/agents/pavel-bc-opus-v17d-dc12m19-fc2-m68
CXX=/home/pavel/anaconda3/envs/kagbuild/bin/x86_64-conda-linux-gnu-c++
[ -e $name ] && { echo "exists: $name"; exit 1; }
mkdir $name
for f in AGENT.toml BUILD.txt main.py README.md source standalone; do cp -rL $S/$f $name/; done
for f in $files; do cp --remove-destination $dc/$f $name/source/dc11/$f; done
cmake -S $name/standalone -B build_$name -DCMAKE_CXX_COMPILER=$CXX > build_$name.log 2>&1
cmake --build build_$name -j 2 >> build_$name.log 2>&1
cp build_$name/libopus_lb_bridge.so $name/
cp -rL $model $name/model
chmod -R u+w $name
[ -z "$(find $name -type l)" ] || { echo "stop: symlinks in $name"; exit 1; }
{ echo "dc11 from $dc: $files"; echo "model $model"; sha256sum $name/libopus_lb_bridge.so; } > $name/SRC.txt
echo "made $name"
