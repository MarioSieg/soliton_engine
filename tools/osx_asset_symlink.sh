#!/usr/bin/bash

cd ../bin/release/soliton_engine.app/Contents/Resources
ln -s ../../../../../assets assets
cd -
cd ../bin/debug/soliton_engine.app/Contents/Resources
ln -s ../../../../../assets assets
