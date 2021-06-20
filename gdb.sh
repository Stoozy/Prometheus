#!/bin/bash

gdb --ex "target remote localhost:1234"  --ex "set architecture i386:x86-64" --ex "file kernel/dead.kernel"
