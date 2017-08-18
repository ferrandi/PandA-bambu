#!/bin/sh
echo `$1 -v 2>&1 | grep Target | awk '{ print $2}'`
