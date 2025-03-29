import json
from google.cloud import firestore, pubsub_v1, bigquery
from datetime import datetime

db = firestore.Client()
bq_client = bigquery.Client()

subscription_name = "projects/homework-2-454510/subscriptions/real-time-subscription"
subscriber = pubsub_v1.SubscriberClient()
subscription_path = subscriber.subscription_path("homework-2-454510", "real-time-subscription")

def save_to_firestore(sensor_id, temperature, humidity):
    doc_ref = db.collection("sensor_data").document(sensor_id)
    doc_ref.set({
        "temperature": temperature,
        "humidity": humidity,
        "timestamp": firestore.SERVER_TIMESTAMP
    })

def save_to_bigquery(sensor_id, temperature, humidity):
    table_id = "homework-2-454510.sensors.sensors_data"
    rows_to_insert = [{
        "sensor_id": sensor_id,
        "temperature": temperature,
        "humidity": humidity,
        "timestamp": datetime.utcnow().isoformat()
    }]
    errors = bq_client.insert_rows_json(table_id, rows_to_insert)
    if errors:
        print(f"BigQuery insert error: {errors}")


def callback(message):
    data = json.loads(message.data.decode("utf-8"))
    sensor_id = data["sensor_id"]
    temperature = data["temperature"]
    humidity = data["humidity"]

    print(f"Processing data from {sensor_id} -> Temp: {temperature}, Humidity: {humidity}")

    save_to_firestore(sensor_id, temperature, humidity)
    save_to_bigquery(sensor_id, temperature, humidity)

    message.ack()

subscriber.subscribe(subscription_path, callback=callback)
print("Listening for messages on Pub/Sub...")
try:
    while True:
        pass
except KeyboardInterrupt:
    print("Interrupted by user")