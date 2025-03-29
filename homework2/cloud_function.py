import json
import base64
import os
import smtplib
from email.mime.text import MIMEText
from flask import Flask, request

TEMP_THRESHOLD = 30.0
HUMIDITY_THRESHOLD = 80.0

SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
EMAIL_SENDER = os.getenv("EMAIL_SENDER", "")
EMAIL_PASSWORD = os.getenv("EMAIL_PASSWORD", "")
EMAIL_RECEIVER = os.getenv("EMAIL_RECEIVER", "")

if not EMAIL_SENDER or not EMAIL_PASSWORD or not EMAIL_RECEIVER:
    raise ValueError("Lipsește o variabilă de mediu necesară pentru email.")


def send_email(sensor_id, temperature, humidity):
    subject = f"Alertă: Senzor {sensor_id} a depășit pragul!"
    body = f"""
    Senzorul {sensor_id} a înregistrat valori ridicate:
    - Temperatură: {temperature}°C (prag: {TEMP_THRESHOLD}°C)
    - Umiditate: {humidity}% (prag: {HUMIDITY_THRESHOLD}%)
    """
    print("sender: ", body, EMAIL_SENDER)

    msg = MIMEText(body)
    msg["Subject"] = subject
    msg["From"] = EMAIL_SENDER
    msg["To"] = EMAIL_RECEIVER

    try:
        with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:
            server.set_debuglevel(1)
            server.starttls()
            server.login(EMAIL_SENDER, EMAIL_PASSWORD)
            server.sendmail(EMAIL_SENDER, EMAIL_RECEIVER, msg.as_string())
        print(f"Email trimis către {EMAIL_RECEIVER} pentru senzor {sensor_id}.")
    except smtplib.SMTPAuthenticationError:
        print("Eroare de autentificare SMTP. Verifică email-ul și parola.")
    except smtplib.SMTPException as e:
        print(f"Eroare SMTP: {e}")


app = Flask(__name__)


@app.route("/", methods=["POST"])
def notify_if_exceeded(request):
    try:
        req_data = request.get_json()
        if not req_data:
            return "Invalid JSON payload", 400

        print(f"Primit date: {req_data}")

        encoded_data = req_data.get("message", {}).get("data", "")
        if not encoded_data:
            return "Missing 'data' field", 400

        decoded_data = base64.b64decode(encoded_data).decode("utf-8")

        sensor_data = json.loads(decoded_data)
        print(f"Data decodată: {sensor_data}")

        sensor_id = sensor_data.get("sensor_id", "unknown")
        temperature = float(sensor_data.get("temperature", 0))
        humidity = float(sensor_data.get("humidity", 0))

        print(f"Temperatură: {temperature}, Umiditate: {humidity}")

        if temperature > TEMP_THRESHOLD or humidity > HUMIDITY_THRESHOLD:
            send_email(sensor_id, temperature, humidity)

        return "Processed", 200
    except Exception as e:
        print(f"Eroare la procesarea mesajului: {e}")
        return "Internal Server Error", 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)
