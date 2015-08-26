#!/bin/sh

#echo "FILE: $1"
#echo "PREFIX: $2"
#echo "WORKDIR: $3"

ESCAPED_PREFIX=`echo $2|sed "s/\//\\\\\\\\\//g"`

#echo "ESCAPED_PREFIX: $ESCAPED_PREFIX"

cat $1|sed "s/^\(.*\)rootDirectory=\".*\",.*/\1rootDirectory=\"$ESCAPED_PREFIX\/lib\/crtmpserver\/applications\",/" >$3/tempconf
mv $3/tempconf $1

cat $1|sed "s/^\(.*\)fileName=\".*/\1fileName=\"$ESCAPED_PREFIX\/var\/log\/crtmpserver\/crtmpserver\",/" >$3/tempconf
mv $3/tempconf $1

