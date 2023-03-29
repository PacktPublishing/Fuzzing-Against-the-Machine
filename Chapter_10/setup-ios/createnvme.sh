#!/bin/bash

../qemu-t8030/build/qemu-img create -f qcow2 nvme.1.qcow2 32G
../qemu-t8030/build/qemu-img create -f qcow2 nvme.2.qcow2 8M
../qemu-t8030/build/qemu-img create -f qcow2 nvme.3.qcow2 128K
../qemu-t8030/build/qemu-img create -f qcow2 nvme.4.qcow2 8K
../qemu-t8030/build/qemu-img create -f qcow2 nvram.qcow2 8K
../qemu-t8030/build/qemu-img create -f qcow2 nvme.6.qcow2 4K
../qemu-t8030/build/qemu-img create -f qcow2 nvme.7.qcow2 1M
