from enum import Enum
from typing import Optional

from pydantic import BaseModel


class VaralMode(str, Enum):
    AUTO = "AUTO"
    FORCE_OPEN = "FORCE_OPEN"
    FORCE_CLOSE = "FORCE_CLOSE"


class Heartbeat(BaseModel):
    temp_c: Optional[float] = None
    humidity: Optional[float] = None
    rain: Optional[bool] = None
    mode: Optional[VaralMode] = None  # <-- novo
    uptime_ms: Optional[int] = None
    received_at: float  # timestamp local (servidor)
