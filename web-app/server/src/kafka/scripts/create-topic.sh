docker exec -it kafka ~/Downloads/kafka_2.13-2.8.0/bin/kafka-topics.sh \
    --create \
    --zookeeper zookeeper:2181 \
    --replication-factor 1 \
    --partitions 1 \
    --topic tasks