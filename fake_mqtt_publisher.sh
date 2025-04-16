#!/bin/bash

# Infinite loop to publish data every second
while true; do
  # Generate random sensor data
  temperature=$(awk -v min=15 -v max=30 'BEGIN{srand(); printf "%.1f", min+rand()*(max-min)}')
  humidity=$(awk -v min=30 -v max=90 'BEGIN{srand(); printf "%.1f", min+rand()*(max-min)}')
  light=$(shuf -i 100-1000 -n 1)
  pressure=$(awk -v min=980 -v max=1050 'BEGIN{srand(); printf "%.1f", min+rand()*(max-min)}')

  # Create JSON message
  message="{\"temperature\": $temperature, \"humidity\": $humidity, \"light\": $light, \"pressure\": $pressure}"

  nodeId=$(shuf -i 1-3 -n 1)

  # Publish the message using mosquitto_pub
  mosquitto_pub -h localhost -t "wot/sensors/$nodeId" -m "$message"

  # Wait for 3 seconds
  sleep 3
done
