#!/bin/bash

echo "This script will export img.svg to all parts of bubble as png."
echo "This needs InkScape and XMLStarlet to be installed"

mkdir -p img/bubble-orange
mkdir -p img/bubble-grey
mkdir -p img/bubble-red
mkdir -p img/bubble-blue
mkdir -p img/bubble-green


echo "Making Orange ones..."
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='borderColor']/_:stop[@id='brdLGStop1']/@stop-color" --value "#ff9600" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intRaisedColor']/_:stop[@id='intRLGStop1']/@stop-color" --value "#ffef56" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickColor']/_:stop[@id='intTLGStop1']/@stop-color" --value "#ff9e37" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickSideColor']/_:stop[@id='intTSLGStop1']/@stop-color" --value "#ffd147" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop1']/@stop-color" --value "#ff9133" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop2']/@stop-color" --value "#ffcf47" img.svg

inkscape --export-use-hints --export-id="rect_TL;rect_TC;rect_TR;rect_CL;rect_CC;rect_CR;rect_BL;rect_BC;rect_BR;rect_tick;rect_tick-left;rect_tick-right" img.svg
mv img/*.png img/bubble-orange/


echo "Making Grey ones..."
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='borderColor']/_:stop[@id='brdLGStop1']/@stop-color" --value "#d0d0d0" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intRaisedColor']/_:stop[@id='intRLGStop1']/@stop-color" --value "#f3f3f3" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickColor']/_:stop[@id='intTLGStop1']/@stop-color" --value "#d5d5d5" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickSideColor']/_:stop[@id='intTSLGStop1']/@stop-color" --value "#e8e8e8" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop1']/@stop-color" --value "#d0d0d0" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop2']/@stop-color" --value "#e7e7e7" img.svg

inkscape --export-use-hints --export-id="rect_TL;rect_TC;rect_TR;rect_CL;rect_CC;rect_CR;rect_BL;rect_BC;rect_BR;rect_tick;rect_tick-left;rect_tick-right" img.svg
mv img/*.png img/bubble-grey/


echo "Making Red ones..."
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='borderColor']/_:stop[@id='brdLGStop1']/@stop-color" --value "#ff383b" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intRaisedColor']/_:stop[@id='intRLGStop1']/@stop-color" --value "#ffb184" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickColor']/_:stop[@id='intTLGStop1']/@stop-color" --value "#ff6977" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickSideColor']/_:stop[@id='intTSLGStop1']/@stop-color" --value "#ff8f77" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop1']/@stop-color" --value "#ff657e" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop2']/@stop-color" --value "#ff8d77" img.svg

inkscape --export-use-hints --export-id="rect_TL;rect_TC;rect_TR;rect_CL;rect_CC;rect_CR;rect_BL;rect_BC;rect_BR;rect_tick;rect_tick-left;rect_tick-right" img.svg
mv img/*.png img/bubble-red/


echo "Making Blue ones..."
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='borderColor']/_:stop[@id='brdLGStop1']/@stop-color" --value "#1789cd" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intRaisedColor']/_:stop[@id='intRLGStop1']/@stop-color" --value "#80cdee" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickColor']/_:stop[@id='intTLGStop1']/@stop-color" --value "#b0f4ff" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickSideColor']/_:stop[@id='intTSLGStop1']/@stop-color" --value "#4de3ff" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop1']/@stop-color" --value "#c6f9ff" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop2']/@stop-color" --value "#5bbfee" img.svg

inkscape --export-use-hints --export-id="rect_TL;rect_TC;rect_TR;rect_CL;rect_CC;rect_CR;rect_BL;rect_BC;rect_BR;rect_tick;rect_tick-left;rect_tick-right" img.svg
mv img/*.png img/bubble-blue/


echo "Making Green ones..."
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='borderColor']/_:stop[@id='brdLGStop1']/@stop-color" --value "#17cd1d" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intRaisedColor']/_:stop[@id='intRLGStop1']/@stop-color" --value "#8fff47" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickColor']/_:stop[@id='intTLGStop1']/@stop-color" --value "#b8fc66" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intTickSideColor']/_:stop[@id='intTSLGStop1']/@stop-color" --value "#82ea2e" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop1']/@stop-color" --value "#c2ff6f" img.svg
xml ed --inplace --update "//_:defs[1]/_:linearGradient[@id='intLinearGradientStops']/_:stop[@id='intLGStop2']/@stop-color" --value "#82ea2e" img.svg

inkscape --export-use-hints --export-id="rect_TL;rect_TC;rect_TR;rect_CL;rect_CC;rect_CR;rect_BL;rect_BC;rect_BR;rect_tick;rect_tick-left;rect_tick-right" img.svg
mv img/*.png img/bubble-green/

################
# What was used to get color from orange
#
#
#for file in img/bubble-orange/*.png;
#do
#	convert $file -set option:modulate:colorspace hsb -modulate 100,50,100 -colorspace Gray img/bubble-grey/${file##*/}
#	convert $file -set option:modulate:colorspace hsb -modulate 110,80,80 img/bubble-red/${file##*/}
#	convert $file -set option:modulate:colorspace hsb -modulate 110,80,10 img/bubble-blue/${file##*/}
#	convert $file -set option:modulate:colorspace hsb -modulate 100,100,130 img/bubble-green/${file##*/}
#done
