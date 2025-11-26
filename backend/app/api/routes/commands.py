from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

from app.core.mqtt_client import mqtt_manager

router = APIRouter(prefix="/cmd", tags=["Commands"])


class CommandRequest(BaseModel):
    command: str  # "OPEN", "CLOSE", "AUTO" etc.


@router.post("/")
def send_command(body: CommandRequest):
    """Envia um comando para o ESP32 via MQTT (AWS IoT Core)."""
    cmd = body.command.upper().strip()
    if cmd not in ("OPEN", "CLOSE", "AUTO"):
        raise HTTPException(
            status_code=400,
            detail="Comando inválido. Use OPEN, CLOSE ou AUTO.",
        )

    ok = mqtt_manager.publish_command(cmd)
    if not ok:
        raise HTTPException(
            status_code=500,
            detail="Falha ao publicar comando no MQTT.",
        )

    return {"status": "ok", "sent": cmd}
