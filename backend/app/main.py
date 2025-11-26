from fastapi import FastAPI

from app.api.routes import heartbeat, commands
from app.core.mqtt_client import mqtt_manager


def create_app() -> FastAPI:
    app = FastAPI(
        title="Varal IoT Backend",
        version="1.0.0",
    )

    # Rotas
    app.include_router(heartbeat.router)
    app.include_router(commands.router)

    @app.on_event("startup")
    def on_startup() -> None:
        """Inicia o cliente MQTT quando a API sobe."""
        mqtt_manager.start()

    @app.on_event("shutdown")
    def on_shutdown() -> None:
        """Encerra o cliente MQTT quando a API desce."""
        mqtt_manager.stop()

    return app


app = create_app()
