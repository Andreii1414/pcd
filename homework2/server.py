import asyncio
import json
import websockets
from google.cloud import pubsub_v1

publisher = pubsub_v1.PublisherClient()
topic_path = publisher.topic_path("homework-2-454510", "real-time-topic")

TEMP_THRESHOLD = 30.0
HUMIDITY_THRESHOLD = 80.0

def publish_to_pubsub(sensor_id, temperature, humidity):
    message = json.dumps({"sensor_id": sensor_id, "temperature": temperature, "humidity": humidity}).encode("utf-8")
    publisher.publish(topic_path, message)

async def websocket_handler(websocket):
    try:
        async for message in websocket:
            data = json.loads(message)
            sensor_id = data.get("sensor_id", "unknown")
            temperature = float(data.get("temperature", 0))
            humidity = float(data.get("humidity", 0))

            print(f"Received data from {sensor_id}: Temp={temperature}°C, Hum={humidity}%")

            publish_to_pubsub(sensor_id, temperature, humidity)

            response = {"status": "ok"}
            if temperature > TEMP_THRESHOLD or humidity > HUMIDITY_THRESHOLD:
                response = {
                    "alert": f"Sensor {sensor_id} exceeded limits!",
                    "temperature": temperature,
                    "humidity": humidity
                }
            await websocket.send(json.dumps(response))
    except Exception as e:
        print(f"Error: {e}")


async def main():
    server = await websockets.serve(websocket_handler, "0.0.0.0", 8765)
    print("WebSocket Server running on ws://localhost:8765")
    await server.wait_closed()


if __name__ == "__main__":
    asyncio.run(main())
