"""
Simple client that connects to a local mqtt broker and prints messages on all
topics to the CLI
"""

import asyncio

from amqtt.client import MQTTClient


async def main():
    client = MQTTClient()
    await client.connect("mqtt://localhost:1883")
    await client.subscribe([("#", 0)])  # subscribe to ALL topics

    print("Listening for messages...")
    while True:
        msg = await client.deliver_message()
        if (
            msg is None
            or msg.publish_packet is None
            or msg.publish_packet.payload is None
        ):
            print("Skipping message because some fields are none.")
            continue
        topic = msg.publish_packet.variable_header.topic_name
        payload = msg.publish_packet.payload.data.decode()
        print(f"[{topic}] {payload}")


asyncio.run(main())
