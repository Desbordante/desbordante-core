set -e

git clone https://github.com/Desbordante/desbordante-data.git

# Merge all archives
cd desbordante-data
mkdir all-datasets
for fname in *.zip; do unzip -d all-datasets -o -u $fname; done
zip -r all-datasets.zip all-datasets
cd ..

mv desbordante-data/all-datasets.zip datasets/datasets.zip
rm -rf desbordante-data
