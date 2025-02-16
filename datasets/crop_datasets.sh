#!/bin/sh
# Crop and merge datasets for performance tests
cut -d';' -f -9 adult.csv >adult9attr.csv
cut -d, -f -20 CIPublicHighway.csv | head -n55001 >CIPublicHighway20attr55k.csv
head -n550001 iowa1kk.csv >iowa550k.csv
head -n650001 iowa1kk.csv >iowa650k.csv
# Mushroom with 3 additional attributes
# Values are rounded not to get a full column of unique values
head -n1501 cfd_data/mushroom.csv | awk 'function rnd() { return int(rand() * 100) }; \
    {if (FNR == 1) { print "rand1,rand2,rand3," $0 } else { print rnd() "," rnd() "," rnd() \
    "," $0 } }' >'mushroom+3attr1500.csv'
# Mushroom with 4 additional attributes
head -n1301 mushroom+3attr1500.csv | \
	awk '{if (FNR == 1) { print "rand0," $0 } else { print int(rand() * 100) "," $0 } }' \
    >'mushroom+4attr1300.csv'
