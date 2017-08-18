#!/bin/sh
echo `$1 -v 2>&1 | grep Configured | awk '{ print $3 }'`
