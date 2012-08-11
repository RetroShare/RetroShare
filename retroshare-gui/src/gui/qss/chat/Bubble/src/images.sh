#!/bin/bash

cd 'img/bubble-orange/'
for file in *.png;
do
	#convert $file -set option:modulate:colorspace hsb -modulate 100,50,100 -colorspace Gray ../bubble-grey/$file
	#convert $file -set option:modulate:colorspace hsb -modulate 110,80,80 ../bubble-red/$file
	convert $file -set option:modulate:colorspace hsb -modulate 110,80,10 ../bubble-blue/$file
	convert $file -set option:modulate:colorspace hsb -modulate 110,80,130 ../bubble-green/$file
done



	#echo "hello ${file} - "
	#convert $file -set option:modulate:colorspace hsb -modulate 100,20,100 ../bubble-grey/$file
