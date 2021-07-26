docker exec -it kafka ~/Downloads/kafka_2.13-2.8.0/bin/kafka-console-producer.sh \
    --broker-list localhost:9092 \
    --topic tasks