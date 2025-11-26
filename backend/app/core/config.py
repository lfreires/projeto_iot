from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """Configurações da aplicação, carregadas de variáveis de ambiente / .env."""

    # AWS IoT
    aws_iot_endpoint: str
    aws_iot_port: int = 8883

    aws_iot_client_id_backend: str = "esp32_varal_backend"

    aws_iot_topic_heartbeat: str = "casa/varal1/heartbeat"
    aws_iot_topic_cmd: str = "casa/varal1/cmd"

    aws_iot_ca_path: str = "certs/AmazonRootCA1.pem"
    aws_iot_cert_path: str = "certs/certificate.crt"
    aws_iot_key_path: str = "certs/private.key"

    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"


settings = Settings()
