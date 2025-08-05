git lfs pull
if [ $? -eq 0 ]; then
    echo "PULLED VIA GIT LFS"
else
    echo "FAILED TO PULL VIA GIT LFS, RETRIEVING FROM 'desbordante-data' REPOSITORY"
    git clone https://github.com/Desbordante/desbordante-data.git
    mv desbordante-data/datasets.zip datasets/datasets.zip
    rm -rf desbordante-data
fi
