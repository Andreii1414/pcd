import asyncio
import websockets
import json
import random

async def send_sensor_data():
    uri = "ws://localhost:8765"
    async with websockets.connect(uri) as websocket:
        while True:
            data = {
                "sensor_id": "sensor_1",
                "temperature": round(random.uniform(20, 35), 2),
                "humidity": round(random.uniform(60, 90), 2),
            }
            await websocket.send(json.dumps(data))
            print(f"Sent: {data}")

            response = await websocket.recv()
            print(f"Received: {response}")

            await asyncio.sleep(2)

asyncio.run(send_sensor_data())
