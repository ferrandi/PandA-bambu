#!/bin/bash
echo '"\' > $2
sed -e 's/\\/\\\\/g' $1 | sed -e 's/\"/\\\"/g' | sed -e 's/$/\\n\\/g' >> $2
echo '"' >> $2
