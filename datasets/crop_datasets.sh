#!/bin/sh
# Crop and merge datasets for performance tests
cut -d';' -f -5 adult.csv >adult5attr.csv
cut -d, -f -20 CIPublicHighway.csv | head -n60001 >CIPublicHighway20attr60k.csv
head -n450001 iowa1kk.csv >iowa450k.csv
# Mushroom with 3 additional attributes
# Values are rounded not to get a full column of unique values
head -n1001 cfd_data/mushroom.csv | awk 'function rnd() { return int(rand() * 100) }; \
    {if (FNR == 1) { print "rand1,rand2,rand3," $0 } else { print rnd() "," rnd() "," rnd() \
    "," $0 } }' >'mushroom+3attr1k.csv'
# Mushroom with 4 additional attributes
awk '{if (FNR == 1) { print "rand0," $0 } else { print int(rand() * 100) "," $0 } }' \
    'mushroom+3attr1k.csv' >'mushroom+4attr1k.csv'
