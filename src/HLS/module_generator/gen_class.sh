#!/bin/bash

classname="$1"

define_name="$(sed -r 's/([a-z0-9])([A-Z])/\1_\L\2/g' <<< $classname)"

out_hpp="${classname}ModuleGenerator.hpp"
out_cpp="${classname}ModuleGenerator.cpp"

cp ClassName.cpp ${out_cpp}
cp ClassName.hpp ${out_hpp}

sed -i "s/CLASSNAME/${define_name^^}/g" ${out_hpp}
sed -i "s/ClassName/${classname}/g" ${out_hpp}
sed -i "s/ClassName/${classname}/g" ${out_cpp}