import json
import time
import threading
from typing import Optional, Dict, Any

import paho.mqtt.client as mqtt

from app.core.config import settings
from app.models.heartbeat import Heartbeat


class MqttManager:
    """
    Responsável por:
    - Conectar no AWS IoT Core via MQTT
    - Assinar heartbeat do ESP32
    - Disponibilizar último heartbeat recebido
    - Publicar comandos para o ESP32
    """

    def __init__(self) -> None:
        self._client = mqtt.Client(
            client_id=settings.aws_iot_client_id_backend
        )

        # Configuração de callbacks
        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        self._client.on_disconnect = self._on_disconnect

        # TLS / certificados
        self._client.tls_set(
            ca_certs=settings.aws_iot_ca_path,
            certfile=settings.aws_iot_cert_path,
            keyfile=settings.aws_iot_key_path,
        )

        self._lock = threading.Lock()
        self._last_heartbeat: Optional[Heartbeat] = None

    # ---------- Callbacks MQTT ----------

    def _on_connect(self, client, userdata, flags, rc):
        print(f"[MQTT] Conectado ao AWS IoT (rc={rc})")
        if rc == 0:
            topic = settings.aws_iot_topic_heartbeat
            client.subscribe(topic)
            print(f"[MQTT] Inscrito em {topic}")
        else:
            print("[MQTT] Erro na conexão MQTT")

    def _on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode("utf-8", errors="ignore")

        if topic == settings.aws_iot_topic_heartbeat:
            # Ignora mensagens que não parecem JSON
            if not payload.strip().startswith("{"):
                print("[MQTT] Mensagem ignorada em heartbeat (não-JSON):", payload)
                return

            try:
                data = json.loads(payload)
            except Exception as e:
                print("[MQTT] Erro ao parsear heartbeat:", e)
                return

            hb_dict: Dict[str, Any] = {
                "temp_c": data.get("temp_c"),
                "humidity": data.get("humidity"),
                "rain": data.get("rain"),
                "mode": data.get("mode"),
                "uptime_ms": data.get("uptime_ms"),
                "received_at": time.time(),
            }


            heartbeat = Heartbeat(**hb_dict)
            with self._lock:
                self._last_heartbeat = heartbeat

            print("[MQTT] Heartbeat atualizado:", heartbeat.model_dump())


    def _on_disconnect(self, client, userdata, rc):
        print(f"[MQTT] Desconectado do AWS IoT (rc={rc})")

    # ---------- API pública ----------

    def start(self) -> None:
        """Inicia conexão e loop MQTT em uma thread própria."""
        print("[MQTT] Conectando ao broker...")
        self._client.connect(
            settings.aws_iot_endpoint,
            settings.aws_iot_port,
            keepalive=60,
        )
        # loop_start é não-bloqueante
        self._client.loop_start()

    def stop(self) -> None:
        """Para loop e desconecta o cliente MQTT."""
        try:
            self._client.loop_stop()
            self._client.disconnect()
        except Exception as e:
            print("[MQTT] Erro ao parar cliente MQTT:", e)

    def get_last_heartbeat(self) -> Optional[Heartbeat]:
        """Retorna o último heartbeat recebido (ou None)."""
        with self._lock:
            return self._last_heartbeat

    def publish_command(self, command: str) -> bool:
        """Publica um comando simples no tópico de controle do varal."""
        topic = settings.aws_iot_topic_cmd
        result = self._client.publish(topic, command)
        ok = result.rc == mqtt.MQTT_ERR_SUCCESS
        if not ok:
            print(f"[MQTT] Falha ao publicar comando '{command}' (rc={result.rc})")
        return ok


# Instância única para a aplicação inteira
mqtt_manager = MqttManager()
