#!/usr/bin/bash

cd ../bin/release/lunam.app/Contents/Resources
ln -s ../../../../../assets assets
cd -
cd ../bin/debug/lunam.app/Contents/Resources
ln -s ../../../../../assets assets
