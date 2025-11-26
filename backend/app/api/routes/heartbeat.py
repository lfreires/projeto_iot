from fastapi import APIRouter, HTTPException

from app.core.mqtt_client import mqtt_manager
from app.models.heartbeat import Heartbeat

router = APIRouter(prefix="/heartbeat", tags=["Heartbeat"])


@router.get("/", response_model=Heartbeat)
def get_heartbeat():
    """Retorna o último heartbeat recebido do ESP32."""
    hb = mqtt_manager.get_last_heartbeat()
    if hb is None:
        raise HTTPException(
            status_code=404,
            detail="Ainda não recebi heartbeat do ESP32.",
        )
    return hb
