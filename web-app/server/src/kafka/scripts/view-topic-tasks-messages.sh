docker exec -it kafka ~/Downloads/kafka_2.13-2.8.0/bin/kafka-console-consumer.sh \
  --bootstrap-server localhost:9092 \
  --from-beginning \
  --topic tasks