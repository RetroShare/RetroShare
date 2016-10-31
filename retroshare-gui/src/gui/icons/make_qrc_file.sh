#!/bin/sh

echo '<RCC>'
echo '\t<qresource prefix="/">'

for i in `ls *.png */*png */*svg`; do
	echo '\t\t<file>icons/'$i'</file>'
done

echo '\t</qresource>'
echo '</RCC>'
