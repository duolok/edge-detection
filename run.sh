#!/bin/bash
source_image="resources/color.bmp"
cp "$source_image" resources/serial_prewitt.bmp 
cp "$source_image" resources/serial_edge.bmp 
cp "$source_image" resources/parallel_prewitt.bmp 
cp "$source_image" resources/parallel_edge.bmp 
cd src/ && ./waf build && ./waf run --app=ImageProcessing