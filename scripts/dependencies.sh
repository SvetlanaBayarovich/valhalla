#!/bin/bash
set -e

export LD_LIBRARY_PATH=.:`cat /etc/ld.so.conf.d/* | grep -v -E "#" | tr "\\n" ":" | sed -e "s/:$//g"`
sudo add-apt-repository -y ppa:kevinkreiser/prime-server
sudo apt-get update -o Dir::Etc::sourcelist="sources.list.d/kevinkreiser-prime-server-$(lsb_release -c -s).list" -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0"
sudo apt-get install -y autoconf automake pkg-config libtool make pkg-config gcc g++ libboost1.54-all-dev libprotobuf-dev libprime-server-dev

if [ -n "$1" ] && [ -d "$1" ]; then
        pushd "$1"
else
        pushd .
fi

#clone
for dep in midgard baldr skadi sif loki odin thor tyr; do
	rm -rf $dep
	git clone --depth=1 --recurse-submodules --single-branch --branch=master https://github.com/valhalla/$dep.git $dep
done

#build sync
for dep in midgard baldr sif skadi loki odin thor tyr; do
	pushd $dep
	./autogen.sh
	./configure CPPFLAGS=-DBOOST_SPIRIT_THREADSAFE
	make -j$(nproc)
	sudo make install
	popd
done

popd
